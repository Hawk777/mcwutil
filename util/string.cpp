#include "util/string.h"
#include <locale>
#include <sstream>

namespace mcwutil::string {
namespace {
/**
 * \brief Wraps a Glib UTF-8 string in a C++ UTF-8 string view.
 *
 * \param[in] s the string to wrap.
 *
 * \return a view of \p s, which remains valid until \p s is modified.
 */
std::u8string_view utf8_wrap(const Glib::ustring &s) {
	return std::u8string_view(reinterpret_cast<const char8_t *>(s.data()), s.bytes());
}
}
}

/**
 * \brief Converts a UTF-8 C-style string (typically a string literal) to a
 * GLib string.
 *
 * \param[in] s the string to convert.
 *
 * \return the converted string.
 */
Glib::ustring mcwutil::string::utf8_literal(const char8_t *s) {
	return reinterpret_cast<const char *>(s);
}

/**
 * \brief Compares a Glib UTF-8 string with a C++ UTF-8 string.
 *
 * \param[in] x the first string.
 *
 * \param[in] y the second string.
 *
 * \retval true \p x and \p y are equal.
 * \retval false \p x and \p y are unequal.
 */
bool mcwutil::string::operator==(const Glib::ustring &x, std::u8string_view y) {
	return utf8_wrap(x) == y;
}

/**
 * \brief Compares a Glib UTF-8 string with a C++ UTF-8 string.
 *
 * \param[in] x the first string.
 *
 * \param[in] y the second string.
 *
 * \retval true \p x and \p y are equal.
 * \retval false \p x and \p y are unequal.
 */
bool mcwutil::string::operator==(std::u8string_view x, const Glib::ustring &y) {
	return x == utf8_wrap(y);
}

/**
 * \brief Converts an unsigned integer of any type to a fixed-width decimal string.
 *
 * \param[in] value the value to convert.
 *
 * \param[in] width the width, in characters, of the output to produce.
 *
 * \return the decimal string.
 */
Glib::ustring mcwutil::string::todecu(uintmax_t value, unsigned int width) {
	std::wostringstream oss;
	oss.imbue(std::locale("C"));
	oss.flags(std::ios::uppercase | std::ios::dec | std::ios::right);
	oss.width(width);
	oss.fill(L'0');
	oss << value;
	return Glib::ustring::format(oss.str());
}

/**
 * \brief Converts a signed integer of any type to a fixed-width decimal string.
 *
 * \param[in] value the value to convert.
 *
 * \param[in] width the width, in characters, of the output to produce.
 *
 * \return the decimal string.
 */
Glib::ustring mcwutil::string::todecs(intmax_t value, unsigned int width) {
	std::wostringstream oss;
	oss.imbue(std::locale("C"));
	oss.flags(std::ios::uppercase | std::ios::dec | std::ios::right);
	oss.width(width);
	oss.fill(L'0');
	oss << value;
	return Glib::ustring::format(oss.str());
}

/**
 * \brief Converts a wide-character string to a UTF-8 string.
 *
 * \param[in] wstr the string to convert.
 *
 * \return the converted string.
 */
Glib::ustring mcwutil::string::w2u(const std::wstring &wstr) {
	return Glib::ustring::format(wstr);
}

/**
 * \brief Converts a UTF-8 string to a wide-character string.
 *
 * \param[in] ustr the string to convert.
 *
 * \return the converted string.
 */
std::wstring mcwutil::string::u2w(const Glib::ustring &ustr) {
	std::wostringstream oss;
	oss.imbue(std::locale("C"));
	oss << ustr;
	return oss.str();
}
