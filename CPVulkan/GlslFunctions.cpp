#include "GlslFunctions.h"

#include "Base.h"
#include "Formats.h"
#include "Half.h"
#include "Image.h"
#include "ImageSampler.h"
#include "ImageView.h"
#include "Sampler.h"

#include <Converter.h>

#include <glm/glm.hpp>

template<typename T>
static T SAbs(T value)
{
	return std::abs(value);
}

template<typename T>
static T VSAbs(T value)
{
	T result{};
	for (auto i = 0; i < T::length(); i++)
	{
		result[i] = SAbs(value[i]);
	}
	return result;
}

template<typename T>
static T SSign(T value)
{
	return T(T(0) < value) - T(value < T(0));
}

template<typename T>
static T VSSign(T value)
{
	T result{};
	for (auto i = 0; i < T::length(); i++)
	{
		result[i] = SSign(value[i]);
	}
	return result;
}

template<typename T>
static T Pow(T left, T right)
{
	return std::pow(left, right);
}

template<typename T>
static T VPow(T left, T right)
{
	T result{0};
	for (auto i = 0; i < T::length(); i++)
	{
		result[i] = Pow(left[i], right[i]);
	}
	return result;
}

template<typename T>
static T Min(T left, T right)
{
	return std::min(left, right);
}

template<typename T>
static T VMin(T left, T right)
{
	T result{0};
	for (auto i = 0; i < T::length(); i++)
	{
		result[i] = Min(left[i], right[i]);
	}
	return result;
}

template<typename T>
static T Max(T left, T right)
{
	return std::max(left, right);
}

template<typename T>
static T VMax(T left, T right)
{
	T result{0};
	for (auto i = 0; i < T::length(); i++)
	{
		result[i] = Max(left[i], right[i]);
	}
	return result;
}

template<typename T>
static T Clamp(T value, T min, T max)
{
	return std::clamp(value, min, max);
}

template<typename T>
static T VClamp(T value, T min, T max)
{
	T result{};
	for (auto i = 0; i < T::length(); i++)
	{
		result[i] = Clamp(value[i], min[i], max[i]);
	}
	return result;
}

template<typename T>
static T Mix(T x, T y, T a)
{
	return x * (1 - a) + y * a;
}

template<typename T>
static T VMix(T x, T y, T a)
{
	T result{0};
	for (auto i = 0; i < T::length(); i++)
	{
		result[i] = Mix(x[i], y[i], a[i]);
	}
	return result;
}

template<typename T>
static T Normalise(T value)
{
	return std::signbit(value) ? T(1) : T(-1);
}

template<typename T>
static T VNormalise(T value)
{
	return glm::normalize(value);
}

template<typename T>
static T Reflect(T i, T n)
{
	return glm::reflect(i, n);
}

template<typename T>
static T FindILsb(T value)
{
	static_assert(sizeof(T) == 4);
	if (value == 0)
	{
		return -1;
	}
	return __builtin_ctz(value);
}

template<typename T>
static T VFindILsb(T value)
{
	T result{};
	for (auto i = 0; i < T::length(); i++)
	{
		result[i] = FindILsb(value[i]);
	}
	return result;
}

template<typename T>
static T FindSMsb(T value)
{
	static_assert(sizeof(T) == 4);
	static_assert(std::numeric_limits<T>::is_signed);
	
	if (value < 0)
	{
		value = ~value;
	}
	
	if (value == 0)
	{
		return -1;
	}
	
	return 31 - __builtin_clz(value);
}

template<typename T>
static T VFindSMsb(T value)
{
	T result{};
	for (auto i = 0; i < T::length(); i++)
	{
		result[i] = FindSMsb(value[i]);
	}
	return result;
}

template<typename T>
static T FindUMsb(T value)
{
	static_assert(sizeof(T) == 4);
	if (value == 0)
	{
		return -1;
	}
	return 31 - __builtin_clz(value);
}

template<typename T>
static T VFindUMsb(T value)
{
	T result{};
	for (auto i = 0; i < T::length(); i++)
	{
		result[i] = FindUMsb(value[i]);
	}
	return result;
}


template<typename ReturnType, typename CoordinateType>
static void ImageSampleImplicitLod(ReturnType* result, VkDescriptorImageInfo* sampledImage, CoordinateType* coordinate)
{
	// TODO: Use ImageSampler.cpp
	// TODO: Calculate LOD
	const auto sampler = UnwrapVulkan<Sampler>(sampledImage->sampler);
	auto imageView = UnwrapVulkan<ImageView>(sampledImage->imageView);

	// Instructions with ExplicitLod in the name determine the LOD used in the sampling operation based on additional coordinates.
	// https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html#textures-level-of-detail-operation 
	
	if (imageView->getViewType() != VK_IMAGE_VIEW_TYPE_2D)
	{
		FATAL_ERROR();
	}
	
	if (imageView->getSubresourceRange().aspectMask != VK_IMAGE_ASPECT_COLOR_BIT)
	{
		FATAL_ERROR();
	}
	
	if (imageView->getComponents().r != VK_COMPONENT_SWIZZLE_R && imageView->getComponents().r != VK_COMPONENT_SWIZZLE_IDENTITY)
	{
		FATAL_ERROR();
	}
	
	if (imageView->getComponents().g != VK_COMPONENT_SWIZZLE_G && imageView->getComponents().g != VK_COMPONENT_SWIZZLE_IDENTITY)
	{
		FATAL_ERROR();
	}
	
	if (imageView->getComponents().b != VK_COMPONENT_SWIZZLE_B && imageView->getComponents().b != VK_COMPONENT_SWIZZLE_IDENTITY)
	{
		FATAL_ERROR();
	}
	
	if (imageView->getComponents().a != VK_COMPONENT_SWIZZLE_A && imageView->getComponents().a != VK_COMPONENT_SWIZZLE_IDENTITY)
	{
		FATAL_ERROR();
	}
	
	if (imageView->getSubresourceRange().baseMipLevel != 0)
	{
		FATAL_ERROR();
	}
	
	if (imageView->getSubresourceRange().levelCount != 1)
	{
		FATAL_ERROR();
	}
	
	if (imageView->getSubresourceRange().baseArrayLayer != 0)
	{
		FATAL_ERROR();
	}
	
	if (imageView->getSubresourceRange().layerCount != 1)
	{
		FATAL_ERROR();
	}
	
	if (sampler->getFlags())
	{
		FATAL_ERROR();
	}
	
	if (sampler->getMagFilter() != VK_FILTER_NEAREST)
	{
		FATAL_ERROR();
	}
	
	if (sampler->getMinFilter() != VK_FILTER_NEAREST)
	{
		FATAL_ERROR();
	}
	
	if (sampler->getMipmapMode() != VK_SAMPLER_MIPMAP_MODE_NEAREST)
	{
		FATAL_ERROR();
	}
	
	if (sampler->getAnisotropyEnable())
	{
		FATAL_ERROR();
	}
	
	if (sampler->getCompareEnable())
	{
		FATAL_ERROR();
	}
	
	if (sampler->getUnnormalisedCoordinates())
	{
		FATAL_ERROR();
	}
	
	if (coordinate->x < 0 || coordinate->x > 1 || coordinate->y < 0 || coordinate->y > 1)
	{
		FATAL_ERROR();
	}
	
	const auto& formatInformation = GetFormatInformation(imageView->getFormat());
	
	const auto x = static_cast<uint32_t>(coordinate->x * imageView->getImage()->getWidth());
	const auto y = static_cast<uint32_t>(coordinate->y * imageView->getImage()->getHeight());

	// TODO: Use SampleImage
	float values[4];
	GetPixel(formatInformation, imageView->getImage(), x, y, 0, values);
	*result = glm::vec4(values[0], values[1], values[2], values[3]);
}

template<typename ReturnType, typename CoordinateType>
static void ImageSampleExplicitLod(ReturnType* result, VkDescriptorImageInfo* sampledImage, CoordinateType* coordinate, float lod)
{
	// TODO: Use ImageSampler.cpp
	const auto sampler = UnwrapVulkan<Sampler>(sampledImage->sampler);
	auto imageView = UnwrapVulkan<ImageView>(sampledImage->imageView);

	// Instructions with ExplicitLod in the name determine the LOD used in the sampling operation based on additional coordinates.
	// https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html#textures-level-of-detail-operation 
	
	if (imageView->getViewType() != VK_IMAGE_VIEW_TYPE_2D)
	{
		FATAL_ERROR();
	}
	
	if (imageView->getSubresourceRange().aspectMask != VK_IMAGE_ASPECT_COLOR_BIT)
	{
		FATAL_ERROR();
	}
	
	if (imageView->getComponents().r != VK_COMPONENT_SWIZZLE_R && imageView->getComponents().r != VK_COMPONENT_SWIZZLE_IDENTITY)
	{
		FATAL_ERROR();
	}
	
	if (imageView->getComponents().g != VK_COMPONENT_SWIZZLE_G && imageView->getComponents().g != VK_COMPONENT_SWIZZLE_IDENTITY)
	{
		FATAL_ERROR();
	}
	
	if (imageView->getComponents().b != VK_COMPONENT_SWIZZLE_B && imageView->getComponents().b != VK_COMPONENT_SWIZZLE_IDENTITY)
	{
		FATAL_ERROR();
	}
	
	if (imageView->getComponents().a != VK_COMPONENT_SWIZZLE_A && imageView->getComponents().a != VK_COMPONENT_SWIZZLE_IDENTITY)
	{
		FATAL_ERROR();
	}
	
	if (imageView->getSubresourceRange().baseMipLevel != 0)
	{
		FATAL_ERROR();
	}
	
	if (imageView->getSubresourceRange().levelCount != 1)
	{
		FATAL_ERROR();
	}
	
	if (imageView->getSubresourceRange().baseArrayLayer != 0)
	{
		FATAL_ERROR();
	}
	
	if (imageView->getSubresourceRange().layerCount != 1)
	{
		FATAL_ERROR();
	}
	
	if (sampler->getFlags())
	{
		FATAL_ERROR();
	}
	
	if (sampler->getMagFilter() != VK_FILTER_NEAREST)
	{
		FATAL_ERROR();
	}
	
	if (sampler->getMinFilter() != VK_FILTER_NEAREST)
	{
		FATAL_ERROR();
	}
	
	if (sampler->getMipmapMode() != VK_SAMPLER_MIPMAP_MODE_NEAREST)
	{
		FATAL_ERROR();
	}
	
	if (sampler->getAnisotropyEnable())
	{
		FATAL_ERROR();
	}
	
	if (sampler->getCompareEnable())
	{
		FATAL_ERROR();
	}
	
	if (sampler->getUnnormalisedCoordinates())
	{
		FATAL_ERROR();
	}
	
	if (coordinate->x < 0 || coordinate->x > 1 || coordinate->y < 0 || coordinate->y > 1)
	{
		FATAL_ERROR();
	}
	
	const auto& formatInformation = GetFormatInformation(imageView->getFormat());
	
	const auto x = static_cast<uint32_t>(coordinate->x * imageView->getImage()->getWidth());
	const auto y = static_cast<uint32_t>(coordinate->y * imageView->getImage()->getHeight());

	// TODO: Use SampleImage
	float values[4];
	GetPixel(formatInformation, imageView->getImage(), x, y, 0, values);
	*result = glm::vec4(values[0], values[1], values[2], values[3]);
}

template<typename ReturnType, typename CoordinateType>
static void ImageFetch(ReturnType* result, BufferView* image, CoordinateType* coordinate)
{
	FATAL_ERROR();
}

void AddGlslFunctions(SpirvJit* jit)
{
	// Round = 1,
	// RoundEven = 2,
	// Trunc = 3,
	// FAbs = 4,
	 
	AddSpirvFunction("@SAbs.I8", reinterpret_cast<FunctionPointer>(SAbs<int8_t>));
	AddSpirvFunction("@SAbs.I8[2]", reinterpret_cast<FunctionPointer>(VSAbs<glm::i8vec2>));
	AddSpirvFunction("@SAbs.I8[3]", reinterpret_cast<FunctionPointer>(VSAbs<glm::i8vec3>));
	AddSpirvFunction("@SAbs.I8[4]", reinterpret_cast<FunctionPointer>(VSAbs<glm::i8vec4>));
	AddSpirvFunction("@SAbs.I16", reinterpret_cast<FunctionPointer>(SAbs<int16_t>));
	AddSpirvFunction("@SAbs.I16[2]", reinterpret_cast<FunctionPointer>(VSAbs<glm::i16vec2>));
	AddSpirvFunction("@SAbs.I16[3]", reinterpret_cast<FunctionPointer>(VSAbs<glm::i16vec3>));
	AddSpirvFunction("@SAbs.I16[4]", reinterpret_cast<FunctionPointer>(VSAbs<glm::i16vec4>));
	AddSpirvFunction("@SAbs.I32", reinterpret_cast<FunctionPointer>(SAbs<int32_t>));
	AddSpirvFunction("@SAbs.I32[2]", reinterpret_cast<FunctionPointer>(VSAbs<glm::i32vec2>));
	AddSpirvFunction("@SAbs.I32[3]", reinterpret_cast<FunctionPointer>(VSAbs<glm::i32vec3>));
	AddSpirvFunction("@SAbs.I32[4]", reinterpret_cast<FunctionPointer>(VSAbs<glm::i32vec4>));
	AddSpirvFunction("@SAbs.I64", reinterpret_cast<FunctionPointer>(SAbs<int64_t>));
	AddSpirvFunction("@SAbs.I64[2]", reinterpret_cast<FunctionPointer>(VSAbs<glm::i64vec2>));
	AddSpirvFunction("@SAbs.I64[3]", reinterpret_cast<FunctionPointer>(VSAbs<glm::i64vec3>));
	AddSpirvFunction("@SAbs.I64[4]", reinterpret_cast<FunctionPointer>(VSAbs<glm::i64vec4>));
	
	// FSign = 6,
	
	AddSpirvFunction("@SSign.I8", reinterpret_cast<FunctionPointer>(SSign<int8_t>));
	AddSpirvFunction("@SSign.I8[2]", reinterpret_cast<FunctionPointer>(VSSign<glm::i8vec2>));
	AddSpirvFunction("@SSign.I8[3]", reinterpret_cast<FunctionPointer>(VSSign<glm::i8vec3>));
	AddSpirvFunction("@SSign.I8[4]", reinterpret_cast<FunctionPointer>(VSSign<glm::i8vec4>));
	AddSpirvFunction("@SSign.I16", reinterpret_cast<FunctionPointer>(SSign<int16_t>));
	AddSpirvFunction("@SSign.I16[2]", reinterpret_cast<FunctionPointer>(VSSign<glm::i16vec2>));
	AddSpirvFunction("@SSign.I16[3]", reinterpret_cast<FunctionPointer>(VSSign<glm::i16vec3>));
	AddSpirvFunction("@SSign.I16[4]", reinterpret_cast<FunctionPointer>(VSSign<glm::i16vec4>));
	AddSpirvFunction("@SSign.I32", reinterpret_cast<FunctionPointer>(SSign<int32_t>));
	AddSpirvFunction("@SSign.I32[2]", reinterpret_cast<FunctionPointer>(VSSign<glm::i32vec2>));
	AddSpirvFunction("@SSign.I32[3]", reinterpret_cast<FunctionPointer>(VSSign<glm::i32vec3>));
	AddSpirvFunction("@SSign.I32[4]", reinterpret_cast<FunctionPointer>(VSSign<glm::i32vec4>));
	AddSpirvFunction("@SSign.I64", reinterpret_cast<FunctionPointer>(SSign<int64_t>));
	AddSpirvFunction("@SSign.I64[2]", reinterpret_cast<FunctionPointer>(VSSign<glm::i64vec2>));
	AddSpirvFunction("@SSign.I64[3]", reinterpret_cast<FunctionPointer>(VSSign<glm::i64vec3>));
	AddSpirvFunction("@SSign.I64[4]", reinterpret_cast<FunctionPointer>(VSSign<glm::i64vec4>));
	
	// SSign = 7,
	// Floor = 8,
	// Ceil = 9,
	// Fract = 10,
	// Radians = 11,
	// Degrees = 12,
	// Sin = 13,
	// Cos = 14,
	// Tan = 15,
	// Asin = 16,
	// Acos = 17,
	// Atan = 18,
	// Sinh = 19,
	// Cosh = 20,
	// Tanh = 21,
	// Asinh = 22,
	// Acosh = 23,
	// Atanh = 24,
	// Atan2 = 25,

	AddSpirvFunction("@Pow.F16.F16", reinterpret_cast<FunctionPointer>(Pow<half>));
	AddSpirvFunction("@Pow.F16[2].F16[2]", reinterpret_cast<FunctionPointer>(VPow<glm::f16vec2>));
	AddSpirvFunction("@Pow.F16[3].F16[3]", reinterpret_cast<FunctionPointer>(VPow<glm::f16vec3>));
	AddSpirvFunction("@Pow.F16[4].F16[4]", reinterpret_cast<FunctionPointer>(VPow<glm::f16vec4>));
	AddSpirvFunction("@Pow.F32.F32", reinterpret_cast<FunctionPointer>(Pow<float>));
	AddSpirvFunction("@Pow.F32[2].F32[2]", reinterpret_cast<FunctionPointer>(VPow<glm::f32vec2>));
	AddSpirvFunction("@Pow.F32[3].F32[3]", reinterpret_cast<FunctionPointer>(VPow<glm::f32vec3>));
	AddSpirvFunction("@Pow.F32[4].F32[4]", reinterpret_cast<FunctionPointer>(VPow<glm::f32vec4>));
	
	// Exp = 27,
	// Log = 28,
	// Exp2 = 29,
	// Log2 = 30,
	// Sqrt = 31,
	// InverseSqrt = 32,
	// Determinant = 33,
	// MatrixInverse = 34,
	// Modf = 35,
	// ModfStruct = 36,

	AddSpirvFunction("@FMin.F16.F16", reinterpret_cast<FunctionPointer>(Min<half>));
	AddSpirvFunction("@FMin.F16[2].F16[2]", reinterpret_cast<FunctionPointer>(VMin<glm::f16vec2>));
	AddSpirvFunction("@FMin.F16[3].F16[3]", reinterpret_cast<FunctionPointer>(VMin<glm::f16vec3>));
	AddSpirvFunction("@FMin.F16[4].F16[4]", reinterpret_cast<FunctionPointer>(VMin<glm::f16vec4>));
	AddSpirvFunction("@FMin.F32.F32", reinterpret_cast<FunctionPointer>(Min<float>));
	AddSpirvFunction("@FMin.F32[2].F32[2]", reinterpret_cast<FunctionPointer>(VMin<glm::f32vec2>));
	AddSpirvFunction("@FMin.F32[3].F32[3]", reinterpret_cast<FunctionPointer>(VMin<glm::f32vec3>));
	AddSpirvFunction("@FMin.F32[4].F32[4]", reinterpret_cast<FunctionPointer>(VMin<glm::f32vec4>));
	AddSpirvFunction("@FMin.F64.F64", reinterpret_cast<FunctionPointer>(Min<double>));
	AddSpirvFunction("@FMin.F64[2].F64[2]", reinterpret_cast<FunctionPointer>(VMin<glm::f64vec2>));
	AddSpirvFunction("@FMin.F64[3].F64[3]", reinterpret_cast<FunctionPointer>(VMin<glm::f64vec3>));
	AddSpirvFunction("@FMin.F64[4].F64[4]", reinterpret_cast<FunctionPointer>(VMin<glm::f64vec4>));
	
	AddSpirvFunction("@UMin.U8.U8", reinterpret_cast<FunctionPointer>(Min<uint8_t>));
	AddSpirvFunction("@UMin.U8[2].U8[2]", reinterpret_cast<FunctionPointer>(VMin<glm::u8vec2>));
	AddSpirvFunction("@UMin.U8[3].U8[3]", reinterpret_cast<FunctionPointer>(VMin<glm::u8vec3>));
	AddSpirvFunction("@UMin.U8[4].U8[4]", reinterpret_cast<FunctionPointer>(VMin<glm::u8vec4>));
	AddSpirvFunction("@UMin.U16.U16", reinterpret_cast<FunctionPointer>(Min<uint16_t>));
	AddSpirvFunction("@UMin.U16[2].U16[2]", reinterpret_cast<FunctionPointer>(VMin<glm::u16vec2>));
	AddSpirvFunction("@UMin.U16[3].U16[3]", reinterpret_cast<FunctionPointer>(VMin<glm::u16vec3>));
	AddSpirvFunction("@UMin.U16[4].U16[4]", reinterpret_cast<FunctionPointer>(VMin<glm::u16vec4>));
	AddSpirvFunction("@UMin.U32.U32", reinterpret_cast<FunctionPointer>(Min<uint32_t>));
	AddSpirvFunction("@UMin.U32[2].U32[2]", reinterpret_cast<FunctionPointer>(VMin<glm::u32vec2>));
	AddSpirvFunction("@UMin.U32[3].U32[3]", reinterpret_cast<FunctionPointer>(VMin<glm::u32vec3>));
	AddSpirvFunction("@UMin.U32[4].U32[4]", reinterpret_cast<FunctionPointer>(VMin<glm::u32vec4>));
	AddSpirvFunction("@UMin.U64.U64", reinterpret_cast<FunctionPointer>(Min<uint64_t>));
	AddSpirvFunction("@UMin.U64[2].U64[2]", reinterpret_cast<FunctionPointer>(VMin<glm::u64vec2>));
	AddSpirvFunction("@UMin.U64[3].U64[3]", reinterpret_cast<FunctionPointer>(VMin<glm::u64vec3>));
	AddSpirvFunction("@UMin.U64[4].U64[4]", reinterpret_cast<FunctionPointer>(VMin<glm::u64vec4>));

	AddSpirvFunction("@SMin.I8.I8", reinterpret_cast<FunctionPointer>(Min<int8_t>));
	AddSpirvFunction("@SMin.I8[2].I8[2]", reinterpret_cast<FunctionPointer>(VMin<glm::i8vec2>));
	AddSpirvFunction("@SMin.I8[3].I8[3]", reinterpret_cast<FunctionPointer>(VMin<glm::i8vec3>));
	AddSpirvFunction("@SMin.I8[4].I8[4]", reinterpret_cast<FunctionPointer>(VMin<glm::i8vec4>));
	AddSpirvFunction("@SMin.I16.I16", reinterpret_cast<FunctionPointer>(Min<int16_t>));
	AddSpirvFunction("@SMin.I16[2].I16[2]", reinterpret_cast<FunctionPointer>(VMin<glm::i16vec2>));
	AddSpirvFunction("@SMin.I16[3].I16[3]", reinterpret_cast<FunctionPointer>(VMin<glm::i16vec3>));
	AddSpirvFunction("@SMin.I16[4].I16[4]", reinterpret_cast<FunctionPointer>(VMin<glm::i16vec4>));
	AddSpirvFunction("@SMin.I32.I32", reinterpret_cast<FunctionPointer>(Min<int32_t>));
	AddSpirvFunction("@SMin.I32[2].I32[2]", reinterpret_cast<FunctionPointer>(VMin<glm::i32vec2>));
	AddSpirvFunction("@SMin.I32[3].I32[3]", reinterpret_cast<FunctionPointer>(VMin<glm::i32vec3>));
	AddSpirvFunction("@SMin.I32[4].I32[4]", reinterpret_cast<FunctionPointer>(VMin<glm::i32vec4>));
	AddSpirvFunction("@SMin.I64.I64", reinterpret_cast<FunctionPointer>(Min<int64_t>));
	AddSpirvFunction("@SMin.I64[2].I64[2]", reinterpret_cast<FunctionPointer>(VMin<glm::i64vec2>));
	AddSpirvFunction("@SMin.I64[3].I64[3]", reinterpret_cast<FunctionPointer>(VMin<glm::i64vec3>));
	AddSpirvFunction("@SMin.I64[4].I64[4]", reinterpret_cast<FunctionPointer>(VMin<glm::i64vec4>));

	AddSpirvFunction("@FMax.F16.F16", reinterpret_cast<FunctionPointer>(Max<half>));
	AddSpirvFunction("@FMax.F16[2].F16[2]", reinterpret_cast<FunctionPointer>(VMax<glm::f16vec2>));
	AddSpirvFunction("@FMax.F16[3].F16[3]", reinterpret_cast<FunctionPointer>(VMax<glm::f16vec3>));
	AddSpirvFunction("@FMax.F16[4].F16[4]", reinterpret_cast<FunctionPointer>(VMax<glm::f16vec4>));
	AddSpirvFunction("@FMax.F32.F32", reinterpret_cast<FunctionPointer>(Max<float>));
	AddSpirvFunction("@FMax.F32[2].F32[2]", reinterpret_cast<FunctionPointer>(VMax<glm::f32vec2>));
	AddSpirvFunction("@FMax.F32[3].F32[3]", reinterpret_cast<FunctionPointer>(VMax<glm::f32vec3>));
	AddSpirvFunction("@FMax.F32[4].F32[4]", reinterpret_cast<FunctionPointer>(VMax<glm::f32vec4>));
	AddSpirvFunction("@FMax.F64.F64", reinterpret_cast<FunctionPointer>(Max<double>));
	AddSpirvFunction("@FMax.F64[2].F64[2]", reinterpret_cast<FunctionPointer>(VMax<glm::f64vec2>));
	AddSpirvFunction("@FMax.F64[3].F64[3]", reinterpret_cast<FunctionPointer>(VMax<glm::f64vec3>));
	AddSpirvFunction("@FMax.F64[4].F64[4]", reinterpret_cast<FunctionPointer>(VMax<glm::f64vec4>));
	
	AddSpirvFunction("@UMax.U8.U8", reinterpret_cast<FunctionPointer>(Max<uint8_t>));
	AddSpirvFunction("@UMax.U8[2].U8[2]", reinterpret_cast<FunctionPointer>(VMax<glm::u8vec2>));
	AddSpirvFunction("@UMax.U8[3].U8[3]", reinterpret_cast<FunctionPointer>(VMax<glm::u8vec3>));
	AddSpirvFunction("@UMax.U8[4].U8[4]", reinterpret_cast<FunctionPointer>(VMax<glm::u8vec4>));
	AddSpirvFunction("@UMax.U16.U16", reinterpret_cast<FunctionPointer>(Max<uint16_t>));
	AddSpirvFunction("@UMax.U16[2].U16[2]", reinterpret_cast<FunctionPointer>(VMax<glm::u16vec2>));
	AddSpirvFunction("@UMax.U16[3].U16[3]", reinterpret_cast<FunctionPointer>(VMax<glm::u16vec3>));
	AddSpirvFunction("@UMax.U16[4].U16[4]", reinterpret_cast<FunctionPointer>(VMax<glm::u16vec4>));
	AddSpirvFunction("@UMax.U32.U32", reinterpret_cast<FunctionPointer>(Max<uint32_t>));
	AddSpirvFunction("@UMax.U32[2].U32[2]", reinterpret_cast<FunctionPointer>(VMax<glm::u32vec2>));
	AddSpirvFunction("@UMax.U32[3].U32[3]", reinterpret_cast<FunctionPointer>(VMax<glm::u32vec3>));
	AddSpirvFunction("@UMax.U32[4].U32[4]", reinterpret_cast<FunctionPointer>(VMax<glm::u32vec4>));
	AddSpirvFunction("@UMax.U64.U64", reinterpret_cast<FunctionPointer>(Max<uint64_t>));
	AddSpirvFunction("@UMax.U64[2].U64[2]", reinterpret_cast<FunctionPointer>(VMax<glm::u64vec2>));
	AddSpirvFunction("@UMax.U64[3].U64[3]", reinterpret_cast<FunctionPointer>(VMax<glm::u64vec3>));
	AddSpirvFunction("@UMax.U64[4].U64[4]", reinterpret_cast<FunctionPointer>(VMax<glm::u64vec4>));

	AddSpirvFunction("@SMax.I8.I8", reinterpret_cast<FunctionPointer>(Max<int8_t>));
	AddSpirvFunction("@SMax.I8[2].I8[2]", reinterpret_cast<FunctionPointer>(VMax<glm::i8vec2>));
	AddSpirvFunction("@SMax.I8[3].I8[3]", reinterpret_cast<FunctionPointer>(VMax<glm::i8vec3>));
	AddSpirvFunction("@SMax.I8[4].I8[4]", reinterpret_cast<FunctionPointer>(VMax<glm::i8vec4>));
	AddSpirvFunction("@SMax.I16.I16", reinterpret_cast<FunctionPointer>(Max<int16_t>));
	AddSpirvFunction("@SMax.I16[2].I16[2]", reinterpret_cast<FunctionPointer>(VMax<glm::i16vec2>));
	AddSpirvFunction("@SMax.I16[3].I16[3]", reinterpret_cast<FunctionPointer>(VMax<glm::i16vec3>));
	AddSpirvFunction("@SMax.I16[4].I16[4]", reinterpret_cast<FunctionPointer>(VMax<glm::i16vec4>));
	AddSpirvFunction("@SMax.I32.I32", reinterpret_cast<FunctionPointer>(Max<int32_t>));
	AddSpirvFunction("@SMax.I32[2].I32[2]", reinterpret_cast<FunctionPointer>(VMax<glm::i32vec2>));
	AddSpirvFunction("@SMax.I32[3].I32[3]", reinterpret_cast<FunctionPointer>(VMax<glm::i32vec3>));
	AddSpirvFunction("@SMax.I32[4].I32[4]", reinterpret_cast<FunctionPointer>(VMax<glm::i32vec4>));
	AddSpirvFunction("@SMax.I64.I64", reinterpret_cast<FunctionPointer>(Max<int64_t>));
	AddSpirvFunction("@SMax.I64[2].I64[2]", reinterpret_cast<FunctionPointer>(VMax<glm::i64vec2>));
	AddSpirvFunction("@SMax.I64[3].I64[3]", reinterpret_cast<FunctionPointer>(VMax<glm::i64vec3>));
	AddSpirvFunction("@SMax.I64[4].I64[4]", reinterpret_cast<FunctionPointer>(VMax<glm::i64vec4>));
	
	// FClamp = 43,
	
	AddSpirvFunction("@UClamp.U8.U8.U8", reinterpret_cast<FunctionPointer>(Clamp<uint8_t>));
	AddSpirvFunction("@UClamp.U8[2].U8[2].U8[2]", reinterpret_cast<FunctionPointer>(VClamp<glm::u8vec2>));
	AddSpirvFunction("@UClamp.U8[3].U8[3].U8[3]", reinterpret_cast<FunctionPointer>(VClamp<glm::u8vec3>));
	AddSpirvFunction("@UClamp.U8[4].U8[4].U8[4]", reinterpret_cast<FunctionPointer>(VClamp<glm::u8vec4>));
	AddSpirvFunction("@UClamp.U16.U16.U16", reinterpret_cast<FunctionPointer>(Clamp<uint16_t>));
	AddSpirvFunction("@UClamp.U16[2].U16[2].U16[2]", reinterpret_cast<FunctionPointer>(VClamp<glm::u16vec2>));
	AddSpirvFunction("@UClamp.U16[3].U16[3].U16[3]", reinterpret_cast<FunctionPointer>(VClamp<glm::u16vec3>));
	AddSpirvFunction("@UClamp.U16[4].U16[4].U16[4]", reinterpret_cast<FunctionPointer>(VClamp<glm::u16vec4>));
	AddSpirvFunction("@UClamp.U32.U32.U32", reinterpret_cast<FunctionPointer>(Clamp<uint32_t>));
	AddSpirvFunction("@UClamp.U32[2].U32[2].U32[2]", reinterpret_cast<FunctionPointer>(VClamp<glm::u32vec2>));
	AddSpirvFunction("@UClamp.U32[3].U32[3].U32[3]", reinterpret_cast<FunctionPointer>(VClamp<glm::u32vec3>));
	AddSpirvFunction("@UClamp.U32[4].U32[4].U32[4]", reinterpret_cast<FunctionPointer>(VClamp<glm::u32vec4>));
	AddSpirvFunction("@UClamp.U64.U64.U64", reinterpret_cast<FunctionPointer>(Clamp<uint64_t>));
	AddSpirvFunction("@UClamp.U64[2].U64[2].U64[2]", reinterpret_cast<FunctionPointer>(VClamp<glm::u64vec2>));
	AddSpirvFunction("@UClamp.U64[3].U64[3].U64[3]", reinterpret_cast<FunctionPointer>(VClamp<glm::u64vec3>));
	AddSpirvFunction("@UClamp.U64[4].U64[4].U64[4]", reinterpret_cast<FunctionPointer>(VClamp<glm::u64vec4>));

	AddSpirvFunction("@SClamp.I8.I8.I8", reinterpret_cast<FunctionPointer>(Clamp<int8_t>));
	AddSpirvFunction("@SClamp.I8[2].I8[2].I8[2]", reinterpret_cast<FunctionPointer>(VClamp<glm::i8vec2>));
	AddSpirvFunction("@SClamp.I8[3].I8[3].I8[3]", reinterpret_cast<FunctionPointer>(VClamp<glm::i8vec3>));
	AddSpirvFunction("@SClamp.I8[4].I8[4].I8[4]", reinterpret_cast<FunctionPointer>(VClamp<glm::i8vec4>));
	AddSpirvFunction("@SClamp.I16.I16.I16", reinterpret_cast<FunctionPointer>(Clamp<int16_t>));
	AddSpirvFunction("@SClamp.I16[2].I16[2].I16[2]", reinterpret_cast<FunctionPointer>(VClamp<glm::i16vec2>));
	AddSpirvFunction("@SClamp.I16[3].I16[3].I16[3]", reinterpret_cast<FunctionPointer>(VClamp<glm::i16vec3>));
	AddSpirvFunction("@SClamp.I16[4].I16[4].I16[4]", reinterpret_cast<FunctionPointer>(VClamp<glm::i16vec4>));
	AddSpirvFunction("@SClamp.I32.I32.I32", reinterpret_cast<FunctionPointer>(Clamp<int32_t>));
	AddSpirvFunction("@SClamp.I32[2].I32[2].I32[2]", reinterpret_cast<FunctionPointer>(VClamp<glm::i32vec2>));
	AddSpirvFunction("@SClamp.I32[3].I32[3].I32[3]", reinterpret_cast<FunctionPointer>(VClamp<glm::i32vec3>));
	AddSpirvFunction("@SClamp.I32[4].I32[4].I32[4]", reinterpret_cast<FunctionPointer>(VClamp<glm::i32vec4>));
	AddSpirvFunction("@SClamp.I64.I64.I64", reinterpret_cast<FunctionPointer>(Clamp<int64_t>));
	AddSpirvFunction("@SClamp.I64[2].I64[2].I64[2]", reinterpret_cast<FunctionPointer>(VClamp<glm::i64vec2>));
	AddSpirvFunction("@SClamp.I64[3].I64[3].I64[3]", reinterpret_cast<FunctionPointer>(VClamp<glm::i64vec3>));
	AddSpirvFunction("@SClamp.I64[4].I64[4].I64[4]", reinterpret_cast<FunctionPointer>(VClamp<glm::i64vec4>));

	AddSpirvFunction("@FMix.F16.F16.F16", reinterpret_cast<FunctionPointer>(Mix<half>));
	AddSpirvFunction("@FMix.F16[2].F16[2].F16[2]", reinterpret_cast<FunctionPointer>(VMix<glm::f16vec2>));
	AddSpirvFunction("@FMix.F16[3].F16[3].F16[3]", reinterpret_cast<FunctionPointer>(VMix<glm::f16vec3>));
	AddSpirvFunction("@FMix.F16[4].F16[4].F16[4]", reinterpret_cast<FunctionPointer>(VMix<glm::f16vec4>));
	AddSpirvFunction("@FMix.F32.F32.F32", reinterpret_cast<FunctionPointer>(Mix<float>));
	AddSpirvFunction("@FMix.F32[2].F32[2].F32[2]", reinterpret_cast<FunctionPointer>(VMix<glm::f32vec2>));
	AddSpirvFunction("@FMix.F32[3].F32[3].F32[3]", reinterpret_cast<FunctionPointer>(VMix<glm::f32vec3>));
	AddSpirvFunction("@FMix.F32[4].F32[4].F32[4]", reinterpret_cast<FunctionPointer>(VMix<glm::f32vec4>));
	AddSpirvFunction("@FMix.F64.F64.F64", reinterpret_cast<FunctionPointer>(Mix<double>));
	AddSpirvFunction("@FMix.F64[2].F64[2].F64[2]", reinterpret_cast<FunctionPointer>(VMix<glm::f64vec2>));
	AddSpirvFunction("@FMix.F64[3].F64[3].F64[3]", reinterpret_cast<FunctionPointer>(VMix<glm::f64vec3>));
	AddSpirvFunction("@FMix.F64[4].F64[4].F64[4]", reinterpret_cast<FunctionPointer>(VMix<glm::f64vec4>));
	
	// Step = 48,
	// SmoothStep = 49,
	// Fma = 50,
	// Frexp = 51,
	// FrexpStruct = 52,
	// Ldexp = 53,
	// PackSnorm4x8 = 54,
	// PackUnorm4x8 = 55,
	// PackSnorm2x16 = 56,
	// PackUnorm2x16 = 57,
	// PackHalf2x16 = 58,
	// PackDouble2x32 = 59,
	// UnpackSnorm2x16 = 60,
	// UnpackUnorm2x16 = 61,
	// UnpackHalf2x16 = 62,
	// UnpackSnorm4x8 = 63,
	// UnpackUnorm4x8 = 64,
	// UnpackDouble2x32 = 65,
	// Length = 66,
	// Distance = 67,
	// Cross = 68,

	AddSpirvFunction("@Normalise.F16", reinterpret_cast<FunctionPointer>(Normalise<half>));
	AddSpirvFunction("@Normalise.F16[2]", reinterpret_cast<FunctionPointer>(VNormalise<glm::f16vec2>));
	AddSpirvFunction("@Normalise.F16[3]", reinterpret_cast<FunctionPointer>(VNormalise<glm::f16vec3>));
	AddSpirvFunction("@Normalise.F16[4]", reinterpret_cast<FunctionPointer>(VNormalise<glm::f16vec4>));
	AddSpirvFunction("@Normalise.F32", reinterpret_cast<FunctionPointer>(Normalise<float>));
	AddSpirvFunction("@Normalise.F32[2]", reinterpret_cast<FunctionPointer>(VNormalise<glm::f32vec2>));
	AddSpirvFunction("@Normalise.F32[3]", reinterpret_cast<FunctionPointer>(VNormalise<glm::f32vec3>));
	AddSpirvFunction("@Normalise.F32[4]", reinterpret_cast<FunctionPointer>(VNormalise<glm::f32vec4>));
	AddSpirvFunction("@Normalise.F64", reinterpret_cast<FunctionPointer>(Normalise<double>));
	AddSpirvFunction("@Normalise.F64[2]", reinterpret_cast<FunctionPointer>(VNormalise<glm::f64vec2>));
	AddSpirvFunction("@Normalise.F64[3]", reinterpret_cast<FunctionPointer>(VNormalise<glm::f64vec3>));
	AddSpirvFunction("@Normalise.F64[4]", reinterpret_cast<FunctionPointer>(VNormalise<glm::f64vec4>));
	
	// FaceForward = 70,

	AddSpirvFunction("@Reflect.F16.F16", reinterpret_cast<FunctionPointer>(Reflect<half>));
	AddSpirvFunction("@Reflect.F16[2].F16[2]", reinterpret_cast<FunctionPointer>(Reflect<glm::f16vec2>));
	AddSpirvFunction("@Reflect.F16[3].F16[3]", reinterpret_cast<FunctionPointer>(Reflect<glm::f16vec3>));
	AddSpirvFunction("@Reflect.F16[4].F16[4]", reinterpret_cast<FunctionPointer>(Reflect<glm::f16vec4>));
	AddSpirvFunction("@Reflect.F32.F32", reinterpret_cast<FunctionPointer>(Reflect<float>));
	AddSpirvFunction("@Reflect.F32[2].F32[2]", reinterpret_cast<FunctionPointer>(Reflect<glm::f32vec2>));
	AddSpirvFunction("@Reflect.F32[3].F32[3]", reinterpret_cast<FunctionPointer>(Reflect<glm::f32vec3>));
	AddSpirvFunction("@Reflect.F32[4].F32[4]", reinterpret_cast<FunctionPointer>(Reflect<glm::f32vec4>));
	AddSpirvFunction("@Reflect.F64.F64", reinterpret_cast<FunctionPointer>(Reflect<double>));
	AddSpirvFunction("@Reflect.F64[2].F64[2]", reinterpret_cast<FunctionPointer>(Reflect<glm::f64vec2>));
	AddSpirvFunction("@Reflect.F64[3].F64[3]", reinterpret_cast<FunctionPointer>(Reflect<glm::f64vec3>));
	AddSpirvFunction("@Reflect.F64[4].F64[4]", reinterpret_cast<FunctionPointer>(Reflect<glm::f64vec4>));
	
	// Refract = 72,

	AddSpirvFunction("@FindILsb.I32", reinterpret_cast<FunctionPointer>(FindILsb<int32_t>));
	AddSpirvFunction("@FindILsb.I32[2]", reinterpret_cast<FunctionPointer>(VFindILsb<glm::i32vec2>));
	AddSpirvFunction("@FindILsb.I32[3]", reinterpret_cast<FunctionPointer>(VFindILsb<glm::i32vec3>));
	AddSpirvFunction("@FindILsb.I32[4]", reinterpret_cast<FunctionPointer>(VFindILsb<glm::i32vec4>));
	AddSpirvFunction("@FindILsb.U32", reinterpret_cast<FunctionPointer>(FindILsb<uint32_t>));
	AddSpirvFunction("@FindILsb.U32[2]", reinterpret_cast<FunctionPointer>(VFindILsb<glm::u32vec2>));
	AddSpirvFunction("@FindILsb.U32[3]", reinterpret_cast<FunctionPointer>(VFindILsb<glm::u32vec3>));
	AddSpirvFunction("@FindILsb.U32[4]", reinterpret_cast<FunctionPointer>(VFindILsb<glm::u32vec4>));

	AddSpirvFunction("@FindSMsb.I32", reinterpret_cast<FunctionPointer>(FindSMsb<int32_t>));
	AddSpirvFunction("@FindSMsb.I32[2]", reinterpret_cast<FunctionPointer>(VFindSMsb<glm::i32vec2>));
	AddSpirvFunction("@FindSMsb.I32[3]", reinterpret_cast<FunctionPointer>(VFindSMsb<glm::i32vec3>));
	AddSpirvFunction("@FindSMsb.I32[4]", reinterpret_cast<FunctionPointer>(VFindSMsb<glm::i32vec4>));

	AddSpirvFunction("@FindUMsb.U32", reinterpret_cast<FunctionPointer>(FindUMsb<uint32_t>));
	AddSpirvFunction("@FindUMsb.U32[2]", reinterpret_cast<FunctionPointer>(VFindUMsb<glm::u32vec2>));
	AddSpirvFunction("@FindUMsb.U32[3]", reinterpret_cast<FunctionPointer>(VFindUMsb<glm::u32vec3>));
	AddSpirvFunction("@FindUMsb.U32[4]", reinterpret_cast<FunctionPointer>(VFindUMsb<glm::u32vec4>));

	// InterpolateAtCentroid = 76,
	// InterpolateAtSample = 77,
	// InterpolateAtOffset = 78,
	// NMin = 79,
	// NMax = 80,
	// NClamp = 81,
	
	AddSpirvFunction("@Image.Sample.Implicit.F32[4].F32[2].Lod", reinterpret_cast<FunctionPointer>(ImageSampleImplicitLod<glm::vec4, glm::vec2>));
	AddSpirvFunction("@Image.Sample.Explicit.F32[4].F32[2].Lod", reinterpret_cast<FunctionPointer>(ImageSampleExplicitLod<glm::vec4, glm::vec2>));
	AddSpirvFunction("@Image.Fetch.U32[4].I32", reinterpret_cast<FunctionPointer>(ImageFetch<glm::uvec4, glm::ivec1>));
}
