#ifndef NBT_TAGS_H
#define NBT_TAGS_H

namespace mcwutil {
namespace nbt {
/**
 * \brief The possible NBT data types.
 */
enum tag {
	/**
	 * \brief A marker indicating the end of the contents of a \ref
	 * TAG_COMPOUND.
	 *
	 * There is no content.
	 */
	TAG_END,

	/**
	 * \brief An 8-bit signed integer.
	 *
	 * The content is the byte value.
	 */
	TAG_BYTE,

	/**
	 * \brief A 16-bit signed integer.
	 *
	 * The content is two bytes making up the integer value, in big-endian
	 * twos-complement encoding.
	 */
	TAG_SHORT,

	/**
	 * \brief A 32-bit signed integer.
	 *
	 * The content is four bytes making up the integer value, in big-endian
	 * twos-complement encoding.
	 */
	TAG_INT,

	/**
	 * \brief A 64-bit signed integer.
	 *
	 * The content is eight bytes making up the integer value, in big-endian
	 * twos-complement encoding.
	 */
	TAG_LONG,

	/**
	 * \brief A 32-bit floating-point value.
	 *
	 * The content is four bytes making up the value, in big-endian IEEE 754
	 * single-precision encoding.
	 */
	TAG_FLOAT,

	/**
	 * \brief A 64-bit floating-point value.
	 *
	 * The content is eight bytes making up the value, in big-endian IEEE 754
	 * double-precision encoding.
	 */
	TAG_DOUBLE,

	/**
	 * \brief A packed array of bytes.
	 *
	 * The content is:
	 * 1. The number of bytes, a signed 32-bit integer, in big-endian
	 *    twos-complement encoding.
	 * 2. The bytes, of the specified length.
	 */
	TAG_BYTE_ARRAY,

	/**
	 * \brief A text string.
	 *
	 * The content is:
	 * 1. The number of bytes of text, an unsigned 16-bit integer, in
	 *    big-endian encoding.
	 * 2. The text, in modified UTF-8 (i.e. with NUL encoded as C0 80 instead
	 *    of 00, and with surrogate pairs encoded per CESU-8).
	 */
	TAG_STRING,

	/**
	 * \brief A sequence of data items.
	 *
	 * The content is:
	 * 1. The \ref tag value of the items in the list, as a single byte.
	 * 2. The number of items in the list, a signed 32-bit integer, in
	 *    big-endian twos-complement encoding.
	 * 3. The items in the list, encoded according as appropriate for their
	 *    data type.
	 */
	TAG_LIST,

	/**
	 * \brief A key-value mapping.
	 *
	 * The content is:
	 * 1. Zero or more key/value pairs, each encoded as indicated below.
	 * 2. A single byte of value \ref TAG_END.
	 *
	 * Each key/value pair is encoded as follows:
	 * 1. The \ref tag value of the value, a single byte.
	 * 2. The key, encoded as if the content of a \ref TAG_STRING.
	 * 3. The value, encoded according to its type.
	 */
	TAG_COMPOUND,

	/**
	 * \brief A packed array of signed 32-bit integers.
	 *
	 * The content is:
	 * 1. The number of integers, a signed 32-bit integer, in big-endian
	 *    twos-complement encoding.
	 * 2. The integers, each in big-endian twos-complement encoding.
	 */
	TAG_INT_ARRAY,

	/**
	 * \brief A packed array of signed 64-bit integers.
	 *
	 * The content is:
	 * 1. The number of integers, a signed 32-bit integer, in big-endian
	 *    twos-complement encoding.
	 * 2. The integers, each in big-endian twos-complement encoding.
	 */
	TAG_LONG_ARRAY,
};
}
}

#endif
