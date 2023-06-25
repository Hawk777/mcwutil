#include "util/string.h"
#include <cstddef>
#include <cuchar>
#include <locale>
#include <sstream>
#include <system_error>
#include <utility>

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

/**
 * \brief Converts an unsigned integer of any type to a fixed-width decimal string.
 *
 * \param[in] value the value to convert.
 *
 * \param[in] width the width, in characters, of the output to produce.
 *
 * \return the decimal string.
 */
std::string mcwutil::string::todecu_std(uintmax_t value, unsigned int width) {
	std::ostringstream oss;
	oss.imbue(std::locale("C"));
	oss.flags(std::ios::uppercase | std::ios::dec | std::ios::right);
	oss.width(width);
	oss.fill(L'0');
	oss << value;
	return std::move(oss).str();
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
std::string mcwutil::string::todecs_std(intmax_t value, unsigned int width) {
	std::ostringstream oss;
	oss.imbue(std::locale("C"));
	oss.flags(std::ios::uppercase | std::ios::dec | std::ios::right);
	oss.width(width);
	oss.fill(L'0');
	oss << value;
	return std::move(oss).str();
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
