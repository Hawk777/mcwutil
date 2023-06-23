#include "nbt/nbt.h"
#include "nbt/tags.h"
#include "util/codec.h"
#include "util/file_descriptor.h"
#include "util/globals.h"
#include "util/mapped_file.h"
#include "util/string.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <cstdlib>
#include <fcntl.h>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <sys/stat.h>
#include <sys/types.h>
#include <vector>

using namespace std::literals::string_view_literals;

namespace mcwutil::nbt {
namespace {
typedef std::array<uint16_t, 16 * 16 * 16> Section;

const std::vector<std::u8string_view> PATH_TO_SECTIONS = {u8""sv, u8"Level"sv, u8"Sections"sv};
const std::vector<std::u8string_view> PATH_TO_BLOCKS = {u8""sv, u8"Level"sv, u8"Sections"sv, u8"Blocks"sv};
const std::vector<std::u8string_view> PATH_TO_ADD = {u8""sv, u8"Level"sv, u8"Sections"sv, u8"Add"sv};

void check_left(std::size_t needed, std::size_t left) {
	if(left < needed) {
		throw std::runtime_error("Malformed NBT: input truncated.");
	}
}

void eat(std::size_t n, uint8_t *&input_ptr, std::size_t &input_left) {
	assert(n <= input_left);
	input_ptr += n;
	input_left -= n;
}

void handle_named(nbt::tag tag, uint8_t *&input_ptr, std::size_t &input_left, const uint16_t *sub_table, const FileDescriptor &output_fd, Section &section_blocks, std::vector<std::u8string_view> &path);

void handle_content(nbt::tag tag, uint8_t *&input_ptr, std::size_t &input_left, const uint16_t *sub_table, const FileDescriptor &output_fd, Section &section_blocks, std::vector<std::u8string_view> &path) {
	switch(tag) {
		case nbt::TAG_END:
			throw std::runtime_error("Malformed NBT: unexpected TAG_END.");

		case nbt::TAG_BYTE:
			check_left(1, input_left);
			output_fd.write(input_ptr, 1);
			eat(1, input_ptr, input_left);
			return;

		case nbt::TAG_SHORT:
			check_left(2, input_left);
			output_fd.write(input_ptr, 2);
			eat(2, input_ptr, input_left);
			return;

		case nbt::TAG_INT:
		case nbt::TAG_FLOAT:
			check_left(4, input_left);
			output_fd.write(input_ptr, 4);
			eat(4, input_ptr, input_left);
			return;

		case nbt::TAG_LONG:
		case nbt::TAG_DOUBLE:
			check_left(8, input_left);
			output_fd.write(input_ptr, 8);
			eat(8, input_ptr, input_left);
			return;

		case nbt::TAG_BYTE_ARRAY: {
			check_left(4, input_left);
			int32_t length = codec::decode_u32(input_ptr);
			eat(4, input_ptr, input_left);
			if(length < 0) {
				throw std::runtime_error("Malformed NBT: negative byte array length.");
			}
			const uint8_t *barray_ptr = input_ptr;
			eat(length, input_ptr, input_left);

			if(path == PATH_TO_BLOCKS) {
				for(std::size_t i = 0; i < 16 * 16 * 16; ++i) {
					section_blocks[i] = static_cast<uint16_t>(section_blocks[i] | barray_ptr[i]);
				}
			} else if(path == PATH_TO_ADD) {
				for(std::size_t i = 0; i < 16 * 16 * 16; i += 2) {
					section_blocks[i] = static_cast<uint16_t>(section_blocks[i] | static_cast<uint16_t>(barray_ptr[i / 2] & 0xF) << 8);
					section_blocks[i + 1] = static_cast<uint16_t>(section_blocks[i + 1] | static_cast<uint16_t>(barray_ptr[i / 2] & 0xF0) << 4);
				}
			} else {
				uint8_t header[4];
				codec::encode_u32(&header[0], length);
				output_fd.write(header, sizeof(header));
				output_fd.write(barray_ptr, length);
			}
			return;
		}

		case nbt::TAG_STRING: {
			check_left(2, input_left);
			int16_t length = codec::decode_u16(input_ptr);
			eat(2, input_ptr, input_left);
			if(length < 0) {
				throw std::runtime_error("Malformed NBT: negative string length.");
			}
			check_left(length, input_left);
			const uint8_t *string_ptr = input_ptr;
			eat(length, input_ptr, input_left);

			uint8_t header[2];
			codec::encode_u16(&header[0], length);
			output_fd.write(header, sizeof(header));
			output_fd.write(string_ptr, length);
			return;
		}

		case nbt::TAG_LIST: {
			check_left(5, input_left);
			nbt::tag subtype = static_cast<nbt::tag>(codec::decode_u8(input_ptr));
			eat(1, input_ptr, input_left);
			int32_t length = codec::decode_u32(input_ptr);
			eat(4, input_ptr, input_left);
			if(length < 0) {
				throw std::runtime_error("Malformed NBT: negative list length.");
			}

			uint8_t header[5];
			codec::encode_u8(&header[0], subtype);
			codec::encode_u32(&header[1], length);
			output_fd.write(header, sizeof(header));

			for(int32_t i = 0; i < length; ++i) {
				handle_content(subtype, input_ptr, input_left, sub_table, output_fd, section_blocks, path);
			}
			return;
		}

		case nbt::TAG_COMPOUND: {
			if(path == PATH_TO_SECTIONS) {
				std::fill(section_blocks.begin(), section_blocks.end(), 0);
			}
			for(;;) {
				check_left(1, input_left);
				nbt::tag subtype = static_cast<nbt::tag>(codec::decode_u8(input_ptr));
				eat(1, input_ptr, input_left);

				if(subtype == nbt::TAG_END) {
					if(path == PATH_TO_SECTIONS) {
						bool any_extended = false;
						for(auto i = section_blocks.begin(), iend = section_blocks.end(); i != iend; ++i) {
							*i = sub_table[*i];
							if(*i > 255) {
								any_extended = true;
							}
						}
						uint8_t header[4];
						codec::encode_u8(&header[0], nbt::TAG_BYTE_ARRAY);
						codec::encode_u16(&header[1], sizeof(u8"Blocks") - 1);
						output_fd.write(header, 3);
						output_fd.write(u8"Blocks", sizeof(u8"Blocks") - 1);
						codec::encode_u32(&header[0], 16 * 16 * 16);
						output_fd.write(header, 4);
						uint8_t buffer[16 * 16 * 16];
						for(std::size_t i = 0; i < 16 * 16 * 16; ++i) {
							buffer[i] = static_cast<uint8_t>(section_blocks[i] & 0xFF);
						}
						output_fd.write(buffer, 16 * 16 * 16);
						if(any_extended) {
							codec::encode_u8(&header[0], nbt::TAG_BYTE_ARRAY);
							codec::encode_u16(&header[1], sizeof(u8"Add") - 1);
							output_fd.write(header, 3);
							output_fd.write(u8"Add", sizeof(u8"Add") - 1);
							codec::encode_u32(&header[0], 16 * 16 * 16 / 2);
							output_fd.write(header, 4);
							for(std::size_t i = 0; i < 16 * 16 * 16; i += 2) {
								buffer[i / 2] = static_cast<uint8_t>((section_blocks[i] >> 8) | ((section_blocks[i + 1] >> 8) << 4));
							}
							output_fd.write(buffer, 16 * 16 * 16 / 2);
						}
					}
					uint8_t footer;
					codec::encode_u8(&footer, nbt::TAG_END);
					output_fd.write(&footer, sizeof(footer));
					return;
				}
				handle_named(subtype, input_ptr, input_left, sub_table, output_fd, section_blocks, path);
			}
		}

		case nbt::TAG_INT_ARRAY: {
			check_left(4, input_left);
			int32_t length = codec::decode_u32(input_ptr);
			eat(4, input_ptr, input_left);
			if(length < 0) {
				throw std::runtime_error("Malformed NBT: negative integer array length.");
			}

			uint8_t header[4];
			codec::encode_u32(&header[0], length);
			output_fd.write(header, sizeof(header));

			output_fd.write(input_ptr, length);
			eat(length, input_ptr, input_left);
			output_fd.write(input_ptr, length);
			eat(length, input_ptr, input_left);
			output_fd.write(input_ptr, length);
			eat(length, input_ptr, input_left);
			output_fd.write(input_ptr, length);
			eat(length, input_ptr, input_left);
			return;
		}

		case nbt::TAG_LONG_ARRAY: {
			check_left(4, input_left);
			int32_t length = codec::decode_u32(input_ptr);
			eat(4, input_ptr, input_left);
			if(length < 0) {
				throw std::runtime_error("Malformed NBT: negative long array length.");
			}

			uint8_t header[4];
			codec::encode_u32(&header[0], length);
			output_fd.write(header, sizeof(header));

			output_fd.write(input_ptr, length);
			eat(length, input_ptr, input_left);
			output_fd.write(input_ptr, length);
			eat(length, input_ptr, input_left);
			output_fd.write(input_ptr, length);
			eat(length, input_ptr, input_left);
			output_fd.write(input_ptr, length);
			eat(length, input_ptr, input_left);
			output_fd.write(input_ptr, length);
			eat(length, input_ptr, input_left);
			output_fd.write(input_ptr, length);
			eat(length, input_ptr, input_left);
			output_fd.write(input_ptr, length);
			eat(length, input_ptr, input_left);
			output_fd.write(input_ptr, length);
			eat(length, input_ptr, input_left);
			return;
		}
	}

	throw std::runtime_error("Malformed NBT: unrecognized tag.");
}

void handle_named(nbt::tag tag, uint8_t *&input_ptr, std::size_t &input_left, const uint16_t *sub_table, const FileDescriptor &output_fd, Section &section_blocks, std::vector<std::u8string_view> &path) {
	// Read name length.
	check_left(2, input_left);
	int16_t name_len = codec::decode_u16(input_ptr);
	eat(2, input_ptr, input_left);
	if(name_len < 0) {
		throw std::runtime_error("Malformed NBT: negative name length.");
	}

	// Read name and add to path.
	check_left(name_len, input_left);
	const char8_t *name_ptr = reinterpret_cast<const char8_t *>(input_ptr);
	eat(name_len, input_ptr, input_left);
	path.emplace_back(name_ptr, name_len);

	// Write to output.
	if(!(tag == nbt::TAG_BYTE_ARRAY && (path == PATH_TO_BLOCKS || path == PATH_TO_ADD))) {
		uint8_t header[3];
		codec::encode_u8(&header[0], tag);
		codec::encode_u16(&header[1], name_len);
		output_fd.write(header, sizeof(header));
		output_fd.write(name_ptr, name_len);
	}

	// Handle content.
	handle_content(tag, input_ptr, input_left, sub_table, output_fd, section_blocks, path);

	// Fix path.
	path.pop_back();
}

void usage() {
	std::cerr << "Usage:\n";
	std::cerr << appname << " nbt-block-substitute infile outfile from1 to1 [from2 to2 ...]\n";
	std::cerr << '\n';
	std::cerr << "Changes block IDs in an NBT file.\n";
	std::cerr << "Only the terrain arrays are affected; items in inventories should be handled separately if they also need to be changed.\n";
	std::cerr << "It is also not possible to use this tool to replace air in omitted sections with another block.\n";
	std::cerr << '\n';
	std::cerr << "Arguments:\n";
	std::cerr << "  infile - the NBT file to modify\n";
	std::cerr << "  outfile - the location at which to save the new NBT file (must not be equal to infile)\n";
	std::cerr << "  from1 - the first block ID to change to something else (an integer between 0 and 4095)\n";
	std::cerr << "  to1 - the block ID to change blocks equal to \"from1\" to (an integer between 0 and 4095)\n";
}
}
}

int mcwutil::nbt::block_substitute(std::ranges::subrange<char **> args) {
	// Check parameters.
	if(args.size() < 4 || (args.size() % 2) != 0) {
		usage();
		return 1;
	}

	// Build the substitution table.
	uint16_t sub_table[4096];
	for(unsigned int i = 0; i < 4096; ++i) {
		sub_table[i] = static_cast<uint16_t>(i);
	}
	for(std::size_t i = 2; i < args.size(); i += 2) {
		if(!args[i][0] || !args[i + 1][0]) {
			usage();
			return 1;
		}
		unsigned long from, to;
		{
			char *end;
			from = std::strtoul(args[i], &end, 10);
			if(*end) {
				usage();
				return 1;
			}
		}
		{
			char *end;
			to = std::strtoul(args[i + 1], &end, 10);
			if(*end) {
				usage();
				return 1;
			}
		}
		if(from > 4095 || to > 4095) {
			usage();
			return 1;
		}
		sub_table[from] = static_cast<uint16_t>(to);
	}

	// Open and map input NBT file.
	FileDescriptor input_fd = FileDescriptor::create_open(args[0], O_RDONLY, 0);
	MappedFile input_mapped(input_fd, PROT_READ);

	// Open the output file.
	FileDescriptor output_fd = FileDescriptor::create_open(args[1], O_WRONLY | O_CREAT | O_TRUNC, 0666);

	// Do the thing.
	Section section_blocks;
	std::vector<std::u8string_view> path;
	std::fill(section_blocks.begin(), section_blocks.end(), 0);
	uint8_t *input_ptr = static_cast<uint8_t *>(input_mapped.data());
	std::size_t input_left = input_mapped.size();
	check_left(1, input_left);
	nbt::tag root_tag = static_cast<nbt::tag>(codec::decode_u8(input_ptr));
	eat(1, input_ptr, input_left);
	handle_named(root_tag, input_ptr, input_left, sub_table, output_fd, section_blocks, path);

	return 0;
}
