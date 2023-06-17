#ifndef UTIL_STRING_H
#define UTIL_STRING_H

#include <stdint.h>
#include <string>
#include <string_view>
#include <glibmm/ustring.h>

/**
 * \brief Converts a UTF-8 C-style string (typically a string literal) to a
 * GLib string.
 *
 * \param[in] s the string to convert.
 *
 * \return the converted string.
 */
Glib::ustring utf8_literal(const char8_t *s);

/**
 * \brief Wraps a Glib UTF-8 string in a C++ UTF-8 string view.
 *
 * \param[in] s the string to wrap.
 *
 * \return a view of \p s, which remains valid until \p s is modified.
 */
std::u8string_view utf8_wrap(const Glib::ustring &s);

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
bool operator==(const Glib::ustring &x, std::u8string_view y);

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
bool operator==(std::u8string_view x, const Glib::ustring &y);

/**
 * \brief Converts an unsigned integer of any type to a fixed-width decimal string.
 *
 * \param[in] value the value to convert.
 *
 * \param[in] width the width, in characters, of the output to produce.
 *
 * \return the decimal string.
 */
Glib::ustring todecu(uintmax_t value, unsigned int width = 0);

/**
 * \brief Converts a signed integer of any type to a fixed-width decimal string.
 *
 * \param[in] value the value to convert.
 *
 * \param[in] width the width, in characters, of the output to produce.
 *
 * \return the decimal string.
 */
Glib::ustring todecs(intmax_t value, unsigned int width = 0);

/**
 * \brief Converts an unsigned integer of any type to a hexadecimal string.
 *
 * \param[in] value the value to convert.
 *
 * \param[in] width the width, in characters, of the output to produce.
 *
 * \return the hex string.
 */
Glib::ustring tohex(uintmax_t value, unsigned int width = 0);

/**
 * \brief Converts a wide-character string to a UTF-8 string.
 *
 * \param[in] wstr the string to convert.
 *
 * \return the converted string.
 */
Glib::ustring wstring2ustring(const std::wstring &wstr);

/**
 * \brief Converts a UTF-8 string to a wide-character string.
 *
 * \param[in] ustr the string to convert.
 *
 * \return the converted string.
 */
std::wstring ustring2wstring(const Glib::ustring &ustr);

#endif

