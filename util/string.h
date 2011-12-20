#ifndef UTIL_STRING_H
#define UTIL_STRING_H

#include <stdint.h>
#include <string>
#include <glibmm/ustring.h>

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

