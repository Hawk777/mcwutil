#include "region/unpack.h"
#include "util/codec.h"
#include "util/fd.h"
#include "util/globals.h"
#include "util/string.h"
#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <fcntl.h>
#include <filesystem>
#include <iostream>
#include <libxml++/document.h>
#include <libxml++/nodes/element.h>
#include <libxml++/nodes/node.h>
#include <stdexcept>
#include <string>
#include <string_view>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

using namespace std::literals::string_literals;
using namespace std::literals::string_view_literals;

int Region::unpack(std::ranges::subrange<char **> args) {
	// Check parameters.
	if(args.size() != 2) {
		std::cerr << "Usage:\n";
		std::cerr << appname << " region-unpack regionfile outdir\n";
		std::cerr << '\n';
		std::cerr << "Unpacks a region file into its constituent chunks.\n";
		std::cerr << '\n';
		std::cerr << "Arguments:\n";
		std::cerr << "  regionfile - the .mcr file to unpack\n";
		std::cerr << "  outdir - the directory to unpack into\n";
		return 1;
	}

	// Extract provided pathnames.
	const char *region_filename = args[0];
	const char *output_directory = args[1];

	// Open the region file.
	FileDescriptor region_fd = FileDescriptor::create_open(region_filename, O_RDONLY, 0);

	// Read the header.
	uint8_t header[8192];
	region_fd.pread(header, sizeof(header), 0);

	// Iterate the chunks, filling in the metadata document and extracting the chunks to files.
	xmlpp::Document metadata_document;
	metadata_document.set_internal_subset(utf8_literal(u8"minecraft-region-metadata"), utf8_literal(u8""), utf8_literal(u8"urn:uuid:5e7a5ee0-2a7b-11e1-9e08-1c4bd68d068e"));
	xmlpp::Element *metadata_root_elt = metadata_document.create_root_node(utf8_literal(u8"minecraft-region-metadata"));
	for(unsigned int i = 0; i < 1024; ++i) {
		// Decode the header for this chunk.
		uint32_t offset_sectors = decode_u24(&header[i * 4]);
		uint8_t size_sectors = decode_u8(&header[i * 4 + 3]);
		uint32_t timestamp = decode_u32(&header[4096 + i * 4]);

		// Sanity check.
		if((offset_sectors && !size_sectors) || (size_sectors && !offset_sectors)) {
			throw std::runtime_error("Malformed region header: chunk is half-present.");
		}

		// Construct a metadata element.
		xmlpp::Element *metadata_chunk_elt = metadata_root_elt->add_child(utf8_literal(u8"chunk"));
		metadata_chunk_elt->set_attribute(utf8_literal(u8"index"), todecu(i));

		if(offset_sectors) {
			// Record the chunk's metadata.
			metadata_chunk_elt->set_attribute(utf8_literal(u8"present"), utf8_literal(u8"1"));
			metadata_chunk_elt->set_attribute(utf8_literal(u8"timestamp"), todecu(timestamp));

			// Compute the location and size of the chunk.
			off_t offset_bytes = static_cast<off_t>(offset_sectors) * 4096;
			std::size_t rough_size_bytes = static_cast<std::size_t>(size_sectors) * 4096;

			// Read the chunk's data.
			uint8_t chunk_data[rough_size_bytes];
			region_fd.pread(chunk_data, rough_size_bytes, offset_bytes);

			// Extract and sanity-check the chunk's header.
			uint32_t precise_size_bytes = decode_u32(&chunk_data[0]);
			if(precise_size_bytes < 1) {
				throw std::runtime_error("Malformed chunk: precise size < 1.");
			}
			if(precise_size_bytes > rough_size_bytes) {
				throw std::runtime_error("Malformed chunk: precise size > rough size.");
			}
			uint8_t compression_type = decode_u8(&chunk_data[4]);
			if(compression_type != 2) {
				throw std::runtime_error("Malformed chunk: unrecognized compression type.");
			}
			std::size_t payload_size_bytes = precise_size_bytes - 1;
			const void *payload = &chunk_data[5];

			// Copy the chunk's data out to a file.
			std::string name_part("chunk-"s);
			name_part += todecu(i, 4);
			name_part += ".nbt.zlib"sv;
			std::filesystem::path chunk_filename(output_directory);
			chunk_filename /= name_part;
			FileDescriptor chunk_fd = FileDescriptor::create_open(chunk_filename, O_WRONLY | O_CREAT, 0666);
			chunk_fd.write(payload, payload_size_bytes);
		} else {
			// Mark the chunk as non-present in the metadata document.
			metadata_chunk_elt->set_attribute(utf8_literal(u8"present"), utf8_literal(u8"0"));
		}
	}

	// Write out the metadata file.
	std::filesystem::path metadata_filename(output_directory);
	metadata_filename /= "metadata.xml";
	metadata_document.write_to_file_formatted(metadata_filename.native());

	return 0;
}
