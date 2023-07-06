#include "util/string.h"
#include <cassert>
#include <charconv>
#include <concepts>
#include <cstddef>
#include <cuchar>
#include <limits>
#include <system_error>

namespace mcwutil::string {
namespace {
/**
 * \brief Pads a string on the left with zeroes to a specified width.
 *
 * \param[in, out] s the string to pad.
 *
 * \param[in] width the minimum width to pad \p s to.
 */
void pad(std::string &s, std::size_t width) {
	if(s.size() < width) {
		std::size_t to_add = width - s.size();
		s.insert(0, to_add, '0');
	}
}

/**
 * \brief Converts an integer of any type to a fixed-width decimal string.
 *
 * \tparam T the type of integer to convert.
 *
 * \param[in] value the value to convert.
 *
 * \param[in] width the width, in characters, of the output to produce.
 *
 * \return the decimal string.
 */
template<std::integral T>
std::string todec_integer(T value, std::size_t width) {
	constexpr std::size_t sign_size = std::signed_integral<T> ? 1 : 0;
	// Can’t use max_digits10 because that is only specified for
	// floating-point-types. digits10 will typically be one lower, because it
	// means all N-digit numbers can be represented as a uintmax_t, but since
	// uintmax_t can represent *some* but not all N+1-digit numbers, we need
	// N+1.
	constexpr std::size_t max_size = sign_size + std::numeric_limits<uintmax_t>::digits10 + 1;
	std::string buf(max_size, '\0');
	std::to_chars_result res = std::to_chars(buf.data(), buf.data() + buf.size(), value);
	// This should be impossible, since we should have sized the buffer
	// sufficiently to hold any value.
	assert(res.ec == std::errc());
	buf.resize(res.ptr - buf.data());
	pad(buf, width);
	return buf;
}

/**
 * \brief Converts a floating-point value to a decimal string, potentially in
 * scientific notation.
 *
 * \tparam T the type of value to convert.
 *
 * \param[in] value the value to convert.
 *
 * \return the decimal string.
 */
template<std::floating_point T>
std::string todec_floating(T value) {
	constexpr std::size_t initial_buffer_size =
		1 /* sign */
		+ std::numeric_limits<T>::max_digits10 /* mantissa */
		+ 1 /* decimal point */
		+ 1 /* e */
		+ 1 /* sign of exponent */
		+ 3 /* magnitude of exponent, hopefully large enough */
		+ 10 /* extra margin */;
	std::string buf(initial_buffer_size, '\0');
	for(;;) {
		std::to_chars_result res = std::to_chars(buf.data(), buf.data() + buf.size(), value);
		if(res.ec == std::errc()) {
			buf.resize(res.ptr - buf.data());
			return buf;
		}
		buf.resize(buf.size() * 2);
	}
}

/**
 * \brief Converts a decimal string, potentially in scientific notation in case
 * of a floating-point return type, to a numeric value.
 *
 * \tparam T the type of value to convert to.
 *
 * \param[in] s the string to convert.
 *
 * \return the converted value.
 */
template<typename T>
T fromdec(std::string_view s)
	requires std::integral<T> || std::floating_point<T>
{
	T ret;
	std::from_chars_result res = std::from_chars(s.data(), s.data() + s.size(), ret);
	if(res.ec != std::errc()) {
		throw std::system_error(std::make_error_code(res.ec));
	}
	if(res.ptr != s.data() + s.size()) {
		throw std::system_error(std::make_error_code(std::errc::result_out_of_range));
	}
	return ret;
}
}
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
std::string mcwutil::string::todecu(uintmax_t value, unsigned int width) {
	return todec_integer(value, width);
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
std::string mcwutil::string::todecs(intmax_t value, unsigned int width) {
	return todec_integer(value, width);
}

/**
 * \brief Converts a single-precision floating-point value to a decimal string,
 * potentially in scientific notation.
 *
 * \param[in] value the value to convert.
 *
 * \return the decimal string.
 */
std::string mcwutil::string::todecf(float value) {
	return todec_floating(value);
}

/**
 * \brief Converts a double-precision floating-point value to a decimal string,
 * potentially in scientific notation.
 *
 * \param[in] value the value to convert.
 *
 * \return the decimal string.
 */
std::string mcwutil::string::todecd(double value) {
	return todec_floating(value);
}

/**
 * \brief Converts a decimal string to a signed 8-bit integer.
 *
 * \param[in] s the string to convert.
 *
 * \return the integer value.
 */
int8_t mcwutil::string::fromdecs8(std::string_view s) {
	return fromdec<int8_t>(s);
}

/**
 * \brief Converts a decimal string to a signed 16-bit integer.
 *
 * \param[in] s the string to convert.
 *
 * \return the integer value.
 */
int16_t mcwutil::string::fromdecs16(std::string_view s) {
	return fromdec<int16_t>(s);
}

/**
 * \brief Converts a decimal string to a signed 32-bit integer.
 *
 * \param[in] s the string to convert.
 *
 * \return the integer value.
 */
int32_t mcwutil::string::fromdecs32(std::string_view s) {
	return fromdec<int32_t>(s);
}

/**
 * \brief Converts a decimal string to a signed 64-bit integer.
 *
 * \param[in] s the string to convert.
 *
 * \return the integer value.
 */
int64_t mcwutil::string::fromdecs64(std::string_view s) {
	return fromdec<int64_t>(s);
}

/**
 * \brief Converts a decimal string to an unsigned 32-bit integer.
 *
 * \param[in] s the string to convert.
 *
 * \return the integer value.
 */
uint32_t mcwutil::string::fromdecu32(std::string_view s) {
	return fromdec<uint32_t>(s);
}

/**
 * \brief Converts a decimal string to an unsigned native-sized integer.
 *
 * \param[in] s the string to convert.
 *
 * \return the integer value.
 */
unsigned int mcwutil::string::fromdecui(std::string_view s) {
	return fromdec<unsigned int>(s);
}

/**
 * \brief Converts a decimal string, possibly in scientific notation, to a
 * single-precision floating-point value.
 *
 * \param[in] s the string to convert.
 *
 * \return the floating-point value.
 */
float mcwutil::string::fromdecf(std::string_view s) {
	return fromdec<float>(s);
}

/**
 * \brief Converts a decimal string, possibly in scientific notation, to a
 * double-precision floating-point value.
 *
 * \param[in] s the string to convert.
 *
 * \return the floating-point value.
 */
double mcwutil::string::fromdecd(std::string_view s) {
	return fromdec<double>(s);
}

/**
 * \brief Converts a locale string to a UTF-8 string.
 *
 * \param[in] lstr the string to convert.
 *
 * \return the converted string.
 */
std::u8string mcwutil::string::l2u(std::string_view lstr) {
	std::u8string ret;
	ret.reserve(lstr.size());
	std::mbstate_t mbs{};
	for(;;) {
		char8_t converted;
		// Artificially introduce a terminating NUL, which may not actually be
		// present in lstr, to allow mbrtoc8 to flush out any accumulated
		// multi-code-unit state.
		const char *lstr_pointer = lstr.size() ? lstr.data() : "";
		std::size_t lstr_size = lstr.size() ? lstr.size() : 1;
		std::size_t consumed = std::mbrtoc8(&converted, lstr_pointer, lstr_size, &mbs);
		if(consumed == 0 && lstr.size()) {
			// A NUL was converted, but the NUL actually existed in lstr. Treat
			// that as a normal conversion, not a termination condition.
			consumed = 1;
		}
		switch(consumed) {
			case 0:
				// The artificial terminating NUL was reached. Stop, without
				// storing it in the output.
				return ret;

			case static_cast<std::size_t>(-1):
				// An encoding error occurred.
			case static_cast<std::size_t>(-2):
				// The input ends with an incomplete multibyte character.
				throw std::system_error(std::make_error_code(std::errc::illegal_byte_sequence));

			case static_cast<std::size_t>(-3):
				// The last multibyte character consumed produced more than one
				// UTF-8 code unit, and this is one of the second and
				// subsequent ones.
				ret.push_back(converted);
				break;

			default:
				// A multibyte character was consumed and produced a UTF-8 code
				// unit.
				ret.push_back(converted);
				lstr.remove_prefix(consumed);
				break;
		}
	}
	return ret;
}

/**
 * \brief Converts a UTF-8 string to a locale string.
 *
 * \param[in] ustr the string to convert.
 *
 * \return the converted string.
 */
std::string mcwutil::string::u2l(std::u8string_view ustr) {
	std::string ret;
	ret.reserve(ustr.size());
	std::mbstate_t mbs{};
	for(char8_t i : ustr) {
		char buffer[MB_CUR_MAX];
		std::size_t written = std::c8rtomb(buffer, i, &mbs);
		ret.append(buffer, written);
	}
	return ret;
}
