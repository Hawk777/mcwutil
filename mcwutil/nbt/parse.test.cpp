#include <mcwutil/nbt/parse.hpp>
#include <mcwutil/util/string.hpp>
#include <cassert>
#include <cppunit/TestAssert.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>
#include <ostream>
#include <ranges>
#include <test_helpers/helpers.hpp>
#include <variant>
#include <vector>

using namespace std::literals::string_literals;
using namespace std::literals::string_view_literals;

namespace mcwutil::nbt::parse {
namespace {
/**
 * \brief The possible callbacks.
 */
enum class callback {
	SCALAR_BYTE,
	SCALAR_SHORT,
	SCALAR_INT,
	SCALAR_LONG,
	SCALAR_FLOAT,
	SCALAR_DOUBLE,
	STRING,
	BYTE_ARRAY,
	INT_ARRAY,
	LONG_ARRAY,
	COMPOUND_START,
	COMPOUND_END,
	LIST_START,
	LIST_END,
};

std::ostream &operator<<(std::ostream &stream, callback object) {
	switch(object) {
		case callback::SCALAR_BYTE:
			return stream << "callback::SCALAR_BYTE";
		case callback::SCALAR_SHORT:
			return stream << "callback::SCALAR_SHORT";
		case callback::SCALAR_INT:
			return stream << "callback::SCALAR_INT";
		case callback::SCALAR_LONG:
			return stream << "callback::SCALAR_LONG";
		case callback::SCALAR_FLOAT:
			return stream << "callback::SCALAR_FLOAT";
		case callback::SCALAR_DOUBLE:
			return stream << "callback::SCALAR_DOUBLE";
		case callback::STRING:
			return stream << "callback::STRING";
		case callback::BYTE_ARRAY:
			return stream << "callback::BYTE_ARRAY";
		case callback::INT_ARRAY:
			return stream << "callback::INT_ARRAY";
		case callback::LONG_ARRAY:
			return stream << "callback::LONG_ARRAY";
		case callback::COMPOUND_START:
			return stream << "callback::COMPOUND_START";
		case callback::COMPOUND_END:
			return stream << "callback::COMPOUND_END";
		case callback::LIST_START:
			return stream << "callback::LIST_START";
		case callback::LIST_END:
			return stream << "callback::LIST_END";
	}
	assert(false);
}

/**
 * \brief Information about an NBT list start or end call.
 */
struct list_metadata {
	/**
	 * \brief The subtype.
	 */
	tag subtype;

	/**
	 * \brief The length.
	 */
	uint32_t length;

	auto operator<=>(const list_metadata &) const = default;
};

std::ostream &operator<<(std::ostream &stream, const list_metadata &object) {
	return stream << "list_metadata{" << object.subtype << ", " << object.length << '}';
}

/**
 * \brief Information about the non-position parameters to a single callback
 * invocation.
 */
using callback_params = std::variant<std::monostate, int8_t, int16_t, int32_t, int64_t, float, double, std::u8string, std::vector<uint8_t>, std::vector<int32_t>, std::vector<int64_t>, list_metadata>;

/**
 * \brief Formats a \ref callback_params into a stream.
 *
 * \param[out] stream the stream to write to.
 *
 * \param[in] object the parameters to format.
 */
void format_callback_params(std::ostream &stream, const callback_params &object) {
	if(std::holds_alternative<std::monostate>(object)) {
	} else if(std::holds_alternative<int8_t>(object)) {
		stream << static_cast<int>(std::get<int8_t>(object));
	} else if(std::holds_alternative<int16_t>(object)) {
		stream << std::get<int16_t>(object);
	} else if(std::holds_alternative<int32_t>(object)) {
		stream << std::get<int32_t>(object);
	} else if(std::holds_alternative<int64_t>(object)) {
		stream << std::get<int64_t>(object);
	} else if(std::holds_alternative<float>(object)) {
		stream << std::get<float>(object);
	} else if(std::holds_alternative<double>(object)) {
		stream << std::get<double>(object);
	} else if(std::holds_alternative<std::u8string>(object)) {
		stream << mcwutil::string::u2l(std::get<std::u8string>(object));
	} else if(std::holds_alternative<std::vector<uint8_t>>(object)) {
		stream << CppUnit::assertion_traits<std::span<const uint8_t>>::toString(std::get<std::vector<uint8_t>>(object));
	} else if(std::holds_alternative<std::vector<int32_t>>(object)) {
		stream << CppUnit::assertion_traits<std::span<const int32_t>>::toString(std::get<std::vector<int32_t>>(object));
	} else if(std::holds_alternative<std::vector<int64_t>>(object)) {
		stream << CppUnit::assertion_traits<std::span<const int64_t>>::toString(std::get<std::vector<int64_t>>(object));
	} else if(std::holds_alternative<list_metadata>(object)) {
		stream << std::get<list_metadata>(object);
	} else {
		assert(false);
	}
}

/**
 * \brief Information about a single callback invocation.
 */
struct callback_info {
	/**
	 * \brief Which callback was invoked.
	 */
	callback cb;

	/**
	 * \brief The value of the position parameter.
	 */
	callbacks::position_t position;

	/**
	 * \brief The value of the non-position parameters.
	 */
	callback_params params;

	auto operator<=>(const callback_info &) const = default;
};

std::ostream &operator<<(std::ostream &stream, const callback_info &object) {
	stream << "callback_info{" << object.cb << ", ";
	if(std::holds_alternative<uint32_t>(object.position)) {
		stream << std::get<uint32_t>(object.position);
	} else {
		stream << '"' << mcwutil::string::u2l(std::get<std::u8string_view>(object.position)) << '"';
	}
	stream << ", ";
	format_callback_params(stream, object.params);
	return stream << '}';
}

/**
 * \brief A callback implementation that logs each callback invocation into a
 * vector.
 */
class logging_callbacks final : public callbacks {
	public:
	/**
	 * \brief The callback invocations so far.
	 */
	std::vector<callback_info> calls;

	void scalar_byte(position_t position, int8_t value) override;
	void scalar_short(position_t position, int16_t value) override;
	void scalar_int(position_t position, int32_t value) override;
	void scalar_long(position_t position, int64_t value) override;
	void scalar_float(position_t position, float value) override;
	void scalar_double(position_t position, double value) override;

	void string(position_t position, std::u8string_view value) override;
	void byte_array(position_t position, std::span<const uint8_t> value) override;
	void int_array(position_t position, integer_span<int32_t> value) override;
	void long_array(position_t position, integer_span<int64_t> value) override;

	void compound_start(position_t position) override;
	void compound_end(position_t position) override;

	void list_start(position_t position, tag subtype, uint32_t length) override;
	void list_end(position_t position, tag subtype, uint32_t length) override;
};

/**
 * \brief A single test case for successfully parsing an NBT file.
 */
struct success_test_case {
	/**
	 * \brief A function that, when invoked, returns the byte sequence to
	 * parse.
	 */
	std::vector<uint8_t> (&input)();

	/**
	 * \brief A function that, when invoked, returns the expected callback
	 * sequence.
	 */
	std::vector<callback_info> (&calls)();

	/**
	 * \brief The number of bytes of input that should \em not be consumed by
	 * the parser (i.e. the number of extra bytes left beyond the NBT
	 * structure).
	 */
	std::size_t extra_bytes;
};

/**
 * \brief The test cases for successfully parsing an NBT file.
 */
constinit const auto success_cases = std::array{
	// The smallest possible valid NBT: a TAG_Byte with a zero-length name.
	success_test_case{
		*[]() -> std::vector<uint8_t> {
			return {1, 0, 0, 42};
		},
		*[]() -> std::vector<callback_info> {
			return {
				{callback::SCALAR_BYTE, u8""sv, static_cast<int8_t>(42)},
			};
		},
		0,
	},
	// A TAG_Byte with a name.
	success_test_case{
		*[]() -> std::vector<uint8_t> {
			return {1, 0, 5, u8'h', u8'e', u8'l', u8'l', u8'o', 42};
		},
		*[]() -> std::vector<callback_info> {
			return {
				{callback::SCALAR_BYTE, u8"hello"sv, static_cast<int8_t>(42)},
			};
		},
		0,
	},
	// A TAG_Byte with the name “é”.
	success_test_case{
		*[]() -> std::vector<uint8_t> {
			return {1, 0, 2, 0xC3, 0xA9, 42};
		},
		*[]() -> std::vector<callback_info> {
			return {
				{callback::SCALAR_BYTE, u8"é"sv, static_cast<int8_t>(42)},
			};
		},
		0,
	},
	// A TAG_Byte with a 40,000-character-long name, long enough that, if the
	// length were signed, it would be negative.
	success_test_case{
		*[]() -> std::vector<uint8_t> {
			std::vector<uint8_t> ret{1, 156, 64};
			ret.reserve(ret.size() + 40'000 + 1);
			for(unsigned int i = 0; i != 40'000; ++i) {
				ret.push_back(u8'a');
			}
			ret.push_back(42);
			return ret;
		},
		*[]() -> std::vector<callback_info> {
			static std::u8string big(40'000, u8'a');
			return {
				{callback::SCALAR_BYTE, big, static_cast<int8_t>(42)},
			};
		},
		0,
	},
	// A TAG_Byte with a negative value.
	success_test_case{
		*[]() -> std::vector<uint8_t> {
			return {1, 0, 0, 128};
		},
		*[]() -> std::vector<callback_info> {
			return {
				{callback::SCALAR_BYTE, u8""sv, static_cast<int8_t>(-128)},
			};
		},
		0,
	},
	// A TAG_Short.
	success_test_case{
		*[]() -> std::vector<uint8_t> {
			return {2, 0, 3, u8'a', u8'b', u8'c', 0x12, 0x34};
		},
		*[]() -> std::vector<callback_info> {
			return {
				{callback::SCALAR_SHORT, u8"abc"sv, static_cast<int16_t>(0x1234)},
			};
		},
		0,
	},
	// A TAG_Short with a negative value.
	success_test_case{
		*[]() -> std::vector<uint8_t> {
			return {2, 0, 3, u8'a', u8'b', u8'c', 0xF0, 0x34};
		},
		*[]() -> std::vector<callback_info> {
			return {
				{callback::SCALAR_SHORT, u8"abc"sv, static_cast<int16_t>(-4044)},
			};
		},
		0,
	},
	// A TAG_Int.
	success_test_case{
		*[]() -> std::vector<uint8_t> {
			return {3, 0, 3, u8'a', u8'b', u8'c', 0x12, 0x34, 0x56, 0x78};
		},
		*[]() -> std::vector<callback_info> {
			return {
				{callback::SCALAR_INT, u8"abc"sv, static_cast<int32_t>(0x12345678)},
			};
		},
		0,
	},
	// A TAG_Int with a negative value.
	success_test_case{
		*[]() -> std::vector<uint8_t> {
			return {3, 0, 3, u8'a', u8'b', u8'c', 0xF0, 0x34, 0x56, 0x78};
		},
		*[]() -> std::vector<callback_info> {
			return {
				{callback::SCALAR_INT, u8"abc"sv, static_cast<int32_t>(-265'005'448)},
			};
		},
		0,
	},
	// A TAG_Long.
	success_test_case{
		*[]() -> std::vector<uint8_t> {
			return {4, 0, 3, u8'a', u8'b', u8'c', 0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xEF, 0xFF};
		},
		*[]() -> std::vector<callback_info> {
			return {
				{callback::SCALAR_LONG, u8"abc"sv, static_cast<int64_t>(0x123456789ABCEFFF)},
			};
		},
		0,
	},
	// A TAG_Long with a negative value.
	success_test_case{
		*[]() -> std::vector<uint8_t> {
			return {4, 0, 3, u8'a', u8'b', u8'c', 0xF0, 0x34, 0x56, 0x78, 0x76, 0x54, 0x32, 0x10};
		},
		*[]() -> std::vector<callback_info> {
			return {
				{callback::SCALAR_LONG, u8"abc"sv, static_cast<int64_t>(-1'138'189'730'436'599'280)},
			};
		},
		0,
	},
	// A TAG_Float.
	success_test_case{
		*[]() -> std::vector<uint8_t> {
			return {5, 0, 3, u8'a', u8'b', u8'c', 0xC0, 0x20, 0x00, 0x00};
		},
		*[]() -> std::vector<callback_info> {
			return {
				{callback::SCALAR_FLOAT, u8"abc"sv, -2.5f},
			};
		},
		0,
	},
	// A TAG_Double.
	success_test_case{
		*[]() -> std::vector<uint8_t> {
			return {6, 0, 3, u8'a', u8'b', u8'c', 0xC0, 0x10, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00};
		},
		*[]() -> std::vector<callback_info> {
			return {
				{callback::SCALAR_DOUBLE, u8"abc"sv, -4.0000019073486328125},
			};
		},
		0,
	},
	// A zero-length TAG_Byte_Array.
	success_test_case{
		*[]() -> std::vector<uint8_t> {
			return {7, 0, 3, u8'a', u8'b', u8'c', 0, 0, 0, 0};
		},
		*[]() -> std::vector<callback_info> {
			return {
				{callback::BYTE_ARRAY, u8"abc"sv, std::vector<uint8_t>{}},
			};
		},
		0,
	},
	// A seven-length TAG_Byte_Array.
	success_test_case{
		*[]() -> std::vector<uint8_t> {
			return {7, 0, 3, u8'a', u8'b', u8'c', 0, 0, 0, 7, 1, 2, 3, 4, 5, 6, 7};
		},
		*[]() -> std::vector<callback_info> {
			return {
				{callback::BYTE_ARRAY, u8"abc"sv, std::vector<uint8_t>{1, 2, 3, 4, 5, 6, 7}},
			};
		},
		0,
	},
	// A 257-length TAG_Byte_Array.
	success_test_case{
		*[]() -> std::vector<uint8_t> {
			std::vector<uint8_t> ret{7, 0, 3, u8'a', u8'b', u8'c', 0, 0, 1, 1};
			ret.reserve(ret.size() + 257);
			for(unsigned int i = 0; i != 257; ++i) {
				ret.push_back(static_cast<uint8_t>(i));
			}
			return ret;
		},
		*[]() -> std::vector<callback_info> {
			std::vector<uint8_t> v;
			v.reserve(257);
			for(unsigned int i = 0; i != 257; ++i) {
				v.push_back(static_cast<uint8_t>(i));
			}
			return {
				{callback::BYTE_ARRAY, u8"abc"sv, std::move(v)},
			};
		},
		0,
	},
	// An empty TAG_String.
	success_test_case{
		*[]() -> std::vector<uint8_t> {
			return {8, 0, 3, u8'a', u8'b', u8'c', 0, 0};
		},
		*[]() -> std::vector<callback_info> {
			return {
				{callback::STRING, u8"abc"sv, u8""s},
			};
		},
		0,
	},
	// A non-empty TAG_String.
	success_test_case{
		*[]() -> std::vector<uint8_t> {
			return {8, 0, 3, u8'a', u8'b', u8'c', 0, 4, u8'e', u8'f', u8'g', u8'h'};
		},
		*[]() -> std::vector<callback_info> {
			return {
				{callback::STRING, u8"abc"sv, u8"efgh"s},
			};
		},
		0,
	},
	// A TAG_String “é”.
	success_test_case{
		*[]() -> std::vector<uint8_t> {
			return {8, 0, 3, u8'a', u8'b', u8'c', 0, 2, 0xC3, 0xA9};
		},
		*[]() -> std::vector<callback_info> {
			return {
				{callback::STRING, u8"abc"sv, u8"é"s},
			};
		},
		0,
	},
	// A 258-length TAG_String.
	success_test_case{
		*[]() -> std::vector<uint8_t> {
			std::vector<uint8_t> ret{8, 0, 3, u8'a', u8'b', u8'c', 1, 2};
			for(unsigned int i = 0; i != 258; ++i) {
				ret.push_back(u8'a');
			}
			return ret;
		},
		*[]() -> std::vector<callback_info> {
			return {
				{callback::STRING, u8"abc"sv, std::u8string(258, u8'a')},
			};
		},
		0,
	},
	// A 40,000-length TAG_String (long enough that, if the length were signed,
	// it would be negative).
	success_test_case{
		*[]() -> std::vector<uint8_t> {
			std::vector<uint8_t> ret{8, 0, 3, u8'a', u8'b', u8'c', 156, 64};
			ret.reserve(ret.size() + 40'000);
			for(unsigned int i = 0; i != 40'000; ++i) {
				ret.push_back(u8'a');
			}
			return ret;
		},
		*[]() -> std::vector<callback_info> {
			return {
				{callback::STRING, u8"abc"sv, std::u8string(40'000, u8'a')},
			};
		},
		0,
	},
	// A TAG_List of TAG_Ints.
	success_test_case{
		*[]() -> std::vector<uint8_t> {
			return {
				9, 0, 3, u8'a', u8'b', u8'c', 3, 0, 0, 0, 4, // TAG_List(TAG_Int, 4)
				0, 0, 0, 1, // Element 0
				0, 0, 0, 2, // Element 1
				0, 0, 0, 3, // Element 2
				0xFF, 0xFF, 0xFF, 0xFF, // Element 3
			};
		},
		*[]() -> std::vector<callback_info> {
			return {
				{callback::LIST_START, u8"abc"sv, list_metadata{TAG_INT, 4}},
				{callback::SCALAR_INT, static_cast<uint32_t>(0), static_cast<int32_t>(1)},
				{callback::SCALAR_INT, static_cast<uint32_t>(1), static_cast<int32_t>(2)},
				{callback::SCALAR_INT, static_cast<uint32_t>(2), static_cast<int32_t>(3)},
				{callback::SCALAR_INT, static_cast<uint32_t>(3), static_cast<int32_t>(-1)},
				{callback::LIST_END, u8"abc"sv, list_metadata{TAG_INT, 4}},
			};
		},
		0,
	},
	// An empty TAG_List of TAG_End (TAG_End is not valid for nonempty lists,
	// but it is for empty lists).
	success_test_case{
		*[]() -> std::vector<uint8_t> {
			return {
				9, 0, 3, u8'a', u8'b', u8'c', 0, 0, 0, 0, 0, // TAG_List(TAG_End, 0)
			};
		},
		*[]() -> std::vector<callback_info> {
			return {
				{callback::LIST_START, u8"abc"sv, list_metadata{TAG_END, 0}},
				{callback::LIST_END, u8"abc"sv, list_metadata{TAG_END, 0}},
			};
		},
		0,
	},
	// An empty TAG_Compound.
	success_test_case{
		*[]() -> std::vector<uint8_t> {
			return {10, 0, 3, u8'a', u8'b', u8'c', 0};
		},
		*[]() -> std::vector<callback_info> {
			return {
				{callback::COMPOUND_START, u8"abc"sv, std::monostate{}},
				{callback::COMPOUND_END, u8"abc"sv, std::monostate{}},
			};
		},
		0,
	},
	// A TAG_Compound with a few different elements in it.
	success_test_case{
		*[]() -> std::vector<uint8_t> {
			return {
				10, 0, 3, u8'a', u8'b', u8'c', // TAG_Compound
				1, 0, 2, u8'd', u8'e', 42, // TAG_Byte
				8, 0, 4, u8'f', u8'g', u8'h', u8'i', 0, 2, 0xC3, 0xA9, // TAG_String
				0, // TAG_End
			};
		},
		*[]() -> std::vector<callback_info> {
			return {
				{callback::COMPOUND_START, u8"abc"sv, std::monostate{}},
				{callback::SCALAR_BYTE, u8"de"sv, static_cast<int8_t>(42)},
				{callback::STRING, u8"fghi"sv, u8"é"s},
				{callback::COMPOUND_END, u8"abc"sv, std::monostate{}},
			};
		},
		0,
	},
	// A TAG_Int_Array.
	success_test_case{
		*[]() -> std::vector<uint8_t> {
			return {
				11, 0, 3, u8'a', u8'b', u8'c', 0, 0, 0, 3, // Header
				0, 0, 0, 1, // Element 0
				0, 0, 0, 2, // Element 1
				0xFF, 0xFF, 0xFF, 0xFF, // Element 2
			};
		},
		*[]() -> std::vector<callback_info> {
			return {
				{callback::INT_ARRAY, u8"abc"sv, std::vector<int32_t>{1, 2, -1}},
			};
		},
		0,
	},
	// A TAG_Long_Array.
	success_test_case{
		*[]() -> std::vector<uint8_t> {
			return {
				12, 0, 3, u8'a', u8'b', u8'c', 0, 0, 0, 3, // Header
				0, 0, 0, 0, 0, 0, 0, 1, // Element 0
				0x12, 0x23, 0x34, 0x45, 0x56, 0x67, 0x78, 0x89, // Element 1
				0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // Element 2
			};
		},
		*[]() -> std::vector<callback_info> {
			return {
				{
					callback::LONG_ARRAY,
					u8"abc"sv,
					std::vector<int64_t>{
						1,
						0x1223344556677889,
						-1,
					},
				},
			};
		},
		0,
	},
	// A TAG_Short with trailing bytes.
	success_test_case{
		*[]() -> std::vector<uint8_t> {
			return {2, 0, 5, u8'a', u8'b', u8'c', u8'd', u8'e', 1, 2, 3, 4, 5};
		},
		*[]() -> std::vector<callback_info> {
			return {
				{callback::SCALAR_SHORT, u8"abcde"sv, static_cast<int16_t>(0x0102)},
			};
		},
		3,
	},
	// A complicated data structure.
	success_test_case{
		*[]() -> std::vector<uint8_t> {
			return {
				// clang-format off
				10, 0, 0, // TAG_Compound
					1, 0, 4, u8'b', u8'y', u8't', u8'e', 42, // TAG_Byte
					9, 0, 4, u8'l', u8'i', u8's', u8't', 8, 0, 0, 0, 2, // TAG_List(TAG_String, 2)
						0, 2, u8's', u8'1', // TAG_String
						0, 2, u8's', u8'2', // TAG_String
					9, 0, 5, u8'l', u8'i', u8's', u8't', u8'2', 9, 0, 0, 0, 2, // TAG_List(TAG_List, 2)
						2, 0, 0, 0, 3, // TAG_List(TAG_Short, 3)
							1, 2, // TAG_Short
							3, 4, // TAG_Short
							5, 6, // TAG_Short
						1, 0, 0, 0, 1, // TAG_List(TAG_Byte, 1)
							42, // TAG_Byte
					10, 0, 5, u8'i', u8'n', u8'n', u8'e', u8'r', // TAG_Compound
						1, 0, 2, u8'i', u8'b', 27, // TAG_Byte
						8, 0, 2, u8'i', u8's', 0, 3, u8'x', u8'y', u8'z', // TAG_String
					0, // TAG_End
					2, 0, 5, u8's', u8'h', u8'o', u8'r', u8't', 7, 8, // TAG_Short
				0, // TAG_End
				100, 100, 100, // Trailing bytes
				// clang-format on
			};
		},
		*[]() -> std::vector<callback_info> {
			constexpr callbacks::position_t zero(static_cast<uint32_t>(0));
			constexpr callbacks::position_t one(static_cast<uint32_t>(1));
			constexpr callbacks::position_t two(static_cast<uint32_t>(2));
			return {
				// clang-format off
				{callback::COMPOUND_START, u8""sv, std::monostate{}},
					{callback::SCALAR_BYTE, u8"byte"sv, static_cast<int8_t>(42)},
					{callback::LIST_START, u8"list"sv, list_metadata{TAG_STRING, 2}},
						{callback::STRING, zero, u8"s1"s},
						{callback::STRING, one, u8"s2"s},
					{callback::LIST_END, u8"list"sv, list_metadata{TAG_STRING, 2}},
					{callback::LIST_START, u8"list2"sv, list_metadata{TAG_LIST, 2}},
						{callback::LIST_START, zero, list_metadata{TAG_SHORT, 3}},
							{callback::SCALAR_SHORT, zero, static_cast<int16_t>(0x0102)},
							{callback::SCALAR_SHORT, one, static_cast<int16_t>(0x0304)},
							{callback::SCALAR_SHORT, two, static_cast<int16_t>(0x0506)},
						{callback::LIST_END, zero, list_metadata{TAG_SHORT, 3}},
						{callback::LIST_START, one, list_metadata{TAG_BYTE, 1}},
							{callback::SCALAR_BYTE, zero, static_cast<int8_t>(42)},
						{callback::LIST_END, one, list_metadata{TAG_BYTE, 1}},
					{callback::LIST_END, u8"list2"sv, list_metadata{TAG_LIST, 2}},
					{callback::COMPOUND_START, u8"inner"sv, std::monostate{}},
						{callback::SCALAR_BYTE, u8"ib"sv, static_cast<int8_t>(27)},
						{callback::STRING, u8"is"sv, u8"xyz"s},
					{callback::COMPOUND_END, u8"inner"sv, std::monostate{}},
					{callback::SCALAR_SHORT, u8"short"sv, static_cast<int16_t>(0x0708)},
				{callback::COMPOUND_END, u8""sv, std::monostate{}},
				// clang-format on
			};
		},
		3,
	},
};

/**
 * \brief A single test case for attempting to parse an invalid NBT file.
 */
struct failure_test_case {
	/**
	 * \brief A function that, when invoked, returns the byte sequence to
	 * parse.
	 */
	std::vector<uint8_t> (&input)();
};

/**
 * \brief The test cases for attempt to parse an invalid NBT file.
 */
constinit const auto failure_cases = std::array{
	// A totally empty NBT.
	failure_test_case{*[]() -> std::vector<uint8_t> {
		return {};
	}},
	// An NBT that comprises only a TAG_End.
	failure_test_case{*[]() -> std::vector<uint8_t> {
		return {0};
	}},
	// A root tag with a tag type but no name length.
	failure_test_case{*[]() -> std::vector<uint8_t> {
		return {1};
	}},
	// A root tag with a tag type but only half the name length.
	failure_test_case{*[]() -> std::vector<uint8_t> {
		return {1, 0};
	}},
	// A root tag with a tag type and nonzero name length but no name.
	failure_test_case{*[]() -> std::vector<uint8_t> {
		return {1, 0, 1};
	}},
	// A root tag with a tag type and nonzero name length but only part of the
	// name.
	failure_test_case{*[]() -> std::vector<uint8_t> {
		return {1, 0, 2, u8'h'};
	}},
	// A root tag with an invalid tag type.
	failure_test_case{*[]() -> std::vector<uint8_t> {
		return {13, 0, 2, u8'h', u8'i', 1, 2, 3, 4, 5, 6, 7, 8};
	}},
	// A root tag with a name that is invalid UTF-8.
	failure_test_case{*[]() -> std::vector<uint8_t> {
		return {1, 0, 2, 0xC0, 0x00, 42};
	}},
	// A root TAG_Byte with one byte of data missing.
	failure_test_case{*[]() -> std::vector<uint8_t> {
		return {1, 0, 2, u8'h', u8'i'};
	}},
	// A root TAG_Short with one byte of data missing.
	failure_test_case{*[]() -> std::vector<uint8_t> {
		return {2, 0, 2, u8'h', u8'i', 1};
	}},
	// A root TAG_Int with one byte of data missing.
	failure_test_case{*[]() -> std::vector<uint8_t> {
		return {3, 0, 2, u8'h', u8'i', 1, 2, 3};
	}},
	// A root TAG_Long with one byte of data missing.
	failure_test_case{*[]() -> std::vector<uint8_t> {
		return {4, 0, 2, u8'h', u8'i', 1, 2, 3, 4, 5, 6, 7};
	}},
	// A root TAG_Float with one byte of data missing.
	failure_test_case{*[]() -> std::vector<uint8_t> {
		return {5, 0, 2, u8'h', u8'i', 1, 2, 3};
	}},
	// A root TAG_Double with one byte of data missing.
	failure_test_case{*[]() -> std::vector<uint8_t> {
		return {6, 0, 2, u8'h', u8'i', 1, 2, 3, 4, 5, 6, 7};
	}},
	// A root TAG_Byte_Array with part of the array length missing.
	failure_test_case{*[]() -> std::vector<uint8_t> {
		return {7, 0, 2, u8'h', u8'i', 0, 0, 0};
	}},
	// A root TAG_Byte_Array with part of the array contents missing.
	failure_test_case{*[]() -> std::vector<uint8_t> {
		return {7, 0, 2, u8'h', u8'i', 0, 0, 0, 5, 1, 2, 3, 4};
	}},
	// A root TAG_Byte_Array with a negative length.
	failure_test_case{*[]() -> std::vector<uint8_t> {
		return {7, 0, 2, u8'h', u8'i', 0xFF, 0xFF, 0xFF, 0xFF, 1, 2, 3, 4, 5, 6, 7, 8};
	}},
	// A root TAG_String with part of the string length missing.
	failure_test_case{*[]() -> std::vector<uint8_t> {
		return {8, 0, 2, u8'h', u8'i', 0};
	}},
	// A root TAG_String with part of the string contents missing.
	failure_test_case{*[]() -> std::vector<uint8_t> {
		return {8, 0, 2, u8'h', u8'i', 0, 3, u8'a', u8'b'};
	}},
	// A root TAG_List with the subtype missing.
	failure_test_case{*[]() -> std::vector<uint8_t> {
		return {9, 0, 2, u8'h', u8'i'};
	}},
	// A root TAG_List(TAG_End) with part of the list length missing.
	failure_test_case{*[]() -> std::vector<uint8_t> {
		return {9, 0, 2, u8'h', u8'i', 0, 0, 0, 0};
	}},
	// A root TAG_List(TAG_Short, 3) with part of the list contents missing.
	failure_test_case{*[]() -> std::vector<uint8_t> {
		return {9, 0, 2, u8'h', u8'i', 2, 0, 0, 0, 3, 1, 2, 3, 4, 5};
	}},
	// A root TAG_List(TAG_End, 1) which is invalid because TAG_End-subtyped
	// lists must be empty.
	failure_test_case{*[]() -> std::vector<uint8_t> {
		return {9, 0, 2, u8'h', u8'i', 0, 0, 0, 0, 1, 1, 2, 3, 4, 5, 6, 7, 8};
	}},
	// A root TAG_List(TAG_Byte) with a negative length.
	failure_test_case{*[]() -> std::vector<uint8_t> {
		return {9, 0, 2, u8'h', u8'i', 1, 0xFF, 0xFF, 0xFF, 0xFF, 1, 2, 3, 4, 5, 6, 7, 8};
	}},
	// A root TAG_Compound with no contents.
	failure_test_case{*[]() -> std::vector<uint8_t> {
		return {10, 0, 2, u8'h', u8'i'};
	}},
	// A root TAG_Compound with a partial TAG_Byte element.
	failure_test_case{*[]() -> std::vector<uint8_t> {
		return {10, 0, 2, u8'h', u8'i', 1, 0, 5, u8'i', u8'n', u8'n', u8'e', u8'r'};
	}},
	// A root TAG_Compound with a full TAG_Byte but no TAG_End.
	failure_test_case{*[]() -> std::vector<uint8_t> {
		return {10, 0, 2, u8'h', u8'i', 1, 0, 5, u8'i', u8'n', u8'n', u8'e', u8'r', 42};
	}},
	// A root TAG_Compound with a TAG_Byte with a name that is invalid UTF-8.
	failure_test_case{*[]() -> std::vector<uint8_t> {
		return {
			10, 0, 2, u8'h', u8'i', // TAG_Compound
			1, 0, 2, 0xC0, 0x00, 42, // TAG_Byte
			0, // TAG_End
		};
	}},
	// A pair of nested TAG_Compounds, only the inner of which is ended.
	failure_test_case{*[]() -> std::vector<uint8_t> {
		return {
			10, 0, 5, u8'o', u8'u', u8't', u8'e', u8'r', // TAG_Compound
			10, 0, 5, u8'i', u8'n', u8'n', u8'e', u8'r', // TAG_Compound
			0, // TAG_End
		};
	}},
	// A root TAG_Compound with an invalid tag inside.
	failure_test_case{*[]() -> std::vector<uint8_t> {
		return {10, 0, 2, u8'h', u8'i', 13, 0, 2, u8'h', u8'i', 1, 2, 3, 4, 5, 6, 7, 8};
	}},
	// A root TAG_Int_Array with a partial length.
	failure_test_case{*[]() -> std::vector<uint8_t> {
		return {11, 0, 2, u8'h', u8'i', 0, 0, 0};
	}},
	// A root TAG_Int_Array with partial contents.
	failure_test_case{*[]() -> std::vector<uint8_t> {
		return {11, 0, 2, u8'h', u8'i', 0, 0, 0, 3, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
	}},
	// A root TAG_Int_Array with negative length.
	failure_test_case{*[]() -> std::vector<uint8_t> {
		return {11, 0, 2, u8'h', u8'i', 0xFF, 0xFF, 0xFF, 0xFF, 1, 2, 3, 4, 5, 6, 7, 8};
	}},
	// A root TAG_Long_Array with a partial length.
	failure_test_case{*[]() -> std::vector<uint8_t> {
		return {12, 0, 2, u8'h', u8'i', 0, 0, 0};
	}},
	// A root TAG_Long_Array with partial contents.
	failure_test_case{*[]() -> std::vector<uint8_t> {
		return {12, 0, 2, u8'h', u8'i', 0, 0, 0, 2, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
	}},
	// A root TAG_Long_Array with negative length.
	failure_test_case{*[]() -> std::vector<uint8_t> {
		return {12, 0, 2, u8'h', u8'i', 0xFF, 0xFF, 0xFF, 0xFF, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
	}},
};

/**
 * \brief The test suite for NBT parsing.
 */
class parse_test final : public CppUnit::TestFixture {
	public:
	CPPUNIT_TEST_SUITE(parse_test);
	CPPUNIT_TEST(test_success);
	CPPUNIT_TEST(test_failure);
	CPPUNIT_TEST_SUITE_END();

	private:
	void test_success();
	void test_failure();
};
}
}

void mcwutil::nbt::parse::logging_callbacks::scalar_byte(position_t position, int8_t value) {
	calls.emplace_back(callback::SCALAR_BYTE, position, value);
}

void mcwutil::nbt::parse::logging_callbacks::scalar_short(position_t position, int16_t value) {
	calls.emplace_back(callback::SCALAR_SHORT, position, value);
}

void mcwutil::nbt::parse::logging_callbacks::scalar_int(position_t position, int32_t value) {
	calls.emplace_back(callback::SCALAR_INT, position, value);
}

void mcwutil::nbt::parse::logging_callbacks::scalar_long(position_t position, int64_t value) {
	calls.emplace_back(callback::SCALAR_LONG, position, value);
}

void mcwutil::nbt::parse::logging_callbacks::scalar_float(position_t position, float value) {
	calls.emplace_back(callback::SCALAR_FLOAT, position, value);
}

void mcwutil::nbt::parse::logging_callbacks::scalar_double(position_t position, double value) {
	calls.emplace_back(callback::SCALAR_DOUBLE, position, value);
}

void mcwutil::nbt::parse::logging_callbacks::string(position_t position, std::u8string_view value) {
	calls.emplace_back(callback::STRING, position, std::u8string(value));
}

void mcwutil::nbt::parse::logging_callbacks::byte_array(position_t position, std::span<const uint8_t> value) {
	calls.emplace_back(callback::BYTE_ARRAY, position, std::vector(value.begin(), value.end()));
}

void mcwutil::nbt::parse::logging_callbacks::int_array(position_t position, mcwutil::nbt::integer_span<int32_t> value) {
	calls.emplace_back(callback::INT_ARRAY, position, std::vector(value.begin(), value.end()));
}

void mcwutil::nbt::parse::logging_callbacks::long_array(position_t position, mcwutil::nbt::integer_span<int64_t> value) {
	calls.emplace_back(callback::LONG_ARRAY, position, std::vector(value.begin(), value.end()));
}

void mcwutil::nbt::parse::logging_callbacks::compound_start(position_t position) {
	calls.emplace_back(callback::COMPOUND_START, position, std::monostate{});
}

void mcwutil::nbt::parse::logging_callbacks::compound_end(position_t position) {
	calls.emplace_back(callback::COMPOUND_END, position, std::monostate{});
}

void mcwutil::nbt::parse::logging_callbacks::list_start(position_t position, tag subtype, uint32_t length) {
	calls.emplace_back(callback::LIST_START, position, list_metadata{subtype, length});
}

void mcwutil::nbt::parse::logging_callbacks::list_end(position_t position, tag subtype, uint32_t length) {
	calls.emplace_back(callback::LIST_END, position, list_metadata{subtype, length});
}

/**
 * \brief Tests parsing valid NBT data and that the proper callbacks are
 * invoked.
 */
void mcwutil::nbt::parse::parse_test::test_success() {
	for(const success_test_case &i : success_cases) {
		std::vector<uint8_t> input = i.input();
		std::vector<callback_info> expected_calls = i.calls();
		logging_callbacks actual_cbs;
		std::size_t actual_bytes_consumed = parse(input, actual_cbs);
		CPPUNIT_ASSERT_EQUAL(expected_calls, actual_cbs.calls);
		CPPUNIT_ASSERT_EQUAL(input.size() - i.extra_bytes, actual_bytes_consumed);
	}
}

/**
 * \brief Tests that parsing invalid NBT data results in an exception.
 */
void mcwutil::nbt::parse::parse_test::test_failure() {
	for(const failure_test_case &i : failure_cases) {
		std::vector<uint8_t> input = i.input();
		logging_callbacks actual_cbs;
		try {
			parse(input, actual_cbs);
			CPPUNIT_FAIL("expected failure to parse");
		} catch(const bad_nbt &) {
			// Expected, swallow.
		}
	}
}

CPPUNIT_TEST_SUITE_REGISTRATION(mcwutil::nbt::parse::parse_test);
