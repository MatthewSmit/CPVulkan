#pragma once

// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
// Modified for CPVulkan

template<typename T>
struct FloatFormat;

struct half;
struct UF10;

template<>
struct FloatFormat<UF10>
{
	using Type = uint16_t;
	using IntType = uint16_t;

	static constexpr auto SIGN_BITS = 0;
	static constexpr auto EXPONENT_BITS = 5;
	static constexpr auto MANTISSA_BITS = 5;

	FloatFormat(uint16_t value)
	{
		bits = *reinterpret_cast<uint16_t*>(&value);
	}

	static Type Convert(IntType value)
	{
		return value;
	}

	uint16_t bits;
};

struct UF11;

template<>
struct FloatFormat<UF11>
{
	using Type = uint16_t;
	using IntType = uint16_t;

	static constexpr auto SIGN_BITS = 0;
	static constexpr auto EXPONENT_BITS = 5;
	static constexpr auto MANTISSA_BITS = 6;

	FloatFormat(uint16_t value)
	{
		bits = *reinterpret_cast<uint16_t*>(&value);
	}

	static Type Convert(IntType value)
	{
		return value;
	}

	uint16_t bits;
};

struct UF14;

template<>
struct FloatFormat<UF14>
{
	using Type = uint16_t;
	using IntType = uint16_t;

	static constexpr auto SIGN_BITS = 0;
	static constexpr auto EXPONENT_BITS = 5;
	static constexpr auto MANTISSA_BITS = 9;

	FloatFormat(uint16_t value)
	{
		bits = *reinterpret_cast<uint16_t*>(&value);
	}

	static Type Convert(IntType value)
	{
		return value;
	}

	uint16_t bits;
};

template<>
struct FloatFormat<half>
{
	using Type = uint16_t;
	using IntType = uint16_t;

	static constexpr auto SIGN_BITS = 1;
	static constexpr auto EXPONENT_BITS = 5;
	static constexpr auto MANTISSA_BITS = 10;

	static constexpr auto MANTISSA_MASK = (1 << MANTISSA_BITS) - 1;
	static constexpr auto EXPONENT_MASK = ((1 << MANTISSA_BITS) - 1) << EXPONENT_BITS;
	static constexpr auto SIGN_MASK = ((1 << SIGN_BITS) - 1) << (EXPONENT_BITS + MANTISSA_BITS);

	FloatFormat(uint16_t value)
	{
		bits = *reinterpret_cast<uint16_t*>(&value);
	}

	static Type Convert(IntType value)
	{
		return value;
	}

	uint16_t bits;
};

template<>
struct FloatFormat<float>
{
	using Type = float;
	using IntType = uint32_t;

	static constexpr auto SIGN_BITS = 1;
	static constexpr auto EXPONENT_BITS = 8;
	static constexpr auto MANTISSA_BITS = 23;

	FloatFormat(float value)
	{
		bits = *reinterpret_cast<uint32_t*>(&value);
	}

	static Type Convert(IntType value)
	{
		return *reinterpret_cast<Type*>(&value);
	}

	uint32_t bits;
};

template<typename InputType, typename OutputType>
typename FloatFormat<OutputType>::Type Truncate(typename FloatFormat<InputType>::Type value)
{
	static_assert(std::is_same<typename FloatFormat<OutputType>::Type, typename FloatFormat<OutputType>::IntType>::value);
	static_assert(!std::is_same<typename FloatFormat<InputType>::Type, typename FloatFormat<InputType>::IntType>::value);
	
	using Input = typename FloatFormat<InputType>::IntType;
	using Output = typename FloatFormat<OutputType>::IntType;
		
	// Various constants whose values follow from the type parameters.
	// Any reasonable optimizer will fold and propagate all of these.
	constexpr int srcBits = sizeof(Input) * 8;
	constexpr int srcSigBits = FloatFormat<InputType>::MANTISSA_BITS;
	constexpr int srcExpBits = FloatFormat<InputType>::EXPONENT_BITS;
	constexpr auto srcInfExp = (1 << srcExpBits) - 1;
	constexpr auto srcExpBias = srcInfExp >> 1;

	constexpr int dstBits = sizeof(Output) * 8;
	constexpr int dstSigBits = FloatFormat<OutputType>::MANTISSA_BITS;
	constexpr int dstExpBits = FloatFormat<OutputType>::EXPONENT_BITS;
	constexpr auto dstInfExp = (1 << dstExpBits) - 1;
	constexpr auto dstExpBias = dstInfExp >> 1;

	constexpr Input srcMinNormal = Input(1) << srcSigBits;
	constexpr Input srcSignificandMask = srcMinNormal - 1;
	constexpr Input srcInfinity = static_cast<Input>(srcInfExp) << srcSigBits;
	constexpr Input srcSignMask = Input(1) << (srcSigBits + srcExpBits);
	constexpr Input srcAbsMask = srcSignMask - 1;
	constexpr Input roundMask = (Input(1) << (srcSigBits - dstSigBits)) - 1;
	constexpr Input halfway = Input(1) << (srcSigBits - dstSigBits - 1);
	constexpr Input srcQNaN = Input(1) << (srcSigBits - 1);
	constexpr Input srcNaNCode = srcQNaN - 1;

	constexpr auto underflowExponent = srcExpBias + 1 - dstExpBias;
	constexpr auto overflowExponent = srcExpBias + dstInfExp - dstExpBias;
	constexpr Input underflow = static_cast<Input>(underflowExponent) << srcSigBits;
	constexpr Input overflow = static_cast<Input>(overflowExponent) << srcSigBits;

	constexpr Output dstQNaN = Output(1) << (dstSigBits - 1);
	constexpr Output dstNaNCode = dstQNaN - 1;

	// Break a into a sign and representation of the absolute value.
	const Input aRep = FloatFormat<InputType>(value).bits;
	const Input aAbs = aRep & srcAbsMask;
	const Input sign = (FloatFormat<InputType>::SIGN_BITS == 1 && FloatFormat<OutputType>::SIGN_BITS == 1) ? aRep & srcSignMask : 0;
	Output absResult;

	if (aAbs - underflow < aAbs - overflow)
	{
		// The exponent of a is within the range of normal numbers in the
		// destination format.  We can convert by simply right-shifting with
		// rounding and adjusting the exponent.
		absResult = aAbs >> (srcSigBits - dstSigBits);
		absResult -= static_cast<Output>(static_cast<Output>(srcExpBias - dstExpBias) << dstSigBits);

		const Input roundBits = aAbs & roundMask;
		// Round to nearest.
		if (roundBits > halfway)
		{
			++absResult;
			// Tie to even.
		}
		else if (roundBits == halfway)
		{
			absResult += absResult & 1;
		}
	}
	else if (aAbs > srcInfinity)
	{
		// a is NaN.
		// Conjure the result by beginning with infinity, setting the qNaN
		// bit and inserting the (truncated) trailing NaN field.
		absResult = static_cast<Output>(dstInfExp) << dstSigBits;
		absResult |= dstQNaN;
		absResult |= ((aAbs & srcNaNCode) >> (srcSigBits - dstSigBits)) & dstNaNCode;
	}
	else if (aAbs >= overflow)
	{
		// a overflows to infinity.
		absResult = static_cast<Output>(dstInfExp) << dstSigBits;
	}
	else
	{
		// a underflows on conversion to the destination type or is an exact
		// zero.  The result may be a denormal or zero.  Extract the exponent
		// to get the shift amount for the denormalization.
		const int aExp = aAbs >> srcSigBits;
		const auto shift = srcExpBias - dstExpBias - aExp + 1;

		const Input significand = (aRep & srcSignificandMask) | srcMinNormal;

		// Right shift by the denormalization amount with sticky.
		if (shift > srcSigBits)
		{
			absResult = 0;
		}
		else
		{
			const Input sticky = (significand << (srcBits - shift)) ? 1 : 0;
			Input denormalizedSignificand = significand >> shift | sticky;
			absResult = denormalizedSignificand >> (srcSigBits - dstSigBits);
			const Input roundBits = denormalizedSignificand & roundMask;
			// Round to nearest
			if (roundBits > halfway)
			{
				++absResult;
				// Ties to even
			}
			else if (roundBits == halfway)
			{
				absResult += absResult & 1;
			}
		}
	}

	// Apply the signbit to the absolute value.
	return absResult | sign >> (srcBits - dstBits);
}

template<typename InputType, typename OutputType>
typename FloatFormat<OutputType>::Type Extend(typename FloatFormat<InputType>::Type value)
{
	static_assert(!std::is_same<typename FloatFormat<OutputType>::Type, typename FloatFormat<OutputType>::IntType>::value);
	static_assert(std::is_same<typename FloatFormat<InputType>::Type, typename FloatFormat<InputType>::IntType>::value);

	using Input = typename FloatFormat<InputType>::IntType;
	using Output = typename FloatFormat<OutputType>::IntType;
	
	// Various constants whose values follow from the type parameters.
	// Any reasonable optimizer will fold and propagate all of these.
	constexpr int srcBits = sizeof(Input) * 8;
	constexpr int srcSigBits = FloatFormat<InputType>::MANTISSA_BITS;
	constexpr int srcExpBits = FloatFormat<InputType>::EXPONENT_BITS;
	constexpr auto srcInfExp = (1 << srcExpBits) - 1;
	constexpr auto srcExpBias = srcInfExp >> 1;

	constexpr int dstBits = sizeof(Output) * 8;
	constexpr int dstSigBits = FloatFormat<OutputType>::MANTISSA_BITS;
	constexpr int dstExpBits = FloatFormat<OutputType>::EXPONENT_BITS;
	constexpr auto dstInfExp = (1 << dstExpBits) - 1;
	constexpr auto dstExpBias = dstInfExp >> 1;

	constexpr Input srcMinNormal = Input(1) << srcSigBits;
	constexpr Input srcInfinity = static_cast<Input>(srcInfExp) << srcSigBits;
	constexpr Input srcSignMask = Input(1) << (srcSigBits + srcExpBits);
	constexpr Input srcAbsMask = srcSignMask - 1;
	constexpr Input srcQNaN = Input(1) << (srcSigBits - 1);
	constexpr Input srcNaNCode = srcQNaN - 1;

	constexpr Output dstMinNormal = Output(1) << dstSigBits;

	// Break a into a sign and representation of the absolute value.
	const Input aRep = value;
	const Input aAbs = aRep & srcAbsMask;
	const Input sign = (FloatFormat<InputType>::SIGN_BITS == 1 && FloatFormat<OutputType>::SIGN_BITS == 1) ? aRep & srcSignMask : 0;
	Output absResult;

	// If sizeof(src_rep_t) < sizeof(int), the subtraction result is promoted
	// to (signed) int.  To avoid that, explicitly cast to src_rep_t.
	if (static_cast<Input>(aAbs - srcMinNormal) < srcInfinity - srcMinNormal)
	{
		// a is a normal number.
		// Extend to the destination type by shifting the significand and
		// exponent into the proper position and rebiasing the exponent.
		absResult = static_cast<Output>(aAbs) << (dstSigBits - srcSigBits);
		absResult += static_cast<Output>(dstExpBias - srcExpBias) << dstSigBits;
	}
	else if (aAbs >= srcInfinity)
	{
		// a is NaN or infinity.
		// Conjure the result by beginning with infinity, then setting the qNaN
		// bit (if needed) and right-aligning the rest of the trailing NaN
		// payload field.
		absResult = static_cast<Output>(dstInfExp) << dstSigBits;
		absResult |= static_cast<Output>(aAbs & srcQNaN) << (dstSigBits - srcSigBits);
		absResult |= static_cast<Output>(aAbs & srcNaNCode) << (dstSigBits - srcSigBits);
	}
	else if (aAbs)
	{
		// a is denormal.
		// renormalize the significand and clear the leading bit, then insert
		// the correct adjusted exponent in the destination type.
		const int scale = __builtin_clz(aAbs) - __builtin_clz(srcMinNormal);
		absResult = static_cast<Output>(aAbs) << (dstSigBits - srcSigBits + scale);
		absResult ^= dstMinNormal;
		const auto resultExponent = dstExpBias - srcExpBias - scale + 1;
		absResult |= static_cast<Output>(resultExponent) << dstSigBits;
	}
	else
	{
		// a is zero.
		absResult = 0;
	}

	// Apply the signbit to the absolute value.
	const Output result = absResult | static_cast<Output>(sign) << (dstBits - srcBits);
	return FloatFormat<OutputType>::Convert(result);
}

template<typename InputType, typename OutputType>
typename FloatFormat<OutputType>::Type ConvertBits(typename FloatFormat<InputType>::Type input)
{
	if constexpr (sizeof(typename FloatFormat<InputType>::Type) > sizeof(typename FloatFormat<OutputType>::Type))
	{
		return Truncate<InputType, OutputType>(input);
	}
	else
	{
		return Extend<InputType, OutputType>(input);
	}
}