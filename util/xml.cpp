#include "util/xml.h"
#include "util/file_descriptor.h"
#include "util/mapped_file.h"
#include "util/string.h"
#include <cassert>
#include <cstdarg>
#include <cstdio>
#include <fcntl.h>
#include <libxml/parser.h>
#include <libxml/xmlIO.h>
#include <libxml/xmlerror.h>
#include <libxml/xmlsave.h>
#include <utility>

using namespace mcwutil::xml;

namespace mcwutil::xml {
namespace {
/**
 * \brief Records whether or not an external entity reference has been
 * rejected during the current parsing operation.
 */
thread_local bool external_entity_reference_rejected;

/**
 * \brief Collects libxml2 errors.
 */
struct error_collector final {
	/**
	 * \brief The collected errors.
	 */
	xml::error error;

	explicit error_collector();
	~error_collector();
};

/**
 * \brief The generic error handler used with \ref error_collector.
 */
void generic_error_handler(void *context, const char *msg, ...) {
	// Turn the message into a string.
	std::string buffer(32, '\0');
	int written;
	std::size_t old_size;
	do {
		old_size = buffer.size();
		va_list ap;
		va_start(ap, msg);
		written = std::vsnprintf(buffer.data(), old_size + 1, msg, ap);
		if(written < 0) {
			// An error in vsnprintf isn’t really something we can usefully
			// handle.
			return;
		}
		va_end(ap);
		buffer.resize(written);
	} while(static_cast<unsigned int>(written) > old_size);

	// Add the string to the error list.
	error_collector *ec = static_cast<error_collector *>(context);
	ec->error.errors.emplace_back(std::move(buffer));
}

/**
 * \brief The structured error handler used with \ref error_collector.
 */
void structured_error_handler(void *context, xmlError *e) {
	// Duplicate the error so the contained pointers will not be invalidated
	// when this function returns.
	xmlError dup{};
	xmlCopyError(e, &dup);

	// Add the error to the error list.
	error_collector *ec = static_cast<error_collector *>(context);
	ec->error.errors.emplace_back(dup);
}

/**
 * \brief An external entity loader that always fails, to allow safe usage on
 * untrusted XML files.
 */
xmlParserInput *null_entity_loader(const char *url, const char *, xmlParserCtxt *context) {
	xmlParserError(context, "external entity reference to %s rejected", url);
	external_entity_reference_rejected = true;
	return nullptr;
}

/**
 * \brief Installs an error collector.
 */
error_collector::error_collector() :
		error() {
	xmlSetGenericErrorFunc(this, &generic_error_handler);
	xmlSetStructuredErrorFunc(this, &structured_error_handler);
}

/**
 * \brief Removes an error collector.
 */
error_collector::~error_collector() {
	xmlSetGenericErrorFunc(nullptr, nullptr);
	xmlSetStructuredErrorFunc(nullptr, nullptr);
}
}
}

/**
 * \brief Constructs a new parse error with no underlying errors.
 */
error::error() :
		errors() {
}

/**
 * \brief Destroys a parse error, clearing the contained details.
 */
error::~error() {
	for(auto &i : errors) {
		if(std::holds_alternative<xmlError>(i)) {
			xmlResetError(&std::get<xmlError>(i));
		}
	}
}

/**
 * \brief Deletes an XML document.
 *
 * \param[in] doc the document.
 */
void doc_deleter::operator()(xmlDoc *doc) {
	xmlFreeDoc(doc);
}

/**
 * \brief Parses an XML file.
 *
 * \param[in] filename the name of the file to parse.
 *
 * \return the document.
 */
std::unique_ptr<xmlDoc, doc_deleter> mcwutil::xml::parse(const char *filename) {
	// Open and memory-map the file.
	file_descriptor fd = file_descriptor::create_open(filename, O_RDONLY, 0);
	mapped_file mapped(fd);

	// Check if the file is insanely large. bad_alloc is not *exactly* the
	// right error here, but it’s kind of close enough.
	if(mapped.size() > std::numeric_limits<int>::max()) {
		throw std::bad_alloc();
	}

	// Parse the file.
	error_collector ec;
	external_entity_reference_rejected = false;
	xmlSetExternalEntityLoader(&null_entity_loader);
	std::unique_ptr<xmlDoc, doc_deleter> doc(xmlReadMemory(static_cast<const char *>(mapped.data()), static_cast<int>(mapped.size()), filename, nullptr, XML_PARSE_NOENT | XML_PARSE_NOBLANKS | XML_PARSE_NONET | XML_PARSE_NOCDATA));

	// Throw the errors on failure.
	if(!doc || external_entity_reference_rejected) {
		throw error(std::move(ec.error));
	}

	// Make sure there are no entity references. In XML_PARSE_NOENT mode there
	// shouldn’t be, but they will be left in if they are unresolvable (e.g.
	// because they refer to undeclared entities). Checking that here ensures
	// that by the time any consumer looks at the XML, any entity references
	// that might have been in there originally have been resolved to text.
	{
		const xmlNode *cur = xmlDocGetRootElement(doc.get());
		for(;;) {
			if(cur->type == XML_ENTITY_REF_NODE) {
				std::ostringstream oss;
				oss << filename << ": unresolved entity reference " << string::u2l(reinterpret_cast<const char8_t *>(cur->name));
				error e;
				e.errors.emplace_back(std::move(oss).str());
				throw error(std::move(e));
			}

			// Perform a depth-first traversal of the entire node tree.
			if(cur->children) {
				cur = cur->children;
			} else if(cur->next) {
				cur = cur->next;
			} else {
				while(cur && !cur->next) {
					cur = cur->parent;
				}
				if(cur) {
					cur = cur->next;
				} else {
					break;
				}
			}
		}
	}

	return doc;
}

/**
 * \brief Creates an empty document.
 *
 * \return the document.
 */
std::unique_ptr<xmlDoc, doc_deleter> mcwutil::xml::empty() {
	xmlDoc *doc = xmlNewDoc(reinterpret_cast<const xmlChar *>(u8"1.0"));
	if(!doc) {
		throw std::bad_alloc();
	}
	return std::unique_ptr<xmlDoc, doc_deleter>(doc);
}

/**
 * \brief Returns the name of a node.
 *
 * \param[in] node the node.
 *
 * \return the name, which points into the node and may be invalidated when the
 * node is modified.
 */
const char8_t *mcwutil::xml::node_name(const xmlNode &node) {
	return reinterpret_cast<const char8_t *>(node.name);
}

/**
 * \brief Returns the content of a node.
 *
 * \param[in] node the node.
 *
 * \return the content, which points into the node and may be invalidated when
 * the node is modified.
 */
const char8_t *mcwutil::xml::node_content(const xmlNode &node) {
	return reinterpret_cast<const char8_t *>(node.content);
}

/**
 * \brief Returns the value of an attribute on a node.
 *
 * \param[in] node the node.
 *
 * \param[in] attr the name of the attribute.
 *
 * \return the value of the attribute, or \c nullptr if \p node does not
 * possess \p attr.
 */
const char8_t *mcwutil::xml::node_attr(const xmlNode &node, const char8_t *attr) {
	const xmlAttr *attr_node = xmlHasProp(&node, reinterpret_cast<const xmlChar *>(attr));
	if(!attr_node) {
		// The attribute does not exist.
		return nullptr;
	}
	if(!attr_node->children) {
		// The attribute exists but has empty content.
		return u8"";
	}

	// Because of parsing options, there should not be CDATA or ENTITY_REF
	// nodes under the attribute; there should only ever be a TEXT node. Since
	// libxml2 always merges adjacent text nodes while parsing, there should
	// only ever be ONE such node. So this can be done without allocation.
	const xmlNode *child = attr_node->children;
	assert(child->type == XML_TEXT_NODE);
	assert(!child->next);
	return reinterpret_cast<const char8_t *>(child->content);
}

/**
 * \brief Sets the value of an attribute on a node.
 *
 * \param[in] node the node.
 *
 * \param[in] attr the name of the attribute.
 *
 * \param[in] value the new value to give the attribute.
 */
void mcwutil::xml::node_attr(xmlNode &node, const char8_t *attr, const char8_t *value) {
	assert(node.type == XML_ELEMENT_NODE);
	if(!xmlSetNsProp(&node, nullptr, reinterpret_cast<const xmlChar *>(attr), reinterpret_cast<const xmlChar *>(value))) {
		throw std::bad_alloc();
	}
}

/**
 * \brief Appends a new child element to a parent element.
 *
 * \param[in] parent the element to append a child to.
 *
 * \param[in] name the name of the new child.
 *
 * \return the new child, which does not need to be independently freed because
 * it is part of the same document as \p parent.
 */
xmlNode &mcwutil::xml::node_append_child(xmlNode &parent, const char8_t *name) {
	assert(name);
	xmlNode *ret = xmlNewChild(&parent, nullptr, reinterpret_cast<const xmlChar *>(name), nullptr);
	if(!ret) {
		throw std::bad_alloc();
	}
	return *ret;
}

/**
 * \brief Appends a new child text node to a parent element.
 *
 * \param[in] parent the element to append text to.
 *
 * \param[in] text the text to append.
 */
void mcwutil::xml::node_append_text(xmlNode &parent, std::u8string_view text) {
	if(text.size() > std::numeric_limits<int>::max()) {
		throw std::bad_alloc();
	}
	xmlNode *n = xmlNewDocTextLen(parent.doc, reinterpret_cast<const xmlChar *>(text.data()), static_cast<int>(text.size()));
	if(!n) {
		throw std::bad_alloc();
	}
	xmlAddChild(&parent, n);
}

/**
 * \brief Creates a new root element in a document.
 *
 * \param[in] parent the document to place the element in.
 *
 * \param[in] name the name of the new child.
 *
 * \return the new child, which does not need to be independently freed because
 * it is part of \p parent.
 */
xmlNode &mcwutil::xml::node_create_root(xmlDoc &parent, const char8_t *name) {
	assert(name);
	xmlNode *ret = xmlNewDocNode(&parent, nullptr, reinterpret_cast<const xmlChar *>(name), nullptr);
	if(!ret) {
		throw std::bad_alloc();
	}
	xmlDocSetRootElement(&parent, ret);
	return *ret;
}

/**
 * \brief Sets the internal subset of a document.
 *
 * \param[in] doc the document to modify.
 *
 * \param[in] name the DTD name.
 *
 * \param[in] external_id the external ID.
 *
 * \param[in] system_id the system ID.
 */
void mcwutil::xml::internal_subset(xmlDoc &doc, const char8_t *name, const char8_t *external_id, const char8_t *system_id) {
	if(!xmlCreateIntSubset(&doc, reinterpret_cast<const xmlChar *>(name), reinterpret_cast<const xmlChar *>(external_id), reinterpret_cast<const xmlChar *>(system_id))) {
		throw std::bad_alloc();
	}
}

/**
 * \brief Writes a document to a file descriptor.
 *
 * \param[in] doc the document to save.
 *
 * \param[out] fd the descriptor to write to.
 */
void mcwutil::xml::write(xmlDoc &doc, file_descriptor &fd) {
	xmlSaveCtxt *context = xmlSaveToFd(fd.fd(), "UTF-8", XML_SAVE_FORMAT);
	if(!context) {
		throw std::bad_alloc();
	}
	error_collector ec;
	long save_result = xmlSaveDoc(context, &doc);
	int close_result = xmlSaveClose(context);
	if(save_result < 0 || close_result < 0) {
		throw error(std::move(ec.error));
	}
}
