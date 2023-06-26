#ifndef UTIL_STRING_H
#define UTIL_STRING_H

#include <stdint.h>
#include <string>
#include <string_view>

namespace mcwutil {
namespace string {
std::string todecu_std(uintmax_t value, unsigned int width = 0);
std::string todecs_std(intmax_t value, unsigned int width = 0);
std::u8string l2u(std::string_view lstr);
std::string u2l(std::u8string_view ustr);
}
}

#endif
