#ifndef UTIL_CODEC_H
#define UTIL_CODEC_H

#include <cstddef>
#include <stdint.h>

uint32_t encode_float_to_u32(float x);
float decode_u32_to_float(uint32_t x);
uint64_t encode_double_to_u64(double x);
double decode_u64_to_double(uint64_t x);

/**
 * \brief Encodes an 8-bit integer to a byte array.
 *
 * \param[out] b the buffer into which to encode.
 *
 * \param[in] x the integer to encode.
 */
inline void encode_u8(void *b, uint8_t x) {
	uint8_t *buf = static_cast<uint8_t *>(b);
	buf[0] = x;
}

/**
 * \brief Encodes a 16-bit integer to a byte array.
 *
 * \param[out] b the buffer into which to encode.
 *
 * \param[in] x the integer to encode.
 */
inline void encode_u16(void *b, uint16_t x) {
	uint8_t *buf = static_cast<uint8_t *>(b);
	buf[0] = static_cast<uint8_t>(x >> 8);
	buf[1] = static_cast<uint8_t>(x);
}

/**
 * \brief Encodes a 24-bit integer to a byte array.
 *
 * \param[out] b the buffer into which to encode.
 *
 * \param[in] x the integer to encode.
 */
inline void encode_u24(void *b, uint32_t x) {
	uint8_t *buf = static_cast<uint8_t *>(b);
	buf[0] = static_cast<uint8_t>(x >> 16);
	buf[1] = static_cast<uint8_t>(x >> 8);
	buf[2] = static_cast<uint8_t>(x);
}

/**
 * \brief Encodes a 32-bit integer to a byte array.
 *
 * \param[out] b the buffer into which to encode.
 *
 * \param[in] x the integer to encode.
 */
inline void encode_u32(void *b, uint32_t x) {
	uint8_t *buf = static_cast<uint8_t *>(b);
	buf[0] = static_cast<uint8_t>(x >> 24);
	buf[1] = static_cast<uint8_t>(x >> 16);
	buf[2] = static_cast<uint8_t>(x >> 8);
	buf[3] = static_cast<uint8_t>(x);
}

/**
 * \brief Encodes a 64-bit integer to a byte array.
 *
 * \param[out] b the buffer into which to encode.
 *
 * \param[in] x the integer to encode.
 */
inline void encode_u64(void *b, uint64_t x) {
	uint8_t *buf = static_cast<uint8_t *>(b);
	buf[0] = static_cast<uint8_t>(x >> 56);
	buf[1] = static_cast<uint8_t>(x >> 48);
	buf[2] = static_cast<uint8_t>(x >> 40);
	buf[3] = static_cast<uint8_t>(x >> 32);
	buf[4] = static_cast<uint8_t>(x >> 24);
	buf[5] = static_cast<uint8_t>(x >> 16);
	buf[6] = static_cast<uint8_t>(x >> 8);
	buf[7] = static_cast<uint8_t>(x);
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
	encode_u64(b, encode_double_to_u64(x));
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
 * The floating-point number must be 8 bytes wide.
 *
 * \param[in] buffer the data to extract from.
 *
 * \return the floating-point number.
 */
inline double decode_double(const void *buffer) {
	return decode_u64_to_double(decode_u64(buffer));
}

#endif
