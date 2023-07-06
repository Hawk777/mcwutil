#ifndef UTIL_CODEC_H
#define UTIL_CODEC_H

#include <concepts>
#include <cstddef>
#include <stdint.h>

namespace mcwutil {
/**
 * \brief Symbols related to converting between bytes and primitive data types.
 */
namespace codec {
uint32_t encode_float_to_u32(float x);
float decode_u32_to_float(uint32_t x);
uint64_t encode_double_to_u64(double x);
double decode_u64_to_double(uint64_t x);

/**
 * \brief Encodes an integer to a byte array.
 *
 * \tparam T the type of integer to encode.
 *
 * \tparam N the number of bytes to encode into, which defaults to the size of
 * \p T.
 *
 * \param[out] b the buffer into which to encode.
 *
 * \param[in] x the integer to encode.
 */
template<std::unsigned_integral T, std::size_t N = sizeof(T)>
inline void encode_integer(void *b, T x) {
	uint8_t *buf = static_cast<uint8_t *>(b);
	for(std::size_t i = N - 1; i < N; --i) {
		buf[i] = static_cast<uint8_t>(x);
		x >>= 8;
	}
}

/**
 * \brief Encodes a floating-point number to a byte array.
 *
 * The floating-point number will consume 4 bytes of storage.
 *
 * \param[out] b the buffer into which to encode.
 *
 * \param[in] x the floating-point number to encode.
 */
inline void encode_float(void *b, float x) {
	encode_integer(b, encode_float_to_u32(x));
}

/**
 * \brief Encodes a floating-point number to a byte array.
 *
 * The floating-point number will consume 8 bytes of storage.
 *
 * \param[out] b the buffer into which to encode.
 *
 * \param[in] x the floating-point number to encode.
 */
inline void encode_double(void *b, double x) {
	encode_integer(b, encode_double_to_u64(x));
}

/**
 * \brief Extracts an 8-bit integer from a data buffer.
 *
 * \param[in] buffer the data to extract from.
 *
 * \return the integer.
 */
inline uint8_t decode_u8(const void *buffer) {
	const uint8_t *buf = static_cast<const uint8_t *>(buffer);
	return buf[0];
}

/**
 * \brief Extracts a 16-bit integer from a data buffer.
 *
 * \param[in] buffer the data to extract from.
 *
 * \return the integer.
 */
inline uint16_t decode_u16(const void *buffer) {
	const uint8_t *buf = static_cast<const uint8_t *>(buffer);
	uint16_t val = 0;
	for(std::size_t i = 0; i < 2; ++i) {
		val = static_cast<uint16_t>((val << 8) | buf[i]);
	}
	return val;
}

/**
 * \brief Extracts a 24-bit integer from a data buffer.
 *
 * \param[in] buffer the data to extract from.
 *
 * \return the integer.
 */
inline uint32_t decode_u24(const void *buffer) {
	const uint8_t *buf = static_cast<const uint8_t *>(buffer);
	uint32_t val = 0;
	for(std::size_t i = 0; i < 3; ++i) {
		val = static_cast<uint32_t>((val << 8) | buf[i]);
	}
	return val;
}

/**
 * \brief Extracts a 32-bit integer from a data buffer.
 *
 * \param[in] buffer the data to extract from.
 *
 * \return the integer.
 */
inline uint32_t decode_u32(const void *buffer) {
	const uint8_t *buf = static_cast<const uint8_t *>(buffer);
	uint32_t val = 0;
	for(std::size_t i = 0; i < 4; ++i) {
		val = static_cast<uint32_t>((val << 8) | buf[i]);
	}
	return val;
}

/**
 * \brief Extracts a 64-bit integer from a data buffer.
 *
 * \param[in] buffer the data to extract from.
 *
 * \return the integer.
 */
inline uint64_t decode_u64(const void *buffer) {
	const uint8_t *buf = static_cast<const uint8_t *>(buffer);
	uint64_t val = 0;
	for(std::size_t i = 0; i < 8; ++i) {
		val = static_cast<uint64_t>((val << 8) | buf[i]);
	}
	return val;
}

/**
 * \brief Extracts a floating-point number from a data buffer.
 *
 * The floating-point number must be 4 bytes wide.
 *
 * \param[in] buffer the data to extract from.
 *
 * \return the floating-point number.
 */
inline float decode_float(const void *buffer) {
	return decode_u32_to_float(decode_u32(buffer));
}

/**
 * \brief Extracts a floating-point number from a data buffer.
 *
 * The floating-point number must be 8 bytes wide.
 *
 * \param[in] buffer the data to extract from.
 *
 * \return the floating-point number.
 */
inline double decode_double(const void *buffer) {
	return decode_u64_to_double(decode_u64(buffer));
}
}
}

#endif
