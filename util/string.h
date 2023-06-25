#ifndef UTIL_STRING_H
#define UTIL_STRING_H

#include <glibmm/ustring.h>
#include <stdint.h>
#include <string>
#include <string_view>

namespace mcwutil {
namespace string {
Glib::ustring utf8_literal(const char8_t *s);
bool operator==(const Glib::ustring &x, std::u8string_view y);
bool operator==(std::u8string_view x, const Glib::ustring &y);
Glib::ustring todecu(uintmax_t value, unsigned int width = 0);
Glib::ustring todecs(intmax_t value, unsigned int width = 0);
Glib::ustring w2u(const std::wstring &wstr);
std::wstring u2w(const Glib::ustring &ustr);
std::string todecu_std(uintmax_t value, unsigned int width = 0);
std::string todecs_std(intmax_t value, unsigned int width = 0);
std::u8string l2u(std::string_view lstr);
std::string u2l(std::u8string_view ustr);
}
}

#endif
