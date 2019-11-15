#include <Base.h>
#include <Half.h>

extern "C" CP_DLL_EXPORT uint16_t __gnu_f2h_ieee(float value)
{
	return half(value).toRaw();
}

extern "C" CP_DLL_EXPORT float __gnu_h2f_ieee(half value)
{
	return half(value).toFloat();
}

extern "C" CP_DLL_EXPORT uint32_t FloatToB10G11R11(float values[4])
{
	const auto r = ConvertBits<float, UF11>(values[0]);
	const auto g = ConvertBits<float, UF11>(values[1]);
	const auto b = ConvertBits<float, UF10>(values[2]);
	return r | g << 11 | b << 22;
}

extern "C" CP_DLL_EXPORT void B10G11R11ToFloat(uint32_t value, float result[4])
{
	result[0] = ConvertBits<UF11, float>(value & ((1 << 11) - 1));
	result[1] = ConvertBits<UF11, float>((value >> 11) & ((1 << 11) - 1));
	result[2] = ConvertBits<UF10, float>((value >> 22) & ((1 << 10) - 1));
	result[3] = 1; 
}

// Copied from MESA under MIT license

extern "C" CP_DLL_EXPORT uint32_t FloatToE5B9G9R9(float values[4])
{
	constexpr auto EXPONENT_SHIFT = FloatFormat<UF14>::MANTISSA_BITS;
	constexpr auto EXPONENT_MASK = (1 << FloatFormat<UF14>::EXPONENT_BITS) - 1;
	constexpr auto EXPONENT_BIAS = (1 << (FloatFormat<UF14>::EXPONENT_BITS - 1)) - 1;
	constexpr auto MANTISSA_MASK = (1 << FloatFormat<UF14>::MANTISSA_BITS) - 1;

	union
	{
		float f;
		uint32_t u;
	} rc, bc, gc, maxrgb, revdenom;

	rc.f = values[0];
	gc.f = values[1];
	bc.f = values[2];
	maxrgb.u = std::max(std::max(rc.u, gc.u), bc.u);

	/*
	 * Compared to what the spec suggests, instead of conditionally adjusting
	 * the exponent after the fact do it here by doing the equivalent of +0.5 -
	 * the int add will spill over into the exponent in this case.
	 */
	maxrgb.u += maxrgb.u & (1 << (23 - 9));
	const auto exp_shared = std::max(static_cast<int>(maxrgb.u >> 23), -EXPONENT_BIAS - 1 + 127) + 1 + EXPONENT_BIAS - 127;
	const uint32_t revdenom_biasedexp = 127 - (exp_shared - EXPONENT_BIAS - EXPONENT_SHIFT) + 1;
	revdenom.u = revdenom_biasedexp << 23;
	assert(exp_shared <= EXPONENT_MASK);

	/*
	 * The spec uses strict round-up behavior (d3d10 disagrees, but in any case
	 * must match what is done above for figuring out exponent).
	 * We avoid the doubles ((int) rc * revdenom + 0.5) by doing the rounding
	 * ourselves (revdenom was adjusted by +1, above).
	 */
	auto rm = static_cast<int>(rc.f * revdenom.f);
	auto gm = static_cast<int>(gc.f * revdenom.f);
	auto bm = static_cast<int>(bc.f * revdenom.f);
	rm = (rm & 1) + (rm >> 1);
	gm = (gm & 1) + (gm >> 1);
	bm = (bm & 1) + (bm >> 1);

	assert(rm <= MANTISSA_MASK);
	assert(gm <= MANTISSA_MASK);
	assert(bm <= MANTISSA_MASK);
	assert(rm >= 0);
	assert(gm >= 0);
	assert(bm >= 0);

	return (exp_shared << 27) | (bm << 18) | (gm << 9) | rm;
}

extern "C" CP_DLL_EXPORT void E5B9G9R9ToFloat(uint32_t value, float result[4])
{
	constexpr auto RGB9E5_EXP_BIAS = 15;
	constexpr auto RGB9E5_MANTISSA_BITS = 9;
	union
	{
		float f;
		uint32_t u;
	} scale;

	const int exponent = (value >> 27) - RGB9E5_EXP_BIAS - RGB9E5_MANTISSA_BITS;
	scale.u = (exponent + 127) << 23;

	result[0] = (value & 0x1ff) * scale.f;
	result[1] = ((value >> 9) & 0x1ff) * scale.f;
	result[2] = ((value >> 18) & 0x1ff) * scale.f;
	result[3] = 1;
}