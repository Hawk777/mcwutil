#include "nbt/nbt.h"
#include "nbt/tags.h"
#include "util/codec.h"
#include "util/exception.h"
#include "util/fd.h"
#include "util/file_utils.h"
#include "util/globals.h"
#include "util/string.h"
#include <algorithm>
#include <fcntl.h>
#include <glibmm/ustring.h>
#include <iostream>
#include <libxml++/attribute.h>
#include <libxml++/document.h>
#include <libxml++/nodes/element.h>
#include <libxml++/nodes/node.h>
#include <libxml++/nodes/textnode.h>
#include <libxml++/parsers/domparser.h>
#include <limits>
#include <locale>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <vector>

namespace {
NBT::Tag tag_for_child_of_named_or_list(const Glib::ustring &name, const char *message) {
	if(name == u8"byte")
		return NBT::TAG_BYTE;
	if(name == u8"short")
		return NBT::TAG_SHORT;
	if(name == u8"int")
		return NBT::TAG_INT;
	if(name == u8"long")
		return NBT::TAG_LONG;
	if(name == u8"float")
		return NBT::TAG_FLOAT;
	if(name == u8"double")
		return NBT::TAG_DOUBLE;
	if(name == u8"barray")
		return NBT::TAG_BYTE_ARRAY;
	if(name == u8"string")
		return NBT::TAG_STRING;
	if(name == u8"list")
		return NBT::TAG_LIST;
	if(name == u8"compound")
		return NBT::TAG_COMPOUND;
	if(name == u8"iarray")
		return NBT::TAG_INT_ARRAY;
	if(name == u8"larray")
		return NBT::TAG_LONG_ARRAY;
	throw std::runtime_error(message);
}

NBT::Tag tag_for_child_of_named(const Glib::ustring &name) {
	return tag_for_child_of_named_or_list(name, "Malformed NBT XML: child of named must be one of (byte|short|int|long|float|double|barray|string|list|compound|iarray).");
}

NBT::Tag tag_for_child_of_list(const Glib::ustring &name) {
	return tag_for_child_of_named_or_list(name, "Malformed NBT XML: child of list must be one of (byte|short|int|long|float|double|barray|string|list|compound|iarray).");
}

void check_list_subtype(NBT::Tag subtype) {
	switch(subtype) {
		case NBT::TAG_END:
		case NBT::TAG_BYTE:
		case NBT::TAG_SHORT:
		case NBT::TAG_INT:
		case NBT::TAG_LONG:
		case NBT::TAG_FLOAT:
		case NBT::TAG_DOUBLE:
		case NBT::TAG_BYTE_ARRAY:
		case NBT::TAG_STRING:
		case NBT::TAG_LIST:
		case NBT::TAG_COMPOUND:
		case NBT::TAG_INT_ARRAY:
		case NBT::TAG_LONG_ARRAY:
			return;
	}
	throw std::runtime_error("Malformed NBT XML: list has bad subtype.");
}

void write_nbt(const FileDescriptor &nbt_fd, const xmlpp::Element *elt) {
	if(elt->get_name() == u8"named") {
		const xmlpp::Node::NodeList &children = elt->get_children();
		const xmlpp::Element *relevant_child = 0;
		NBT::Tag subtype = NBT::TAG_END;
		for(auto i = children.begin(), iend = children.end(); i != iend; ++i) {
			const xmlpp::Element *elt = dynamic_cast<const xmlpp::Element *>(*i);
			if(elt) {
				subtype = tag_for_child_of_named(elt->get_name());
				if(relevant_child) {
					throw std::runtime_error("Malformed NBT XML: named must have only one child.");
				}
				relevant_child = elt;
			}
		}
		if(!relevant_child) {
			throw std::runtime_error("Malformed NBT XML: named must have a child.");
		}
		const xmlpp::Attribute *name_attr = elt->get_attribute(utf8_literal(u8"name"));
		if(!name_attr) {
			throw std::runtime_error("Malformed NBT XML: named must have a name.");
		}
		const std::string name_utf8 = name_attr->get_value().raw();
		if(name_utf8.size() > static_cast<std::size_t>(std::numeric_limits<int16_t>::max())) {
			throw std::runtime_error("Malformed NBT XML: name too long.");
		}
		uint8_t header[3];
		encode_u8(&header[0], subtype);
		encode_u16(&header[1], static_cast<int16_t>(name_utf8.size()));
		FileUtils::write(nbt_fd, header, sizeof(header));
		FileUtils::write(nbt_fd, name_utf8.data(), name_utf8.size());
		write_nbt(nbt_fd, relevant_child);
	} else if(elt->get_name() == u8"byte") {
		const xmlpp::Attribute *value_attr = elt->get_attribute(utf8_literal(u8"value"));
		if(!value_attr) {
			throw std::runtime_error("Malformed NBT XML: byte must have a value.");
		}
		std::wistringstream iss(ustring2wstring(value_attr->get_value()));
		iss.imbue(std::locale("C"));
		int value;
		iss >> value;
		if(value < std::numeric_limits<int8_t>::min() || value > std::numeric_limits<int8_t>::max()) {
			throw std::runtime_error("Malformed NBT XML: byte value out of range.");
		}
		uint8_t buffer[1];
		encode_u8(&buffer[0], static_cast<int8_t>(value));
		FileUtils::write(nbt_fd, buffer, sizeof(buffer));
	} else if(elt->get_name() == u8"short") {
		const xmlpp::Attribute *value_attr = elt->get_attribute(utf8_literal(u8"value"));
		if(!value_attr) {
			throw std::runtime_error("Malformed NBT XML: short must have a value.");
		}
		std::wistringstream iss(ustring2wstring(value_attr->get_value()));
		iss.imbue(std::locale("C"));
		int16_t value;
		iss >> value;
		uint8_t buffer[sizeof(value)];
		encode_u16(&buffer[0], value);
		FileUtils::write(nbt_fd, buffer, sizeof(buffer));
	} else if(elt->get_name() == u8"int") {
		const xmlpp::Attribute *value_attr = elt->get_attribute(utf8_literal(u8"value"));
		if(!value_attr) {
			throw std::runtime_error("Malformed NBT XML: int must have a value.");
		}
		std::wistringstream iss(ustring2wstring(value_attr->get_value()));
		iss.imbue(std::locale("C"));
		int32_t value;
		iss >> value;
		uint8_t buffer[sizeof(value)];
		encode_u32(&buffer[0], value);
		FileUtils::write(nbt_fd, buffer, sizeof(buffer));
	} else if(elt->get_name() == u8"long") {
		const xmlpp::Attribute *value_attr = elt->get_attribute(utf8_literal(u8"value"));
		if(!value_attr) {
			throw std::runtime_error("Malformed NBT XML: long must have a value.");
		}
		std::wistringstream iss(ustring2wstring(value_attr->get_value()));
		iss.imbue(std::locale("C"));
		int64_t value;
		iss >> value;
		uint8_t buffer[sizeof(value)];
		encode_u64(&buffer[0], value);
		FileUtils::write(nbt_fd, buffer, sizeof(buffer));
	} else if(elt->get_name() == u8"float") {
		const xmlpp::Attribute *value_attr = elt->get_attribute(utf8_literal(u8"value"));
		if(!value_attr) {
			throw std::runtime_error("Malformed NBT XML: float must have a value.");
		}
		std::wistringstream iss(ustring2wstring(value_attr->get_value()));
		iss.imbue(std::locale("C"));
		float value;
		iss >> value;
		uint32_t raw = encode_float_to_u32(value);
		uint8_t buffer[sizeof(raw)];
		encode_u32(&buffer[0], raw);
		FileUtils::write(nbt_fd, buffer, sizeof(buffer));
	} else if(elt->get_name() == u8"double") {
		const xmlpp::Attribute *value_attr = elt->get_attribute(utf8_literal(u8"value"));
		if(!value_attr) {
			throw std::runtime_error("Malformed NBT XML: float must have a value.");
		}
		std::wistringstream iss(ustring2wstring(value_attr->get_value()));
		iss.imbue(std::locale("C"));
		double value;
		iss >> value;
		uint64_t raw = encode_double_to_u64(value);
		uint8_t buffer[sizeof(raw)];
		encode_u64(&buffer[0], raw);
		FileUtils::write(nbt_fd, buffer, sizeof(buffer));
	} else if(elt->get_name() == u8"barray") {
		const xmlpp::TextNode *text_node = elt->get_child_text();
		if(text_node) {
			static const char8_t DIGITS[] = u8"0123456789ABCDEF";
			const Glib::ustring &value = text_node->get_content();
			Glib::ustring filtered_value;
			filtered_value.reserve(value.size());
			for(auto i = value.begin(), iend = value.end(); i != iend; ++i) {
				static const char8_t SPACES[] = u8" \t\n\r";
				if(std::find(DIGITS, DIGITS + sizeof(DIGITS), *i) != DIGITS + sizeof(DIGITS)) {
					filtered_value.push_back(*i);
				} else if(std::find(SPACES, SPACES + sizeof(SPACES), *i) == SPACES + sizeof(SPACES)) {
					throw std::runtime_error("Malformed NBT XML: non-hex, non-whitespace character in barray.");
				}
			}
			if(filtered_value.size() % 2) {
				throw std::runtime_error("Malformed NBT XML: odd number of hex digits in barray.");
			}
			std::vector<uint8_t> data;
			data.reserve(filtered_value.size() / 2);
			{
				auto i = filtered_value.begin(), iend = filtered_value.end();
				while(i != iend) {
					uint8_t byte = 0;
					for(unsigned int j = 0; j < 2; ++j) {
						byte = static_cast<uint8_t>(byte << 4);
						byte = static_cast<uint8_t>(byte | (std::find(DIGITS, DIGITS + sizeof(DIGITS), *i) - DIGITS));
						++i;
					}
					data.push_back(byte);
				}
			}
			if(static_cast<uintmax_t>(data.size()) > static_cast<uintmax_t>(std::numeric_limits<int32_t>::max())) {
				throw std::runtime_error("Malformed NBT XML: byte array too long.");
			}
			uint8_t header[4];
			encode_u32(&header[0], static_cast<int32_t>(data.size()));
			FileUtils::write(nbt_fd, header, sizeof(header));
			FileUtils::write(nbt_fd, &data[0], data.size());
		} else {
			uint8_t header[4];
			encode_u32(&header[0], 0);
			FileUtils::write(nbt_fd, header, sizeof(header));
		}
	} else if(elt->get_name() == u8"string") {
		const xmlpp::Attribute *value_attr = elt->get_attribute(utf8_literal(u8"value"));
		if(!value_attr) {
			throw std::runtime_error("Malformed NBT XML: string must have a value.");
		}
		const std::string value_utf8 = value_attr->get_value().raw();
		if(static_cast<uintmax_t>(value_utf8.size()) > static_cast<uintmax_t>(std::numeric_limits<int16_t>::max())) {
			throw std::runtime_error("Malformed NBT XML: string too long.");
		}
		uint8_t header[2];
		encode_u16(&header[0], static_cast<int16_t>(value_utf8.size()));
		FileUtils::write(nbt_fd, header, sizeof(header));
		FileUtils::write(nbt_fd, value_utf8.data(), value_utf8.size());
	} else if(elt->get_name() == u8"list") {
		const xmlpp::Attribute *subtype_attr = elt->get_attribute(utf8_literal(u8"subtype"));
		if(!subtype_attr) {
			throw std::runtime_error("Malformed NBT XML: list must have a subtype.");
		}
		std::wistringstream iss(ustring2wstring(subtype_attr->get_value()));
		iss.imbue(std::locale("C"));
		unsigned int subtype_int;
		iss >> subtype_int;
		NBT::Tag subtype = static_cast<NBT::Tag>(subtype_int);
		check_list_subtype(subtype);
		std::vector<const xmlpp::Element *> child_elts;
		const xmlpp::Node::NodeList &children = elt->get_children();
		for(auto i = children.begin(), iend = children.end(); i != iend; ++i) {
			const xmlpp::Element *child_elt = dynamic_cast<const xmlpp::Element *>(*i);
			if(child_elt) {
				if(tag_for_child_of_list(child_elt->get_name()) != subtype) {
					throw std::runtime_error("Malformed NBT XML: child of list does not match subtype specification.");
				}
				child_elts.push_back(child_elt);
			}
		}
		if(static_cast<uintmax_t>(child_elts.size()) > static_cast<uintmax_t>(std::numeric_limits<int32_t>::max())) {
			throw std::runtime_error("Malformed NBT XML: list too long.");
		}
		uint8_t header[5];
		encode_u8(&header[0], static_cast<int8_t>(subtype));
		encode_u32(&header[1], static_cast<int32_t>(child_elts.size()));
		FileUtils::write(nbt_fd, header, sizeof(header));
		for(auto i = child_elts.begin(), iend = child_elts.end(); i != iend; ++i) {
			write_nbt(nbt_fd, *i);
		}
	} else if(elt->get_name() == u8"compound") {
		const xmlpp::Node::NodeList &children = elt->get_children();
		for(auto i = children.begin(), iend = children.end(); i != iend; ++i) {
			const xmlpp::Element *child_elt = dynamic_cast<const xmlpp::Element *>(*i);
			if(child_elt) {
				if(child_elt->get_name() != u8"named") {
					throw std::runtime_error("Malformed NBT XML: child of compound is not named.");
				}
				write_nbt(nbt_fd, child_elt);
			}
		}
		uint8_t footer[1];
		encode_u8(&footer[0], NBT::TAG_END);
		FileUtils::write(nbt_fd, footer, sizeof(footer));
	} else if(elt->get_name() == u8"iarray") {
		const xmlpp::TextNode *text_node = elt->get_child_text();
		if(text_node) {
			static const char8_t DIGITS[] = u8"0123456789ABCDEF";
			const Glib::ustring &value = text_node->get_content();
			Glib::ustring filtered_value;
			filtered_value.reserve(value.size());
			for(auto i = value.begin(), iend = value.end(); i != iend; ++i) {
				static const char8_t SPACES[] = u8" \t\n\r";
				if(std::find(DIGITS, DIGITS + sizeof(DIGITS), *i) != DIGITS + sizeof(DIGITS)) {
					filtered_value.push_back(*i);
				} else if(std::find(SPACES, SPACES + sizeof(SPACES), *i) == SPACES + sizeof(SPACES)) {
					throw std::runtime_error("Malformed NBT XML: non-hex, non-whitespace character in iarray.");
				}
			}
			if(filtered_value.size() % 8) {
				throw std::runtime_error("Malformed NBT XML: number of hex digits in iarray is not a multiple of eight.");
			}
			std::vector<uint8_t> data;
			data.reserve(filtered_value.size() / 2);
			{
				auto i = filtered_value.begin(), iend = filtered_value.end();
				while(i != iend) {
					uint32_t integer = 0;
					for(unsigned int j = 0; j < 8; ++j) {
						integer = static_cast<uint32_t>(integer << 4);
						integer = static_cast<uint32_t>(integer | (std::find(DIGITS, DIGITS + sizeof(DIGITS), *i) - DIGITS));
						++i;
					}
					data.push_back(static_cast<uint8_t>(integer >> 24));
					data.push_back(static_cast<uint8_t>(integer >> 16));
					data.push_back(static_cast<uint8_t>(integer >> 8));
					data.push_back(static_cast<uint8_t>(integer));
				}
			}
			if(static_cast<uintmax_t>(data.size() / 4) > static_cast<uintmax_t>(std::numeric_limits<int32_t>::max())) {
				throw std::runtime_error("Malformed NBT XML: integer array too long.");
			}
			uint8_t header[4];
			encode_u32(&header[0], static_cast<int32_t>(data.size() / 4));
			FileUtils::write(nbt_fd, header, sizeof(header));
			FileUtils::write(nbt_fd, &data[0], data.size());
		} else {
			uint8_t header[4];
			encode_u32(&header[0], 0);
			FileUtils::write(nbt_fd, header, sizeof(header));
		}
	} else if(elt->get_name() == u8"larray") {
		const xmlpp::TextNode *text_node = elt->get_child_text();
		if(text_node) {
			static const char8_t DIGITS[] = u8"0123456789ABCDEF";
			const Glib::ustring &value = text_node->get_content();
			Glib::ustring filtered_value;
			filtered_value.reserve(value.size());
			for(auto i = value.begin(), iend = value.end(); i != iend; ++i) {
				static const char8_t SPACES[] = u8" \t\n\r";
				if(std::find(DIGITS, DIGITS + sizeof(DIGITS), *i) != DIGITS + sizeof(DIGITS)) {
					filtered_value.push_back(*i);
				} else if(std::find(SPACES, SPACES + sizeof(SPACES), *i) == SPACES + sizeof(SPACES)) {
					throw std::runtime_error("Malformed NBT XML: non-hex, non-whitespace character in iarray.");
				}
			}
			if(filtered_value.size() % 16) {
				throw std::runtime_error("Malformed NBT XML: number of hex digits in larray is not a multiple of 16.");
			}
			std::vector<uint8_t> data;
			data.reserve(filtered_value.size() / 2);
			{
				auto i = filtered_value.begin(), iend = filtered_value.end();
				while(i != iend) {
					uint64_t integer = 0;
					for(unsigned int j = 0; j < 16; ++j) {
						integer = static_cast<uint64_t>(integer << 4);
						integer = static_cast<uint64_t>(integer | (std::find(DIGITS, DIGITS + sizeof(DIGITS), *i) - DIGITS));
						++i;
					}
					data.push_back(static_cast<uint8_t>(integer >> 56));
					data.push_back(static_cast<uint8_t>(integer >> 48));
					data.push_back(static_cast<uint8_t>(integer >> 40));
					data.push_back(static_cast<uint8_t>(integer >> 32));
					data.push_back(static_cast<uint8_t>(integer >> 24));
					data.push_back(static_cast<uint8_t>(integer >> 16));
					data.push_back(static_cast<uint8_t>(integer >> 8));
					data.push_back(static_cast<uint8_t>(integer));
				}
			}
			if(static_cast<uintmax_t>(data.size() / 8) > static_cast<uintmax_t>(std::numeric_limits<int32_t>::max())) {
				throw std::runtime_error("Malformed NBT XML: integer array too long.");
			}
			uint8_t header[4];
			encode_u32(&header[0], static_cast<int32_t>(data.size() / 8));
			FileUtils::write(nbt_fd, header, sizeof(header));
			FileUtils::write(nbt_fd, &data[0], data.size());
		} else {
			uint8_t header[4];
			encode_u32(&header[0], 0);
			FileUtils::write(nbt_fd, header, sizeof(header));
		}
	} else {
		throw std::runtime_error("Malformed NBT XML: unrecognized element.");
	}
}

void write_nbt(const FileDescriptor &nbt_fd, const xmlpp::Document *doc) {
	const xmlpp::Element *root = doc->get_root_node();
	if(root->get_name() != u8"minecraft-nbt") {
		throw std::runtime_error("Malformed NBT XML: improper root node name.");
	}

	const xmlpp::Node::NodeList &children = root->get_children();
	const xmlpp::Element *named_elt = 0;
	for(auto i = children.begin(), iend = children.end(); i != iend; ++i) {
		const xmlpp::Element *elt = dynamic_cast<const xmlpp::Element *>(*i);
		if(elt) {
			if(elt->get_name() != u8"named") {
				throw std::runtime_error("Malformed NBT XML: top-level element must be named.");
			}
			if(named_elt) {
				throw std::runtime_error("Malformed NBT XML: must be exactly one top-level element.");
			}
			named_elt = elt;
		}
	}
	if(!named_elt) {
		throw std::runtime_error("Malformed NBT XML: top-level element must exist.");
	}
	write_nbt(nbt_fd, named_elt);
}
}

int NBT::from_xml(const std::vector<std::string> &args) {
	// Check parameters.
	if(args.size() != 2) {
		std::cerr << "Usage:\n";
		std::cerr << appname << " nbt-from-xml xmlfile nbtfile\n";
		std::cerr << '\n';
		std::cerr << "Converts a human-readable and -editable XML file into an NBT file.\n";
		std::cerr << '\n';
		std::cerr << "Arguments:\n";
		std::cerr << "  xmlfile - the XML file to convert\n";
		std::cerr << "  nbtfile - the NBT file to write\n";
		return 1;
	}

	// Read input file.
	xmlpp::DomParser parser;
	parser.set_substitute_entities();
	parser.parse_file(args[0]);

	// Write output file.
	FileDescriptor nbt_fd = FileDescriptor::create_open(args[1].c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
	write_nbt(nbt_fd, parser.get_document());

	return 0;
}
