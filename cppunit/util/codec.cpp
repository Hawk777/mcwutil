#include "util/codec.h"
#include "cppunit/helpers.h"
#include <array>
#include <cmath>
#include <cppunit/TestAssert.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>
#include <limits>
#include <ranges>

namespace mcwutil::codec {
namespace {
/**
 * \brief A single test case for encoding and decoding a number.
 *
 * \tparam T the type of number.
 * \tparam Bits the number of bits in the encoded form.
 */
template<typename T, std::size_t Bits>
struct test_case {
	static_assert(!(Bits % 8));

	/**
	 * \brief The numeric value.
	 */
	T value;

	/**
	 * \brief The encoded bytes.
	 */
	std::array<uint8_t, Bits / 8> bytes;
};

/**
 * \brief Test cases and metadata about encoding and decoding of a specific
 * integer bit width.
 *
 * \tparam Bits the with, in bits, of the integers.
 */
template<std::size_t Bits>
struct integer_info {
};

template<>
struct integer_info<8> {
	static constexpr std::size_t bits = 8;
	using type = uint8_t;
	static constexpr auto cases = std::array{
		test_case<uint8_t, bits>{0, {0}},
		test_case<uint8_t, bits>{1, {1}},
		test_case<uint8_t, bits>{0xFF, {0xFF}},
	};
	static constexpr auto encode = encode_integer<uint8_t>;
	static constexpr auto decode = decode_integer<uint8_t>;
};

template<>
struct integer_info<16> {
	static constexpr std::size_t bits = 16;
	using type = uint16_t;
	static constexpr auto cases = std::array{
		test_case<uint16_t, bits>{0, {0x00, 0x00}},
		test_case<uint16_t, bits>{1, {0x00, 0x01}},
		test_case<uint16_t, bits>{0xFF, {0x00, 0xFF}},
		test_case<uint16_t, bits>{0x100, {0x01, 0x00}},
		test_case<uint16_t, bits>{0x1000, {0x10, 0x00}},
		test_case<uint16_t, bits>{0x1234, {0x12, 0x34}},
		test_case<uint16_t, bits>{0xFFFF, {0xFF, 0xFF}},
	};
	static constexpr auto encode = encode_integer<uint16_t>;
	static constexpr auto decode = decode_integer<uint16_t>;
};

template<>
struct integer_info<24> {
	static constexpr std::size_t bits = 24;
	using type = uint32_t;
	static constexpr auto cases = std::array{
		test_case<uint32_t, bits>{0, {0x00, 0x00, 0x00}},
		test_case<uint32_t, bits>{1, {0x00, 0x00, 0x01}},
		test_case<uint32_t, bits>{0xFF, {0x00, 0x00, 0xFF}},
		test_case<uint32_t, bits>{0x100, {0x00, 0x01, 0x00}},
		test_case<uint32_t, bits>{0x1000, {0x00, 0x10, 0x00}},
		test_case<uint32_t, bits>{0x1234, {0x00, 0x12, 0x34}},
		test_case<uint32_t, bits>{0xFFFF, {0x00, 0xFF, 0xFF}},
		test_case<uint32_t, bits>{0x123456, {0x12, 0x34, 0x56}},
		test_case<uint32_t, bits>{0xFFFFFF, {0xFF, 0xFF, 0xFF}},
	};
	static constexpr auto encode = encode_integer<uint32_t, 3>;
	static constexpr auto decode = decode_integer<uint32_t, 3>;
};

template<>
struct integer_info<32> {
	static constexpr std::size_t bits = 32;
	using type = uint32_t;
	static constexpr auto cases = std::array{
		test_case<uint32_t, bits>{0, {0x00, 0x00, 0x00, 0x00}},
		test_case<uint32_t, bits>{1, {0x00, 0x00, 0x00, 0x01}},
		test_case<uint32_t, bits>{0xFF, {0x00, 0x00, 0x00, 0xFF}},
		test_case<uint32_t, bits>{0x100, {0x00, 0x00, 0x01, 0x00}},
		test_case<uint32_t, bits>{0x1000, {0x00, 0x00, 0x10, 0x00}},
		test_case<uint32_t, bits>{0x1234, {0x00, 0x00, 0x12, 0x34}},
		test_case<uint32_t, bits>{0xFFFF, {0x00, 0x00, 0xFF, 0xFF}},
		test_case<uint32_t, bits>{0x123456, {0x00, 0x12, 0x34, 0x56}},
		test_case<uint32_t, bits>{0xFFFFFF, {0x00, 0xFF, 0xFF, 0xFF}},
		test_case<uint32_t, bits>{0x12345678, {0x12, 0x34, 0x56, 0x78}},
		test_case<uint32_t, bits>{0xFFFFFFFF, {0xFF, 0xFF, 0xFF, 0xFF}},
	};
	static constexpr auto encode = encode_integer<uint32_t>;
	static constexpr auto decode = decode_integer<uint32_t>;
};

template<>
struct integer_info<64> {
	static constexpr std::size_t bits = 64;
	using type = uint64_t;
	static constexpr auto cases = std::array{
		test_case<uint64_t, bits>{0, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
		test_case<uint64_t, bits>{1, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01}},
		test_case<uint64_t, bits>{0xFF, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF}},
		test_case<uint64_t, bits>{0x100, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00}},
		test_case<uint64_t, bits>{0x1000, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00}},
		test_case<uint64_t, bits>{0x1234, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x12, 0x34}},
		test_case<uint64_t, bits>{0xFFFF, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF}},
		test_case<uint64_t, bits>{0x123456, {0x00, 0x00, 0x00, 0x00, 0x00, 0x12, 0x34, 0x56}},
		test_case<uint64_t, bits>{0xFFFFFF, {0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF}},
		test_case<uint64_t, bits>{0x12345678, {0x00, 0x00, 0x00, 0x00, 0x12, 0x34, 0x56, 0x78}},
		test_case<uint64_t, bits>{0xFFFFFFFF, {0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF}},
		test_case<uint64_t, bits>{0x1122334455667788, {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88}},
		test_case<uint64_t, bits>{0xFFFFFFFFFFFFFFFF, {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}},
	};
	static constexpr auto encode = encode_integer<uint64_t>;
	static constexpr auto decode = decode_integer<uint64_t>;
};

/**
 * \brief Verifies that integer encoding and decoding works properly.
 *
 * \tparam Bits the width, in bits, of the integers.
 */
template<std::size_t Bits>
class integer_test final : public CppUnit::TestFixture {
	public:
	CPPUNIT_TEST_SUITE(integer_test<Bits>);
	CPPUNIT_TEST(test_encode);
	CPPUNIT_TEST(test_decode);
	CPPUNIT_TEST_SUITE_END();

	private:
	void test_encode();
	void test_decode();
};

/**
 * \brief Test cases and metadata about encoding and decoding of a specific
 * floating-point type.
 *
 * \tparam T the type of floating-point value.
 */
template<typename T>
struct floating_info {
};

template<>
struct floating_info<float> {
	static constexpr std::size_t bits = 32;
	using type = float;
	static constexpr auto cases = std::array{
		// Ordinary numbers.
		test_case<float, bits>{1.0f, {0x3F, 0x80, 0x00, 0x00}},
		test_case<float, bits>{-1.0f, {0xBF, 0x80, 0x00, 0x00}},
		test_case<float, bits>{27.0f, {0x41, 0xD8, 0x00, 0x00}},
		test_case<float, bits>{-27.0f, {0xC1, 0xD8, 0x00, 0x00}},
		// Positive and negative zero.
		test_case<float, bits>{0.0f, {0x00, 0x00, 0x00, 0x00}},
		test_case<float, bits>{-0.0f, {0x80, 0x00, 0x00, 0x00}},
		// Very large and very small numbers with lots of digits.
		test_case<float, bits>{1.234567e-37f, {0x02, 0x28, 0x0A, 0x62}},
		test_case<float, bits>{1.234567e+38f, {0x7E, 0xB9, 0xC1, 0xCB}},
		// Subnormal numbers.
		test_case<float, bits>{1.0e-40f, {0x00, 0x01, 0x16, 0xC2}},
		test_case<float, bits>{-1.0e-40f, {0x80, 0x01, 0x16, 0xC2}},
		// Special numbers.
		test_case<float, bits>{std::numeric_limits<float>::infinity(), {0x7F, 0x80, 0x00, 0x00}},
		test_case<float, bits>{-std::numeric_limits<float>::infinity(), {0xFF, 0x80, 0x00, 0x00}},
		test_case<float, bits>{std::numeric_limits<float>::quiet_NaN(), {0x7F, 0x80, 0x00, 0x01}},
	};
	static constexpr auto encode = encode_float;
	static constexpr auto decode = decode_float;
};

template<>
struct floating_info<double> {
	static constexpr std::size_t bits = 64;
	using type = double;
	static constexpr auto cases = std::array{
		// Ordinary numbers.
		test_case<double, bits>{1.0, {0x3F, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
		test_case<double, bits>{-1.0, {0xBF, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
		test_case<double, bits>{27.0, {0x40, 0x3B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
		test_case<double, bits>{-27.0, {0xC0, 0x3B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
		// Positive and negative zero.
		test_case<double, bits>{0.0, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
		test_case<double, bits>{-0.0, {0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
		// Very large and very small numbers with lots of digits.
		test_case<double, bits>{1.2345678901234e-308, {0x00, 0x08, 0xE0, 0xA3, 0xA2, 0xBC, 0x2F, 0xAC}},
		test_case<double, bits>{1.2345678901234e+307, {0x7F, 0xB1, 0x94, 0xB1, 0x4B, 0xE2, 0x79, 0x01}},
		// Subnormal numbers.
		test_case<double, bits>{1.0e-310, {0x00, 0x00, 0x12, 0x68, 0x8B, 0x70, 0xE6, 0x2B}},
		test_case<double, bits>{-1.0e-310, {0x80, 0x00, 0x12, 0x68, 0x8B, 0x70, 0xE6, 0x2B}},
		// Special numbers.
		test_case<double, bits>{std::numeric_limits<double>::infinity(), {0x7F, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
		test_case<double, bits>{-std::numeric_limits<double>::infinity(), {0xFF, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
		test_case<double, bits>{std::numeric_limits<double>::quiet_NaN(), {0x7F, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01}},
	};
	static constexpr auto encode = encode_double;
	static constexpr auto decode = decode_double;
};

/**
 * \brief Verifies that floating-point encoding and decoding works properly.
 *
 * \tparam T the type of floating-point value.
 */
template<typename T>
class floating_test final : public CppUnit::TestFixture {
	public:
	CPPUNIT_TEST_SUITE(floating_test<T>);
	CPPUNIT_TEST(test_encode);
	CPPUNIT_TEST(test_decode);
	CPPUNIT_TEST_SUITE_END();

	private:
	void test_encode();
	void test_decode();
};
}
}

/**
 * \brief Tests encoding integers of a specific width.
 *
 * \tparam Bits the width, in bits, of the integers.
 */
template<std::size_t Bits>
void mcwutil::codec::integer_test<Bits>::test_encode() {
	using info = integer_info<Bits>;
	static_assert(!(info::bits % 8));
	constexpr std::size_t bytes = info::bits / 8;
	for(const auto &i : info::cases) {
		// Allocate a two-byte-wider buffer to encode into. Fill it with 0x55
		// except for 0xAA in the first and last positions.
		std::array<uint8_t, bytes + 2> buffer;
		buffer.fill(0x55);
		buffer.front() = buffer.back() = 0xAA;

		// Perform the encoding.
		info::encode(buffer.data() + 1, i.value);

		// Construct the expected buffer contents, which should be 0xAA in the
		// first and last positions and the encoded bytes in the middle.
		std::array<uint8_t, bytes + 2> expected{};
		expected.front() = expected.back() = 0xAA;
		std::copy(i.bytes.begin(), i.bytes.end(), expected.begin() + 1);

		// Verify.
		CPPUNIT_ASSERT_EQUAL(expected, buffer);
	}
}

/**
 * \brief Tests decoding an integer of a specific width.
 *
 * \tparam Bits the width, in bits, of the integers.
 */
template<std::size_t Bits>
void mcwutil::codec::integer_test<Bits>::test_decode() {
	using info = integer_info<Bits>;
	for(const auto &i : info::cases) {
		// Test decoding.
		typename info::type decoded = info::decode(i.bytes.data());
		CPPUNIT_ASSERT_EQUAL(i.value, decoded);
	}
}

/**
 * \brief Tests encoding floating-point numbers of a specific type.
 *
 * \tparam T the type of floating-point value.
 */
template<typename T>
void mcwutil::codec::floating_test<T>::test_encode() {
	using info = floating_info<T>;
	static_assert(!(info::bits % 8));
	constexpr std::size_t bytes = info::bits / 8;
	for(const auto &i : info::cases) {
		// Allocate a two-byte-wider buffer to encode into. Fill it with 0x55
		// except for 0xAA in the first and last positions.
		std::array<uint8_t, bytes + 2> buffer;
		buffer.fill(0x55);
		buffer.front() = buffer.back() = 0xAA;

		// Perform the encoding.
		info::encode(buffer.data() + 1, i.value);

		// Construct the expected buffer contents, which should be 0xAA in the
		// first and last positions and the encoded bytes in the middle.
		std::array<uint8_t, bytes + 2> expected{};
		expected.front() = expected.back() = 0xAA;
		std::copy(i.bytes.begin(), i.bytes.end(), expected.begin() + 1);

		// Verify.
		CPPUNIT_ASSERT_EQUAL(expected, buffer);
	}
}

/**
 * \brief Tests decoding a floating-point numbers of a specific type.
 *
 * \tparam T the type of floating-point value.
 */
template<typename T>
void mcwutil::codec::floating_test<T>::test_decode() {
	using info = floating_info<T>;
	for(const auto &i : info::cases) {
		// Test decoding.
		T decoded = info::decode(i.bytes.data());
		if(std::isnan(i.value)) {
			// NaNs cannot be directly compared, so if a NaN is expected, just
			// verify that a NaN (any NaN) was produced.
			CPPUNIT_ASSERT(std::isnan(decoded));
		} else {
			CPPUNIT_ASSERT_EQUAL(i.value, decoded);
		}
	}
}

CPPUNIT_TEST_SUITE_REGISTRATION(mcwutil::codec::integer_test<8>);
CPPUNIT_TEST_SUITE_REGISTRATION(mcwutil::codec::integer_test<16>);
CPPUNIT_TEST_SUITE_REGISTRATION(mcwutil::codec::integer_test<24>);
CPPUNIT_TEST_SUITE_REGISTRATION(mcwutil::codec::integer_test<32>);
CPPUNIT_TEST_SUITE_REGISTRATION(mcwutil::codec::integer_test<64>);
CPPUNIT_TEST_SUITE_REGISTRATION(mcwutil::codec::floating_test<float>);
CPPUNIT_TEST_SUITE_REGISTRATION(mcwutil::codec::floating_test<double>);
