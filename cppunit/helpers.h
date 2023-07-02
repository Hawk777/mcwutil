#ifndef CPPUNIT_HELPERS_H
#define CPPUNIT_HELPERS_H

#include <array>
#include <cppunit/TestAssert.h>
#include <iomanip>
#include <sstream>
#include <string>
#include <utility>

namespace CppUnit {
template<typename T, std::size_t N>
struct assertion_traits<std::array<T, N>> {
	using array_type = std::array<T, N>;
	static bool equal(const array_type &x, const array_type &y);
	static bool less(const array_type &x, const array_type &y);
	static bool lessEqual(const array_type &x, const array_type &y);
	static std::string toString(const array_type &x);
};
}

template<typename T, std::size_t N>
bool CppUnit::assertion_traits<std::array<T, N>>::equal(const array_type &x, const array_type &y) {
	return x == y;
}

template<typename T, std::size_t N>
bool CppUnit::assertion_traits<std::array<T, N>>::less(const array_type &x, const array_type &y) {
	return x < y;
}

template<typename T, std::size_t N>
bool CppUnit::assertion_traits<std::array<T, N>>::lessEqual(const array_type &x, const array_type &y) {
	return x <= y;
}

template<typename T, std::size_t N>
std::string CppUnit::assertion_traits<std::array<T, N>>::toString(const array_type &x) {
	std::ostringstream oss;
	oss << std::hex;
	oss << '{';
	bool first = true;
	for(const T &i : x) {
		if(!first) {
			oss << ", ";
		}
		oss << "0x" << static_cast<unsigned int>(i);
		first = false;
	}
	oss << '}';
	return std::move(oss).str();
}

#endif
