#include "GlslFunctions.h"

#include "Base.h"
#include "Buffer.h"
#include "BufferView.h"
#include "DescriptorSet.h"
#include "DeviceState.h"
#include "Half.h"
#include "Image.h"
#include "ImageSampler.h"
#include "ImageView.h"
#include "Sampler.h"

#include <Formats.h>
#include <Jit.h>

#include <glm/glm.hpp>

template<typename T>
static T Abs(T value)
{
	return std::abs(value);
}

template<typename T>
static T VAbs(T value)
{
	T result{0};
	for (auto i = 0; i < T::length(); i++)
	{
		result[i] = Abs(value[i]);
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
static T Sin(T value)
{
	return std::sin(value);
}

template<typename T>
static T VSin(T value)
{
	T result{0};
	for (auto i = 0; i < T::length(); i++)
	{
		result[i] = Sin(value[i]);
	}
	return result;
}

template<typename T>
static T Cos(T value)
{
	return std::cos(value);
}

template<typename T>
static T VCos(T value)
{
	T result{0};
	for (auto i = 0; i < T::length(); i++)
	{
		result[i] = Cos(value[i]);
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
	T result{0};
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

template<typename T>
static T NMin(T left, T right)
{
	if (std::isnan(left))
	{
		return right;
	}
	return std::min(left, right);
}

template<typename T>
static T VNMin(T left, T right)
{
	T result{0};
	for (auto i = 0; i < T::length(); i++)
	{
		result[i] = NMin(left[i], right[i]);
	}
	return result;
}

template<typename T>
static T NMax(T left, T right)
{
	if (std::isnan(left))
	{
		return right;
	}
	return std::max(left, right);
}

template<typename T>
static T VNMax(T left, T right)
{
	T result{0};
	for (auto i = 0; i < T::length(); i++)
	{
		result[i] = NMax(left[i], right[i]);
	}
	return result;
}

template<typename T>
static T NClamp(T x, T min, T max)
{
	return NMin(NMax(x, min), max);
}

template<typename T>
static T VNClamp(T x, T min, T max)
{
	T result{0};
	for (auto i = 0; i < T::length(); i++)
	{
		result[i] = NClamp(x[i], min[i], max[i]);
	}
	return result;
}


static void GetFormatOffset(Image* image, const VkImageSubresourceRange& subresourceRange, uint64_t offset[MAX_MIP_LEVELS], uint64_t size[MAX_MIP_LEVELS], uint32_t& baseLevel, uint32_t& levels, uint32_t layer)
{
	auto layerCount = subresourceRange.layerCount;
	if (layerCount == VK_REMAINING_ARRAY_LAYERS)
	{
		layerCount = image->getArrayLayers() - subresourceRange.baseArrayLayer;
	}

	layer += subresourceRange.baseArrayLayer;
	
	if (layer >= layerCount + subresourceRange.baseArrayLayer)
	{
		layer = layerCount + subresourceRange.baseArrayLayer - 1;
	}

	baseLevel = subresourceRange.baseMipLevel;
	levels = subresourceRange.levelCount;

	if (levels == VK_REMAINING_MIP_LEVELS)
	{
		levels = image->getMipLevels() - baseLevel;
	}

	for (auto i = 0u; i < levels; i++)
	{
		offset[i] = image->getImageSize().Level[i + subresourceRange.baseMipLevel].Offset + layer * image->getImageSize().LayerSize;
		size[i] = image->getImageSize().Level[i + subresourceRange.baseMipLevel].LevelSize;
	}
}

template<int length>
glm::vec<length, uint32_t> GetImageRange(Image* image, const VkImageSubresourceRange& subresourceRange, uint32_t level);

template<>
glm::uvec1 GetImageRange<1>(Image* image, const VkImageSubresourceRange& subresourceRange, uint32_t level)
{
	const auto& size = image->getImageSize().Level[level + subresourceRange.baseMipLevel];
	return glm::uvec1{size.Width};
}

template<>
glm::uvec2 GetImageRange<2>(Image* image, const VkImageSubresourceRange& subresourceRange, uint32_t level)
{
	const auto& size = image->getImageSize().Level[level + subresourceRange.baseMipLevel];
	return glm::uvec2{size.Width, size.Height};
}

template<>
glm::uvec3 GetImageRange<3>(Image* image, const VkImageSubresourceRange& subresourceRange, uint32_t level)
{
	const auto& size = image->getImageSize().Level[level + subresourceRange.baseMipLevel];
	return glm::uvec3{size.Width, size.Height, size.Depth};
}

template<int length>
static void GetImageData(ImageDescriptor* descriptor, VkFormat& format, gsl::span<uint8_t> data[MAX_MIP_LEVELS], glm::vec<length, uint32_t> range[MAX_MIP_LEVELS], uint32_t& baseLevel, uint32_t& levels)
{
	const auto imageView = descriptor->Data.Image;
	const auto bufferView = descriptor->Data.Buffer;

	switch (descriptor->Type)
	{
	case ImageDescriptorType::Buffer:
		if constexpr (length == 1)
		{
			format = bufferView->getFormat();
			data[0] = bufferView->getBuffer()->getData(bufferView->getOffset(), bufferView->getRange());
			range[0] = glm::uvec1{ static_cast<uint32_t>(bufferView->getRange()) };
			baseLevel = 0;
			levels = 1;
		}
		else
		{
			FATAL_ERROR();
		}
		break;

	case ImageDescriptorType::Image:
		if (imageView->getImage()->getSamples() != VK_SAMPLE_COUNT_1_BIT)
		{
			FATAL_ERROR();
		}

		format = imageView->getFormat();
		uint64_t offset[MAX_MIP_LEVELS];
		uint64_t size[MAX_MIP_LEVELS];
		GetFormatOffset(imageView->getImage(), imageView->getSubresourceRange(), offset, size, baseLevel, levels, 0);
		for (auto i = 0u; i < levels; i++)
		{
			data[i] = imageView->getImage()->getData(offset[i], size[i]);
			range[i] = GetImageRange<length>(imageView->getImage(), imageView->getSubresourceRange(), i);
		}
		break;

	default: FATAL_ERROR();
	}
}

template<typename CoordinateType>
static void GetImageDataArray(ImageDescriptor* descriptor, VkFormat& format, CoordinateType coordinates, gsl::span<uint8_t> data[MAX_MIP_LEVELS], glm::vec<CoordinateType::length() - 1, uint32_t> range[MAX_MIP_LEVELS], uint32_t& baseLevel, uint32_t& levels)
{
	assert(descriptor->Type == ImageDescriptorType::Image);

	const auto imageView = descriptor->Data.Image;

	if (imageView->getImage()->getSamples() != VK_SAMPLE_COUNT_1_BIT)
	{
		FATAL_ERROR();
	}

	format = imageView->getFormat();
	uint64_t offset[MAX_MIP_LEVELS];
	uint64_t size[MAX_MIP_LEVELS];
	GetFormatOffset(imageView->getImage(), imageView->getSubresourceRange(), offset, size, baseLevel, levels, static_cast<uint32_t>(round(coordinates[CoordinateType::length() - 1])));
	for (auto i = 0u; i < levels; i++)
	{
		data[i] = imageView->getImage()->getData(offset[i], size[i]);
		range[i] = GetImageRange<CoordinateType::length() - 1>(imageView->getImage(), imageView->getSubresourceRange(), i);
	}
}

template<bool Array>
void GetImageDataCube(ImageDescriptor* descriptor, VkFormat& format, glm::vec<3 + (Array ? 1 : 0), float> coordinates, gsl::span<uint8_t> data[MAX_MIP_LEVELS], glm::uvec2 range[MAX_MIP_LEVELS], uint32_t& baseLevel, uint32_t& levels, glm::fvec2& realCoordinates)
{
	assert(descriptor->Type == ImageDescriptorType::Image);

	const auto imageView = descriptor->Data.Image;

	if (imageView->getImage()->getSamples() != VK_SAMPLE_COUNT_1_BIT)
	{
		FATAL_ERROR();
	}

	const auto abs = glm::abs(coordinates);
	uint32_t layer;
	float sc;
	float tc;
	float rc;
	if (abs.z >= abs.y && abs.z >= abs.x)
	{
		if (coordinates.z < 0)
		{
			layer = 5;
			sc = -coordinates.x;
		}
		else
		{
			layer = 4;
			sc = coordinates.x;
		}
		tc = -coordinates.y;
		rc = coordinates.z;
	}
	else if (abs.y >= abs.x)
	{
		if (coordinates.y < 0)
		{
			layer = 3;
			tc = -coordinates.z;
		}
		else
		{
			layer = 2;
			tc = coordinates.z;
		}
		sc = coordinates.x;
		rc = coordinates.y;
	}
	else
	{
		if (coordinates.x < 0)
		{
			layer = 1;
			sc = coordinates.z;
		}
		else
		{
			layer = 0;
			sc = -coordinates.z;
		}
		tc = -coordinates.y;
		rc = coordinates.x;
	}

	if constexpr (Array)
	{
		layer += static_cast<uint32_t>(round(coordinates.w)) * 6;
	}

	realCoordinates.x = 0.5f * (sc / std::abs(rc)) + 0.5f;
	realCoordinates.y = 0.5f * (tc / std::abs(rc)) + 0.5f;

	format = imageView->getFormat();
	uint64_t offset[MAX_MIP_LEVELS];
	uint64_t size[MAX_MIP_LEVELS];
	GetFormatOffset(imageView->getImage(), imageView->getSubresourceRange(), offset, size, baseLevel, levels, layer);
	for (auto i = 0u; i < levels; i++)
	{
		data[i] = imageView->getImage()->getData(offset[i], size[i]);
		range[i] = GetImageRange<2>(imageView->getImage(), imageView->getSubresourceRange(), i);
	}
}

template<typename T>
static glm::vec<T::length() - 1, typename T::value_type> ReduceDimension(T value)
{
	glm::vec<T::length() - 1, typename T::value_type> result{};
	for (auto i = 0; i < T::length() - 1; i++)
	{
		result[i] = value[i];
	}
	return result;
}

template<typename T>
static T Swizzle(glm::vec<4, T> vector, VkComponentSwizzle swizzle, int index)
{
	switch (swizzle)
	{
	case VK_COMPONENT_SWIZZLE_IDENTITY: return vector[index];
	case VK_COMPONENT_SWIZZLE_ZERO: return 0;
	case VK_COMPONENT_SWIZZLE_ONE: return 1;
	case VK_COMPONENT_SWIZZLE_R: return vector.r;
	case VK_COMPONENT_SWIZZLE_G: return vector.g;
	case VK_COMPONENT_SWIZZLE_B: return vector.b;
	case VK_COMPONENT_SWIZZLE_A: return vector.a;
	default: FATAL_ERROR();
	}
}

template<typename ReturnType, typename CoordinateType, bool Array = false, bool Cube = false>
static void ImageSampleExplicitLod(DeviceState* deviceState, ReturnType* result, ImageDescriptor* descriptor, CoordinateType* coordinates, float lod)
{
	constexpr auto finalLength = CoordinateType::length() + (Array ? -1 : 0) + (Cube ? -1 : 0);
		
	VkFormat format;
	gsl::span<uint8_t> data[MAX_MIP_LEVELS];
	glm::vec<finalLength, uint32_t> range[MAX_MIP_LEVELS];
	glm::vec<finalLength, float> realCoordinates;
	uint32_t baseLevel;
	uint32_t levels;
	if constexpr (Cube)
	{
		GetImageDataCube<Array>(descriptor, format, *coordinates, data, range, baseLevel, levels, realCoordinates);
	}
	else if constexpr (Array)
	{
		GetImageDataArray(descriptor, format, *coordinates, data, range, baseLevel, levels);
		realCoordinates = ReduceDimension(*coordinates);
	}
	else
	{
		GetImageData(descriptor, format, data, range, baseLevel, levels);
		realCoordinates = *coordinates;
	}

	const auto sampler = descriptor->ImageSampler;
	const auto lambdaBase = lod;
	constexpr float bias = 0;
	const auto lambdaPrime = lambdaBase + std::clamp(sampler->getMipLodBias() + bias, -MAX_SAMPLER_LOD_BIAS, MAX_SAMPLER_LOD_BIAS);
	const auto lambda = std::clamp(lambdaPrime, sampler->getMinLod(), sampler->getMaxLod());
	*result = SampleImage<ReturnType>(deviceState,
	                                  format,
	                                  data,
	                                  range,
	                                  baseLevel,
	                                  levels,
	                                  realCoordinates,
	                                  lambda,
	                                  sampler);

	if (descriptor->Type == ImageDescriptorType::Image)
	{
		const auto imageView = descriptor->Data.Image;
		if ((imageView->getComponents().r != VK_COMPONENT_SWIZZLE_IDENTITY && imageView->getComponents().r != VK_COMPONENT_SWIZZLE_R) ||
			(imageView->getComponents().g != VK_COMPONENT_SWIZZLE_IDENTITY && imageView->getComponents().g != VK_COMPONENT_SWIZZLE_G) ||
			(imageView->getComponents().b != VK_COMPONENT_SWIZZLE_IDENTITY && imageView->getComponents().b != VK_COMPONENT_SWIZZLE_B) ||
			(imageView->getComponents().a != VK_COMPONENT_SWIZZLE_IDENTITY && imageView->getComponents().a != VK_COMPONENT_SWIZZLE_A))
		{
			const auto oldResult = *result;
			result->r = Swizzle(oldResult, imageView->getComponents().r, 0);
			result->g = Swizzle(oldResult, imageView->getComponents().g, 1);
			result->b = Swizzle(oldResult, imageView->getComponents().b, 2);
			result->a = Swizzle(oldResult, imageView->getComponents().a, 3);
		}
	}
}

template<typename ReturnType, typename CoordinateType, bool Array = false, bool Cube = false>
static void ImageSampleImplicitLod(DeviceState* deviceState, ReturnType* result, ImageDescriptor* descriptor, CoordinateType* coordinates)
{
	// TODO: Lod
	// 	const auto sampler = descriptor->Sampler;
	// #if defined(SAMPLER_ANISOTROPY)
	// 	auto lambdaBase = 1.0f;
	// 	if (sampler->getAnisotropyEnable())
	// 	{
	// 		const auto eta = std::min(pMax / pMin, sampler->getMaxAnisotropy());
	// 		lambdaBase = std::log2(pMax / eta);
	// 	}
	// #else
	// 	constexpr auto lambdaBase = 0.0f;
	// #endif
	return ImageSampleExplicitLod<ReturnType, CoordinateType, Array, Cube>(deviceState, result, descriptor, coordinates, 0);
}

template<typename ReturnType, typename CoordinateType, bool Array = false, bool Cube = false>
static void ImageFetch(DeviceState* deviceState, ReturnType* result, ImageDescriptor* descriptor, CoordinateType* coordinates)
{
	static_assert(!Cube);
	
	VkFormat format;
	gsl::span<uint8_t> data[MAX_MIP_LEVELS];
	glm::vec<CoordinateType::length() + (Array ? -1 : 0), uint32_t> range[MAX_MIP_LEVELS];
	uint32_t baseLevel;
	uint32_t levels;
	if constexpr (Array)
	{
		GetImageDataArray(descriptor, format, *coordinates, data, range, levels);
	}
	else
	{
		GetImageData(descriptor, format, data, range, baseLevel, levels);
	}
	if (levels != 1)
	{
		FATAL_ERROR();
	}
	if (baseLevel != 0)
	{
		FATAL_ERROR();
	}
	
	if constexpr (Array)
	{
		*result = GetPixel<ReturnType>(deviceState,
		                               format,
		                               data[0],
		                               range[0],
		                               ReduceDimension(*coordinates),
		                               ReturnType{});
	}
	else
	{
		*result = GetPixel<ReturnType>(deviceState,
		                               format,
		                               data[0],
		                               range[0],
		                               *coordinates,
		                               ReturnType{});
	}

	if (descriptor->Type == ImageDescriptorType::Image)
	{
		const auto imageView = descriptor->Data.Image;
		if ((imageView->getComponents().r != VK_COMPONENT_SWIZZLE_IDENTITY && imageView->getComponents().r != VK_COMPONENT_SWIZZLE_R) ||
			(imageView->getComponents().g != VK_COMPONENT_SWIZZLE_IDENTITY && imageView->getComponents().g != VK_COMPONENT_SWIZZLE_G) ||
			(imageView->getComponents().b != VK_COMPONENT_SWIZZLE_IDENTITY && imageView->getComponents().b != VK_COMPONENT_SWIZZLE_B) ||
			(imageView->getComponents().a != VK_COMPONENT_SWIZZLE_IDENTITY && imageView->getComponents().a != VK_COMPONENT_SWIZZLE_A))
		{
			const auto oldResult = *result;
			result->r = Swizzle(oldResult, imageView->getComponents().r, 0);
			result->g = Swizzle(oldResult, imageView->getComponents().g, 1);
			result->b = Swizzle(oldResult, imageView->getComponents().b, 2);
			result->a = Swizzle(oldResult, imageView->getComponents().a, 3);
		}
	}
}

template<typename ReturnType, typename CoordinateType, bool Array = false, bool Cube = false>
static void ImageRead(DeviceState* deviceState, ReturnType* result, ImageDescriptor* descriptor, CoordinateType* coordinates)
{
	return ImageFetch<ReturnType, CoordinateType, Array, Cube>(deviceState, result, descriptor, coordinates);
}

static void ImageCombine(ImageDescriptor* image, ImageDescriptor* sampler, ImageDescriptor* result)
{
	assert(image->Data.Image != nullptr);
	assert(sampler->ImageSampler != nullptr);
	
	result->Type = image->Type;
	result->Data = image->Data;
	result->ImageSampler = sampler->ImageSampler;
}

static void ImageGetRaw(ImageDescriptor* sampledImage, ImageDescriptor* result)
{
	assert(sampledImage->Data.Image != nullptr);
	
	result->Type = sampledImage->Type;
	result->Data = sampledImage->Data;
	result->ImageSampler = nullptr;
}

void AddGlslFunctions(DeviceState* deviceState)
{
	auto jit = deviceState->jit;
	jit->setUserData(deviceState);
	
	// Round = 1,
	// RoundEven = 2,
	// Trunc = 3,
	
	// jit->AddFunction("@FAbs.F16", reinterpret_cast<FunctionPointer>(Abs<half>));
	// jit->AddFunction("@FAbs.F16[2]", reinterpret_cast<FunctionPointer>(VAbs<glm::f16vec2>));
	// jit->AddFunction("@FAbs.F16[3]", reinterpret_cast<FunctionPointer>(VAbs<glm::f16vec3>));
	// jit->AddFunction("@FAbs.F16[4]", reinterpret_cast<FunctionPointer>(VAbs<glm::f16vec4>));
	// jit->AddFunction("@FAbs.F32", reinterpret_cast<FunctionPointer>(Abs<float>));
	// jit->AddFunction("@FAbs.F32[2]", reinterpret_cast<FunctionPointer>(VAbs<glm::fvec2>));
	// jit->AddFunction("@FAbs.F32[3]", reinterpret_cast<FunctionPointer>(VAbs<glm::fvec3>));
	// jit->AddFunction("@FAbs.F32[4]", reinterpret_cast<FunctionPointer>(VAbs<glm::fvec4>));
	// jit->AddFunction("@FAbs.F64", reinterpret_cast<FunctionPointer>(Abs<double>));
	// jit->AddFunction("@FAbs.F64[2]", reinterpret_cast<FunctionPointer>(VAbs<glm::dvec2>));
	// jit->AddFunction("@FAbs.F64[3]", reinterpret_cast<FunctionPointer>(VAbs<glm::dvec3>));
	// jit->AddFunction("@FAbs.F64[4]", reinterpret_cast<FunctionPointer>(VAbs<glm::dvec4>));
	//
	// jit->AddFunction("@SAbs.I8", reinterpret_cast<FunctionPointer>(Abs<int8_t>));
	// jit->AddFunction("@SAbs.I8[2]", reinterpret_cast<FunctionPointer>(VAbs<glm::i8vec2>));
	// jit->AddFunction("@SAbs.I8[3]", reinterpret_cast<FunctionPointer>(VAbs<glm::i8vec3>));
	// jit->AddFunction("@SAbs.I8[4]", reinterpret_cast<FunctionPointer>(VAbs<glm::i8vec4>));
	// jit->AddFunction("@SAbs.I16", reinterpret_cast<FunctionPointer>(Abs<int16_t>));
	// jit->AddFunction("@SAbs.I16[2]", reinterpret_cast<FunctionPointer>(VAbs<glm::i16vec2>));
	// jit->AddFunction("@SAbs.I16[3]", reinterpret_cast<FunctionPointer>(VAbs<glm::i16vec3>));
	// jit->AddFunction("@SAbs.I16[4]", reinterpret_cast<FunctionPointer>(VAbs<glm::i16vec4>));
	// jit->AddFunction("@SAbs.I32", reinterpret_cast<FunctionPointer>(Abs<int32_t>));
	// jit->AddFunction("@SAbs.I32[2]", reinterpret_cast<FunctionPointer>(VAbs<glm::i32vec2>));
	// jit->AddFunction("@SAbs.I32[3]", reinterpret_cast<FunctionPointer>(VAbs<glm::i32vec3>));
	// jit->AddFunction("@SAbs.I32[4]", reinterpret_cast<FunctionPointer>(VAbs<glm::i32vec4>));
	// jit->AddFunction("@SAbs.I64", reinterpret_cast<FunctionPointer>(Abs<int64_t>));
	// jit->AddFunction("@SAbs.I64[2]", reinterpret_cast<FunctionPointer>(VAbs<glm::i64vec2>));
	// jit->AddFunction("@SAbs.I64[3]", reinterpret_cast<FunctionPointer>(VAbs<glm::i64vec3>));
	// jit->AddFunction("@SAbs.I64[4]", reinterpret_cast<FunctionPointer>(VAbs<glm::i64vec4>));
	//
	// // FSign = 6,
	//
	// jit->AddFunction("@SSign.I8", reinterpret_cast<FunctionPointer>(SSign<int8_t>));
	// jit->AddFunction("@SSign.I8[2]", reinterpret_cast<FunctionPointer>(VSSign<glm::i8vec2>));
	// jit->AddFunction("@SSign.I8[3]", reinterpret_cast<FunctionPointer>(VSSign<glm::i8vec3>));
	// jit->AddFunction("@SSign.I8[4]", reinterpret_cast<FunctionPointer>(VSSign<glm::i8vec4>));
	// jit->AddFunction("@SSign.I16", reinterpret_cast<FunctionPointer>(SSign<int16_t>));
	// jit->AddFunction("@SSign.I16[2]", reinterpret_cast<FunctionPointer>(VSSign<glm::i16vec2>));
	// jit->AddFunction("@SSign.I16[3]", reinterpret_cast<FunctionPointer>(VSSign<glm::i16vec3>));
	// jit->AddFunction("@SSign.I16[4]", reinterpret_cast<FunctionPointer>(VSSign<glm::i16vec4>));
	// jit->AddFunction("@SSign.I32", reinterpret_cast<FunctionPointer>(SSign<int32_t>));
	// jit->AddFunction("@SSign.I32[2]", reinterpret_cast<FunctionPointer>(VSSign<glm::i32vec2>));
	// jit->AddFunction("@SSign.I32[3]", reinterpret_cast<FunctionPointer>(VSSign<glm::i32vec3>));
	// jit->AddFunction("@SSign.I32[4]", reinterpret_cast<FunctionPointer>(VSSign<glm::i32vec4>));
	// jit->AddFunction("@SSign.I64", reinterpret_cast<FunctionPointer>(SSign<int64_t>));
	// jit->AddFunction("@SSign.I64[2]", reinterpret_cast<FunctionPointer>(VSSign<glm::i64vec2>));
	// jit->AddFunction("@SSign.I64[3]", reinterpret_cast<FunctionPointer>(VSSign<glm::i64vec3>));
	// jit->AddFunction("@SSign.I64[4]", reinterpret_cast<FunctionPointer>(VSSign<glm::i64vec4>));
	//
	// // SSign = 7,
	// // Floor = 8,
	// // Ceil = 9,
	// // Fract = 10,
	// // Radians = 11,
	// // Degrees = 12,
	//
	// jit->AddFunction("@Sin.F16", reinterpret_cast<FunctionPointer>(Sin<half>));
	// jit->AddFunction("@Sin.F16[2]", reinterpret_cast<FunctionPointer>(VSin<glm::f16vec2>));
	// jit->AddFunction("@Sin.F16[3]", reinterpret_cast<FunctionPointer>(VSin<glm::f16vec3>));
	// jit->AddFunction("@Sin.F16[4]", reinterpret_cast<FunctionPointer>(VSin<glm::f16vec4>));
	// jit->AddFunction("@Sin.F32", reinterpret_cast<FunctionPointer>(Sin<float>));
	// jit->AddFunction("@Sin.F32[2]", reinterpret_cast<FunctionPointer>(VSin<glm::f32vec2>));
	// jit->AddFunction("@Sin.F32[3]", reinterpret_cast<FunctionPointer>(VSin<glm::f32vec3>));
	// jit->AddFunction("@Sin.F32[4]", reinterpret_cast<FunctionPointer>(VSin<glm::f32vec4>));
	//
	// jit->AddFunction("@Cos.F16", reinterpret_cast<FunctionPointer>(Cos<half>));
	// jit->AddFunction("@Cos.F16[2]", reinterpret_cast<FunctionPointer>(VCos<glm::f16vec2>));
	// jit->AddFunction("@Cos.F16[3]", reinterpret_cast<FunctionPointer>(VCos<glm::f16vec3>));
	// jit->AddFunction("@Cos.F16[4]", reinterpret_cast<FunctionPointer>(VCos<glm::f16vec4>));
	// jit->AddFunction("@Cos.F32", reinterpret_cast<FunctionPointer>(Cos<float>));
	// jit->AddFunction("@Cos.F32[2]", reinterpret_cast<FunctionPointer>(VCos<glm::f32vec2>));
	// jit->AddFunction("@Cos.F32[3]", reinterpret_cast<FunctionPointer>(VCos<glm::f32vec3>));
	// jit->AddFunction("@Cos.F32[4]", reinterpret_cast<FunctionPointer>(VCos<glm::f32vec4>));
	//
	// // Tan = 15,
	// // Asin = 16,
	// // Acos = 17,
	// // Atan = 18,
	// // Sinh = 19,
	// // Cosh = 20,
	// // Tanh = 21,
	// // Asinh = 22,
	// // Acosh = 23,
	// // Atanh = 24,
	// // Atan2 = 25,
	//
	// jit->AddFunction("@Pow.F16.F16", reinterpret_cast<FunctionPointer>(Pow<half>));
	// jit->AddFunction("@Pow.F16[2].F16[2]", reinterpret_cast<FunctionPointer>(VPow<glm::f16vec2>));
	// jit->AddFunction("@Pow.F16[3].F16[3]", reinterpret_cast<FunctionPointer>(VPow<glm::f16vec3>));
	// jit->AddFunction("@Pow.F16[4].F16[4]", reinterpret_cast<FunctionPointer>(VPow<glm::f16vec4>));
	// jit->AddFunction("@Pow.F32.F32", reinterpret_cast<FunctionPointer>(Pow<float>));
	// jit->AddFunction("@Pow.F32[2].F32[2]", reinterpret_cast<FunctionPointer>(VPow<glm::f32vec2>));
	// jit->AddFunction("@Pow.F32[3].F32[3]", reinterpret_cast<FunctionPointer>(VPow<glm::f32vec3>));
	// jit->AddFunction("@Pow.F32[4].F32[4]", reinterpret_cast<FunctionPointer>(VPow<glm::f32vec4>));
	//
	// // Exp = 27,
	// // Log = 28,
	// // Exp2 = 29,
	// // Log2 = 30,
	// // Sqrt = 31,
	// // InverseSqrt = 32,
	// // Determinant = 33,
	// // MatrixInverse = 34,
	// // Modf = 35,
	// // ModfStruct = 36,
	//
	// jit->AddFunction("@FMin.F16.F16", reinterpret_cast<FunctionPointer>(Min<half>));
	// jit->AddFunction("@FMin.F16[2].F16[2]", reinterpret_cast<FunctionPointer>(VMin<glm::f16vec2>));
	// jit->AddFunction("@FMin.F16[3].F16[3]", reinterpret_cast<FunctionPointer>(VMin<glm::f16vec3>));
	// jit->AddFunction("@FMin.F16[4].F16[4]", reinterpret_cast<FunctionPointer>(VMin<glm::f16vec4>));
	// jit->AddFunction("@FMin.F32.F32", reinterpret_cast<FunctionPointer>(Min<float>));
	// jit->AddFunction("@FMin.F32[2].F32[2]", reinterpret_cast<FunctionPointer>(VMin<glm::f32vec2>));
	// jit->AddFunction("@FMin.F32[3].F32[3]", reinterpret_cast<FunctionPointer>(VMin<glm::f32vec3>));
	// jit->AddFunction("@FMin.F32[4].F32[4]", reinterpret_cast<FunctionPointer>(VMin<glm::f32vec4>));
	// jit->AddFunction("@FMin.F64.F64", reinterpret_cast<FunctionPointer>(Min<double>));
	// jit->AddFunction("@FMin.F64[2].F64[2]", reinterpret_cast<FunctionPointer>(VMin<glm::f64vec2>));
	// jit->AddFunction("@FMin.F64[3].F64[3]", reinterpret_cast<FunctionPointer>(VMin<glm::f64vec3>));
	// jit->AddFunction("@FMin.F64[4].F64[4]", reinterpret_cast<FunctionPointer>(VMin<glm::f64vec4>));
	//
	// jit->AddFunction("@UMin.U8.U8", reinterpret_cast<FunctionPointer>(Min<uint8_t>));
	// jit->AddFunction("@UMin.U8[2].U8[2]", reinterpret_cast<FunctionPointer>(VMin<glm::u8vec2>));
	// jit->AddFunction("@UMin.U8[3].U8[3]", reinterpret_cast<FunctionPointer>(VMin<glm::u8vec3>));
	// jit->AddFunction("@UMin.U8[4].U8[4]", reinterpret_cast<FunctionPointer>(VMin<glm::u8vec4>));
	// jit->AddFunction("@UMin.U16.U16", reinterpret_cast<FunctionPointer>(Min<uint16_t>));
	// jit->AddFunction("@UMin.U16[2].U16[2]", reinterpret_cast<FunctionPointer>(VMin<glm::u16vec2>));
	// jit->AddFunction("@UMin.U16[3].U16[3]", reinterpret_cast<FunctionPointer>(VMin<glm::u16vec3>));
	// jit->AddFunction("@UMin.U16[4].U16[4]", reinterpret_cast<FunctionPointer>(VMin<glm::u16vec4>));
	// jit->AddFunction("@UMin.U32.U32", reinterpret_cast<FunctionPointer>(Min<uint32_t>));
	// jit->AddFunction("@UMin.U32[2].U32[2]", reinterpret_cast<FunctionPointer>(VMin<glm::u32vec2>));
	// jit->AddFunction("@UMin.U32[3].U32[3]", reinterpret_cast<FunctionPointer>(VMin<glm::u32vec3>));
	// jit->AddFunction("@UMin.U32[4].U32[4]", reinterpret_cast<FunctionPointer>(VMin<glm::u32vec4>));
	// jit->AddFunction("@UMin.U64.U64", reinterpret_cast<FunctionPointer>(Min<uint64_t>));
	// jit->AddFunction("@UMin.U64[2].U64[2]", reinterpret_cast<FunctionPointer>(VMin<glm::u64vec2>));
	// jit->AddFunction("@UMin.U64[3].U64[3]", reinterpret_cast<FunctionPointer>(VMin<glm::u64vec3>));
	// jit->AddFunction("@UMin.U64[4].U64[4]", reinterpret_cast<FunctionPointer>(VMin<glm::u64vec4>));
	//
	// jit->AddFunction("@SMin.I8.I8", reinterpret_cast<FunctionPointer>(Min<int8_t>));
	// jit->AddFunction("@SMin.I8[2].I8[2]", reinterpret_cast<FunctionPointer>(VMin<glm::i8vec2>));
	// jit->AddFunction("@SMin.I8[3].I8[3]", reinterpret_cast<FunctionPointer>(VMin<glm::i8vec3>));
	// jit->AddFunction("@SMin.I8[4].I8[4]", reinterpret_cast<FunctionPointer>(VMin<glm::i8vec4>));
	// jit->AddFunction("@SMin.I16.I16", reinterpret_cast<FunctionPointer>(Min<int16_t>));
	// jit->AddFunction("@SMin.I16[2].I16[2]", reinterpret_cast<FunctionPointer>(VMin<glm::i16vec2>));
	// jit->AddFunction("@SMin.I16[3].I16[3]", reinterpret_cast<FunctionPointer>(VMin<glm::i16vec3>));
	// jit->AddFunction("@SMin.I16[4].I16[4]", reinterpret_cast<FunctionPointer>(VMin<glm::i16vec4>));
	// jit->AddFunction("@SMin.I32.I32", reinterpret_cast<FunctionPointer>(Min<int32_t>));
	// jit->AddFunction("@SMin.I32[2].I32[2]", reinterpret_cast<FunctionPointer>(VMin<glm::i32vec2>));
	// jit->AddFunction("@SMin.I32[3].I32[3]", reinterpret_cast<FunctionPointer>(VMin<glm::i32vec3>));
	// jit->AddFunction("@SMin.I32[4].I32[4]", reinterpret_cast<FunctionPointer>(VMin<glm::i32vec4>));
	// jit->AddFunction("@SMin.I64.I64", reinterpret_cast<FunctionPointer>(Min<int64_t>));
	// jit->AddFunction("@SMin.I64[2].I64[2]", reinterpret_cast<FunctionPointer>(VMin<glm::i64vec2>));
	// jit->AddFunction("@SMin.I64[3].I64[3]", reinterpret_cast<FunctionPointer>(VMin<glm::i64vec3>));
	// jit->AddFunction("@SMin.I64[4].I64[4]", reinterpret_cast<FunctionPointer>(VMin<glm::i64vec4>));
	//
	// jit->AddFunction("@FMax.F16.F16", reinterpret_cast<FunctionPointer>(Max<half>));
	// jit->AddFunction("@FMax.F16[2].F16[2]", reinterpret_cast<FunctionPointer>(VMax<glm::f16vec2>));
	// jit->AddFunction("@FMax.F16[3].F16[3]", reinterpret_cast<FunctionPointer>(VMax<glm::f16vec3>));
	// jit->AddFunction("@FMax.F16[4].F16[4]", reinterpret_cast<FunctionPointer>(VMax<glm::f16vec4>));
	// jit->AddFunction("@FMax.F32.F32", reinterpret_cast<FunctionPointer>(Max<float>));
	// jit->AddFunction("@FMax.F32[2].F32[2]", reinterpret_cast<FunctionPointer>(VMax<glm::f32vec2>));
	// jit->AddFunction("@FMax.F32[3].F32[3]", reinterpret_cast<FunctionPointer>(VMax<glm::f32vec3>));
	// jit->AddFunction("@FMax.F32[4].F32[4]", reinterpret_cast<FunctionPointer>(VMax<glm::f32vec4>));
	// jit->AddFunction("@FMax.F64.F64", reinterpret_cast<FunctionPointer>(Max<double>));
	// jit->AddFunction("@FMax.F64[2].F64[2]", reinterpret_cast<FunctionPointer>(VMax<glm::f64vec2>));
	// jit->AddFunction("@FMax.F64[3].F64[3]", reinterpret_cast<FunctionPointer>(VMax<glm::f64vec3>));
	// jit->AddFunction("@FMax.F64[4].F64[4]", reinterpret_cast<FunctionPointer>(VMax<glm::f64vec4>));
	//
	// jit->AddFunction("@UMax.U8.U8", reinterpret_cast<FunctionPointer>(Max<uint8_t>));
	// jit->AddFunction("@UMax.U8[2].U8[2]", reinterpret_cast<FunctionPointer>(VMax<glm::u8vec2>));
	// jit->AddFunction("@UMax.U8[3].U8[3]", reinterpret_cast<FunctionPointer>(VMax<glm::u8vec3>));
	// jit->AddFunction("@UMax.U8[4].U8[4]", reinterpret_cast<FunctionPointer>(VMax<glm::u8vec4>));
	// jit->AddFunction("@UMax.U16.U16", reinterpret_cast<FunctionPointer>(Max<uint16_t>));
	// jit->AddFunction("@UMax.U16[2].U16[2]", reinterpret_cast<FunctionPointer>(VMax<glm::u16vec2>));
	// jit->AddFunction("@UMax.U16[3].U16[3]", reinterpret_cast<FunctionPointer>(VMax<glm::u16vec3>));
	// jit->AddFunction("@UMax.U16[4].U16[4]", reinterpret_cast<FunctionPointer>(VMax<glm::u16vec4>));
	// jit->AddFunction("@UMax.U32.U32", reinterpret_cast<FunctionPointer>(Max<uint32_t>));
	// jit->AddFunction("@UMax.U32[2].U32[2]", reinterpret_cast<FunctionPointer>(VMax<glm::u32vec2>));
	// jit->AddFunction("@UMax.U32[3].U32[3]", reinterpret_cast<FunctionPointer>(VMax<glm::u32vec3>));
	// jit->AddFunction("@UMax.U32[4].U32[4]", reinterpret_cast<FunctionPointer>(VMax<glm::u32vec4>));
	// jit->AddFunction("@UMax.U64.U64", reinterpret_cast<FunctionPointer>(Max<uint64_t>));
	// jit->AddFunction("@UMax.U64[2].U64[2]", reinterpret_cast<FunctionPointer>(VMax<glm::u64vec2>));
	// jit->AddFunction("@UMax.U64[3].U64[3]", reinterpret_cast<FunctionPointer>(VMax<glm::u64vec3>));
	// jit->AddFunction("@UMax.U64[4].U64[4]", reinterpret_cast<FunctionPointer>(VMax<glm::u64vec4>));
	//
	// jit->AddFunction("@SMax.I8.I8", reinterpret_cast<FunctionPointer>(Max<int8_t>));
	// jit->AddFunction("@SMax.I8[2].I8[2]", reinterpret_cast<FunctionPointer>(VMax<glm::i8vec2>));
	// jit->AddFunction("@SMax.I8[3].I8[3]", reinterpret_cast<FunctionPointer>(VMax<glm::i8vec3>));
	// jit->AddFunction("@SMax.I8[4].I8[4]", reinterpret_cast<FunctionPointer>(VMax<glm::i8vec4>));
	// jit->AddFunction("@SMax.I16.I16", reinterpret_cast<FunctionPointer>(Max<int16_t>));
	// jit->AddFunction("@SMax.I16[2].I16[2]", reinterpret_cast<FunctionPointer>(VMax<glm::i16vec2>));
	// jit->AddFunction("@SMax.I16[3].I16[3]", reinterpret_cast<FunctionPointer>(VMax<glm::i16vec3>));
	// jit->AddFunction("@SMax.I16[4].I16[4]", reinterpret_cast<FunctionPointer>(VMax<glm::i16vec4>));
	// jit->AddFunction("@SMax.I32.I32", reinterpret_cast<FunctionPointer>(Max<int32_t>));
	// jit->AddFunction("@SMax.I32[2].I32[2]", reinterpret_cast<FunctionPointer>(VMax<glm::i32vec2>));
	// jit->AddFunction("@SMax.I32[3].I32[3]", reinterpret_cast<FunctionPointer>(VMax<glm::i32vec3>));
	// jit->AddFunction("@SMax.I32[4].I32[4]", reinterpret_cast<FunctionPointer>(VMax<glm::i32vec4>));
	// jit->AddFunction("@SMax.I64.I64", reinterpret_cast<FunctionPointer>(Max<int64_t>));
	// jit->AddFunction("@SMax.I64[2].I64[2]", reinterpret_cast<FunctionPointer>(VMax<glm::i64vec2>));
	// jit->AddFunction("@SMax.I64[3].I64[3]", reinterpret_cast<FunctionPointer>(VMax<glm::i64vec3>));
	// jit->AddFunction("@SMax.I64[4].I64[4]", reinterpret_cast<FunctionPointer>(VMax<glm::i64vec4>));
	//
	// jit->AddFunction("@FClamp.F16.F16.F16", reinterpret_cast<FunctionPointer>(Clamp<half>));
	// jit->AddFunction("@FClamp.F16[2].F16[2].F16[2]", reinterpret_cast<FunctionPointer>(VClamp<glm::f16vec2>));
	// jit->AddFunction("@FClamp.F16[3].F16[3].F16[3]", reinterpret_cast<FunctionPointer>(VClamp<glm::f16vec3>));
	// jit->AddFunction("@FClamp.F16[4].F16[4].F16[4]", reinterpret_cast<FunctionPointer>(VClamp<glm::f16vec4>));
	// jit->AddFunction("@FClamp.F32.F32.F32", reinterpret_cast<FunctionPointer>(Clamp<float>));
	// jit->AddFunction("@FClamp.F32[2].F32[2].F32[2]", reinterpret_cast<FunctionPointer>(VClamp<glm::f32vec2>));
	// jit->AddFunction("@FClamp.F32[3].F32[3].F32[3]", reinterpret_cast<FunctionPointer>(VClamp<glm::f32vec3>));
	// jit->AddFunction("@FClamp.F32[4].F32[4].F32[4]", reinterpret_cast<FunctionPointer>(VClamp<glm::f32vec4>));
	// jit->AddFunction("@FClamp.F64.F64.F64", reinterpret_cast<FunctionPointer>(Clamp<double>));
	// jit->AddFunction("@FClamp.F64[2].F64[2].F64[2]", reinterpret_cast<FunctionPointer>(VClamp<glm::f64vec2>));
	// jit->AddFunction("@FClamp.F64[3].F64[3].F64[3]", reinterpret_cast<FunctionPointer>(VClamp<glm::f64vec3>));
	// jit->AddFunction("@FClamp.F64[4].F64[4].F64[4]", reinterpret_cast<FunctionPointer>(VClamp<glm::f64vec4>));
	//
	// jit->AddFunction("@UClamp.U8.U8.U8", reinterpret_cast<FunctionPointer>(Clamp<uint8_t>));
	// jit->AddFunction("@UClamp.U8[2].U8[2].U8[2]", reinterpret_cast<FunctionPointer>(VClamp<glm::u8vec2>));
	// jit->AddFunction("@UClamp.U8[3].U8[3].U8[3]", reinterpret_cast<FunctionPointer>(VClamp<glm::u8vec3>));
	// jit->AddFunction("@UClamp.U8[4].U8[4].U8[4]", reinterpret_cast<FunctionPointer>(VClamp<glm::u8vec4>));
	// jit->AddFunction("@UClamp.U16.U16.U16", reinterpret_cast<FunctionPointer>(Clamp<uint16_t>));
	// jit->AddFunction("@UClamp.U16[2].U16[2].U16[2]", reinterpret_cast<FunctionPointer>(VClamp<glm::u16vec2>));
	// jit->AddFunction("@UClamp.U16[3].U16[3].U16[3]", reinterpret_cast<FunctionPointer>(VClamp<glm::u16vec3>));
	// jit->AddFunction("@UClamp.U16[4].U16[4].U16[4]", reinterpret_cast<FunctionPointer>(VClamp<glm::u16vec4>));
	// jit->AddFunction("@UClamp.U32.U32.U32", reinterpret_cast<FunctionPointer>(Clamp<uint32_t>));
	// jit->AddFunction("@UClamp.U32[2].U32[2].U32[2]", reinterpret_cast<FunctionPointer>(VClamp<glm::u32vec2>));
	// jit->AddFunction("@UClamp.U32[3].U32[3].U32[3]", reinterpret_cast<FunctionPointer>(VClamp<glm::u32vec3>));
	// jit->AddFunction("@UClamp.U32[4].U32[4].U32[4]", reinterpret_cast<FunctionPointer>(VClamp<glm::u32vec4>));
	// jit->AddFunction("@UClamp.U64.U64.U64", reinterpret_cast<FunctionPointer>(Clamp<uint64_t>));
	// jit->AddFunction("@UClamp.U64[2].U64[2].U64[2]", reinterpret_cast<FunctionPointer>(VClamp<glm::u64vec2>));
	// jit->AddFunction("@UClamp.U64[3].U64[3].U64[3]", reinterpret_cast<FunctionPointer>(VClamp<glm::u64vec3>));
	// jit->AddFunction("@UClamp.U64[4].U64[4].U64[4]", reinterpret_cast<FunctionPointer>(VClamp<glm::u64vec4>));
	//
	// jit->AddFunction("@SClamp.I8.I8.I8", reinterpret_cast<FunctionPointer>(Clamp<int8_t>));
	// jit->AddFunction("@SClamp.I8[2].I8[2].I8[2]", reinterpret_cast<FunctionPointer>(VClamp<glm::i8vec2>));
	// jit->AddFunction("@SClamp.I8[3].I8[3].I8[3]", reinterpret_cast<FunctionPointer>(VClamp<glm::i8vec3>));
	// jit->AddFunction("@SClamp.I8[4].I8[4].I8[4]", reinterpret_cast<FunctionPointer>(VClamp<glm::i8vec4>));
	// jit->AddFunction("@SClamp.I16.I16.I16", reinterpret_cast<FunctionPointer>(Clamp<int16_t>));
	// jit->AddFunction("@SClamp.I16[2].I16[2].I16[2]", reinterpret_cast<FunctionPointer>(VClamp<glm::i16vec2>));
	// jit->AddFunction("@SClamp.I16[3].I16[3].I16[3]", reinterpret_cast<FunctionPointer>(VClamp<glm::i16vec3>));
	// jit->AddFunction("@SClamp.I16[4].I16[4].I16[4]", reinterpret_cast<FunctionPointer>(VClamp<glm::i16vec4>));
	// jit->AddFunction("@SClamp.I32.I32.I32", reinterpret_cast<FunctionPointer>(Clamp<int32_t>));
	// jit->AddFunction("@SClamp.I32[2].I32[2].I32[2]", reinterpret_cast<FunctionPointer>(VClamp<glm::i32vec2>));
	// jit->AddFunction("@SClamp.I32[3].I32[3].I32[3]", reinterpret_cast<FunctionPointer>(VClamp<glm::i32vec3>));
	// jit->AddFunction("@SClamp.I32[4].I32[4].I32[4]", reinterpret_cast<FunctionPointer>(VClamp<glm::i32vec4>));
	// jit->AddFunction("@SClamp.I64.I64.I64", reinterpret_cast<FunctionPointer>(Clamp<int64_t>));
	// jit->AddFunction("@SClamp.I64[2].I64[2].I64[2]", reinterpret_cast<FunctionPointer>(VClamp<glm::i64vec2>));
	// jit->AddFunction("@SClamp.I64[3].I64[3].I64[3]", reinterpret_cast<FunctionPointer>(VClamp<glm::i64vec3>));
	// jit->AddFunction("@SClamp.I64[4].I64[4].I64[4]", reinterpret_cast<FunctionPointer>(VClamp<glm::i64vec4>));
	//
	// jit->AddFunction("@FMix.F16.F16.F16", reinterpret_cast<FunctionPointer>(Mix<half>));
	// jit->AddFunction("@FMix.F16[2].F16[2].F16[2]", reinterpret_cast<FunctionPointer>(VMix<glm::f16vec2>));
	// jit->AddFunction("@FMix.F16[3].F16[3].F16[3]", reinterpret_cast<FunctionPointer>(VMix<glm::f16vec3>));
	// jit->AddFunction("@FMix.F16[4].F16[4].F16[4]", reinterpret_cast<FunctionPointer>(VMix<glm::f16vec4>));
	// jit->AddFunction("@FMix.F32.F32.F32", reinterpret_cast<FunctionPointer>(Mix<float>));
	// jit->AddFunction("@FMix.F32[2].F32[2].F32[2]", reinterpret_cast<FunctionPointer>(VMix<glm::f32vec2>));
	// jit->AddFunction("@FMix.F32[3].F32[3].F32[3]", reinterpret_cast<FunctionPointer>(VMix<glm::f32vec3>));
	// jit->AddFunction("@FMix.F32[4].F32[4].F32[4]", reinterpret_cast<FunctionPointer>(VMix<glm::f32vec4>));
	// jit->AddFunction("@FMix.F64.F64.F64", reinterpret_cast<FunctionPointer>(Mix<double>));
	// jit->AddFunction("@FMix.F64[2].F64[2].F64[2]", reinterpret_cast<FunctionPointer>(VMix<glm::f64vec2>));
	// jit->AddFunction("@FMix.F64[3].F64[3].F64[3]", reinterpret_cast<FunctionPointer>(VMix<glm::f64vec3>));
	// jit->AddFunction("@FMix.F64[4].F64[4].F64[4]", reinterpret_cast<FunctionPointer>(VMix<glm::f64vec4>));
	//
	// // Step = 48,
	// // SmoothStep = 49,
	// // Fma = 50,
	// // Frexp = 51,
	// // FrexpStruct = 52,
	// // Ldexp = 53,
	// // PackSnorm4x8 = 54,
	// // PackUnorm4x8 = 55,
	// // PackSnorm2x16 = 56,
	// // PackUnorm2x16 = 57,
	// // PackHalf2x16 = 58,
	// // PackDouble2x32 = 59,
	// // UnpackSnorm2x16 = 60,
	// // UnpackUnorm2x16 = 61,
	// // UnpackHalf2x16 = 62,
	// // UnpackSnorm4x8 = 63,
	// // UnpackUnorm4x8 = 64,
	// // UnpackDouble2x32 = 65,
	// // Length = 66,
	// // Distance = 67,
	// // Cross = 68,
	//
	// jit->AddFunction("@Normalise.F16", reinterpret_cast<FunctionPointer>(Normalise<half>));
	// jit->AddFunction("@Normalise.F16[2]", reinterpret_cast<FunctionPointer>(VNormalise<glm::f16vec2>));
	// jit->AddFunction("@Normalise.F16[3]", reinterpret_cast<FunctionPointer>(VNormalise<glm::f16vec3>));
	// jit->AddFunction("@Normalise.F16[4]", reinterpret_cast<FunctionPointer>(VNormalise<glm::f16vec4>));
	// jit->AddFunction("@Normalise.F32", reinterpret_cast<FunctionPointer>(Normalise<float>));
	// jit->AddFunction("@Normalise.F32[2]", reinterpret_cast<FunctionPointer>(VNormalise<glm::f32vec2>));
	// jit->AddFunction("@Normalise.F32[3]", reinterpret_cast<FunctionPointer>(VNormalise<glm::f32vec3>));
	// jit->AddFunction("@Normalise.F32[4]", reinterpret_cast<FunctionPointer>(VNormalise<glm::f32vec4>));
	// jit->AddFunction("@Normalise.F64", reinterpret_cast<FunctionPointer>(Normalise<double>));
	// jit->AddFunction("@Normalise.F64[2]", reinterpret_cast<FunctionPointer>(VNormalise<glm::f64vec2>));
	// jit->AddFunction("@Normalise.F64[3]", reinterpret_cast<FunctionPointer>(VNormalise<glm::f64vec3>));
	// jit->AddFunction("@Normalise.F64[4]", reinterpret_cast<FunctionPointer>(VNormalise<glm::f64vec4>));
	//
	// // FaceForward = 70,
	//
	// jit->AddFunction("@Reflect.F16.F16", reinterpret_cast<FunctionPointer>(Reflect<half>));
	// jit->AddFunction("@Reflect.F16[2].F16[2]", reinterpret_cast<FunctionPointer>(Reflect<glm::f16vec2>));
	// jit->AddFunction("@Reflect.F16[3].F16[3]", reinterpret_cast<FunctionPointer>(Reflect<glm::f16vec3>));
	// jit->AddFunction("@Reflect.F16[4].F16[4]", reinterpret_cast<FunctionPointer>(Reflect<glm::f16vec4>));
	// jit->AddFunction("@Reflect.F32.F32", reinterpret_cast<FunctionPointer>(Reflect<float>));
	// jit->AddFunction("@Reflect.F32[2].F32[2]", reinterpret_cast<FunctionPointer>(Reflect<glm::f32vec2>));
	// jit->AddFunction("@Reflect.F32[3].F32[3]", reinterpret_cast<FunctionPointer>(Reflect<glm::f32vec3>));
	// jit->AddFunction("@Reflect.F32[4].F32[4]", reinterpret_cast<FunctionPointer>(Reflect<glm::f32vec4>));
	// jit->AddFunction("@Reflect.F64.F64", reinterpret_cast<FunctionPointer>(Reflect<double>));
	// jit->AddFunction("@Reflect.F64[2].F64[2]", reinterpret_cast<FunctionPointer>(Reflect<glm::f64vec2>));
	// jit->AddFunction("@Reflect.F64[3].F64[3]", reinterpret_cast<FunctionPointer>(Reflect<glm::f64vec3>));
	// jit->AddFunction("@Reflect.F64[4].F64[4]", reinterpret_cast<FunctionPointer>(Reflect<glm::f64vec4>));
	//
	// // Refract = 72,
	//
	// jit->AddFunction("@FindILsb.I32", reinterpret_cast<FunctionPointer>(FindILsb<int32_t>));
	// jit->AddFunction("@FindILsb.I32[2]", reinterpret_cast<FunctionPointer>(VFindILsb<glm::i32vec2>));
	// jit->AddFunction("@FindILsb.I32[3]", reinterpret_cast<FunctionPointer>(VFindILsb<glm::i32vec3>));
	// jit->AddFunction("@FindILsb.I32[4]", reinterpret_cast<FunctionPointer>(VFindILsb<glm::i32vec4>));
	// jit->AddFunction("@FindILsb.U32", reinterpret_cast<FunctionPointer>(FindILsb<uint32_t>));
	// jit->AddFunction("@FindILsb.U32[2]", reinterpret_cast<FunctionPointer>(VFindILsb<glm::u32vec2>));
	// jit->AddFunction("@FindILsb.U32[3]", reinterpret_cast<FunctionPointer>(VFindILsb<glm::u32vec3>));
	// jit->AddFunction("@FindILsb.U32[4]", reinterpret_cast<FunctionPointer>(VFindILsb<glm::u32vec4>));
	//
	// jit->AddFunction("@FindSMsb.I32", reinterpret_cast<FunctionPointer>(FindSMsb<int32_t>));
	// jit->AddFunction("@FindSMsb.I32[2]", reinterpret_cast<FunctionPointer>(VFindSMsb<glm::i32vec2>));
	// jit->AddFunction("@FindSMsb.I32[3]", reinterpret_cast<FunctionPointer>(VFindSMsb<glm::i32vec3>));
	// jit->AddFunction("@FindSMsb.I32[4]", reinterpret_cast<FunctionPointer>(VFindSMsb<glm::i32vec4>));
	//
	// jit->AddFunction("@FindUMsb.U32", reinterpret_cast<FunctionPointer>(FindUMsb<uint32_t>));
	// jit->AddFunction("@FindUMsb.U32[2]", reinterpret_cast<FunctionPointer>(VFindUMsb<glm::u32vec2>));
	// jit->AddFunction("@FindUMsb.U32[3]", reinterpret_cast<FunctionPointer>(VFindUMsb<glm::u32vec3>));
	// jit->AddFunction("@FindUMsb.U32[4]", reinterpret_cast<FunctionPointer>(VFindUMsb<glm::u32vec4>));
	//
	// // InterpolateAtCentroid = 76,
	// // InterpolateAtSample = 77,
	// // InterpolateAtOffset = 78,
	//
	// jit->AddFunction("@NMin.F16.F16", reinterpret_cast<FunctionPointer>(NMin<half>));
	// jit->AddFunction("@NMin.F16[2].F16[2]", reinterpret_cast<FunctionPointer>(VNMin<glm::f16vec2>));
	// jit->AddFunction("@NMin.F16[3].F16[3]", reinterpret_cast<FunctionPointer>(VNMin<glm::f16vec3>));
	// jit->AddFunction("@NMin.F16[4].F16[4]", reinterpret_cast<FunctionPointer>(VNMin<glm::f16vec4>));
	// jit->AddFunction("@NMin.F32.F32", reinterpret_cast<FunctionPointer>(NMin<float>));
	// jit->AddFunction("@NMin.F32[2].F32[2]", reinterpret_cast<FunctionPointer>(VNMin<glm::f32vec2>));
	// jit->AddFunction("@NMin.F32[3].F32[3]", reinterpret_cast<FunctionPointer>(VNMin<glm::f32vec3>));
	// jit->AddFunction("@NMin.F32[4].F32[4]", reinterpret_cast<FunctionPointer>(VNMin<glm::f32vec4>));
	// jit->AddFunction("@NMin.F64.F64", reinterpret_cast<FunctionPointer>(NMin<double>));
	// jit->AddFunction("@NMin.F64[2].F64[2]", reinterpret_cast<FunctionPointer>(VNMin<glm::f64vec2>));
	// jit->AddFunction("@NMin.F64[3].F64[3]", reinterpret_cast<FunctionPointer>(VNMin<glm::f64vec3>));
	// jit->AddFunction("@NMin.F64[4].F64[4]", reinterpret_cast<FunctionPointer>(VNMin<glm::f64vec4>));
	//
	// jit->AddFunction("@NMax.F16.F16", reinterpret_cast<FunctionPointer>(NMax<half>));
	// jit->AddFunction("@NMax.F16[2].F16[2]", reinterpret_cast<FunctionPointer>(VNMax<glm::f16vec2>));
	// jit->AddFunction("@NMax.F16[3].F16[3]", reinterpret_cast<FunctionPointer>(VNMax<glm::f16vec3>));
	// jit->AddFunction("@NMax.F16[4].F16[4]", reinterpret_cast<FunctionPointer>(VNMax<glm::f16vec4>));
	// jit->AddFunction("@NMax.F32.F32", reinterpret_cast<FunctionPointer>(NMax<float>));
	// jit->AddFunction("@NMax.F32[2].F32[2]", reinterpret_cast<FunctionPointer>(VNMax<glm::f32vec2>));
	// jit->AddFunction("@NMax.F32[3].F32[3]", reinterpret_cast<FunctionPointer>(VNMax<glm::f32vec3>));
	// jit->AddFunction("@NMax.F32[4].F32[4]", reinterpret_cast<FunctionPointer>(VNMax<glm::f32vec4>));
	// jit->AddFunction("@NMax.F64.F64", reinterpret_cast<FunctionPointer>(NMax<double>));
	// jit->AddFunction("@NMax.F64[2].F64[2]", reinterpret_cast<FunctionPointer>(VNMax<glm::f64vec2>));
	// jit->AddFunction("@NMax.F64[3].F64[3]", reinterpret_cast<FunctionPointer>(VNMax<glm::f64vec3>));
	// jit->AddFunction("@NMax.F64[4].F64[4]", reinterpret_cast<FunctionPointer>(VNMax<glm::f64vec4>));
	//
	// jit->AddFunction("@NClamp.F16.F16.F16", reinterpret_cast<FunctionPointer>(NClamp<half>));
	// jit->AddFunction("@NClamp.F16[2].F16[2].F16[2]", reinterpret_cast<FunctionPointer>(VNClamp<glm::f16vec2>));
	// jit->AddFunction("@NClamp.F16[3].F16[3].F16[3]", reinterpret_cast<FunctionPointer>(VNClamp<glm::f16vec3>));
	// jit->AddFunction("@NClamp.F16[4].F16[4].F16[4]", reinterpret_cast<FunctionPointer>(VNClamp<glm::f16vec4>));
	// jit->AddFunction("@NClamp.F32.F32.F32", reinterpret_cast<FunctionPointer>(NClamp<float>));
	// jit->AddFunction("@NClamp.F32[2].F32[2].F32[2]", reinterpret_cast<FunctionPointer>(VNClamp<glm::f32vec2>));
	// jit->AddFunction("@NClamp.F32[3].F32[3].F32[3]", reinterpret_cast<FunctionPointer>(VNClamp<glm::f32vec3>));
	// jit->AddFunction("@NClamp.F32[4].F32[4].F32[4]", reinterpret_cast<FunctionPointer>(VNClamp<glm::f32vec4>));
	// jit->AddFunction("@NClamp.F64.F64.F64", reinterpret_cast<FunctionPointer>(NClamp<double>));
	// jit->AddFunction("@NClamp.F64[2].F64[2].F64[2]", reinterpret_cast<FunctionPointer>(VNClamp<glm::f64vec2>));
	// jit->AddFunction("@NClamp.F64[3].F64[3].F64[3]", reinterpret_cast<FunctionPointer>(VNClamp<glm::f64vec3>));
	// jit->AddFunction("@NClamp.F64[4].F64[4].F64[4]", reinterpret_cast<FunctionPointer>(VNClamp<glm::f64vec4>));
	//
	// jit->AddFunction("@ImageCombine", reinterpret_cast<FunctionPointer>(ImageCombine));
	// jit->AddFunction("@ImageGetRaw", reinterpret_cast<FunctionPointer>(ImageGetRaw));
	//
	// jit->AddFunction("@Image.Sample.Implicit.F32[4].F32", reinterpret_cast<FunctionPointer>(ImageSampleImplicitLod<glm::fvec4, glm::fvec1>));
	// jit->AddFunction("@Image.Sample.Implicit.F32[4].F32[2]", reinterpret_cast<FunctionPointer>(ImageSampleImplicitLod<glm::fvec4, glm::fvec2>));
	// jit->AddFunction("@Image.Sample.Implicit.F32[4].F32[3]", reinterpret_cast<FunctionPointer>(ImageSampleImplicitLod<glm::fvec4, glm::fvec3>));
	// jit->AddFunction("@Image.Sample.Implicit.I32[4].F32", reinterpret_cast<FunctionPointer>(ImageSampleImplicitLod<glm::ivec4, glm::fvec1>));
	// jit->AddFunction("@Image.Sample.Implicit.I32[4].F32[2]", reinterpret_cast<FunctionPointer>(ImageSampleImplicitLod<glm::ivec4, glm::fvec2>));
	// jit->AddFunction("@Image.Sample.Implicit.I32[4].F32[3]", reinterpret_cast<FunctionPointer>(ImageSampleImplicitLod<glm::ivec4, glm::fvec3>));
	// jit->AddFunction("@Image.Sample.Implicit.U32[4].F32", reinterpret_cast<FunctionPointer>(ImageSampleImplicitLod<glm::uvec4, glm::fvec1>));
	// jit->AddFunction("@Image.Sample.Implicit.U32[4].F32[2]", reinterpret_cast<FunctionPointer>(ImageSampleImplicitLod<glm::uvec4, glm::fvec2>));
	// jit->AddFunction("@Image.Sample.Implicit.U32[4].F32[3]", reinterpret_cast<FunctionPointer>(ImageSampleImplicitLod<glm::uvec4, glm::fvec3>));
	//
	// jit->AddFunction("@Image.Sample.Implicit.F32[4].F32[2].Array", reinterpret_cast<FunctionPointer>(ImageSampleImplicitLod<glm::fvec4, glm::fvec2, true>));
	// jit->AddFunction("@Image.Sample.Implicit.F32[4].F32[3].Array", reinterpret_cast<FunctionPointer>(ImageSampleImplicitLod<glm::fvec4, glm::fvec3, true>));
	// jit->AddFunction("@Image.Sample.Implicit.F32[4].F32[4].Array", reinterpret_cast<FunctionPointer>(ImageSampleImplicitLod<glm::fvec4, glm::fvec4, true>));
	// jit->AddFunction("@Image.Sample.Implicit.I32[4].F32[2].Array", reinterpret_cast<FunctionPointer>(ImageSampleImplicitLod<glm::ivec4, glm::fvec2, true>));
	// jit->AddFunction("@Image.Sample.Implicit.I32[4].F32[3].Array", reinterpret_cast<FunctionPointer>(ImageSampleImplicitLod<glm::ivec4, glm::fvec3, true>));
	// jit->AddFunction("@Image.Sample.Implicit.I32[4].F32[4].Array", reinterpret_cast<FunctionPointer>(ImageSampleImplicitLod<glm::ivec4, glm::fvec4, true>));
	// jit->AddFunction("@Image.Sample.Implicit.U32[4].F32[2].Array", reinterpret_cast<FunctionPointer>(ImageSampleImplicitLod<glm::uvec4, glm::fvec2, true>));
	// jit->AddFunction("@Image.Sample.Implicit.U32[4].F32[3].Array", reinterpret_cast<FunctionPointer>(ImageSampleImplicitLod<glm::uvec4, glm::fvec3, true>));
	// jit->AddFunction("@Image.Sample.Implicit.U32[4].F32[4].Array", reinterpret_cast<FunctionPointer>(ImageSampleImplicitLod<glm::uvec4, glm::fvec4, true>));
	//
	// jit->AddFunction("@Image.Sample.Implicit.F32[4].F32[3].Cube", reinterpret_cast<FunctionPointer>(ImageSampleImplicitLod<glm::fvec4, glm::fvec3, false, true>));
	// jit->AddFunction("@Image.Sample.Implicit.I32[4].F32[3].Cube", reinterpret_cast<FunctionPointer>(ImageSampleImplicitLod<glm::ivec4, glm::fvec3, false, true>));
	// jit->AddFunction("@Image.Sample.Implicit.U32[4].F32[3].Cube", reinterpret_cast<FunctionPointer>(ImageSampleImplicitLod<glm::uvec4, glm::fvec3, false, true>));
	//
	// jit->AddFunction("@Image.Sample.Implicit.F32[4].F32[4].Cube.Array", reinterpret_cast<FunctionPointer>(ImageSampleImplicitLod<glm::fvec4, glm::fvec4, true, true>));
	// jit->AddFunction("@Image.Sample.Implicit.I32[4].F32[4].Cube.Array", reinterpret_cast<FunctionPointer>(ImageSampleImplicitLod<glm::ivec4, glm::fvec4, true, true>));
	// jit->AddFunction("@Image.Sample.Implicit.U32[4].F32[4].Cube.Array", reinterpret_cast<FunctionPointer>(ImageSampleImplicitLod<glm::uvec4, glm::fvec4, true, true>));
	//
	// jit->AddFunction("@Image.Sample.Explicit.F32[4].F32.Lod", reinterpret_cast<FunctionPointer>(ImageSampleExplicitLod<glm::fvec4, glm::fvec1>));
	jit->AddFunction("@Image.Sample.Explicit.F32[4].SampledImage[F32,2D].F32[2].Lod.F32", reinterpret_cast<FunctionPointer>(ImageSampleExplicitLod<glm::fvec4, glm::fvec2>));
	// jit->AddFunction("@Image.Sample.Explicit.F32[4].F32[3].Lod", reinterpret_cast<FunctionPointer>(ImageSampleExplicitLod<glm::fvec4, glm::fvec3>));
	// jit->AddFunction("@Image.Sample.Explicit.I32[4].F32.Lod", reinterpret_cast<FunctionPointer>(ImageSampleExplicitLod<glm::ivec4, glm::fvec1>));
	// jit->AddFunction("@Image.Sample.Explicit.I32[4].F32[2].Lod", reinterpret_cast<FunctionPointer>(ImageSampleExplicitLod<glm::ivec4, glm::fvec2>));
	// jit->AddFunction("@Image.Sample.Explicit.I32[4].F32[3].Lod", reinterpret_cast<FunctionPointer>(ImageSampleExplicitLod<glm::ivec4, glm::fvec3>));
	// jit->AddFunction("@Image.Sample.Explicit.U32[4].F32.Lod", reinterpret_cast<FunctionPointer>(ImageSampleExplicitLod<glm::uvec4, glm::fvec1>));
	// jit->AddFunction("@Image.Sample.Explicit.U32[4].F32[2].Lod", reinterpret_cast<FunctionPointer>(ImageSampleExplicitLod<glm::uvec4, glm::fvec2>));
	// jit->AddFunction("@Image.Sample.Explicit.U32[4].F32[3].Lod", reinterpret_cast<FunctionPointer>(ImageSampleExplicitLod<glm::uvec4, glm::fvec3>));
	//
	// jit->AddFunction("@Image.Sample.Explicit.F32[4].F32[2].Lod.Array", reinterpret_cast<FunctionPointer>(ImageSampleExplicitLod<glm::fvec4, glm::fvec2, true>));
	// jit->AddFunction("@Image.Sample.Explicit.F32[4].F32[3].Lod.Array", reinterpret_cast<FunctionPointer>(ImageSampleExplicitLod<glm::fvec4, glm::fvec3, true>));
	// jit->AddFunction("@Image.Sample.Explicit.F32[4].F32[4].Lod.Array", reinterpret_cast<FunctionPointer>(ImageSampleExplicitLod<glm::fvec4, glm::fvec4, true>));
	// jit->AddFunction("@Image.Sample.Explicit.I32[4].F32[2].Lod.Array", reinterpret_cast<FunctionPointer>(ImageSampleExplicitLod<glm::ivec4, glm::fvec2, true>));
	// jit->AddFunction("@Image.Sample.Explicit.I32[4].F32[3].Lod.Array", reinterpret_cast<FunctionPointer>(ImageSampleExplicitLod<glm::ivec4, glm::fvec3, true>));
	// jit->AddFunction("@Image.Sample.Explicit.I32[4].F32[4].Lod.Array", reinterpret_cast<FunctionPointer>(ImageSampleExplicitLod<glm::ivec4, glm::fvec4, true>));
	// jit->AddFunction("@Image.Sample.Explicit.U32[4].F32[2].Lod.Array", reinterpret_cast<FunctionPointer>(ImageSampleExplicitLod<glm::uvec4, glm::fvec2, true>));
	// jit->AddFunction("@Image.Sample.Explicit.U32[4].F32[3].Lod.Array", reinterpret_cast<FunctionPointer>(ImageSampleExplicitLod<glm::uvec4, glm::fvec3, true>));
	// jit->AddFunction("@Image.Sample.Explicit.U32[4].F32[4].Lod.Array", reinterpret_cast<FunctionPointer>(ImageSampleExplicitLod<glm::uvec4, glm::fvec4, true>));
	//
	// jit->AddFunction("@Image.Sample.Explicit.F32[4].F32[3].Lod.Cube", reinterpret_cast<FunctionPointer>(ImageSampleExplicitLod<glm::fvec4, glm::fvec3, false, true>));
	// jit->AddFunction("@Image.Sample.Explicit.I32[4].F32[3].Lod.Cube", reinterpret_cast<FunctionPointer>(ImageSampleExplicitLod<glm::ivec4, glm::fvec3, false, true>));
	// jit->AddFunction("@Image.Sample.Explicit.U32[4].F32[3].Lod.Cube", reinterpret_cast<FunctionPointer>(ImageSampleExplicitLod<glm::uvec4, glm::fvec3, false, true>));
	//
	// jit->AddFunction("@Image.Sample.Explicit.F32[4].F32[4].Lod.Cube.Array", reinterpret_cast<FunctionPointer>(ImageSampleExplicitLod<glm::fvec4, glm::fvec4, true, true>));
	// jit->AddFunction("@Image.Sample.Explicit.I32[4].F32[4].Lod.Cube.Array", reinterpret_cast<FunctionPointer>(ImageSampleExplicitLod<glm::ivec4, glm::fvec4, true, true>));
	// jit->AddFunction("@Image.Sample.Explicit.U32[4].F32[4].Lod.Cube.Array", reinterpret_cast<FunctionPointer>(ImageSampleExplicitLod<glm::uvec4, glm::fvec4, true, true>));
	//
	// jit->AddFunction("@Image.Fetch.F32[4].I32", reinterpret_cast<FunctionPointer>(ImageFetch<glm::fvec4, glm::ivec1>));
	// jit->AddFunction("@Image.Fetch.I32[4].I32", reinterpret_cast<FunctionPointer>(ImageFetch<glm::ivec4, glm::ivec1>));
	// jit->AddFunction("@Image.Fetch.U32[4].I32", reinterpret_cast<FunctionPointer>(ImageFetch<glm::uvec4, glm::ivec1>));
	//
	// jit->AddFunction("@Image.Fetch.F32[4].U32", reinterpret_cast<FunctionPointer>(ImageFetch<glm::fvec4, glm::ivec1>));
	// jit->AddFunction("@Image.Fetch.I32[4].U32", reinterpret_cast<FunctionPointer>(ImageFetch<glm::ivec4, glm::ivec1>));
	// jit->AddFunction("@Image.Fetch.U32[4].U32", reinterpret_cast<FunctionPointer>(ImageFetch<glm::uvec4, glm::ivec1>));
	//
	// jit->AddFunction("@Image.Fetch.F32[4].I32[2]", reinterpret_cast<FunctionPointer>(ImageFetch<glm::fvec4, glm::ivec2>));
	// jit->AddFunction("@Image.Fetch.I32[4].I32[2]", reinterpret_cast<FunctionPointer>(ImageFetch<glm::ivec4, glm::ivec2>));
	// jit->AddFunction("@Image.Fetch.U32[4].I32[2]", reinterpret_cast<FunctionPointer>(ImageFetch<glm::uvec4, glm::ivec2>));
	//
	// jit->AddFunction("@Image.Fetch.F32[4].U32[2]", reinterpret_cast<FunctionPointer>(ImageFetch<glm::fvec4, glm::ivec2>));
	// jit->AddFunction("@Image.Fetch.I32[4].U32[2]", reinterpret_cast<FunctionPointer>(ImageFetch<glm::ivec4, glm::ivec2>));
	// jit->AddFunction("@Image.Fetch.U32[4].U32[2]", reinterpret_cast<FunctionPointer>(ImageFetch<glm::uvec4, glm::ivec2>));
	//
	// jit->AddFunction("@Image.Read.F32[4].I32", reinterpret_cast<FunctionPointer>(ImageRead<glm::fvec4, glm::ivec1>));
	// jit->AddFunction("@Image.Read.I32[4].I32", reinterpret_cast<FunctionPointer>(ImageRead<glm::ivec4, glm::ivec1>));
	// jit->AddFunction("@Image.Read.U32[4].I32", reinterpret_cast<FunctionPointer>(ImageRead<glm::uvec4, glm::ivec1>));
	//
	// jit->AddFunction("@Image.Read.F32[4].U32", reinterpret_cast<FunctionPointer>(ImageRead<glm::fvec4, glm::ivec1>));
	// jit->AddFunction("@Image.Read.I32[4].U32", reinterpret_cast<FunctionPointer>(ImageRead<glm::ivec4, glm::ivec1>));
	// jit->AddFunction("@Image.Read.U32[4].U32", reinterpret_cast<FunctionPointer>(ImageRead<glm::uvec4, glm::ivec1>));
	//
	// jit->AddFunction("@Image.Read.F32[4].U32[2]", reinterpret_cast<FunctionPointer>(ImageRead<glm::fvec4, glm::ivec2>));
	// jit->AddFunction("@Image.Read.I32[4].U32[2]", reinterpret_cast<FunctionPointer>(ImageRead<glm::ivec4, glm::ivec2>));
	// jit->AddFunction("@Image.Read.U32[4].U32[2]", reinterpret_cast<FunctionPointer>(ImageRead<glm::uvec4, glm::ivec2>));
	//
	// jit->AddFunction("@Image.Read.F32[4].I32[2]", reinterpret_cast<FunctionPointer>(ImageRead<glm::fvec4, glm::ivec2>));
	// jit->AddFunction("@Image.Read.I32[4].I32[2]", reinterpret_cast<FunctionPointer>(ImageRead<glm::ivec4, glm::ivec2>));
	// jit->AddFunction("@Image.Read.U32[4].I32[2]", reinterpret_cast<FunctionPointer>(ImageRead<glm::uvec4, glm::ivec2>));
}
