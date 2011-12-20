#include "util/codec.h"

namespace {
	/**
	 * Packs a sign bit, a biased exponent, and a significand into a 32-bit integer in IEEE754 single-precision format.
	 *
	 * \param[in] sign the sign bit, \c true for negative or \c false for positive.
	 *
	 * \param[in] exponent the biased exponent, between 0 and 0xFF.
	 *
	 * \param[in] significand the significand.
	 *
	 * \return the packed 32-bit integer.
	 */
	uint32_t pack_ses32(bool sign, uint8_t exponent, uint32_t significand) {
		return (sign ? UINT32_C(0x80000000) : 0) | (static_cast<uint32_t>(exponent) << 23) | significand;
	}

	/**
	 * Packs a sign bit, a biased exponent, and a significand into a 64-bit integer in IEEE754 double-precision format.
	 *
	 * \param[in] sign the sign bit, \c true for negative or \c false for positive.
	 *
	 * \param[in] exponent the biased exponent, between 0 and 0x7FF.
	 *
	 * \param[in] significand the significand.
	 *
	 * \return the packed 64-bit integer.
	 */
	uint64_t pack_ses64(bool sign, uint16_t exponent, uint64_t significand) {
		return (sign ? UINT64_C(0x8000000000000000) : 0) | (static_cast<uint64_t>(exponent) << 52) | significand;
	}
}

uint32_t encode_float_to_u32(float x) {
	// Break down the number based on its coarse classification.
	int classify = std::fpclassify(x);
	if (classify == FP_NAN) {
		// NaN values are encoded by a biased exponent of 0xFF and a nonzero significand.
		return pack_ses32(false, 0xFF, 1);
	} else if (classify == FP_INFINITE) {
		// Infinities are encoded by a biased exponent of 0xFF and a zero significand.
		return pack_ses32(!!std::signbit(x), 0xFF, 0);
	} else if (classify == FP_ZERO || classify == FP_SUBNORMAL) {
		// Subnormals and zeroes are encoded by a biased exponent of 0x00.
		// Extract sign bit and absolutize input.
		bool sign = !!std::signbit(x);
		if (sign) {
			x = -x;
		}

		// As-close-to-normalize-as-possible input (this value is 2^126).
		x *= 85070591730234615865843651857942052864.0f;

		// Encode the significand by multiplying the input by 2^23 and converting the result to an integer.
		uint32_t significand = static_cast<uint32_t>((x * (UINT32_C(1) << 23)) + 0.5);

		// Encode the number.
		return pack_ses32(sign, 0x00, significand);
	} else {
		// Extract sign bit and absolutize input.
		bool sign = !!std::signbit(x);
		if (sign) {
			x = -x;
		}

		// Extract exponent and normalize input.
		int16_t exponent = 0;
		while (x >= 2.0) {
			x /= 2.0f;
			++exponent;
		}
		while (x < 1.0) {
			x *= 2.0f;
			--exponent;
		}

		// If exponent is below minimum limit, encode a zero.
		if (exponent < -126) {
			return pack_ses32(sign, 0x00, 0);
		}

		// If exponent is above maximum limit, encode an infinity.
		if (exponent > 127) {
			return pack_ses32(sign, 0xFF, 0);
		}

		// The leading 1 bit of the significand is implied; remove it.
		x -= 1.0f;

		// Encode the significand by multiplying the input by 2^23 and converting the result to an integer.
		uint32_t significand = static_cast<uint32_t>((x * (UINT32_C(1) << 23)) + 0.5);

		// Bias the exponent.
		exponent = static_cast<int16_t>(exponent + 127);

		// Encode the number.
		return pack_ses32(sign, static_cast<uint8_t>(exponent), significand);
	}
}

float decode_u32_to_float(uint32_t x) {
	// Extract the sign bit, biased exponent, and significand.
	bool sign = !!(x & UINT32_C(0x80000000));
	int8_t exponent = static_cast<uint8_t>((x >> 23) & 0xFF);
	uint32_t significand = x & UINT32_C(0x007FFFFF);

	// Break down the possible exponents by class.
	if (exponent == static_cast<int8_t>(0xFF)) {
		// Exponent 0xFF encodes NaNs and infinities.
		if (significand) {
			return NAN;
		} else if (sign) {
			return -INFINITY;
		} else {
			return INFINITY;
		}
	} else if (!exponent) {
		// Exponent 0x00 encodes zeroes and subnormals.
		// Shift the significand down into the fraction part.
		float value = static_cast<float>(significand) / (UINT32_C(1) << 23);

		// Apply the exponent (this value is 2^126).
		value /= 85070591730234615865843651857942052864.0f;

		// Apply the sign bit.
		return sign ? -value : value;
	} else {
		// Unbias the exponent.
		exponent = static_cast<int8_t>(static_cast<uint8_t>(exponent) - 127);

		// Shift the significand down into the fraction part and re-add the implicit leading 1 bit.
		double value = 1.0 + static_cast<double>(significand) / (UINT32_C(1) << 23);

		// Apply the exponent.
		while (exponent > 0) {
			value *= 2.0;
			--exponent;
		}
		while (exponent < 0) {
			value /= 2.0;
			++exponent;
		}

		// Apply the sign bit.
		return static_cast<float>(sign ? -value : value);
	}
}

uint64_t encode_double_to_u64(double x) {
	// Break down the number based on its coarse classification.
	int classify = std::fpclassify(x);
	if (classify == FP_NAN) {
		// NaN values are encoded by a biased exponent of 0x7FF and a nonzero significand.
		return pack_ses64(false, 0x7FF, 1);
	} else if (classify == FP_INFINITE) {
		// Infinities are encoded by a biased exponent of 0x7FF and a zero significand.
		return pack_ses64(!!std::signbit(x), 0x7FF, 0);
	} else if (classify == FP_ZERO || classify == FP_SUBNORMAL) {
		// Subnormals and zeroes are encoded by a biased exponent of 0x000.
		// Extract sign bit and absolutize input.
		bool sign = !!std::signbit(x);
		if (sign) {
			x = -x;
		}

		// As-close-to-normalize-as-possible input (this value is 2^1022).
		x *= 44942328371557897693232629769725618340449424473557664318357520289433168951375240783177119330601884005280028469967848339414697442203604155623211857659868531094441973356216371319075554900311523529863270738021251442209537670585615720368478277635206809290837627671146574559986811484619929076208839082406056034304.0;

		// Encode the significand by multiplying the input by 2^52 and converting the result to an integer.
		uint64_t significand = static_cast<uint64_t>((x * (UINT64_C(1) << 52)) + 0.5);

		// Encode the number.
		return pack_ses64(sign, 0x000, significand);
	} else {
		// Extract sign bit and absolutize input.
		bool sign = !!std::signbit(x);
		if (sign) {
			x = -x;
		}

		// Extract exponent and normalize input.
		int16_t exponent = 0;
		while (x >= 2.0) {
			x /= 2.0;
			++exponent;
		}
		while (x < 1.0) {
			x *= 2.0;
			--exponent;
		}

		// If exponent is below minimum limit, encode a zero.
		if (exponent < -1022) {
			return pack_ses64(sign, 0x000, 0);
		}

		// If exponent is above maximum limit, encode an infinity.
		if (exponent > 1023) {
			return pack_ses64(sign, 0x7FF, 0);
		}

		// The leading 1 bit of the significand is implied; remove it.
		x -= 1.0;

		// Encode the significand by multiplying the input by 2^52 and converting the result to an integer.
		uint64_t significand = static_cast<uint64_t>((x * (UINT64_C(1) << 52)) + 0.5);

		// Bias the exponent.
		exponent = static_cast<int16_t>(exponent + 1023);

		// Encode the number.
		return pack_ses64(sign, exponent, significand);
	}
}

double decode_u64_to_double(uint64_t x) {
	// Extract the sign bit, biased exponent, and significand.
	bool sign = !!(x & UINT64_C(0x8000000000000000));
	int16_t exponent = (x >> 52) & 0x7FF;
	uint64_t significand = x & UINT64_C(0x000FFFFFFFFFFFFF);

	// Break down the possible exponents by class.
	if (exponent == 0x7FF) {
		// Exponent 0x7FF encodes NaNs and infinities.
		if (significand) {
			return NAN;
		} else if (sign) {
			return -INFINITY;
		} else {
			return INFINITY;
		}
	} else if (!exponent) {
		// Exponent 0x000 encodes zeroes and subnormals.
		// Shift the significand down into the fraction part.
		double value = static_cast<double>(significand) / (UINT64_C(1) << 52);

		// Apply the exponent (this value is 2^1022).
		value /= 44942328371557897693232629769725618340449424473557664318357520289433168951375240783177119330601884005280028469967848339414697442203604155623211857659868531094441973356216371319075554900311523529863270738021251442209537670585615720368478277635206809290837627671146574559986811484619929076208839082406056034304.0;

		// Apply the sign bit.
		return sign ? -value : value;
	} else {
		// Unbias the exponent.
		exponent = static_cast<int16_t>(exponent - 1023);

		// Shift the significand down into the fraction part and re-add the implicit leading 1 bit.
		double value = 1.0 + static_cast<double>(significand) / (UINT64_C(1) << 52);

		// Apply the exponent.
		while (exponent > 0) {
			value *= 2.0;
			--exponent;
		}
		while (exponent < 0) {
			value /= 2.0;
			++exponent;
		}

		// Apply the sign bit.
		return sign ? -value : value;
	}
}

