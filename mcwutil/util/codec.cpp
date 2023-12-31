#include <mcwutil/util/codec.hpp>
#include <cmath>
#include <limits>

namespace mcwutil::codec {
namespace {
/**
 * \brief Packs a sign bit, a biased exponent, and a significand into a 32-bit integer in IEEE754 single-precision format.
 *
 * \param[in] sign the sign bit, \c true for negative or \c false for positive.
 *
 * \param[in] exponent the biased exponent, between 0 and 0xFF.
 *
 * \param[in] significand the significand.
 *
 * \return the packed 32-bit integer.
 */
std::uint32_t pack_ses32(bool sign, std::uint8_t exponent, std::uint32_t significand) {
	return (sign ? UINT32_C(0x80000000) : 0) | (static_cast<std::uint32_t>(exponent) << 23) | significand;
}

/**
 * \brief Packs a sign bit, a biased exponent, and a significand into a 64-bit integer in IEEE754 double-precision format.
 *
 * \param[in] sign the sign bit, \c true for negative or \c false for positive.
 *
 * \param[in] exponent the biased exponent, between 0 and 0x7FF.
 *
 * \param[in] significand the significand.
 *
 * \return the packed 64-bit integer.
 */
std::uint64_t pack_ses64(bool sign, std::uint16_t exponent, std::uint64_t significand) {
	return (sign ? UINT64_C(0x8000000000000000) : 0) | (static_cast<std::uint64_t>(exponent) << 52) | significand;
}
}
}

/**
 * \brief Encodes a floating-point number in IEEE754 single-precision format.
 *
 * \param[in] x the value to encode.
 *
 * \return the encoded form.
 */
std::uint32_t mcwutil::codec::encode_float_to_u32(float x) {
	// Break down the number based on its coarse classification.
	int classify = std::fpclassify(x);
	if(classify == FP_NAN) {
		// NaN values are encoded by a biased exponent of 0xFF and a nonzero significand.
		return pack_ses32(false, 0xFF, 1);
	} else if(classify == FP_INFINITE) {
		// Infinities are encoded by a biased exponent of 0xFF and a zero significand.
		return pack_ses32(!!std::signbit(x), 0xFF, 0);
	} else if(classify == FP_ZERO || classify == FP_SUBNORMAL) {
		// Subnormals and zeroes are encoded by a biased exponent of 0x00.
		// Extract sign bit and absolutize input.
		bool sign = !!std::signbit(x);
		if(sign) {
			x = -x;
		}

		// Remove the exponent (-126) and shift the result up into the integer part (23 bits).
		x = std::ldexp(x, 126 + 23);

		// Encode the significand by converting the result to an integer.
		std::uint32_t significand = static_cast<std::uint32_t>(x + 0.5f);

		// Encode the number.
		return pack_ses32(sign, 0x00, significand);
	} else {
		// Extract sign bit and absolutize input.
		bool sign = !!std::signbit(x);
		if(sign) {
			x = -x;
		}

		// Extract exponent.
		std::int16_t exponent;
		if constexpr(std::numeric_limits<float>::radix == 2) {
			exponent = static_cast<std::int16_t>(std::ilogb(x));
		} else {
			exponent = static_cast<std::int16_t>(std::floor(std::log2(x)));
		}

		// If exponent is below minimum limit, encode a zero.
		if(exponent < -126) {
			return pack_ses32(sign, 0x00, 0);
		}

		// If exponent is above maximum limit, encode an infinity.
		if(exponent > 127) {
			return pack_ses32(sign, 0xFF, 0);
		}

		// Remove the exponent.
		x = std::ldexp(x, -exponent);

		// The leading 1 bit of the significand is implied; remove it.
		x -= 1.0f;

		// Shift the value up into the integer part.
		x = std::ldexp(x, 23);

		// Encode the significand by converting the result to an integer.
		std::uint32_t significand = static_cast<std::uint32_t>(x + 0.5f);

		// Bias the exponent.
		exponent = static_cast<std::int16_t>(exponent + 127);

		// Encode the number.
		return pack_ses32(sign, static_cast<std::uint8_t>(exponent), significand);
	}
}

/**
 * \brief Decodes a floating-point number from IEEE754 single-precision format.
 *
 * \param[in] x the value to decode.
 *
 * \return the floating-point number.
 */
float mcwutil::codec::decode_u32_to_float(std::uint32_t x) {
	// Extract the sign bit, biased exponent, and significand.
	bool sign = !!(x & UINT32_C(0x80000000));
	std::int8_t exponent = static_cast<std::uint8_t>((x >> 23) & 0xFF);
	std::uint32_t significand = x & UINT32_C(0x007FFFFF);

	// Break down the possible exponents by class.
	if(exponent == static_cast<std::int8_t>(0xFF)) {
		// Exponent 0xFF encodes NaNs and infinities.
		if(significand) {
			return NAN;
		} else if(sign) {
			return -INFINITY;
		} else {
			return INFINITY;
		}
	} else if(!exponent) {
		// Shift the significand down into the fraction part (23 bits) plus apply the exponent (-126).
		float value = std::ldexp(static_cast<float>(significand), -(23 + 126));

		// Apply the sign bit.
		return sign ? -value : value;
	} else {
		// Unbias the exponent.
		exponent = static_cast<std::int8_t>(static_cast<std::uint8_t>(exponent) - 127);

		// Shift the significand down into the fraction part and re-add the implicit leading 1 bit.
		float value = 1.0f + std::ldexp(static_cast<float>(significand), -23);

		// Apply the exponent.
		value = std::ldexp(value, exponent);

		// Apply the sign bit.
		return sign ? -value : value;
	}
}

/**
 * \brief Encodes a floating-point number in IEEE754 double-precision format.
 *
 * \param[in] x the value to encode.
 *
 * \return the encoded form.
 */
std::uint64_t mcwutil::codec::encode_double_to_u64(double x) {
	// Break down the number based on its coarse classification.
	int classify = std::fpclassify(x);
	if(classify == FP_NAN) {
		// NaN values are encoded by a biased exponent of 0x7FF and a nonzero significand.
		return pack_ses64(false, 0x7FF, 1);
	} else if(classify == FP_INFINITE) {
		// Infinities are encoded by a biased exponent of 0x7FF and a zero significand.
		return pack_ses64(!!std::signbit(x), 0x7FF, 0);
	} else if(classify == FP_ZERO || classify == FP_SUBNORMAL) {
		// Subnormals and zeroes are encoded by a biased exponent of 0x000.
		// Extract sign bit and absolutize input.
		bool sign = !!std::signbit(x);
		if(sign) {
			x = -x;
		}

		// Remove the exponent (-1022) and shift the result up into the integer part (52 bits).
		x = std::ldexp(x, 1022 + 52);

		// Encode the significand by converting the result to an integer.
		std::uint64_t significand = static_cast<std::uint64_t>(x + 0.5);

		// Encode the number.
		return pack_ses64(sign, 0x000, significand);
	} else {
		// Extract sign bit and absolutize input.
		bool sign = !!std::signbit(x);
		if(sign) {
			x = -x;
		}

		// Extract exponent.
		std::int16_t exponent;
		if constexpr(std::numeric_limits<double>::radix == 2) {
			exponent = static_cast<std::int16_t>(std::ilogb(x));
		} else {
			exponent = static_cast<std::int16_t>(std::floor(std::log2(x)));
		}

		// If exponent is below minimum limit, encode a zero.
		if(exponent < -1022) {
			return pack_ses64(sign, 0x000, 0);
		}

		// If exponent is above maximum limit, encode an infinity.
		if(exponent > 1023) {
			return pack_ses64(sign, 0x7FF, 0);
		}

		// Remove the exponent.
		x = std::ldexp(x, -exponent);

		// The leading 1 bit of the significand is implied; remove it.
		x -= 1.0;

		// Shift the value up into the integer part.
		x = std::ldexp(x, 52);

		// Encode the significand by converting the result to an integer.
		std::uint64_t significand = static_cast<std::uint64_t>(x + 0.5);

		// Bias the exponent.
		exponent = static_cast<std::int16_t>(exponent + 1023);

		// Encode the number.
		return pack_ses64(sign, exponent, significand);
	}
}

/**
 * \brief Decodes a floating-point number from IEEE754 double-precision format.
 *
 * \param[in] x the value to decode.
 *
 * \return the floating-point number.
 */
double mcwutil::codec::decode_u64_to_double(std::uint64_t x) {
	// Extract the sign bit, biased exponent, and significand.
	bool sign = !!(x & UINT64_C(0x8000000000000000));
	std::int16_t exponent = (x >> 52) & 0x7FF;
	std::uint64_t significand = x & UINT64_C(0x000FFFFFFFFFFFFF);

	// Break down the possible exponents by class.
	if(exponent == 0x7FF) {
		// Exponent 0x7FF encodes NaNs and infinities.
		if(significand) {
			return NAN;
		} else if(sign) {
			return -INFINITY;
		} else {
			return INFINITY;
		}
	} else if(!exponent) {
		// Shift the significand down into the fraction part (52 bits) plus apply the exponent (-1022).
		double value = std::ldexp(static_cast<double>(significand), -(52 + 1022));

		// Apply the sign bit.
		return sign ? -value : value;
	} else {
		// Unbias the exponent.
		exponent = static_cast<std::int16_t>(exponent - 1023);

		// Shift the significand down into the fraction part and re-add the implicit leading 1 bit.
		double value = 1.0 + std::ldexp(static_cast<double>(significand), -52);

		// Apply the exponent.
		value = std::ldexp(value, exponent);

		// Apply the sign bit.
		return sign ? -value : value;
	}
}
