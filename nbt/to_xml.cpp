#include "nbt/nbt.h"
#include "nbt/tags.h"
#include "util/codec.h"
#include "util/file_descriptor.h"
#include "util/globals.h"
#include "util/mapped_file.h"
#include "util/string.h"
#include "util/xml.h"
#include <cassert>
#include <fcntl.h>
#include <iostream>
#include <libxml/tree.h>
#include <limits>
#include <locale>
#include <sstream>
#include <stdexcept>
#include <string>

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
void eat(std::size_t n, const uint8_t *&input_ptr, std::size_t &input_left) {
	assert(n <= input_left);
	input_ptr += n;
	input_left -= n;
}

void parse_name_and_data(const uint8_t *&input_ptr, std::size_t &input_left, nbt::tag tag, xmlNode &parent_elt);

/**
 * \brief Converts the content of a data item to XML form and appends it to a
 * parent XML element.
 *
 * \pre \p input_ptr points at the beginning of the encoded contents of a data
 * item.
 *
 * \post \p input_ptr points just past the contents of the data item.
 *
 * \post \p input_left is decremented by the same amount as \p input_ptr is
 * incremented.
 *
 * \param[in, out] input_ptr the data pointer.
 *
 * \param[in, out] input_left the number of bytes remaining.
 *
 * \param[in] tag the data type.
 *
 * \param[out] parent_elt the XML element to which to append a child
 * representing the data item.
 */
void parse_data(const uint8_t *&input_ptr, std::size_t &input_left, nbt::tag tag, xmlNode &parent_elt) {
	switch(tag) {
		case nbt::TAG_END:
			throw std::runtime_error("Malformed NBT: unexpected TAG_END.");

		case nbt::TAG_BYTE: {
			check_left(1, input_left);
			int8_t value = codec::decode_u8(input_ptr);
			eat(1, input_ptr, input_left);
			xmlNode &byte_elt = xml::node_append_child(parent_elt, u8"byte");
			xml::node_attr(byte_elt, u8"value", string::l2u(string::todecs(value)).c_str());
			return;
		}

		case nbt::TAG_SHORT: {
			check_left(2, input_left);
			int16_t value = codec::decode_u16(input_ptr);
			eat(2, input_ptr, input_left);
			xmlNode &short_elt = xml::node_append_child(parent_elt, u8"short");
			xml::node_attr(short_elt, u8"value", string::l2u(string::todecs(value)).c_str());
			return;
		}

		case nbt::TAG_INT: {
			check_left(4, input_left);
			int32_t value = codec::decode_u32(input_ptr);
			eat(4, input_ptr, input_left);
			xmlNode &int_elt = xml::node_append_child(parent_elt, u8"int");
			xml::node_attr(int_elt, u8"value", string::l2u(string::todecs(value)).c_str());
			return;
		}

		case nbt::TAG_LONG: {
			check_left(8, input_left);
			int64_t value = codec::decode_u64(input_ptr);
			eat(8, input_ptr, input_left);
			xmlNode &long_elt = xml::node_append_child(parent_elt, u8"long");
			xml::node_attr(long_elt, u8"value", string::l2u(string::todecs(value)).c_str());
			return;
		}

		case nbt::TAG_FLOAT: {
			check_left(4, input_left);
			float fl = codec::decode_float(input_ptr);
			eat(4, input_ptr, input_left);
			xmlNode &float_elt = xml::node_append_child(parent_elt, u8"float");
			std::ostringstream oss;
			oss.imbue(std::locale("C"));
			oss.flags(std::ios_base::showpoint | std::ios_base::dec | std::ios_base::scientific | std::ios_base::left);
			oss.precision(12);
			oss << fl;
			xml::node_attr(float_elt, u8"value", string::l2u(oss.view()).c_str());
			return;
		}

		case nbt::TAG_DOUBLE: {
			check_left(8, input_left);
			double db = codec::decode_double(input_ptr);
			eat(8, input_ptr, input_left);
			xmlNode &double_elt = xml::node_append_child(parent_elt, u8"double");
			std::ostringstream oss;
			oss.imbue(std::locale("C"));
			oss.flags(std::ios_base::showpoint | std::ios_base::dec | std::ios_base::scientific | std::ios_base::left);
			oss.precision(20);
			oss << db;
			xml::node_attr(double_elt, u8"value", string::l2u(oss.view()).c_str());
			return;
		}

		case nbt::TAG_BYTE_ARRAY: {
			check_left(4, input_left);
			int32_t len = codec::decode_u32(input_ptr);
			eat(4, input_ptr, input_left);
			if(len < 0) {
				throw std::runtime_error("Malformed NBT: negative byte array length.");
			}
			check_left(len, input_left);
			xmlNode &barray_elt = xml::node_append_child(parent_elt, u8"barray");
			std::u8string ustr;
			ustr.reserve(1 + len * 2 + len / 50 + 1);
			ustr.push_back(u8'\n');
			for(int32_t i = 0; i < len; ++i) {
				static const char8_t DIGITS[] = u8"0123456789ABCDEF";
				uint8_t byte = input_ptr[i];
				ustr.push_back(DIGITS[byte >> 4]);
				ustr.push_back(DIGITS[byte & 0x0F]);
				if((i % 50) == 49) {
					ustr.push_back(u8'\n');
				}
			}
			eat(len, input_ptr, input_left);
			ustr.push_back(u8'\n');
			xml::node_append_text(barray_elt, ustr);
			return;
		}

		case nbt::TAG_STRING: {
			check_left(2, input_left);
			int16_t len = codec::decode_u16(input_ptr);
			eat(2, input_ptr, input_left);
			if(len < 0) {
				throw std::runtime_error("Malformed NBT: negative string length.");
			}
			xmlNode &string_elt = xml::node_append_child(parent_elt, u8"string");
			std::u8string value(input_ptr, input_ptr + len);
			xml::node_attr(string_elt, u8"value", value.c_str());
			eat(len, input_ptr, input_left);
			return;
		}

		case nbt::TAG_LIST: {
			check_left(5, input_left);
			nbt::tag subtype = static_cast<nbt::tag>(codec::decode_u8(input_ptr));
			eat(1, input_ptr, input_left);
			int32_t len = codec::decode_u32(input_ptr);
			eat(4, input_ptr, input_left);
			if(len < 0) {
				throw std::runtime_error("Malformed NBT: negative list length.");
			}
			xmlNode &list_elt = xml::node_append_child(parent_elt, u8"list");
			xml::node_attr(list_elt, u8"subtype", string::l2u(string::todecu(subtype)).c_str());
			for(int32_t i = 0; i < len; ++i) {
				parse_data(input_ptr, input_left, subtype, list_elt);
			}
			return;
		}

		case nbt::TAG_COMPOUND: {
			xmlNode &compound_elt = xml::node_append_child(parent_elt, u8"compound");
			for(;;) {
				check_left(1, input_left);
				nbt::tag subtype = static_cast<nbt::tag>(codec::decode_u8(input_ptr));
				eat(1, input_ptr, input_left);
				if(subtype == nbt::TAG_END) {
					break;
				} else {
					parse_name_and_data(input_ptr, input_left, subtype, compound_elt);
				}
			}
			return;
		}

		case nbt::TAG_INT_ARRAY: {
			check_left(4, input_left);
			int32_t len = codec::decode_u32(input_ptr);
			eat(4, input_ptr, input_left);
			if(len < 0) {
				throw std::runtime_error("Malformed NBT: negative integer array length.");
			}
			if(len > std::numeric_limits<int32_t>::max() / 4) {
				throw std::runtime_error("Unsupported NBT feature: integer array length too big.");
			}
			check_left(len, input_left * 4);
			xmlNode &iarray_elt = xml::node_append_child(parent_elt, u8"iarray");
			std::u8string ustr;
			ustr.reserve(1 + len * 8 + len / 10 + 1);
			ustr.push_back(u8'\n');
			for(int32_t i = 0; i < len; ++i) {
				static const char8_t DIGITS[] = u8"0123456789ABCDEF";
				uint32_t integer = codec::decode_u32(input_ptr + i * 4);
				for(int nybble = 7; nybble >= 0; --nybble) {
					ustr.push_back(DIGITS[(integer >> (4 * nybble)) & 0x0F]);
				}
				if((i % 10) == 9) {
					ustr.push_back(u8'\n');
				}
			}
			eat(len * 4, input_ptr, input_left);
			ustr.push_back(u8'\n');
			xml::node_append_text(iarray_elt, ustr);
			return;
		}

		case nbt::TAG_LONG_ARRAY: {
			check_left(4, input_left);
			int32_t len = codec::decode_u32(input_ptr);
			eat(4, input_ptr, input_left);
			if(len < 0) {
				throw std::runtime_error("Malformed NBT: negative long array length.");
			}
			if(len > std::numeric_limits<int32_t>::max() / 8) {
				throw std::runtime_error("Unsupported NBT feature: long array length too big.");
			}
			check_left(len, input_left * 8);
			xmlNode &larray_elt = xml::node_append_child(parent_elt, u8"larray");
			std::u8string ustr;
			ustr.reserve(1 + len * 16 + len / 10 + 1);
			ustr.push_back(u8'\n');
			for(int32_t i = 0; i < len; ++i) {
				static const char8_t DIGITS[] = u8"0123456789ABCDEF";
				uint64_t integer = codec::decode_u64(input_ptr + i * 8);
				for(int nybble = 15; nybble >= 0; --nybble) {
					ustr.push_back(DIGITS[(integer >> (4 * nybble)) & 0x0F]);
				}
				if((i % 10) == 9) {
					ustr.push_back(u8'\n');
				}
			}
			eat(len * 8, input_ptr, input_left);
			ustr.push_back(u8'\n');
			xml::node_append_text(larray_elt, ustr);
			return;
		}
	}

	throw std::runtime_error("Malformed NBT: unrecognized tag.");
}

/**
 * \brief Converts a single key/value pair in a \ref TAG_COMPOUND to an XML \c
 * named element and appends it to a parent XML element.
 *
 * \pre \p input_ptr points at the beginning of the encoded contents of a data
 * item.
 *
 * \post \p input_ptr points just past the contents of the data item.
 *
 * \post \p input_left is decremented by the same amount as \p input_ptr is
 * incremented.
 *
 * \param[in, out] input_ptr the data pointer.
 *
 * \param[in, out] input_left the number of bytes remaining.
 *
 * \param[in] tag the data type.
 *
 * \param[out] parent_elt the XML element to which to append a \c named
 * element for the key/value pair.
 */
void parse_name_and_data(const uint8_t *&input_ptr, std::size_t &input_left, nbt::tag tag, xmlNode &parent_elt) {
	check_left(2, input_left);
	int16_t name_length = codec::decode_u16(input_ptr);
	eat(2, input_ptr, input_left);
	if(name_length < 0) {
		throw std::runtime_error("Malformed NBT: negative element name length.");
	}
	check_left(name_length, input_left);
	std::u8string name(input_ptr, input_ptr + name_length);
	eat(name_length, input_ptr, input_left);
	xmlNode &named_elt = xml::node_append_child(parent_elt, u8"named");
	xml::node_attr(named_elt, u8"name", name.c_str());
	parse_data(input_ptr, input_left, tag, named_elt);
}
}
}

/**
 * \brief Entry point for the \c nbt-to-xml utility.
 *
 * \param[in] args the command-line arguments.
 *
 * \return the application exit code.
 */
int mcwutil::nbt::to_xml(std::span<char *> args) {
	// Check parameters.
	if(args.size() != 2) {
		std::cerr << "Usage:\n";
		std::cerr << appname << " nbt-to-xml nbtfile xmlfile\n";
		std::cerr << '\n';
		std::cerr << "Converts an NBT file into a human-readable and -editable XML file.\n";
		std::cerr << '\n';
		std::cerr << "Arguments:\n";
		std::cerr << "  nbtfile - the NBT file to convert\n";
		std::cerr << "  xmlfile - the XML file to write\n";
		return 1;
	}

	// Open and map NBT file.
	file_descriptor input_fd = file_descriptor::create_open(args[0], O_RDONLY, 0);
	mapped_file input_mapped(input_fd, PROT_READ);

	// Construct document.
	auto nbt_document = xml::empty();
	xml::internal_subset(*nbt_document, u8"minecraft-nbt", nullptr, u8"urn:uuid:25323dd6-2a7d-11e1-96b7-1c4bd68d068e");
	xmlNode &nbt_root_elt = xml::node_create_root(*nbt_document, u8"minecraft-nbt");
	const uint8_t *input_ptr = static_cast<const uint8_t *>(input_mapped.data());
	std::size_t input_left = input_mapped.size();
	check_left(1, input_left);
	nbt::tag outer_tag = static_cast<nbt::tag>(codec::decode_u8(input_ptr));
	eat(1, input_ptr, input_left);
	parse_name_and_data(input_ptr, input_left, outer_tag, nbt_root_elt);

	// Write output file.
	file_descriptor nbt_fd = file_descriptor::create_open(args[1], O_WRONLY | O_CREAT | O_TRUNC, 0666);
	xml::write(*nbt_document, nbt_fd);
	nbt_fd.close();

	return 0;
}
