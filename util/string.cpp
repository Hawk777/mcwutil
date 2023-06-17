#include "util/string.h"
#include <locale>
#include <sstream>

Glib::ustring utf8_literal(const char8_t *s) {
	return reinterpret_cast<const char *>(s);
}

std::u8string_view utf8_wrap(const Glib::ustring &s) {
	return std::u8string_view(reinterpret_cast<const char8_t *>(s.data()), s.bytes());
}

bool operator==(const Glib::ustring &x, std::u8string_view y) {
	return utf8_wrap(x) == y;
}

bool operator==(std::u8string_view x, const Glib::ustring &y) {
	return x == utf8_wrap(y);
}

Glib::ustring todecu(uintmax_t value, unsigned int width) {
	std::wostringstream oss;
	oss.imbue(std::locale("C"));
	oss.flags(std::ios::uppercase | std::ios::dec | std::ios::right);
	oss.width(width);
	oss.fill(L'0');
	oss << value;
	return Glib::ustring::format(oss.str());
}

Glib::ustring todecs(intmax_t value, unsigned int width) {
	std::wostringstream oss;
	oss.imbue(std::locale("C"));
	oss.flags(std::ios::uppercase | std::ios::dec | std::ios::right);
	oss.width(width);
	oss.fill(L'0');
	oss << value;
	return Glib::ustring::format(oss.str());
}

Glib::ustring tohex(uintmax_t value, unsigned int width) {
	std::wostringstream oss;
	oss.imbue(std::locale("C"));
	oss.flags(std::ios::uppercase | std::ios::hex | std::ios::right);
	oss.width(width);
	oss.fill(L'0');
	oss << value;
	return Glib::ustring::format(oss.str());
}

Glib::ustring wstring2ustring(const std::wstring &wstr) {
	return Glib::ustring::format(wstr);
}

std::wstring ustring2wstring(const Glib::ustring &ustr) {
	std::wostringstream oss;
	oss.imbue(std::locale("C"));
	oss << ustr;
	return oss.str();
}

