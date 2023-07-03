#ifndef UTIL_STRING_H
#define UTIL_STRING_H

#include <stdint.h>
#include <string>
#include <string_view>

namespace mcwutil {
/**
 * \brief Symbols related to converting between types of strings and between
 * strings and numbers.
 */
namespace string {
std::string todecu(uintmax_t value, unsigned int width = 0);
std::string todecs(intmax_t value, unsigned int width = 0);
std::u8string l2u(std::string_view lstr);
std::string u2l(std::u8string_view ustr);
}
}

#endif
