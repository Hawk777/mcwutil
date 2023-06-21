#include "region/pack.h"
#include "util/codec.h"
#include "util/fd.h"
#include "util/file_utils.h"
#include "util/globals.h"
#include "util/string.h"
#include <algorithm>
#include <array>
#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <fcntl.h>
#include <glibmm/convert.h>
#include <iostream>
#include <libxml++/document.h>
#include <libxml++/nodes/element.h>
#include <libxml++/nodes/node.h>
#include <libxml++/parsers/domparser.h>
#include <locale>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

int Region::pack(const std::vector<std::string> &args) {
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
	const std::string &input_directory = args[0];
	const std::string &region_filename = args[1];

	// Load the metadata document.
	xmlpp::DomParser metadata_parser;
	metadata_parser.set_substitute_entities();
	metadata_parser.parse_file(input_directory + G_DIR_SEPARATOR + Glib::filename_from_utf8(utf8_literal(u8"metadata.xml")));
	const xmlpp::Document *metadata_document = metadata_parser.get_document();
	const xmlpp::Element *metadata_root_elt = metadata_document->get_root_node();
	if(metadata_root_elt->get_name() != u8"minecraft-region-metadata") {
		throw std::runtime_error("Malformed metadata.xml: improper root node name.");
	}

	// Open the region file.
	FileDescriptor region_fd = FileDescriptor::create_open(region_filename.c_str(), O_WRONLY | O_TRUNC | O_CREAT, 0666);
	off_t region_write_ptr = 8192;

	// Iterate the chunk elements in the metadata file.
	// There should be 1024 of them with distinct indices.
	// Keep track of which have been seen.
	// Also allocate space to hold the header that we're building.
	std::array<bool, 1024> seen_indices;
	std::fill(seen_indices.begin(), seen_indices.end(), false);
	std::array<uint8_t, 8192> header;
	std::fill(header.begin(), header.end(), 0);
	const xmlpp::Node::NodeList &chunk_elts = metadata_root_elt->get_children(utf8_literal(u8"chunk"));
	for(auto i = chunk_elts.begin(), iend = chunk_elts.end(); i != iend; ++i) {
		const xmlpp::Element *chunk_elt = dynamic_cast<const xmlpp::Element *>(*i);
		unsigned int index;
		{
			std::wistringstream iss(ustring2wstring(chunk_elt->get_attribute_value(utf8_literal(u8"index"))));
			iss.imbue(std::locale("C"));
			iss >> index;
		}
		unsigned int present;
		{
			std::wistringstream iss(ustring2wstring(chunk_elt->get_attribute_value(utf8_literal(u8"present"))));
			iss.imbue(std::locale("C"));
			iss >> present;
		}
		uint32_t timestamp;
		{
			std::wistringstream iss(ustring2wstring(chunk_elt->get_attribute_value(utf8_literal(u8"timestamp"))));
			iss.imbue(std::locale("C"));
			iss >> timestamp;
		}

		if(seen_indices[index]) {
			throw std::runtime_error("Malformed metadata.xml: repeated chunk index.");
		}
		seen_indices[index] = true;

		if(present) {
			// Copy the chunk data into the region file.
			const std::string &chunk_filename = input_directory + G_DIR_SEPARATOR + Glib::filename_from_utf8(Glib::ustring::compose(utf8_literal(u8"chunk-%1.nbt.zlib"), todecu(index, 4)));
			FileDescriptor chunk_fd = FileDescriptor::create_open(chunk_filename.c_str(), O_RDONLY, 0);
			struct stat stbuf;
			FileUtils::fstat(chunk_fd, stbuf);
			uint8_t chunk_data[5 + stbuf.st_size];
			FileUtils::read(chunk_fd, &chunk_data[5], sizeof(chunk_data) - 5);
			encode_u32(&chunk_data[0], static_cast<uint32_t>(stbuf.st_size + 1));
			encode_u8(&chunk_data[4], 2);
			FileUtils::pwrite(region_fd, chunk_data, sizeof(chunk_data), region_write_ptr);
			uint32_t sector_offset = static_cast<uint32_t>(region_write_ptr / 4096);
			encode_u24(&header.data()[4 * index], sector_offset);
			uint8_t sector_count = static_cast<uint8_t>((sizeof(chunk_data) + 4095) / 4096);
			encode_u8(&header.data()[4 * index + 3], sector_count);
			encode_u32(&header.data()[4096 + 4 * index], timestamp);
			region_write_ptr += static_cast<off_t>(sector_count) * 4096;
		}
	}

	// Check if all values have been seen.
	if(std::find(seen_indices.begin(), seen_indices.end(), false) != seen_indices.end()) {
		throw std::runtime_error("Malformed metadata.xml: not every chunk index is present.");
	}

	// Extend the file to a sector boundary.
	FileUtils::ftruncate(region_fd, region_write_ptr);

	// Write the header.
	FileUtils::pwrite(region_fd, header.data(), header.size(), 0);

	return 0;
}
