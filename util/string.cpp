#include "util/string.h"
#include <locale>
#include <sstream>

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

