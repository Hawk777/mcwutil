#include "nbt/nbt.h"
#include "nbt/tags.h"
#include "util/codec.h"
#include "util/fd.h"
#include "util/globals.h"
#include "util/mapped_file.h"
#include "util/string.h"
#include <cassert>
#include <cstdlib>
#include <fcntl.h>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <vector>

namespace mcwutil::nbt {
namespace {
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

void handle_named(nbt::tag tag, uint8_t *&input_ptr, std::size_t &input_left, const uint8_t *sub_table, const std::vector<Glib::ustring>::const_iterator path_first, const std::vector<Glib::ustring>::const_iterator path_last, bool path_ok);

void handle_content(nbt::tag tag, uint8_t *&input_ptr, std::size_t &input_left, const uint8_t *sub_table, const std::vector<Glib::ustring>::const_iterator path_first, const std::vector<Glib::ustring>::const_iterator path_last, bool path_ok) {
	switch(tag) {
		case nbt::TAG_END:
			throw std::runtime_error("Malformed NBT: unexpected TAG_END.");

		case nbt::TAG_BYTE:
			check_left(1, input_left);
			eat(1, input_ptr, input_left);
			return;

		case nbt::TAG_SHORT:
			check_left(2, input_left);
			eat(2, input_ptr, input_left);
			return;

		case nbt::TAG_INT:
		case nbt::TAG_FLOAT:
			check_left(4, input_left);
			eat(4, input_ptr, input_left);
			return;

		case nbt::TAG_LONG:
		case nbt::TAG_DOUBLE:
			check_left(8, input_left);
			eat(8, input_ptr, input_left);
			return;

		case nbt::TAG_BYTE_ARRAY: {
			check_left(4, input_left);
			int32_t length = codec::decode_u32(input_ptr);
			eat(4, input_ptr, input_left);
			if(length < 0) {
				throw std::runtime_error("Malformed NBT: negative byte array length.");
			}
			if(path_ok && path_first == path_last) {
				for(int32_t i = 0; i < length; ++i) {
					input_ptr[i] = sub_table[input_ptr[i]];
				}
			}
			eat(length, input_ptr, input_left);
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
			eat(length, input_ptr, input_left);
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
			int32_t match;
			std::vector<Glib::ustring>::const_iterator new_path_first;
			if(path_ok && path_first != path_last) {
				if(*path_first == u8"*") {
					match = -2;
				} else {
					const std::string &raw = path_first->raw();
					if(raw.size()) {
						char *end;
						unsigned long ul = std::strtoul(raw.data(), &end, 10);
						if(!*end) {
							if(static_cast<uintmax_t>(ul) <= static_cast<uintmax_t>(std::numeric_limits<int32_t>::max())) {
								match = static_cast<int32_t>(ul);
							} else {
								match = -1;
							}
						} else {
							match = -1;
						}
					} else {
						match = -1;
					}
				}
				new_path_first = path_first + 1;
			} else {
				match = -1;
				new_path_first = path_last;
			}
			for(int32_t i = 0; i < length; ++i) {
				handle_content(subtype, input_ptr, input_left, sub_table, new_path_first, path_last, match == -2 || match == i);
			}
			return;
		}

		case nbt::TAG_COMPOUND: {
			for(;;) {
				check_left(1, input_left);
				nbt::tag subtype = static_cast<nbt::tag>(codec::decode_u8(input_ptr));
				eat(1, input_ptr, input_left);
				if(subtype == nbt::TAG_END) {
					return;
				}
				handle_named(subtype, input_ptr, input_left, sub_table, path_first, path_last, path_ok);
			}
		}

		case nbt::TAG_INT_ARRAY: {
			check_left(4, input_left);
			int32_t length = codec::decode_u32(input_ptr);
			eat(4, input_ptr, input_left);
			if(length < 0) {
				throw std::runtime_error("Malformed NBT: negative integer array length.");
			}
			eat(length, input_ptr, input_left);
			eat(length, input_ptr, input_left);
			eat(length, input_ptr, input_left);
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
			eat(length, input_ptr, input_left);
			eat(length, input_ptr, input_left);
			eat(length, input_ptr, input_left);
			eat(length, input_ptr, input_left);
			eat(length, input_ptr, input_left);
			eat(length, input_ptr, input_left);
			eat(length, input_ptr, input_left);
			eat(length, input_ptr, input_left);
			return;
		}
	}

	throw std::runtime_error("Malformed NBT: unrecognized tag.");
}

void handle_named(nbt::tag tag, uint8_t *&input_ptr, std::size_t &input_left, const uint8_t *sub_table, const std::vector<Glib::ustring>::const_iterator path_first, const std::vector<Glib::ustring>::const_iterator path_last, bool path_ok) {
	// Read name length.
	check_left(2, input_left);
	int16_t name_len = codec::decode_u16(input_ptr);
	eat(2, input_ptr, input_left);
	if(name_len < 0) {
		throw std::runtime_error("Malformed NBT: negative name length.");
	}

	// Read name.
	check_left(name_len, input_left);
	Glib::ustring name(std::string(input_ptr, input_ptr + name_len));
	eat(name_len, input_ptr, input_left);

	// Check for match.
	std::vector<Glib::ustring>::const_iterator new_path_first;
	if(path_first != path_last) {
		if(*path_first != u8"*" && *path_first != name) {
			path_ok = false;
		}
		new_path_first = path_first + 1;
	} else {
		path_ok = false;
		new_path_first = path_last;
	}

	// Handle content.
	handle_content(tag, input_ptr, input_left, sub_table, new_path_first, path_last, path_ok);
}

void usage() {
	std::cerr << "Usage:\n";
	std::cerr << appname << " nbt-patch-barray nbtfile barraypath from1 to1 [from2 to2 ...]\n";
	std::cerr << '\n';
	std::cerr << "Patches byte values in byte arrays in an NBT.\n";
	std::cerr << '\n';
	std::cerr << "Arguments:\n";
	std::cerr << "  nbtfile - the NBT file to modify\n";
	std::cerr << "  barraypath - the path of the byte array to patch (see below)\n";
	std::cerr << "  from1 - the first byte value to change to something else (an integer between 0 and 255)\n";
	std::cerr << "  to1 - the value to change bytes equal to \"from1\" to (an integer between 0 and 255)\n";
	std::cerr << '\n';
	std::cerr << "A path is a slash-separated list of path components.\n";
	std::cerr << "Each path component is one of:\n";
	std::cerr << "- A nonnegative integer, which matches the given zero-indexed element of a list,\n";
	std::cerr << "- An arbitrary (possibly-empty) string, which matches the given element of a compound, or\n";
	std::cerr << "- The single character \"*\", which matches any element of a list or compound.\n";
	std::cerr << "For a byte array to be patched, the set of compounds and lists containing it must match the given path.\n";
	std::cerr << "For example, the block array in a chunk NBT has the path \"/Level/Blocks\".\n";
	std::cerr << "Note the leading empty component, reflecting the fact that the root node of the file is named and the name is empty.\n";
}
}
}

int mcwutil::nbt::patch_barray(std::ranges::subrange<char **> args) {
	// Check parameters.
	if(args.size() < 4 || (args.size() % 2) != 0) {
		usage();
		return 1;
	}

	// Build the substitution table.
	uint8_t sub_table[256];
	for(unsigned int i = 0; i < 256; ++i) {
		sub_table[i] = static_cast<uint8_t>(i);
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
		if(from > 255 || to > 255) {
			usage();
			return 1;
		}
		sub_table[from] = static_cast<uint8_t>(to);
	}

	// Build the target path.
	std::vector<Glib::ustring> path_components;
	{
		Glib::ustring current_component;
		for(const char *i = args[1]; *i; ++i) {
			if(*i == '/') {
				path_components.push_back(current_component);
				current_component.clear();
			} else {
				current_component.push_back(*i);
			}
		}
		path_components.push_back(current_component);
	}

	// Open and map NBT file.
	FileDescriptor nbt_fd = FileDescriptor::create_open(args[0], O_RDWR, 0);
	MappedFile nbt_mapped(nbt_fd, PROT_READ | PROT_WRITE);

	// Do the thing.
	uint8_t *input_ptr = static_cast<uint8_t *>(nbt_mapped.data());
	std::size_t input_left = nbt_mapped.size();
	check_left(1, input_left);
	nbt::tag root_tag = static_cast<nbt::tag>(codec::decode_u8(input_ptr));
	eat(1, input_ptr, input_left);
	handle_named(root_tag, input_ptr, input_left, sub_table, path_components.begin(), path_components.end(), true);

	return 0;
}
