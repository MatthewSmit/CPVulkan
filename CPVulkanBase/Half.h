#pragma once
#include "FloatFormat.h"

#include <glm/glm.hpp>

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
	half() noexcept = default;
	half(float f) noexcept;
	
	static half fromRaw(uint16_t raw)
	{
		auto result = half();
		result.value = raw;
		return result;
	}

	[[nodiscard]] float toFloat() const noexcept;
	[[nodiscard]] uint16_t toRaw() const noexcept { return value; }

private:
	uint16_t value = 0;
};

inline half operator+(half lhs, half rhs) noexcept
{
	return lhs.toFloat() + rhs.toFloat();
}

inline half operator-(half lhs, half rhs) noexcept
{
	return lhs.toFloat() - rhs.toFloat();
}

inline half operator*(half lhs, half rhs) noexcept
{
	return lhs.toFloat() * rhs.toFloat();
}

inline half operator/(half lhs, half rhs) noexcept
{
	return lhs.toFloat() / rhs.toFloat();
}

inline bool operator<(half lhs, half rhs) noexcept
{
	return lhs.toFloat() < rhs.toFloat();
}

inline half::half(float f) noexcept
{
	value = ConvertBits<float, half>(f);
}

inline float half::toFloat() const noexcept
{
	return ConvertBits<half, float>(value);
}

static_assert(sizeof(half) == 2);

namespace glm
{
	using f16 = half;

	using f16vec2 = vec<2, f16, defaultp>;
	using f16vec3 = vec<3, f16, defaultp>;
	using f16vec4 = vec<4, f16, defaultp>;
}

namespace std // NOLINT(cert-dcl58-cpp)
{
	inline half abs(half x) noexcept
	{
		return half::fromRaw(x.toRaw() & 0x7FFF);
	}
	
	inline float sin(half x) noexcept
	{
		return std::sin(x.toFloat());
	}

	inline float cos(half x) noexcept
	{
		return std::cos(x.toFloat());
	}

	inline float pow(half x, half y) noexcept
	{
		return std::pow(x.toFloat(), y.toFloat());
	}

	inline bool isnan(half x)
	{
		const FloatFormat<half> format(x.toRaw());
		return (format.bits & FloatFormat<half>::EXPONENT_MASK) == FloatFormat<half>::EXPONENT_MASK
			&& (format.bits & FloatFormat<half>::MANTISSA_MASK) != 0;
	}
	
	inline bool signbit(half value) noexcept
	{
		return value.toRaw() & 0x8000;
	}

	template<>
	class numeric_limits<half>
	{
	public:
		// 	_NODISCARD static constexpr float(min)() noexcept { // return minimum value
		// 		return FLT_MIN;
		// 	}
		//
		// 	_NODISCARD static constexpr float(max)() noexcept { // return maximum value
		// 		return FLT_MAX;
		// 	}
		//
		// 	_NODISCARD static constexpr float lowest() noexcept { // return most negative value
		// 		return -(max)();
		// 	}
		//
		// 	_NODISCARD static constexpr float epsilon() noexcept { // return smallest effective increment from 1.0
		// 		return FLT_EPSILON;
		// 	}
		//
		// 	_NODISCARD static constexpr float round_error() noexcept { // return largest rounding error
		// 		return 0.5F;
		// 	}
		//
		// 	_NODISCARD static constexpr float denorm_min() noexcept { // return minimum denormalized value
		// 		return FLT_TRUE_MIN;
		// 	}
		//
		// 	_NODISCARD static constexpr float infinity() noexcept { // return positive infinity
		// 		return __builtin_huge_valf();
		// 	}
		//
		// 	_NODISCARD static constexpr float quiet_NaN() noexcept { // return non-signaling NaN
		// 		return __builtin_nanf("0");
		// 	}
		//
		// 	_NODISCARD static constexpr float signaling_NaN() noexcept { // return signaling NaN
		// 		return __builtin_nansf("1");
		// 	}
		//
		// 	static constexpr int digits = FLT_MANT_DIG;
		// 	static constexpr int digits10 = FLT_DIG;
		// 	static constexpr int max_digits10 = 9;
		// 	static constexpr int max_exponent = FLT_MAX_EXP;
		// 	static constexpr int max_exponent10 = FLT_MAX_10_EXP;
		// 	static constexpr int min_exponent = FLT_MIN_EXP;
		// 	static constexpr int min_exponent10 = FLT_MIN_10_EXP;
		
		// static constexpr float_denorm_style has_denorm = denorm_present;
		// static constexpr bool has_infinity             = true;
		// static constexpr bool has_quiet_NaN            = true;
		// static constexpr bool has_signaling_NaN        = true;
		// static constexpr bool is_bounded               = true;
		// static constexpr bool is_specialized           = true;
		// static constexpr float_round_style round_style = round_to_nearest;
		// static constexpr int radix                     = FLT_RADIX;

		// static constexpr float_denorm_style has_denorm = denorm_absent;
		// static constexpr bool has_denorm_loss          = false;
		// static constexpr bool has_infinity             = false;
		// static constexpr bool has_quiet_NaN            = false;
		// static constexpr bool has_signaling_NaN        = false;
		// static constexpr bool is_bounded               = false;
		// static constexpr bool is_exact                 = false;
		static constexpr bool is_iec559 = true;
		static constexpr bool is_integer = false;
		// static constexpr bool is_modulo                = false;
		static constexpr bool is_signed = true;
		// static constexpr bool is_specialized           = false;
		// static constexpr bool tinyness_before          = false;
		// static constexpr bool traps                    = false;
		// static constexpr float_round_style round_style = round_toward_zero;
		// static constexpr int digits                    = 0;
		// static constexpr int digits10                  = 0;
		// static constexpr int max_digits10              = 0;
		// static constexpr int max_exponent              = 0;
		// static constexpr int max_exponent10            = 0;
		// static constexpr int min_exponent              = 0;
		// static constexpr int min_exponent10            = 0;
		// static constexpr int radix                     = 0;
	};
}

inline float sqrt(half value) noexcept
{
	return sqrtf(value.toFloat());
}