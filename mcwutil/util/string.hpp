#ifndef UTIL_STRING_H
#define UTIL_STRING_H

#include <cstdint>
#include <span>
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
std::string todecf(float value);
std::string todecd(double value);
std::int8_t fromdecs8(std::string_view s);
std::int16_t fromdecs16(std::string_view s);
std::int32_t fromdecs32(std::string_view s);
std::int64_t fromdecs64(std::string_view s);
std::uint32_t fromdecu32(std::string_view s);
unsigned int fromdecui(std::string_view s);
float fromdecf(std::string_view s);
double fromdecd(std::string_view s);
std::u8string l2u(std::string_view lstr);
std::string u2l(std::u8string_view ustr);
bool utf8_valid(std::span<const uint8_t> bytes);
}
}

#endif
