#include "nbt/nbt.h"
#include "nbt/tags.h"
#include "util/codec.h"
#include "util/exception.h"
#include "util/fd.h"
#include "util/file_utils.h"
#include "util/globals.h"
#include "util/mapped_file.h"
#include "util/string.h"
#include <cassert>
#include <fcntl.h>
#include <glibmm/ustring.h>
#include <iostream>
#include <libxml++/document.h>
#include <libxml++/nodes/element.h>
#include <limits>
#include <locale>
#include <sstream>
#include <string>
#include <vector>

namespace {
void check_left(std::size_t needed, std::size_t left) {
	if(left < needed) {
		throw std::runtime_error("Malformed NBT: input truncated.");
	}
}

void eat(std::size_t n, const uint8_t *&input_ptr, std::size_t &input_left) {
	assert(n <= input_left);
	input_ptr += n;
	input_left -= n;
}

void parse_name_and_data(const uint8_t *&input_ptr, std::size_t &input_left, NBT::Tag tag, xmlpp::Element *parent_elt);

void parse_data(const uint8_t *&input_ptr, std::size_t &input_left, NBT::Tag tag, xmlpp::Element *parent_elt) {
	switch(tag) {
		case NBT::TAG_END:
			throw std::runtime_error("Malformed NBT: unexpected TAG_END.");

		case NBT::TAG_BYTE: {
			check_left(1, input_left);
			int8_t value = decode_u8(input_ptr);
			eat(1, input_ptr, input_left);
			xmlpp::Element *byte_elt = parent_elt->add_child(utf8_literal(u8"byte"));
			byte_elt->set_attribute(utf8_literal(u8"value"), todecs(value));
			return;
		}

		case NBT::TAG_SHORT: {
			check_left(2, input_left);
			int16_t value = decode_u16(input_ptr);
			eat(2, input_ptr, input_left);
			xmlpp::Element *short_elt = parent_elt->add_child(utf8_literal(u8"short"));
			short_elt->set_attribute(utf8_literal(u8"value"), todecs(value));
			return;
		}

		case NBT::TAG_INT: {
			check_left(4, input_left);
			int32_t value = decode_u32(input_ptr);
			eat(4, input_ptr, input_left);
			xmlpp::Element *int_elt = parent_elt->add_child(utf8_literal(u8"int"));
			int_elt->set_attribute(utf8_literal(u8"value"), todecs(value));
			return;
		}

		case NBT::TAG_LONG: {
			check_left(8, input_left);
			int64_t value = decode_u64(input_ptr);
			eat(8, input_ptr, input_left);
			xmlpp::Element *long_elt = parent_elt->add_child(utf8_literal(u8"long"));
			long_elt->set_attribute(utf8_literal(u8"value"), todecs(value));
			return;
		}

		case NBT::TAG_FLOAT: {
			check_left(4, input_left);
			uint32_t raw = decode_u32(input_ptr);
			eat(4, input_ptr, input_left);
			float fl = decode_u32_to_float(raw);
			xmlpp::Element *float_elt = parent_elt->add_child(utf8_literal(u8"float"));
			std::wostringstream oss;
			oss.imbue(std::locale("C"));
			oss.flags(std::ios_base::showpoint | std::ios_base::dec | std::ios_base::scientific | std::ios_base::left);
			oss.precision(12);
			oss << fl;
			float_elt->set_attribute(utf8_literal(u8"value"), wstring2ustring(oss.str()));
			return;
		}

		case NBT::TAG_DOUBLE: {
			check_left(8, input_left);
			uint64_t raw = decode_u64(input_ptr);
			eat(8, input_ptr, input_left);
			double db = decode_u64_to_double(raw);
			xmlpp::Element *double_elt = parent_elt->add_child(utf8_literal(u8"double"));
			std::wostringstream oss;
			oss.imbue(std::locale("C"));
			oss.flags(std::ios_base::showpoint | std::ios_base::dec | std::ios_base::scientific | std::ios_base::left);
			oss.precision(20);
			oss << db;
			double_elt->set_attribute(utf8_literal(u8"value"), wstring2ustring(oss.str()));
			return;
		}

		case NBT::TAG_BYTE_ARRAY: {
			check_left(4, input_left);
			int32_t len = decode_u32(input_ptr);
			eat(4, input_ptr, input_left);
			if(len < 0) {
				throw std::runtime_error("Malformed NBT: negative byte array length.");
			}
			check_left(len, input_left);
			xmlpp::Element *barray_elt = parent_elt->add_child(utf8_literal(u8"barray"));
			Glib::ustring ustr;
			ustr.reserve(1 + len * 2 + len / 50 + 1);
			ustr.append(utf8_literal(u8"\n"));
			for(int32_t i = 0; i < len; ++i) {
				static const char8_t DIGITS[] = u8"0123456789ABCDEF";
				uint8_t byte = input_ptr[i];
				ustr.push_back(static_cast<char>(DIGITS[byte >> 4]));
				ustr.push_back(static_cast<char>(DIGITS[byte & 0x0F]));
				if((i % 50) == 49) {
					ustr.append(utf8_literal(u8"\n"));
				}
			}
			eat(len, input_ptr, input_left);
			ustr.append(utf8_literal(u8"\n"));
			barray_elt->add_child_text(ustr);
			return;
		}

		case NBT::TAG_STRING: {
			check_left(2, input_left);
			int16_t len = decode_u16(input_ptr);
			eat(2, input_ptr, input_left);
			if(len < 0) {
				throw std::runtime_error("Malformed NBT: negative string length.");
			}
			xmlpp::Element *string_elt = parent_elt->add_child(utf8_literal(u8"string"));
			string_elt->set_attribute(utf8_literal(u8"value"), Glib::ustring(std::string(input_ptr, input_ptr + len)));
			eat(len, input_ptr, input_left);
			return;
		}

		case NBT::TAG_LIST: {
			check_left(5, input_left);
			NBT::Tag subtype = static_cast<NBT::Tag>(decode_u8(input_ptr));
			eat(1, input_ptr, input_left);
			int32_t len = decode_u32(input_ptr);
			eat(4, input_ptr, input_left);
			if(len < 0) {
				throw std::runtime_error("Malformed NBT: negative list length.");
			}
			xmlpp::Element *list_elt = parent_elt->add_child(utf8_literal(u8"list"));
			list_elt->set_attribute(utf8_literal(u8"subtype"), todecu(subtype));
			for(int32_t i = 0; i < len; ++i) {
				parse_data(input_ptr, input_left, subtype, list_elt);
			}
			return;
		}

		case NBT::TAG_COMPOUND: {
			xmlpp::Element *compound_elt = parent_elt->add_child(utf8_literal(u8"compound"));
			for(;;) {
				check_left(1, input_left);
				NBT::Tag subtype = static_cast<NBT::Tag>(decode_u8(input_ptr));
				eat(1, input_ptr, input_left);
				if(subtype == NBT::TAG_END) {
					break;
				} else {
					parse_name_and_data(input_ptr, input_left, subtype, compound_elt);
				}
			}
			return;
		}

		case NBT::TAG_INT_ARRAY: {
			check_left(4, input_left);
			int32_t len = decode_u32(input_ptr);
			eat(4, input_ptr, input_left);
			if(len < 0) {
				throw std::runtime_error("Malformed NBT: negative integer array length.");
			}
			if(len > std::numeric_limits<int32_t>::max() / 4) {
				throw std::runtime_error("Unsupported NBT feature: integer array length too big.");
			}
			check_left(len, input_left * 4);
			xmlpp::Element *iarray_elt = parent_elt->add_child(utf8_literal(u8"iarray"));
			Glib::ustring ustr;
			ustr.reserve(1 + len * 8 + len / 10 + 1);
			ustr.append(utf8_literal(u8"\n"));
			for(int32_t i = 0; i < len; ++i) {
				static const char8_t DIGITS[] = u8"0123456789ABCDEF";
				uint32_t integer = decode_u32(input_ptr + i * 4);
				for(int nybble = 7; nybble >= 0; --nybble) {
					ustr.push_back(static_cast<char>(DIGITS[(integer >> (4 * nybble)) & 0x0F]));
				}
				if((i % 10) == 9) {
					ustr.append(utf8_literal(u8"\n"));
				}
			}
			eat(len * 4, input_ptr, input_left);
			ustr.append(utf8_literal(u8"\n"));
			iarray_elt->add_child_text(ustr);
			return;
		}

		case NBT::TAG_LONG_ARRAY: {
			check_left(4, input_left);
			int32_t len = decode_u32(input_ptr);
			eat(4, input_ptr, input_left);
			if(len < 0) {
				throw std::runtime_error("Malformed NBT: negative long array length.");
			}
			if(len > std::numeric_limits<int32_t>::max() / 8) {
				throw std::runtime_error("Unsupported NBT feature: long array length too big.");
			}
			check_left(len, input_left * 8);
			xmlpp::Element *larray_elt = parent_elt->add_child(utf8_literal(u8"larray"));
			Glib::ustring ustr;
			ustr.reserve(1 + len * 16 + len / 10 + 1);
			ustr.append(utf8_literal(u8"\n"));
			for(int32_t i = 0; i < len; ++i) {
				static const char8_t DIGITS[] = u8"0123456789ABCDEF";
				uint64_t integer = decode_u64(input_ptr + i * 8);
				for(int nybble = 15; nybble >= 0; --nybble) {
					ustr.push_back(static_cast<char>(DIGITS[(integer >> (4 * nybble)) & 0x0F]));
				}
				if((i % 10) == 9) {
					ustr.append(utf8_literal(u8"\n"));
				}
			}
			eat(len * 8, input_ptr, input_left);
			ustr.append(utf8_literal(u8"\n"));
			larray_elt->add_child_text(ustr);
			return;
		}
	}

	throw std::runtime_error("Malformed NBT: unrecognized tag.");
}

void parse_name_and_data(const uint8_t *&input_ptr, std::size_t &input_left, NBT::Tag tag, xmlpp::Element *parent_elt) {
	check_left(2, input_left);
	int16_t name_length = decode_u16(input_ptr);
	eat(2, input_ptr, input_left);
	if(name_length < 0) {
		throw std::runtime_error("Malformed NBT: negative element name length.");
	}
	check_left(name_length, input_left);
	Glib::ustring name(std::string(input_ptr, input_ptr + name_length));
	eat(name_length, input_ptr, input_left);
	xmlpp::Element *named_elt = parent_elt->add_child(utf8_literal(u8"named"));
	named_elt->set_attribute(utf8_literal(u8"name"), name);
	parse_data(input_ptr, input_left, tag, named_elt);
}
}

int NBT::to_xml(const std::vector<std::string> &args) {
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
	FileDescriptor input_fd = FileDescriptor::create_open(args[0].c_str(), O_RDONLY, 0);
	MappedFile input_mapped(input_fd, PROT_READ);

	// Construct document.
	xmlpp::Document nbt_document;
	nbt_document.set_internal_subset(utf8_literal(u8"minecraft-nbt"), utf8_literal(u8""), utf8_literal(u8"urn:uuid:25323dd6-2a7d-11e1-96b7-1c4bd68d068e"));
	xmlpp::Element *nbt_root_elt = nbt_document.create_root_node(utf8_literal(u8"minecraft-nbt"));
	const uint8_t *input_ptr = static_cast<const uint8_t *>(input_mapped.data());
	std::size_t input_left = input_mapped.size();
	check_left(1, input_left);
	NBT::Tag outer_tag = static_cast<NBT::Tag>(decode_u8(input_ptr));
	eat(1, input_ptr, input_left);
	parse_name_and_data(input_ptr, input_left, outer_tag, nbt_root_elt);

	// Write output file.
	nbt_document.write_to_file_formatted(args[1].c_str());

	return 0;
}
