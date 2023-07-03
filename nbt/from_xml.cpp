#include "nbt/nbt.h"
#include "nbt/tags.h"
#include "util/codec.h"
#include "util/file_descriptor.h"
#include "util/globals.h"
#include "util/string.h"
#include "util/xml.h"
#include <algorithm>
#include <fcntl.h>
#include <iostream>
#include <libxml/tree.h>
#include <limits>
#include <locale>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

using namespace std::literals::string_view_literals;

namespace mcwutil::nbt {
namespace {
/**
 * \brief Returns the numeric tag value for a given XML element.
 *
 * \param[in] name the XML element name.
 *
 * \param[in] message the message to throw in an exception if \p name is not
 * one of the recognized names.
 *
 * \return the numeric tag value.
 *
 * \exception std::runtime_error if \p name is not recognized.
 */
nbt::tag tag_for_child_of_named_or_list(std::u8string_view name, const char *message) {
	if(name == u8"byte"sv)
		return nbt::TAG_BYTE;
	if(name == u8"short"sv)
		return nbt::TAG_SHORT;
	if(name == u8"int"sv)
		return nbt::TAG_INT;
	if(name == u8"long"sv)
		return nbt::TAG_LONG;
	if(name == u8"float"sv)
		return nbt::TAG_FLOAT;
	if(name == u8"double"sv)
		return nbt::TAG_DOUBLE;
	if(name == u8"barray"sv)
		return nbt::TAG_BYTE_ARRAY;
	if(name == u8"string"sv)
		return nbt::TAG_STRING;
	if(name == u8"list"sv)
		return nbt::TAG_LIST;
	if(name == u8"compound"sv)
		return nbt::TAG_COMPOUND;
	if(name == u8"iarray"sv)
		return nbt::TAG_INT_ARRAY;
	if(name == u8"larray"sv)
		return nbt::TAG_LONG_ARRAY;
	throw std::runtime_error(message);
}

/**
 * \brief Returns the numeric tag value for a given XML element inside a \c
 * named element.
 *
 * \param[in] name the XML element name.
 *
 * \return the numeric tag value.
 *
 * \exception std::runtime_error if \p name is not recognized.
 */
nbt::tag tag_for_child_of_named(std::u8string_view name) {
	return tag_for_child_of_named_or_list(name, "Malformed NBT XML: child of named must be one of (byte|short|int|long|float|double|barray|string|list|compound|iarray|larray).");
}

/**
 * \brief Returns the numeric tag value for a given XML element inside a \c
 * named element.
 *
 * \param[in] name the XML element name.
 *
 * \return the numeric tag value.
 *
 * \exception std::runtime_error if \p name is not recognized.
 */
nbt::tag tag_for_child_of_list(std::u8string_view name) {
	return tag_for_child_of_named_or_list(name, "Malformed NBT XML: child of list must be one of (byte|short|int|long|float|double|barray|string|list|compound|iarray|larray).");
}

/**
 * \brief Checks that the numeric subtype specified for a \c list element is
 * acceptable.
 *
 * \param[in] subtype the numeric subtype.
 *
 * \exception std::runtime_error if \p subtype is not a legal numeric subtype
 * for a \c list element.
 */
void check_list_subtype(nbt::tag subtype) {
	switch(subtype) {
		case nbt::TAG_END:
		case nbt::TAG_BYTE:
		case nbt::TAG_SHORT:
		case nbt::TAG_INT:
		case nbt::TAG_LONG:
		case nbt::TAG_FLOAT:
		case nbt::TAG_DOUBLE:
		case nbt::TAG_BYTE_ARRAY:
		case nbt::TAG_STRING:
		case nbt::TAG_LIST:
		case nbt::TAG_COMPOUND:
		case nbt::TAG_INT_ARRAY:
		case nbt::TAG_LONG_ARRAY:
			return;
	}
	throw std::runtime_error("Malformed NBT XML: list has bad subtype.");
}

/**
 * \brief Converts an XML element to NBT.
 *
 * \param[out] nbt_fd the file descriptor to write to.
 *
 * \param[in] elt the XML element to convert.
 */
void write_nbt(const file_descriptor &nbt_fd, const xmlNode &elt) {
	std::u8string_view elt_name = xml::node_name(elt);
	if(elt_name == u8"named"sv) {
		const xmlNode *child = nullptr;
		nbt::tag subtype = nbt::TAG_END;
		for(const xmlNode *i = elt.children; i; i = i->next) {
			if(i->type == XML_ELEMENT_NODE) {
				subtype = tag_for_child_of_named(xml::node_name(*i));
				if(child) {
					throw std::runtime_error("Malformed NBT XML: named must have only one child.");
				}
				child = i;
			}
		}
		if(!child) {
			throw std::runtime_error("Malformed NBT XML: named must have a child.");
		}
		const char8_t *name_raw = xml::node_attr(elt, u8"name");
		if(!name_raw) {
			throw std::runtime_error("Malformed NBT XML: named must have a name.");
		}
		std::u8string_view name(name_raw);
		if(name.size() > static_cast<std::size_t>(std::numeric_limits<int16_t>::max())) {
			throw std::runtime_error("Malformed NBT XML: name too long.");
		}
		uint8_t header[3];
		codec::encode_u8(&header[0], subtype);
		codec::encode_u16(&header[1], static_cast<int16_t>(name.size()));
		nbt_fd.write(header, sizeof(header));
		nbt_fd.write(name.data(), name.size());
		write_nbt(nbt_fd, *child);
	} else if(elt_name == u8"byte"sv) {
		const char8_t *value_raw = xml::node_attr(elt, u8"value");
		if(!value_raw) {
			throw std::runtime_error("Malformed NBT XML: byte must have a value.");
		}
		std::istringstream iss(string::u2l(value_raw));
		iss.imbue(std::locale("C"));
		int value;
		iss >> value;
		if(value < std::numeric_limits<int8_t>::min() || value > std::numeric_limits<int8_t>::max()) {
			throw std::runtime_error("Malformed NBT XML: byte value out of range.");
		}
		uint8_t buffer[1];
		codec::encode_u8(&buffer[0], static_cast<int8_t>(value));
		nbt_fd.write(buffer, sizeof(buffer));
	} else if(elt_name == u8"short"sv) {
		const char8_t *value_raw = xml::node_attr(elt, u8"value");
		if(!value_raw) {
			throw std::runtime_error("Malformed NBT XML: byte must have a value.");
		}
		std::istringstream iss(string::u2l(value_raw));
		iss.imbue(std::locale("C"));
		int16_t value;
		iss >> value;
		uint8_t buffer[sizeof(value)];
		codec::encode_u16(&buffer[0], value);
		nbt_fd.write(buffer, sizeof(buffer));
	} else if(elt_name == u8"int"sv) {
		const char8_t *value_raw = xml::node_attr(elt, u8"value");
		if(!value_raw) {
			throw std::runtime_error("Malformed NBT XML: byte must have a value.");
		}
		std::istringstream iss(string::u2l(value_raw));
		iss.imbue(std::locale("C"));
		int32_t value;
		iss >> value;
		uint8_t buffer[sizeof(value)];
		codec::encode_u32(&buffer[0], value);
		nbt_fd.write(buffer, sizeof(buffer));
	} else if(elt_name == u8"long"sv) {
		const char8_t *value_raw = xml::node_attr(elt, u8"value");
		if(!value_raw) {
			throw std::runtime_error("Malformed NBT XML: byte must have a value.");
		}
		std::istringstream iss(string::u2l(value_raw));
		iss.imbue(std::locale("C"));
		int64_t value;
		iss >> value;
		uint8_t buffer[sizeof(value)];
		codec::encode_u64(&buffer[0], value);
		nbt_fd.write(buffer, sizeof(buffer));
	} else if(elt_name == u8"float"sv) {
		const char8_t *value_raw = xml::node_attr(elt, u8"value");
		if(!value_raw) {
			throw std::runtime_error("Malformed NBT XML: byte must have a value.");
		}
		std::istringstream iss(string::u2l(value_raw));
		iss.imbue(std::locale("C"));
		float value;
		iss >> value;
		uint8_t buffer[4];
		codec::encode_float(&buffer[0], value);
		nbt_fd.write(buffer, sizeof(buffer));
	} else if(elt_name == u8"double"sv) {
		const char8_t *value_raw = xml::node_attr(elt, u8"value");
		if(!value_raw) {
			throw std::runtime_error("Malformed NBT XML: byte must have a value.");
		}
		std::istringstream iss(string::u2l(value_raw));
		iss.imbue(std::locale("C"));
		double value;
		iss >> value;
		uint8_t buffer[8];
		codec::encode_double(&buffer[0], value);
		nbt_fd.write(buffer, sizeof(buffer));
	} else if(elt_name == u8"barray"sv) {
		const xmlNode *text = nullptr;
		for(const xmlNode *i = elt.children; i; i = i->next) {
			if(i->type == XML_TEXT_NODE) {
				text = i;
				break;
			}
		}
		if(text) {
			static const char8_t DIGITS[] = u8"0123456789ABCDEF";
			std::u8string_view value = xml::node_content(*text);
			std::u8string filtered_value;
			filtered_value.reserve(value.size());
			for(char8_t i : value) {
				static const char8_t SPACES[] = u8" \t\n\r";
				if(std::find(DIGITS, DIGITS + sizeof(DIGITS), i) != DIGITS + sizeof(DIGITS)) {
					filtered_value.push_back(i);
				} else if(std::find(SPACES, SPACES + sizeof(SPACES), i) == SPACES + sizeof(SPACES)) {
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
			codec::encode_u32(&header[0], static_cast<int32_t>(data.size()));
			nbt_fd.write(header, sizeof(header));
			nbt_fd.write(&data[0], data.size());
		} else {
			uint8_t header[4];
			codec::encode_u32(&header[0], 0);
			nbt_fd.write(header, sizeof(header));
		}
	} else if(elt_name == u8"string"sv) {
		const char8_t *value_raw = xml::node_attr(elt, u8"value");
		if(!value_raw) {
			throw std::runtime_error("Malformed NBT XML: string must have a value.");
		}
		std::u8string_view value(value_raw);
		if(static_cast<uintmax_t>(value.size()) > static_cast<uintmax_t>(std::numeric_limits<int16_t>::max())) {
			throw std::runtime_error("Malformed NBT XML: string too long.");
		}
		uint8_t header[2];
		codec::encode_u16(&header[0], static_cast<int16_t>(value.size()));
		nbt_fd.write(header, sizeof(header));
		nbt_fd.write(value.data(), value.size());
	} else if(elt_name == u8"list"sv) {
		const char8_t *subtype_raw = xml::node_attr(elt, u8"subtype");
		if(!subtype_raw) {
			throw std::runtime_error("Malformed NBT XML: list must have a subtype.");
		}
		std::istringstream iss(string::u2l(subtype_raw));
		iss.imbue(std::locale("C"));
		unsigned int subtype_int;
		iss >> subtype_int;
		nbt::tag subtype = static_cast<nbt::tag>(subtype_int);
		check_list_subtype(subtype);
		unsigned int element_count = 0;
		for(const xmlNode *i = elt.children; i; i = i->next) {
			if(i->type == XML_ELEMENT_NODE) {
				if(tag_for_child_of_list(xml::node_name(*i)) != subtype) {
					throw std::runtime_error("Malformed NBT XML: child of list does not match subtype specification.");
				}
				++element_count;
			}
		}
		if(static_cast<uintmax_t>(element_count) > static_cast<uintmax_t>(std::numeric_limits<int32_t>::max())) {
			throw std::runtime_error("Malformed NBT XML: list too long.");
		}
		uint8_t header[5];
		codec::encode_u8(&header[0], static_cast<int8_t>(subtype));
		codec::encode_u32(&header[1], static_cast<int32_t>(element_count));
		nbt_fd.write(header, sizeof(header));
		for(const xmlNode *i = elt.children; i; i = i->next) {
			if(i->type == XML_ELEMENT_NODE) {
				write_nbt(nbt_fd, *i);
			}
		}
	} else if(elt_name == u8"compound"sv) {
		for(xmlNode *i = elt.children; i; i = i->next) {
			if(i->type == XML_ELEMENT_NODE) {
				if(xml::node_name(*i) != u8"named"sv) {
					throw std::runtime_error("Malformed NBT XML: child of compound is not named.");
				}
				write_nbt(nbt_fd, *i);
			}
		}
		uint8_t footer[1];
		codec::encode_u8(&footer[0], nbt::TAG_END);
		nbt_fd.write(footer, sizeof(footer));
	} else if(elt_name == u8"iarray"sv) {
		const xmlNode *text = nullptr;
		for(const xmlNode *i = elt.children; i; i = i->next) {
			if(i->type == XML_TEXT_NODE) {
				text = i;
				break;
			}
		}
		if(text) {
			static const char8_t DIGITS[] = u8"0123456789ABCDEF";
			std::u8string_view value = xml::node_content(*text);
			std::u8string filtered_value;
			filtered_value.reserve(value.size());
			for(char8_t i : value) {
				static const char8_t SPACES[] = u8" \t\n\r";
				if(std::find(DIGITS, DIGITS + sizeof(DIGITS), i) != DIGITS + sizeof(DIGITS)) {
					filtered_value.push_back(i);
				} else if(std::find(SPACES, SPACES + sizeof(SPACES), i) == SPACES + sizeof(SPACES)) {
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
			codec::encode_u32(&header[0], static_cast<int32_t>(data.size() / 4));
			nbt_fd.write(header, sizeof(header));
			nbt_fd.write(&data[0], data.size());
		} else {
			uint8_t header[4];
			codec::encode_u32(&header[0], 0);
			nbt_fd.write(header, sizeof(header));
		}
	} else if(elt_name == u8"larray"sv) {
		const xmlNode *text = nullptr;
		for(const xmlNode *i = elt.children; i; i = i->next) {
			if(i->type == XML_TEXT_NODE) {
				text = i;
				break;
			}
		}
		if(text) {
			static const char8_t DIGITS[] = u8"0123456789ABCDEF";
			std::u8string_view value = xml::node_content(*text);
			std::u8string filtered_value;
			filtered_value.reserve(value.size());
			for(char8_t i : value) {
				static const char8_t SPACES[] = u8" \t\n\r";
				if(std::find(DIGITS, DIGITS + sizeof(DIGITS), i) != DIGITS + sizeof(DIGITS)) {
					filtered_value.push_back(i);
				} else if(std::find(SPACES, SPACES + sizeof(SPACES), i) == SPACES + sizeof(SPACES)) {
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
			codec::encode_u32(&header[0], static_cast<int32_t>(data.size() / 8));
			nbt_fd.write(header, sizeof(header));
			nbt_fd.write(&data[0], data.size());
		} else {
			uint8_t header[4];
			codec::encode_u32(&header[0], 0);
			nbt_fd.write(header, sizeof(header));
		}
	} else {
		throw std::runtime_error("Malformed NBT XML: unrecognized element.");
	}
}

/**
 * \brief Converts an XML document to NBT.
 *
 * \param[out] nbt_fd the file descriptor to write to.
 *
 * \param[in] doc the XML document to convert.
 */
void write_nbt(const file_descriptor &nbt_fd, const xmlDoc &doc) {
	const xmlNode &root = *xmlDocGetRootElement(&doc);
	if(xml::node_name(root) != u8"minecraft-nbt"sv) {
		throw std::runtime_error("Malformed NBT XML: improper root node name.");
	}
	const xmlNode *named = nullptr;
	for(const xmlNode *i = root.children; i; i = i->next) {
		if(i->type == XML_ELEMENT_NODE) {
			if(xml::node_name(*i) != u8"named"sv) {
				throw std::runtime_error("Malformed NBT XML: top-level element must be named.");
			}
			if(named) {
				throw std::runtime_error("Malformed NBT XML: must be exactly one top-level element.");
			}
			named = i;
		}
	}
	if(!named) {
		throw std::runtime_error("Malformed NBT XML: top-level element must exist.");
	}
	write_nbt(nbt_fd, *named);
}
}
}

/**
 * \brief Entry point for the \c nbt-from-xml utility.
 *
 * \param[in] args the command-line arguments.
 *
 * \return the application exit code.
 */
int mcwutil::nbt::from_xml(std::ranges::subrange<char **> args) {
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
	std::unique_ptr<xmlDoc, xml::doc_deleter> document = xml::parse(args[0]);

	// Write output file.
	file_descriptor nbt_fd = file_descriptor::create_open(args[1], O_WRONLY | O_CREAT | O_TRUNC, 0666);
	write_nbt(nbt_fd, *document);
	nbt_fd.close();

	return 0;
}
