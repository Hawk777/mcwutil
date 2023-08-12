#ifndef CPPUNIT_HELPERS_H
#define CPPUNIT_HELPERS_H

#include <algorithm>
#include <array>
#include <cppunit/TestAssert.h>
#include <span>
#include <sstream>
#include <string>
#include <utility>

namespace mcwutil {
namespace test_helpers {
template<typename T>
struct assertion_traits_can_delegate_to_span_impl {
	static constexpr bool value = false;
};

template<typename T, std::size_t N>
struct assertion_traits_can_delegate_to_span_impl<std::array<T, N>> {
	static constexpr bool value = true;
};

template<typename T>
concept assertion_traits_can_delegate_to_span = assertion_traits_can_delegate_to_span_impl<T>::value;
}
}

namespace CppUnit {
template<typename T>
struct assertion_traits<std::span<T>> {
	static bool equal(const std::span<T> &x, const std::span<T> &y);
	static bool less(const std::span<T> &x, const std::span<T> &y);
	static bool lessEqual(const std::span<T> &x, const std::span<T> &y);
	static std::string toString(const std::span<T> &x);
};

template<>
struct assertion_traits<std::span<const uint8_t>> {
	static bool equal(const std::span<const uint8_t> &x, const std::span<const uint8_t> &y);
	static bool less(const std::span<const uint8_t> &x, const std::span<const uint8_t> &y);
	static bool lessEqual(const std::span<const uint8_t> &x, const std::span<const uint8_t> &y);
	static std::string toString(const std::span<const uint8_t> &x);
};

template<mcwutil::test_helpers::assertion_traits_can_delegate_to_span T>
struct assertion_traits<T> {
	static bool equal(const T &x, const T &y);
	static bool less(const T &x, const T &y);
	static bool lessEqual(const T &x, const T &y);
	static std::string toString(const T &x);
};
}

template<typename T>
bool CppUnit::assertion_traits<std::span<T>>::equal(const std::span<T> &x, const std::span<T> &y) {
	return std::ranges::equal(x, y);
}

template<typename T>
bool CppUnit::assertion_traits<std::span<T>>::less(const std::span<T> &x, const std::span<T> &y) {
	return std::ranges::lexicographical_compare(x, y);
}

template<typename T>
bool CppUnit::assertion_traits<std::span<T>>::lessEqual(const std::span<T> &x, const std::span<T> &y) {
	return !std::ranges::lexicographical_compare(y, x);
}

template<typename T>
std::string CppUnit::assertion_traits<std::span<T>>::toString(const std::span<T> &x) {
	std::ostringstream oss;
	oss << '{';
	bool first = true;
	for(const T &i : x) {
		if(!first) {
			oss << ", ";
		}
		oss << i;
		first = false;
	}
	oss << '}';
	return std::move(oss).str();
}

template<mcwutil::test_helpers::assertion_traits_can_delegate_to_span T>
bool CppUnit::assertion_traits<T>::equal(const T &x, const T &y) {
	return assertion_traits<std::span<const typename T::value_type>>::equal(std::span(x), std::span(y));
}

template<mcwutil::test_helpers::assertion_traits_can_delegate_to_span T>
bool CppUnit::assertion_traits<T>::less(const T &x, const T &y) {
	return assertion_traits<std::span<const typename T::value_type>>::less(std::span(x), std::span(y));
}

template<mcwutil::test_helpers::assertion_traits_can_delegate_to_span T>
bool CppUnit::assertion_traits<T>::lessEqual(const T &x, const T &y) {
	return assertion_traits<std::span<const typename T::value_type>>::lessEqual(std::span(x), std::span(y));
}

template<mcwutil::test_helpers::assertion_traits_can_delegate_to_span T>
std::string CppUnit::assertion_traits<T>::toString(const T &x) {
	return assertion_traits<std::span<const typename T::value_type>>::toString(std::span(x));
}

#endif
