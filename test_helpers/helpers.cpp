#include <iomanip>
#include <test_helpers/helpers.hpp>

bool CppUnit::assertion_traits<std::span<const uint8_t>>::equal(const std::span<const uint8_t> &x, const std::span<const uint8_t> &y) {
	return std::ranges::equal(x, y);
}

bool CppUnit::assertion_traits<std::span<const uint8_t>>::less(const std::span<const uint8_t> &x, const std::span<const uint8_t> &y) {
	return std::ranges::lexicographical_compare(x, y);
}

bool CppUnit::assertion_traits<std::span<const uint8_t>>::lessEqual(const std::span<const uint8_t> &x, const std::span<const uint8_t> &y) {
	return !std::ranges::lexicographical_compare(y, x);
}

std::string CppUnit::assertion_traits<std::span<const uint8_t>>::toString(const std::span<const uint8_t> &x) {
	std::ostringstream oss;
	oss << std::hex;
	oss << '{';
	bool first = true;
	for(uint8_t i : x) {
		if(!first) {
			oss << ", ";
		}
		oss << "0x" << static_cast<unsigned int>(i);
		first = false;
	}
	oss << '}';
	return std::move(oss).str();
}
