#include "region/region.h"
#include "util/codec.h"
#include "util/file_descriptor.h"
#include "util/globals.h"
#include "util/string.h"
#include "util/xml.h"
#include <algorithm>
#include <array>
#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <fcntl.h>
#include <filesystem>
#include <iostream>
#include <libxml/tree.h>
#include <locale>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

using namespace std::literals::string_literals;
using namespace std::literals::string_view_literals;

int mcwutil::region::pack(std::ranges::subrange<char **> args) {
	// Check parameters.
	if(args.size() != 2) {
		std::cerr << "Usage:\n";
		std::cerr << appname << " region-pack indir regionfile\n";
		std::cerr << '\n';
		std::cerr << "Builds a region file by packing a collection of chunks.\n";
		std::cerr << '\n';
		std::cerr << "Arguments:\n";
		std::cerr << "  indir - the directory containing the metadata.xml and chunk-*.nbt.zlib files to pack\n";
		std::cerr << "  regionfile - the .mcr file to create or replace\n";
		return 1;
	}

	// Extract provided pathnames.
	const char *input_directory = args[0];
	const char *region_filename = args[1];

	// Load the metadata document.
	std::filesystem::path metadata_file(input_directory);
	metadata_file /= "metadata.xml";
	auto metadata_document = xml::parse(metadata_file.c_str());
	const xmlNode &metadata_root_elt = *xmlDocGetRootElement(metadata_document.get());
	if(xml::node_name(metadata_root_elt) != u8"minecraft-region-metadata"sv) {
		throw std::runtime_error("Malformed metadata.xml: improper root node name.");
	}

	// Open the region file.
	file_descriptor region_fd = file_descriptor::create_open(region_filename, O_WRONLY | O_TRUNC | O_CREAT, 0666);
	off_t region_write_ptr = 8192;

	// Iterate the chunk elements in the metadata file.
	// There should be 1024 of them with distinct indices.
	// Keep track of which have been seen.
	// Also allocate space to hold the header that we're building.
	std::array<bool, 1024> seen_indices;
	std::fill(seen_indices.begin(), seen_indices.end(), false);
	std::array<uint8_t, 8192> header;
	std::fill(header.begin(), header.end(), 0);
	for(const xmlNode *i = metadata_root_elt.children; i; i = i->next) {
		if(i->type != XML_ELEMENT_NODE) {
			continue;
		}
		unsigned int index;
		{
			std::istringstream iss(string::u2l(xml::node_attr(*i, u8"index")));
			iss.imbue(std::locale("C"));
			iss >> index;
		}
		unsigned int present;
		{
			std::istringstream iss(string::u2l(xml::node_attr(*i, u8"present")));
			iss.imbue(std::locale("C"));
			iss >> present;
		}
		uint32_t timestamp;
		{
			const char8_t *timestamp_raw = xml::node_attr(*i, u8"timestamp");
			if(timestamp_raw) {
				std::istringstream iss(string::u2l(timestamp_raw));
				iss.imbue(std::locale("C"));
				iss >> timestamp;
			} else {
				timestamp = 0;
			}
		}

		if(seen_indices[index]) {
			throw std::runtime_error("Malformed metadata.xml: repeated chunk index.");
		}
		seen_indices[index] = true;

		if(present) {
			// Copy the chunk data into the region file.
			std::filesystem::path chunk_filename(input_directory);
			std::string file_part("chunk-"s);
			file_part += string::todecu(index, 4);
			file_part += ".nbt.zlib"sv;
			chunk_filename /= file_part;
			file_descriptor chunk_fd = file_descriptor::create_open(chunk_filename, O_RDONLY, 0);
			struct stat stbuf;
			chunk_fd.fstat(stbuf);
			uint8_t chunk_data[5 + stbuf.st_size];
			chunk_fd.read(&chunk_data[5], sizeof(chunk_data) - 5);
			codec::encode_u32(&chunk_data[0], static_cast<uint32_t>(stbuf.st_size + 1));
			codec::encode_u8(&chunk_data[4], 2);
			region_fd.pwrite(chunk_data, sizeof(chunk_data), region_write_ptr);
			uint32_t sector_offset = static_cast<uint32_t>(region_write_ptr / 4096);
			codec::encode_u24(&header.data()[4 * index], sector_offset);
			uint8_t sector_count = static_cast<uint8_t>((sizeof(chunk_data) + 4095) / 4096);
			codec::encode_u8(&header.data()[4 * index + 3], sector_count);
			codec::encode_u32(&header.data()[4096 + 4 * index], timestamp);
			region_write_ptr += static_cast<off_t>(sector_count) * 4096;
		}
	}

	// Check if all values have been seen.
	if(std::find(seen_indices.begin(), seen_indices.end(), false) != seen_indices.end()) {
		throw std::runtime_error("Malformed metadata.xml: not every chunk index is present.");
	}

	// Extend the file to a sector boundary.
	region_fd.ftruncate(region_write_ptr);

	// Write the header.
	region_fd.pwrite(header.data(), header.size(), 0);

	return 0;
}
