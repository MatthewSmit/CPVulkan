#pragma once
#include <cstdint>

template<int size>
struct RequiredSize;

template<>
struct RequiredSize<2>
{
	using Type = uint16_t;
};

struct half
{
public:
	half()
	{
	}
	
	half(float f);

	[[nodiscard]] float toFloat() const;

private:
	uint16_t value;
};

template<typename T>
struct FloatFormat;

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
typename FloatFormat<OutputType>::Type ConvertBits(typename FloatFormat<InputType>::Type input)
{
	constexpr auto INPUT_EXPONENT_SHIFT = FloatFormat<InputType>::MANTISSA_BITS;
	constexpr auto INPUT_EXPONENT_MASK = (1 << FloatFormat<InputType>::EXPONENT_BITS) - 1;
	constexpr auto INPUT_EXPONENT_BIAS = (1 << (FloatFormat<InputType>::EXPONENT_BITS - 1)) - 1;
	constexpr auto INPUT_MANTISSA_MASK = (1 << FloatFormat<InputType>::MANTISSA_BITS) - 1;

	constexpr auto OUTPUT_EXPONENT_SHIFT = FloatFormat<OutputType>::MANTISSA_BITS;
	constexpr auto OUTPUT_EXPONENT_MASK = (1 << FloatFormat<OutputType>::EXPONENT_BITS) - 1;
	constexpr auto OUTPUT_EXPONENT_BIAS = (1 << (FloatFormat<OutputType>::EXPONENT_BITS - 1)) - 1;

	FloatFormat<InputType> format = input;
	const auto exponent = static_cast<int32_t>((format.bits >> INPUT_EXPONENT_SHIFT) & INPUT_EXPONENT_MASK) - INPUT_EXPONENT_BIAS;
	
	typename FloatFormat<OutputType>::IntType result = 0;

	if constexpr (FloatFormat<OutputType>::MANTISSA_BITS < FloatFormat<InputType>::MANTISSA_BITS)
	{
		constexpr auto SHIFT = FloatFormat<InputType>::MANTISSA_BITS - FloatFormat<OutputType>::MANTISSA_BITS;
		result |= (format.bits & INPUT_MANTISSA_MASK) >> SHIFT;
	}
	else if constexpr (FloatFormat<OutputType>::MANTISSA_BITS == FloatFormat<InputType>::MANTISSA_BITS)
	{
		result |= format.bits & INPUT_MANTISSA_MASK;
	}
	else
	{
		constexpr auto SHIFT = FloatFormat<OutputType>::MANTISSA_BITS - FloatFormat<InputType>::MANTISSA_BITS;
		result |= (format.bits & INPUT_MANTISSA_MASK) << SHIFT;
	}

	result |= (static_cast<typename FloatFormat<OutputType>::IntType>(exponent + OUTPUT_EXPONENT_BIAS) & OUTPUT_EXPONENT_MASK) << OUTPUT_EXPONENT_SHIFT;

	if constexpr (FloatFormat<InputType>::SIGN_BITS == 1 && FloatFormat<OutputType>::SIGN_BITS == 1)
	{
		constexpr auto INPUT_SIGN_SHIFT = FloatFormat<InputType>::MANTISSA_BITS + FloatFormat<InputType>::EXPONENT_BITS;
		constexpr auto OUTPUT_SIGN_SHIFT = FloatFormat<OutputType>::MANTISSA_BITS + FloatFormat<OutputType>::EXPONENT_BITS;
		auto sign = (format.bits >> INPUT_SIGN_SHIFT) & 0x01;
		result |= sign << OUTPUT_SIGN_SHIFT;
	}

	return FloatFormat<OutputType>::Convert(result);
}

inline half::half(float f)
{
	value = ConvertBits<float, half>(f);
}

inline float half::toFloat() const
{
	return ConvertBits<half, float>(value);
}

static_assert(sizeof(half) == 2);
