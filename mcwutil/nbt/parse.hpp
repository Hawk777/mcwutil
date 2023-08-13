#ifndef NBT_PARSE_H
#define NBT_PARSE_H

#include <mcwutil/nbt/integer_span.hpp>
#include <mcwutil/nbt/tags.hpp>
#include <concepts>
#include <cstdint>
#include <iterator>
#include <ranges>
#include <span>
#include <stdexcept>
#include <string_view>
#include <variant>

namespace mcwutil {
namespace nbt {
/**
 * \brief A parser that reads an NBT file and provides it in usable in-memory
 * form.
 */
namespace parse {
/**
 * \brief The callbacks that the parser invokes as it passes through the NBT
 * data.
 *
 * Consumers should subclass this and override the member functions
 * corresponding to the data items they care about. All callbacks have default
 * implementations that do nothing.
 */
class callbacks {
	public:
	/**
	 * \brief The position of a decoded data item within the containing
	 * compound or list.
	 *
	 * The integer form is used for data items contained in lists and indicates
	 * the zero-based position within the list. The string form is used for
	 * data items contained in compounds and for the root data item and
	 * indicates the key associated with the value.
	 */
	using position_t = std::variant<uint32_t, std::u8string_view>;

	virtual void scalar_byte(position_t position, int8_t value);
	virtual void scalar_short(position_t position, int16_t value);
	virtual void scalar_int(position_t position, int32_t value);
	virtual void scalar_long(position_t position, int64_t value);
	virtual void scalar_float(position_t position, float value);
	virtual void scalar_double(position_t position, double value);

	virtual void string(position_t position, std::u8string_view value);
	virtual void byte_array(position_t position, std::span<const uint8_t> value);
	virtual void int_array(position_t position, integer_span<int32_t> value);
	virtual void long_array(position_t position, integer_span<int64_t> value);

	virtual void compound_start(position_t position);
	virtual void compound_end(position_t position);

	virtual void list_start(position_t position, tag subtype, uint32_t length);
	virtual void list_end(position_t position, tag subtype, uint32_t length);
};

/**
 * \brief An exception thrown if an attempt is made to parse an invalid NBT
 * file.
 */
class bad_nbt : public std::runtime_error {
	public:
	bad_nbt(const char *what_arg);
};

std::size_t parse(std::span<const uint8_t> nbt, callbacks &cbs);
}
}
}

#endif
