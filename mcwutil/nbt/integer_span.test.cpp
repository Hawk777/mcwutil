#include <mcwutil/nbt/integer_span.hpp>
#include <array>
#include <cppunit/TestAssert.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>
#include <limits>
#include <test_helpers/helpers.hpp>
#include <vector>

namespace mcwutil::nbt {
namespace {
/**
 * \brief A test case for integer range decoding.
 *
 * \tparam T the type of the integers.
 */
template<std::signed_integral T>
struct test_case final {
	/**
	 * \brief The block of bytes to iterate over.
	 */
	std::vector<uint8_t> input;

	/**
	 * \brief The integers that should be decoded.
	 */
	std::vector<T> output;
};

/**
 * \brief A container for all the test cases for a given type of integer.
 */
template<std::signed_integral T>
struct test_cases final {
};

template<>
struct test_cases<int32_t> final {
	static const test_case<int32_t> cases[];
};

const test_case<int32_t> test_cases<int32_t>::cases[]{
	{{}, {}},
	{{0x00, 0x00, 0x00, 0x01}, {1}},
	{{0x01, 0x00, 0x00, 0x00}, {0x01000000}},
	{{0xFF, 0xFF, 0xFF, 0xFF}, {-1}},
	{{0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x02}, {1, 2}},
	{{0x80, 0x00, 0x00, 0x00}, {-2'147'483'648}},
	{{0x7F, 0xFF, 0xFF, 0xFF}, {2'147'483'647}},
};

template<>
struct test_cases<int64_t> final {
	static const test_case<int64_t> cases[];
};

const test_case<int64_t> test_cases<int64_t>::cases[]{
	{{}, {}},
	{{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01}, {1}},
	{{0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00}, {0x01000000}},
	{{0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF}, {0xFFFFFFFF}},
	{{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02}, {1, 2}},
	{{0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00}, {2'147'483'648}},
	{{0x00, 0x00, 0x00, 0x00, 0x7F, 0xFF, 0xFF, 0xFF}, {2'147'483'647}},
	{{0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, {std::numeric_limits<int64_t>::min()}},
	{{0x7F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {std::numeric_limits<int64_t>::max()}},
};

/**
 * \brief Verifies that iteration over integers works properly.
 *
 * \tparam T the type of the integers.
 */
template<std::signed_integral T>
class iteration_test final : public CppUnit::TestFixture {
	public:
	CPPUNIT_TEST_SUITE(iteration_test<T>);
	CPPUNIT_TEST(test_iterate);
	CPPUNIT_TEST_SUITE_END();

	private:
	void test_iterate();
};
}
}

/**
 * \brief Tests iterating over integers of a specific width.
 *
 * \tparam T the type of the integers.
 */
template<std::signed_integral T>
void mcwutil::nbt::iteration_test<T>::test_iterate() {
	using cases = test_cases<T>;
	for(const test_case<T> &i : cases::cases) {
		const integer_span<T> range(std::span(i.input));
		std::vector<T> actual;
		actual.reserve(i.output.size());
		for(T j : range) {
			actual.push_back(j);
		}
		CPPUNIT_ASSERT_EQUAL(i.output, actual);
	}
}

CPPUNIT_TEST_SUITE_REGISTRATION(mcwutil::nbt::iteration_test<int32_t>);
CPPUNIT_TEST_SUITE_REGISTRATION(mcwutil::nbt::iteration_test<int64_t>);
