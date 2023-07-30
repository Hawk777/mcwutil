#include "mcwutil/region/region.hpp"
#include "mcwutil/util/codec.hpp"
#include "mcwutil/util/file_descriptor.hpp"
#include "mcwutil/util/globals.hpp"
#include "mcwutil/util/string.hpp"
#include "mcwutil/util/xml.hpp"
#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <fcntl.h>
#include <filesystem>
#include <iostream>
#include <libxml/tree.h>
#include <stdexcept>
#include <string>
#include <string_view>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

using namespace std::literals::string_literals;
using namespace std::literals::string_view_literals;

/**
 * \brief Entry point for the \c region-unpack utility.
 *
 * \param[in] args the command-line arguments.
 *
 * \return the application exit code.
 */
int mcwutil::region::unpack(std::span<char *> args) {
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
	file_descriptor region_fd = file_descriptor::create_open(region_filename, O_RDONLY, 0);

	// Read the header.
	uint8_t header[8192];
	region_fd.pread(header, sizeof(header), 0);

	// Iterate the chunks, filling in the metadata document and extracting the chunks to files.
	auto metadata_document = xml::empty();
	xml::internal_subset(*metadata_document, u8"minecraft-region-metadata", nullptr, u8"urn:uuid:5e7a5ee0-2a7b-11e1-9e08-1c4bd68d068e");
	xmlNode &metadata_root_elt = xml::node_create_root(*metadata_document, u8"minecraft-region-metadata");
	for(unsigned int i = 0; i < 1024; ++i) {
		// Decode the header for this chunk.
		uint32_t offset_sectors = codec::decode_integer<uint32_t, 3>(&header[i * 4]);
		uint8_t size_sectors = codec::decode_integer<uint8_t>(&header[i * 4 + 3]);
		uint32_t timestamp = codec::decode_integer<uint32_t>(&header[4096 + i * 4]);

		// Sanity check.
		if((offset_sectors && !size_sectors) || (size_sectors && !offset_sectors)) {
			throw std::runtime_error("Malformed region header: chunk is half-present.");
		}

		// Construct a metadata element.
		xmlNode &metadata_chunk_elt = xml::node_append_child(metadata_root_elt, u8"chunk");
		xml::node_attr(metadata_chunk_elt, u8"index", string::l2u(string::todecu(i)).c_str());

		if(offset_sectors) {
			// Record the chunk's metadata.
			xml::node_attr(metadata_chunk_elt, u8"present", u8"1");
			xml::node_attr(metadata_chunk_elt, u8"timestamp", string::l2u(string::todecu(timestamp)).c_str());

			// Compute the location and size of the chunk.
			off_t offset_bytes = static_cast<off_t>(offset_sectors) * 4096;
			std::size_t rough_size_bytes = static_cast<std::size_t>(size_sectors) * 4096;

			// Read the chunk's data.
			uint8_t chunk_data[rough_size_bytes];
			region_fd.pread(chunk_data, rough_size_bytes, offset_bytes);

			// Extract and sanity-check the chunk's header.
			uint32_t precise_size_bytes = codec::decode_integer<uint32_t>(&chunk_data[0]);
			if(precise_size_bytes < 1) {
				throw std::runtime_error("Malformed chunk: precise size < 1.");
			}
			if(precise_size_bytes > rough_size_bytes) {
				throw std::runtime_error("Malformed chunk: precise size > rough size.");
			}
			uint8_t compression_type = codec::decode_integer<uint8_t>(&chunk_data[4]);
			if(compression_type != 2) {
				throw std::runtime_error("Malformed chunk: unrecognized compression type.");
			}
			std::size_t payload_size_bytes = precise_size_bytes - 1;
			const void *payload = &chunk_data[5];

			// Copy the chunk's data out to a file.
			std::string name_part("chunk-"s);
			name_part += string::todecu(i, 4);
			name_part += ".nbt.zlib"sv;
			std::filesystem::path chunk_filename(output_directory);
			chunk_filename /= name_part;
			file_descriptor chunk_fd = file_descriptor::create_open(chunk_filename, O_WRONLY | O_CREAT | O_TRUNC, 0666);
			chunk_fd.write(payload, payload_size_bytes);
			chunk_fd.close();
		} else {
			// Mark the chunk as non-present in the metadata document.
			xml::node_attr(metadata_chunk_elt, u8"present", u8"0");
		}
	}

	// Write out the metadata file.
	std::filesystem::path metadata_filename(output_directory);
	metadata_filename /= "metadata.xml";
	file_descriptor metadata_fd = file_descriptor::create_open(metadata_filename, O_WRONLY | O_CREAT | O_TRUNC, 0666);
	xml::write(*metadata_document, metadata_fd);
	metadata_fd.close();

	return 0;
}
