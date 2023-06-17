#ifndef UTIL_STRING_H
#define UTIL_STRING_H

#include <stdint.h>
#include <string>
#include <string_view>
#include <glibmm/ustring.h>

Glib::ustring utf8_literal(const char8_t *s);
std::u8string_view utf8_wrap(const Glib::ustring &s);
bool operator==(const Glib::ustring &x, std::u8string_view y);
bool operator==(std::u8string_view x, const Glib::ustring &y);
Glib::ustring todecu(uintmax_t value, unsigned int width = 0);
Glib::ustring todecs(intmax_t value, unsigned int width = 0);
Glib::ustring tohex(uintmax_t value, unsigned int width = 0);
Glib::ustring wstring2ustring(const std::wstring &wstr);
std::wstring ustring2wstring(const Glib::ustring &ustr);

#endif

