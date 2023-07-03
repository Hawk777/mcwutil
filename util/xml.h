#ifndef UTIL_XML_H
#define UTIL_XML_H

#include <libxml/tree.h>
#include <libxml/xmlerror.h>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace mcwutil {
class file_descriptor;

/**
 * \brief Symbols related to XML file loading, saving, and manipulation.
 */
namespace xml {
/**
 * \brief An individual XML parsing or saving error.
 *
 * The error can be either an unstructured string (if an unstructured error
 * occurred) or a structured libxml2 error (if a structured error occurred).
 */
using error_detail = std::variant<std::string, xmlError>;

/**
 * \brief An exception thrown if XML parsing or saving fails.
 */
struct error final {
	/**
	 * \brief The parsing errors.
	 */
	std::vector<error_detail> errors;

	explicit error();
	~error();

	// Because xmlErrors contain pointers to heap allocations, they cannot be
	// naïvely copied. However they can be moved.
	explicit error(const error &) = delete;

	/**
	 * \brief Moves an @c error.
	 *
	 * \param[in] moveref the error to move from.
	 */
	explicit error(error &&moveref) = default;
	void operator=(const error &) = delete;

	/**
	 * \brief Moves an @c error.
	 *
	 * \param[in] moveref the error to move from.
	 *
	 * \return \p *this.
	 */
	error &operator=(error &&moveref) = default;
};

/**
 * \brief A deleter for an XML document.
 */
struct doc_deleter final {
	void operator()(xmlDoc *doc);
};

std::unique_ptr<xmlDoc, doc_deleter> parse(const char *filename);
std::unique_ptr<xmlDoc, doc_deleter> empty();
const char8_t *node_name(const xmlNode &node);
const char8_t *node_content(const xmlNode &node);
const char8_t *node_attr(const xmlNode &node, const char8_t *attr);
void node_attr(xmlNode &node, const char8_t *attr, const char8_t *value);
xmlNode &node_append_child(xmlNode &parent, const char8_t *name);
void node_append_text(xmlNode &parent, std::u8string_view text);
xmlNode &node_create_root(xmlDoc &parent, const char8_t *name);
void internal_subset(xmlDoc &doc, const char8_t *name, const char8_t *external_id, const char8_t *system_id);
void write(xmlDoc &doc, file_descriptor &fd);
}
}

#endif
