#include <mcwutil/nbt/nbt.hpp>
#include <mcwutil/nbt/tags.hpp>
#include <mcwutil/util/codec.hpp>
#include <mcwutil/util/file_descriptor.hpp>
#include <mcwutil/util/mapped_file.hpp>
#include <mcwutil/util/string.hpp>
#include <cassert>
#include <cstdlib>
#include <fcntl.h>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <sys/stat.h>
#include <sys/types.h>
#include <vector>

using namespace std::literals::string_view_literals;

namespace mcwutil::nbt {
namespace {
/**
 * \brief verifies that a required number of bytes are available in the nbt
 * data.
 *
 * \param[in] needed the number of bytes needed for the next decoding step.
 *
 * \param[in] left the number of bytes remaining in source data.
 *
 * \exception std::runtime_error if \p left < \p needed.
 */
void check_left(std::size_t needed, std::size_t left) {
	if(left < needed) {
		throw std::runtime_error("Malformed NBT: input truncated.");
	}
}

/**
 * \brief Updates the input pointer and length to consume a specified number of
 * bytes.
 *
 * \pre \p n ≤ \p input_left.
 *
 * \post \p input_ptr is incremented by \p n.
 *
 * \post \p input_left is decremented by \p n.
 *
 * \param[in] n the number of bytes to consume.
 *
 * \param[in, out] input_ptr the data pointer to increment.
 *
 * \param[in, out] input_left the number of bytes remaining, to decrement.
 */
void eat(std::size_t n, uint8_t *&input_ptr, std::size_t &input_left) {
	assert(n <= input_left);
	input_ptr += n;
	input_left -= n;
}

void handle_named(nbt::tag tag, uint8_t *&input_ptr, std::size_t &input_left, const uint8_t *sub_table, const std::vector<std::u8string>::const_iterator path_first, const std::vector<std::u8string>::const_iterator path_last, bool path_ok);

/**
 * \brief Handles the content of a data item.
 *
 * \pre \p input_ptr points at the beginning of the encoded contents of a data
 * item.
 *
 * \post \p input_ptr points just past the contents of the data item.
 *
 * \post \p input_left is decremented by the same amount as \p input_ptr is
 * incremented.
 *
 * \post If appropriate, the data that \p input_ptr advanced over was modified
 * according to \p sub_table.
 *
 * \param[in] tag the data type.
 *
 * \param[in, out] input_ptr the data pointer, through which data is modified
 * in-place.
 *
 * \param[in, out] input_left the number of bytes remaining.
 *
 * \param[in] sub_table the table of byte value substitutions to apply.
 *
 * \param[in] path_first the start of the portion of the user-specified path
 * that has not been matched yet.
 *
 * \param[in] path_last the past-the-end of the user-specified path.
 *
 * \param[in] path_ok if the path to this point matched the user-specified
 * path, so substitution should be applied in this subtree.
 */
void handle_content(nbt::tag tag, uint8_t *&input_ptr, std::size_t &input_left, const uint8_t *sub_table, const std::vector<std::u8string>::const_iterator path_first, const std::vector<std::u8string>::const_iterator path_last, bool path_ok) {
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
			int32_t length = codec::decode_integer<uint32_t>(input_ptr);
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
			int16_t length = codec::decode_integer<uint16_t>(input_ptr);
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
			nbt::tag subtype = static_cast<nbt::tag>(codec::decode_integer<uint8_t>(input_ptr));
			eat(1, input_ptr, input_left);
			int32_t length = codec::decode_integer<uint32_t>(input_ptr);
			eat(4, input_ptr, input_left);
			if(length < 0) {
				throw std::runtime_error("Malformed NBT: negative list length.");
			}
			int32_t match;
			std::vector<std::u8string>::const_iterator new_path_first;
			if(path_ok && path_first != path_last) {
				if(*path_first == u8"*"sv) {
					match = -2;
				} else {
					const std::string &raw = string::u2l(*path_first);
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
				nbt::tag subtype = static_cast<nbt::tag>(codec::decode_integer<uint8_t>(input_ptr));
				eat(1, input_ptr, input_left);
				if(subtype == nbt::TAG_END) {
					return;
				}
				handle_named(subtype, input_ptr, input_left, sub_table, path_first, path_last, path_ok);
			}
		}

		case nbt::TAG_INT_ARRAY: {
			check_left(4, input_left);
			int32_t length = codec::decode_integer<uint32_t>(input_ptr);
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
			int32_t length = codec::decode_integer<uint32_t>(input_ptr);
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

/**
 * \brief Handles a single key/value pair in a \ref TAG_COMPOUND.
 *
 * \pre \p input_ptr points at the beginning of the name portion of the
 * key/value pair, i.e. just past the tag byte.
 *
 * \post \p input_ptr points just past the contents of the key/value pair.
 *
 * \post \p input_left is decremented by the same amount as \p input_ptr is
 * incremented.
 *
 * \post If appropriate, the data that \p input_ptr advanced over was modified
 * according to \p sub_table.
 *
 * \param[in] tag the data type.
 *
 * \param[in, out] input_ptr the data pointer, through which data is modified
 * in-place.
 *
 * \param[in, out] input_left the number of bytes remaining.
 *
 * \param[in] sub_table the table of byte value substitutions to apply.
 *
 * \param[in] path_first the start of the portion of the user-specified path
 * that has not been matched yet.
 *
 * \param[in] path_last the past-the-end of the user-specified path.
 *
 * \param[in] path_ok if the path to this point matched the user-specified
 * path, so substitution should be applied in this subtree.
 */
void handle_named(nbt::tag tag, uint8_t *&input_ptr, std::size_t &input_left, const uint8_t *sub_table, const std::vector<std::u8string>::const_iterator path_first, const std::vector<std::u8string>::const_iterator path_last, bool path_ok) {
	// Read name length.
	check_left(2, input_left);
	int16_t name_len = codec::decode_integer<uint16_t>(input_ptr);
	eat(2, input_ptr, input_left);
	if(name_len < 0) {
		throw std::runtime_error("Malformed NBT: negative name length.");
	}

	// Read name.
	check_left(name_len, input_left);
	std::u8string_view name(reinterpret_cast<const char8_t *>(input_ptr), name_len);
	eat(name_len, input_ptr, input_left);

	// Check for match.
	std::vector<std::u8string>::const_iterator new_path_first;
	if(path_first != path_last) {
		if(*path_first != u8"*"sv && *path_first != name) {
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

/**
 * \brief Displays the usage help text.
 *
 * \param[in] appname The name of the application.
 */
void usage(std::string_view appname) {
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

/**
 * \brief Entry point for the \c nbt-patch-barray utility.
 *
 * \param[in] appname The name of the application.
 *
 * \param[in] args the command-line arguments.
 *
 * \return the application exit code.
 */
int mcwutil::nbt::patch_barray(std::string_view appname, std::span<char *> args) {
	// Check parameters.
	if(args.size() < 4 || (args.size() % 2) != 0) {
		usage(appname);
		return 1;
	}

	// Build the substitution table.
	uint8_t sub_table[256];
	for(unsigned int i = 0; i < 256; ++i) {
		sub_table[i] = static_cast<uint8_t>(i);
	}
	for(std::size_t i = 2; i < args.size(); i += 2) {
		if(!args[i][0] || !args[i + 1][0]) {
			usage(appname);
			return 1;
		}
		unsigned long from, to;
		{
			char *end;
			from = std::strtoul(args[i], &end, 10);
			if(*end) {
				usage(appname);
				return 1;
			}
		}
		{
			char *end;
			to = std::strtoul(args[i + 1], &end, 10);
			if(*end) {
				usage(appname);
				return 1;
			}
		}
		if(from > 255 || to > 255) {
			usage(appname);
			return 1;
		}
		sub_table[from] = static_cast<uint8_t>(to);
	}

	// Build the target path.
	std::vector<std::u8string> path_components;
	{
		std::u8string path = string::l2u(args[1]);
		std::u8string current_component;
		for(const char8_t i : path) {
			if(i == u8'/') {
				path_components.push_back(current_component);
				current_component.clear();
			} else {
				current_component.push_back(i);
			}
		}
		path_components.push_back(current_component);
	}

	// Open and map NBT file.
	file_descriptor nbt_fd = file_descriptor::create_open(args[0], O_RDWR, 0);
	mapped_file nbt_mapped(nbt_fd, PROT_READ | PROT_WRITE);
	static_assert(sizeof(uint8_t) == 1, "This code assumes 8-bit bytes in mapped files.");

	// Do the thing.
	uint8_t *input_ptr = static_cast<uint8_t *>(nbt_mapped.data());
	std::size_t input_left = nbt_mapped.size();
	check_left(1, input_left);
	nbt::tag root_tag = static_cast<nbt::tag>(codec::decode_integer<uint8_t>(input_ptr));
	eat(1, input_ptr, input_left);
	handle_named(root_tag, input_ptr, input_left, sub_table, path_components.begin(), path_components.end(), true);

	return 0;
}
