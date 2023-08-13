#include <mcwutil/nbt/parse.hpp>
#include <mcwutil/util/codec.hpp>
#include <mcwutil/util/string.hpp>
#include <cassert>
#include <concepts>
#include <stack>
#include <type_traits>

using namespace mcwutil::nbt::parse;

namespace mcwutil::nbt::parse {
namespace {
/**
 * \brief Consumes one scalar integer payload from NBT data and returns it.
 *
 * \throws bad_nbt if there are not enough bytes in \p nbt.
 *
 * \tparam T the (signed or unsigned) type of integer.
 *
 * \param[in, out] nbt the NBT bytes, which are replaced with the span of bytes
 * following the payload on success.
 *
 * \return the integer value.
 */
template<std::integral T>
T consume_integer(std::span<const uint8_t> &nbt) {
	using UT = std::make_unsigned_t<T>;
	constexpr std::size_t ut_octets = sizeof(UT) / sizeof(uint8_t);
	if(nbt.size() < ut_octets) {
		throw bad_nbt("premature end of NBT");
	}
	UT u = codec::decode_integer<UT>(nbt.data());
	nbt = nbt.subspan(ut_octets);
	return static_cast<T>(u);
}

/**
 * \brief Consumes one string with two-byte length prefix from the NBT data and
 * returns it.
 *
 * \throws bad_nbt if there are not enough bytes in \p nbt or the string
 * contains invalid UTF-8.
 *
 * \param[in, out] nbt the NBT bytes, which are replaced with the span of bytes
 * following the string on success.
 *
 * \return the string.
 */
std::u8string_view consume_string(std::span<const uint8_t> &nbt) {
	uint16_t length = consume_integer<uint16_t>(nbt);
	if(nbt.size() < length) {
		throw bad_nbt("premature end of NBT");
	}
	std::span<const uint8_t> name_bytes = nbt.first(length);
	nbt = nbt.subspan(length);
	if(!string::utf8_valid(name_bytes)) {
		throw bad_nbt("invalid UTF-8");
	}
	return std::u8string_view(reinterpret_cast<const char8_t *>(name_bytes.data()), name_bytes.size());
}

/**
 * \brief Consumes one byte array with four-byte length prefix from the NBT
 * data and returns it.
 *
 * \throws bad_nbt if there are not enough bytes in \p nbt or the array length
 * is negative.
 *
 * \param[in, out] nbt the NBT bytes, which are replaced by the span of bytes
 * following the array on success.
 *
 * \return the array.
 */
std::span<const uint8_t> consume_byte_array(std::span<const uint8_t> &nbt) {
	int32_t length_signed = consume_integer<int32_t>(nbt);
	if(length_signed < 0) {
		throw bad_nbt("negative array length");
	}
	uint32_t length = static_cast<uint32_t>(length_signed);
	if(nbt.size() < length) {
		throw bad_nbt("premature end of NBT");
	}
	std::span<const uint8_t> ret = nbt.first(length);
	nbt = nbt.subspan(length);
	return ret;
}

/**
 * \brief Consumes one integer array with four-byte length prefix from the NBT
 * data and returns it.
 *
 * \throws bad_nbt if there are not enough bytes in \p nbt or the array length
 * is negative.
 *
 * \tparam T the type of integers in the array.
 *
 * \param[in, out] nbt the NBT bytes, which are replaced by the span of bytes
 * following the array on success.
 *
 * \return the array.
 */
template<std::signed_integral T>
integer_span<T> consume_integer_array(std::span<const uint8_t> &nbt) {
	using UT = std::make_unsigned_t<T>;
	constexpr std::size_t ut_octets = sizeof(UT) / sizeof(uint8_t);
	int32_t length_signed = consume_integer<int32_t>(nbt);
	if(length_signed < 0) {
		throw bad_nbt("negative array length");
	}
	uint32_t length = static_cast<uint32_t>(length_signed);
	if(nbt.size() / ut_octets < length) {
		throw bad_nbt("premature end of NBT");
	}
	std::size_t length_octets = static_cast<std::size_t>(length) * ut_octets;
	std::span<const uint8_t> bytes = nbt.first(length_octets);
	nbt = nbt.subspan(length_octets);
	return integer_span<T>(bytes);
}
}
}

/**
 * \brief Invoked for an 8-bit integer (other than a member of a byte array).
 *
 * \param[in] position the position in the containing list or compound.
 *
 * \param[in] value the value of the integer.
 */
void callbacks::scalar_byte(position_t, int8_t) {
}

/**
 * \brief Invoked for an 16-bit integer.
 *
 * \param[in] position the position in the containing list or compound.
 *
 * \param[in] value the value of the integer.
 */
void callbacks::scalar_short(position_t, int16_t) {
}

/**
 * \brief Invoked for an 32-bit integer (other than a member of an int array).
 *
 * \param[in] position the position in the containing list or compound.
 *
 * \param[in] value the value of the integer.
 */
void callbacks::scalar_int(position_t, int32_t) {
}

/**
 * \brief Invoked for an 64-bit integer (other than a member of a long array).
 *
 * \param[in] position the position in the containing list or compound.
 *
 * \param[in] value the value of the integer.
 */
void callbacks::scalar_long(position_t, int64_t) {
}

/**
 * \brief Invoked for a single-precision floating-point value.
 *
 * \param[in] position the position in the containing list or compound.
 *
 * \param[in] value the floating-point value.
 */
void callbacks::scalar_float(position_t, float) {
}

/**
 * \brief Invoked for a double-precision floating-point value.
 *
 * \param[in] position the position in the containing list or compound.
 *
 * \param[in] value the floating-point value.
 */
void callbacks::scalar_double(position_t, double) {
}

/**
 * \brief Invoked for a string value.
 *
 * \param[in] position the position in the containing list or compound.
 *
 * \param[in] value the string value.
 */
void callbacks::string(position_t, std::u8string_view) {
}

/**
 * \brief Invoked for a byte array.
 *
 * \param[in] position the position in the containing list or compound.
 *
 * \param[in] value the byte array.
 */
void callbacks::byte_array(position_t, std::span<const uint8_t>) {
}

/**
 * \brief Invoked for a 32-bit integer array.
 *
 * \param[in] position the position in the containing list or compound.
 *
 * \param[in] value the integer array.
 */
void callbacks::int_array(position_t, integer_span<int32_t>) {
}

/**
 * \brief Invoked for a 64-bit integer array.
 *
 * \param[in] position the position in the containing list or compound.
 *
 * \param[in] value the integer array.
 */
void callbacks::long_array(position_t, integer_span<int64_t>) {
}

/**
 * \brief Invoked for entering a compound.
 *
 * After this call, until the matching call to \ref compound_end, any
 * invocation of any function in this class refers to a data item contained,
 * directly or indirectly, within the newly started compound.
 *
 * \post a matching call to \ref compound_end will be made.
 *
 * \param[in] position the position in the containing list or compound.
 */
void callbacks::compound_start(position_t) {
}

/**
 * \brief Invoked for exiting a compound.
 *
 * \pre a matching call to \ref compound_start was made.
 *
 * \pre all the calls for values contained within this compound have been made.
 *
 * \param[in] position the position in the containing list or compound.
 */
void callbacks::compound_end(position_t) {
}

/**
 * \brief Invoked for entering a list.
 *
 * After this call, until the matching call to \ref list_end, any invocation of
 * any function in this class refers to a data item contained, directly or
 * indirectly, within the newly started list. All directly contained values
 * will agree with the \p subtype parameter (e.g. if \p subtype is \ref
 * TAG_SHORT, then only \ref scalar_short will be called until the call to \ref
 * list_end). In the case of a list containing compounds or other lists, the
 * indirectly contained values may be of other types.
 *
 * \post a matching call to \ref list_end will be made.
 *
 * \param[in] position the position in the containing list or compound.
 *
 * \param[in] subtype the type of value directly contained within the list.
 *
 * \param[in] length the number of values directly contained within the list.
 */
void callbacks::list_start(position_t, tag, uint32_t) {
}

/**
 * \brief Invoked for exiting a list.
 *
 * \pre a matching call to \ref list_start was made.
 *
 * \pre all the calls for values contained within this list have been made.
 *
 * \param[in] position the position in the containing list or compound.
 *
 * \param[in] subtype the type of value directly contained within the list.
 *
 * \param[in] length the number of values directly contained within the list.
 */
void callbacks::list_end(position_t, tag, uint32_t) {
}



/**
 * \brief Constructs a new \c bad_nbt.
 *
 * \param[in] what_arg the explanatory string.
 */
bad_nbt::bad_nbt(const char *what_arg) :
		std::runtime_error(what_arg) {
}



/**
 * \brief Parses an NBT file.
 *
 * The first callback call will always have a string position, not an integer
 * position.
 *
 * \throws bad_nbt if the NBT file is malformed.
 *
 * \param[in] nbt the NBT file.
 *
 * \param[in] cbs the callbacks to invoke as the file is parsed.
 *
 * \return the number of bytes parsed, which is normally equal to the size of
 * \p nbt but may differ if there is additional trailing data after the
 * (self-delimiting) NBT structure itself.
 */
std::size_t mcwutil::nbt::parse::parse(std::span<const uint8_t> nbt, callbacks &cbs) {
	// Create a stack of parsing contexts representing the compounds and lists
	// we are currently inside of.
	using compound_context = std::monostate;
	struct list_context {
		tag subtype;
		uint32_t length;
		uint32_t next_pos;
	};
	using context_inner = std::variant<compound_context, list_context>;
	struct context {
		callbacks::position_t position;
		context_inner inner;
	};
	context root_context{
		.position = 0U,
		.inner = compound_context{},
	};
	std::stack<context> stack;

	// Parse one root tag, then continue parsing until not inside any
	// container. Keep track of the remaining bytes.
	std::span<const uint8_t> rest = nbt;
	do {
		// Grab the immediately containing context.
		context &ctx = stack.empty() ? root_context : stack.top();

		// Figure out the tag and position of the current element, as well as
		// whether or not it even actually exists.
		tag current_tag;
		callbacks::position_t current_pos;
		bool end_compound;
		if(compound_context *compound_ctx = std::get_if<compound_context>(&ctx.inner); compound_ctx) {
			// We’re inside a compound (or the fake compound used to force the
			// root element to take this code path). We have either a full
			// tag/name/value element, or a TAG_End.
			if(rest.empty()) {
				throw bad_nbt("premature end of NBT");
			}
			current_tag = static_cast<tag>(rest[0]);
			rest = rest.subspan(1);
			if(current_tag == TAG_END) {
				// This should only ever happen in a real compound, not the
				// root.
				if(stack.empty()) {
					throw bad_nbt("unexpected TAG_End as root element");
				}
				current_pos = 0U;
				end_compound = true;
			} else {
				current_pos = consume_string(rest);
				end_compound = false;
			}
		} else {
			// We’re inside a list. We have only a payload, with the tag given
			// by the list’s subtype.
			list_context *list_ctx = std::get_if<list_context>(&ctx.inner);
			assert(list_ctx);
			current_tag = list_ctx->subtype;
			current_pos = list_ctx->next_pos;
			end_compound = false;
			++list_ctx->next_pos;
		}

		// If we have a legitimate element, handle it.
		if(!end_compound) {
			switch(current_tag) {
				case TAG_END:
					throw bad_nbt("unexpected TAG_End in TAG_List");

				case TAG_BYTE:
					cbs.scalar_byte(current_pos, consume_integer<int8_t>(rest));
					break;

				case TAG_SHORT:
					cbs.scalar_short(current_pos, consume_integer<int16_t>(rest));
					break;

				case TAG_INT:
					cbs.scalar_int(current_pos, consume_integer<int32_t>(rest));
					break;

				case TAG_LONG:
					cbs.scalar_long(current_pos, consume_integer<int64_t>(rest));
					break;

				case TAG_FLOAT:
					cbs.scalar_float(current_pos, codec::decode_u32_to_float(consume_integer<uint32_t>(rest)));
					break;

				case TAG_DOUBLE:
					cbs.scalar_double(current_pos, codec::decode_u64_to_double(consume_integer<uint64_t>(rest)));
					break;

				case TAG_BYTE_ARRAY:
					cbs.byte_array(current_pos, consume_byte_array(rest));
					break;

				case TAG_STRING:
					cbs.string(current_pos, consume_string(rest));
					break;

				case TAG_LIST: {
					tag subtype = static_cast<tag>(consume_integer<uint8_t>(rest));
					int32_t length_signed = consume_integer<int32_t>(rest);
					if(length_signed < 0) {
						throw bad_nbt("negative list length");
					}
					uint32_t length = static_cast<uint32_t>(length_signed);
					cbs.list_start(current_pos, subtype, length);
					stack.push(context{
						.position = current_pos,
						.inner = list_context{
							.subtype = subtype,
							.length = length,
							.next_pos = 0,
						},
					});
				} break;

				case TAG_COMPOUND:
					cbs.compound_start(current_pos);
					stack.push(context{
						.position = current_pos,
						.inner = compound_context{},
					});
					break;

				case TAG_INT_ARRAY:
					cbs.int_array(current_pos, consume_integer_array<int32_t>(rest));
					break;

				case TAG_LONG_ARRAY:
					cbs.long_array(current_pos, consume_integer_array<int64_t>(rest));
					break;

				default:
					throw bad_nbt("invalid tag byte");
			}
		}

		// If a compound just ended, pop its context.
		if(end_compound) {
			assert(std::holds_alternative<compound_context>(ctx.inner));
			cbs.compound_end(ctx.position);
			stack.pop();
		}

		// If one or more lists just ended, pop their contexts.
		for(;;) {
			if(stack.empty()) {
				break;
			}
			const context &ctx = stack.top();
			const list_context *list_ctx = std::get_if<list_context>(&ctx.inner);
			if(!list_ctx) {
				break;
			}
			if(list_ctx->next_pos != list_ctx->length) {
				break;
			}
			cbs.list_end(ctx.position, list_ctx->subtype, list_ctx->length);
			stack.pop();
		}
	} while(!stack.empty());

	// Report how many bytes were parsed.
	return nbt.size() - rest.size();
}
