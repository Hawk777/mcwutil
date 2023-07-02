#include "util/codec.h"
#include "cppunit/helpers.h"
#include <array>
#include <cppunit/TestAssert.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>
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
	static constexpr auto encode = encode_u8;
	static constexpr auto decode = decode_u8;
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
	static constexpr auto encode = encode_u16;
	static constexpr auto decode = decode_u16;
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
	static constexpr auto encode = encode_u24;
	static constexpr auto decode = decode_u24;
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
	static constexpr auto encode = encode_u32;
	static constexpr auto decode = decode_u32;
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
	static constexpr auto encode = encode_u64;
	static constexpr auto decode = decode_u64;
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

CPPUNIT_TEST_SUITE_REGISTRATION(mcwutil::codec::integer_test<8>);
CPPUNIT_TEST_SUITE_REGISTRATION(mcwutil::codec::integer_test<16>);
CPPUNIT_TEST_SUITE_REGISTRATION(mcwutil::codec::integer_test<24>);
CPPUNIT_TEST_SUITE_REGISTRATION(mcwutil::codec::integer_test<32>);
CPPUNIT_TEST_SUITE_REGISTRATION(mcwutil::codec::integer_test<64>);
