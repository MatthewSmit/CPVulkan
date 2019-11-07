#include "ImageSampler.h"

#include "DeviceState.h"
#include "Formats.h"
#include "Image.h"

#include <ImageCompiler.h>

// #include <glm/glm.hpp>
//
// #include <algorithm>
//
// static float frac(float value)
// {
// 	return value - std::floor(value);
// }
//
// template<typename T>
// static T lerp(T min, T max, double delta)
// {
// 	return glm::dvec4(min) + glm::dvec4(max - min) * delta;
// }
//
// static int32_t wrap(int32_t v, int32_t size, VkSamplerAddressMode addressMode)
// {
// 	switch (addressMode)
// 	{
// 	case VK_SAMPLER_ADDRESS_MODE_REPEAT:
// 		return v % size;
// 		
// 	case VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT:
// 		{
// 			const auto n = v % (2 * size) - size;
// 			return size - 1 - (n >= 0 ? n : -(1 + n));
// 		}
// 		
// 	case VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE:
// 		return std::clamp(v, 0, size - 1);
// 		
// 	case VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER:
// 		return std::clamp(v, -1, size);
// 		
// 	case VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE:
// 		return std::clamp(v >= 0 ? v : -(1 + v), 0, size - 1);
//
// 	default:
// 		FATAL_ERROR();
// 	}
// }
//
// static float clamp(float v, float min, float max)
// {
// 	return std::min(std::max(v, min), max);
// }
//
// template<typename T>
// static void clampSInput(const T input[4], T output[4])
// {
// 	FATAL_ERROR();
// }
//
// template<typename T>
// static void clampUInput(const T input[4], T output[4])
// {
// 	FATAL_ERROR();
// }
//
// template<>
// static void clampSInput<float>(const float input[4], float output[4])
// {
// 	output[0] = clamp(input[0], -1, 1);
// 	output[1] = clamp(input[1], -1, 1);
// 	output[2] = clamp(input[2], -1, 1);
// 	output[3] = clamp(input[3], -1, 1);
// }
//
// template<>
// static void clampUInput<float>(const float input[4], float output[4])
// {
// 	output[0] = clamp(input[0], 0, 1);
// 	output[1] = clamp(input[1], 0, 1);
// 	output[2] = clamp(input[2], 0, 1);
// 	output[3] = clamp(input[3], 0, 1);
// }
//
// template<typename T>
// void LinearToSRGB(const T input[4], T output[4])
// {
// 	FATAL_ERROR();
// }
//
// template<>
// void LinearToSRGB(const float input[4], float output[4])
// {
// 	output[0] = input[0] <= 0.0031308 ? input[0] * 12.92f : 1.055f * std::pow(input[0], 1.0f / 2.4f) - 0.055f;
// 	output[1] = input[1] <= 0.0031308 ? input[1] * 12.92f : 1.055f * std::pow(input[1], 1.0f / 2.4f) - 0.055f;
// 	output[2] = input[2] <= 0.0031308 ? input[2] * 12.92f : 1.055f * std::pow(input[2], 1.0f / 2.4f) - 0.055f;
// 	output[3] = input[3];
// }
//
// template<typename T>
// void SRGBToLinear(const T input[4], T output[4])
// {
// 	FATAL_ERROR();
// }
//
// template<>
// void SRGBToLinear(const float input[4], float output[4])
// {
// 	output[0] = input[0] <= 0.04045 ? input[0] / 12.92f : std::pow((input[0] + 0.055f) / 1.055f, 2.4f);
// 	output[1] = input[1] <= 0.04045 ? input[1] / 12.92f : std::pow((input[1] + 0.055f) / 1.055f, 2.4f);
// 	output[2] = input[2] <= 0.04045 ? input[2] / 12.92f : std::pow((input[2] + 0.055f) / 1.055f, 2.4f);
// 	output[3] = input[3];
// }
//
// template<typename Size>
// void GetPixel(const FormatInformation& format, gsl::span<uint8_t> data, uint32_t i, uint32_t j, uint32_t k, uint32_t width, uint32_t height, uint32_t depth, uint32_t arrayLayers, uint64_t values[4])
// {
// 	static_assert(std::numeric_limits<Size>::is_integer);
// 	static_assert(!std::numeric_limits<Size>::is_signed);
//
// 	if (format.ElementSize != sizeof(Size))
// 	{
// 		FATAL_ERROR();
// 	}
//
// 	const auto pixel = reinterpret_cast<const uint8_t*>(GetFormatPixelOffset(format, data, i, j, k, width, height, depth, arrayLayers, 0, 0));
// 	if (format.RedOffset != -1) values[0] = *reinterpret_cast<const Size*>(pixel + format.RedOffset);
// 	if (format.GreenOffset != -1) values[1] = *reinterpret_cast<const Size*>(pixel + format.GreenOffset);
// 	if (format.BlueOffset != -1) values[2] = *reinterpret_cast<const Size*>(pixel + format.BlueOffset);
// 	if (format.AlphaOffset != -1) values[3] = *reinterpret_cast<const Size*>(pixel + format.AlphaOffset);
// }
//
// template<typename OutputType>
// void GetPixel(const FormatInformation& format, const Image* image, int32_t i, int32_t j, int32_t k, OutputType output[4])
// {
// 	// TODO: Border colour
// 	assert(i >= 0 && j >= 0 && k >= 0 && i < image->getWidth() && j < image->getHeight() && k < image->getDepth());
//
// 	uint64_t rawValues[4]{};
// 	if (format.Base == BaseType::UNorm || format.Base == BaseType::UInt || format.Base == BaseType::SFloat)
// 	{
// 		if (format.ElementSize == 1)
// 		{
// 			GetPixel<uint8_t>(format, image->getData(), i, j, k, image->getWidth(), image->getHeight(), image->getDepth(), image->getArrayLayers(), rawValues);
// 		}
// 		else if (format.ElementSize == 2)
// 		{
// 			GetPixel<uint16_t>(format, image->getData(), i, j, k, image->getWidth(), image->getHeight(), image->getDepth(), image->getArrayLayers(), rawValues);
// 		}
// 		else if (format.ElementSize == 4)
// 		{
// 			GetPixel<uint32_t>(format, image->getData(), i, j, k, image->getWidth(), image->getHeight(), image->getDepth(), image->getArrayLayers(), rawValues);
// 		}
// 		else
// 		{
// 			FATAL_ERROR();
// 		}
// 	}
// 	else
// 	{
// 		FATAL_ERROR();
// 	}
// 	
// 	ConvertPixelsFromTemp<OutputType>(format, rawValues, output);
// }
//
// template<typename OutputType>
// OutputType GetPixel(const FormatInformation& format, const Image* image, int32_t i, int32_t j, int32_t k)
// {
// 	static_assert(OutputType::length() == 4);
// 	OutputType result{};
// 	GetPixel<typename OutputType::value_type>(format, image, i, j, k, &result.x);
// 	return result;
// }
//
//
// template<typename Size>
// void SetPixel(const FormatInformation& format, gsl::span<uint8_t> data, uint32_t i, uint32_t j, uint32_t k, uint32_t width, uint32_t height, uint32_t depth, uint32_t arrayLayers, uint32_t mipLevel, uint32_t layer, const uint64_t values[4])
// {
// 	static_assert(std::numeric_limits<Size>::is_integer);
// 	static_assert(!std::numeric_limits<Size>::is_signed);
// 	
// 	if (format.ElementSize != sizeof(Size))
// 	{
// 		FATAL_ERROR();
// 	}
//
// 	const auto pixel = reinterpret_cast<uint8_t*>(GetFormatPixelOffset(format, data, i, j, k, width, height, depth, arrayLayers, mipLevel, layer));
// 	if (format.RedOffset != -1) *reinterpret_cast<Size*>(pixel + format.RedOffset) = static_cast<Size>(values[0]);
// 	if (format.GreenOffset != -1) *reinterpret_cast<Size*>(pixel + format.GreenOffset) = static_cast<Size>(values[1]);
// 	if (format.BlueOffset != -1) *reinterpret_cast<Size*>(pixel + format.BlueOffset) = static_cast<Size>(values[2]);
// 	if (format.AlphaOffset != -1) *reinterpret_cast<Size*>(pixel + format.AlphaOffset) = static_cast<Size>(values[3]);
// }
//
// template<typename Size>
// static void SetPackedPixel(const FormatInformation& format, gsl::span<uint8_t> data, uint32_t i, uint32_t j, uint32_t k, uint32_t width, uint32_t height, uint32_t depth, uint32_t arrayLayers, uint32_t mipLevel, uint32_t layer, uint64_t values[4])
// {
// 	static_assert(std::numeric_limits<Size>::is_integer);
// 	static_assert(!std::numeric_limits<Size>::is_signed);
//
// 	const auto pixel = reinterpret_cast<Size*>(GetFormatPixelOffset(format, data, i, j, k, width, height, depth, arrayLayers, mipLevel, layer));
// 	*pixel = static_cast<Size>(values[0]);
// }

float GetDepthPixel(DeviceState* deviceState, VkFormat format, const Image* image, int32_t i, int32_t j, int32_t k, uint32_t mipLevel, uint32_t layer)
{
	const auto& information = GetFormatInformation(format);
	const auto offset = GetFormatPixelOffset(information, i, j, k, image->getWidth(), image->getHeight(), image->getDepth(), image->getArrayLayers(), mipLevel, layer);
	auto& functions = deviceState->imageFunctions[format];
	if (!functions.GetPixelDepth)
	{
		functions.GetPixelDepth = reinterpret_cast<decltype(functions.GetPixelDepth)>(CompileGetPixelDepth(deviceState->jit, &information));
	}

	return functions.GetPixelDepth(image->getDataPtr(offset, information.TotalSize));
}


void SetPixel(DeviceState* deviceState, VkFormat format, Image* image, int32_t i, int32_t j, int32_t k, uint32_t mipLevel, uint32_t layer, VkClearDepthStencilValue value)
{
	const auto& information = GetFormatInformation(format);
	const auto offset = GetFormatPixelOffset(information, i, j, k, image->getWidth(), image->getHeight(), image->getDepth(), image->getArrayLayers(), mipLevel, layer);
	auto& functions = deviceState->imageFunctions[format];
	if (!functions.SetPixelDepthStencil)
	{
		functions.SetPixelDepthStencil = reinterpret_cast<decltype(functions.SetPixelDepthStencil)>(CompileSetPixelDepthStencil(deviceState->jit, &information));
	}

	functions.SetPixelDepthStencil(image->getDataPtr(offset, information.TotalSize), value.depth, value.stencil);
}

void SetPixel(DeviceState* deviceState, VkFormat format, Image* image, int32_t i, int32_t j, int32_t k, uint32_t mipLevel, uint32_t layer, VkClearColorValue value)
{
	switch (GetFormatInformation(format).Base)
	{
	case BaseType::UNorm:
	case BaseType::SNorm:
	case BaseType::UFloat:
	case BaseType::SFloat:
	case BaseType::SRGB:
		SetPixel(deviceState, format, image, i, j, k, mipLevel, layer, value.float32);
		break;

	case BaseType::UScaled:
	case BaseType::UInt:
		SetPixel(deviceState, format, image, i, j, k, mipLevel, layer, value.uint32);
		break;
		
	case BaseType::SScaled:
	case BaseType::SInt:
		SetPixel(deviceState, format, image, i, j, k, mipLevel, layer, value.int32);
		break;
		
	default: FATAL_ERROR();
	}
}

void SetPixel(DeviceState* deviceState, VkFormat format, Image* image, int32_t i, int32_t j, int32_t k, uint32_t mipLevel, uint32_t layer, float values[4])
{
	const auto& information = GetFormatInformation(format);
	const auto offset = GetFormatPixelOffset(information, i, j, k, image->getWidth(), image->getHeight(), image->getDepth(), image->getArrayLayers(), mipLevel, layer);
	auto& functions = deviceState->imageFunctions[format];
	if (!functions.SetPixelFloat)
	{
		functions.SetPixelFloat = reinterpret_cast<decltype(functions.SetPixelFloat)>(CompileSetPixelFloat(deviceState->jit, &information));
	}

	functions.SetPixelFloat(image->getDataPtr(offset, information.TotalSize), values);
}

void SetPixel(DeviceState* deviceState, VkFormat format, Image* image, int32_t i, int32_t j, int32_t k, uint32_t mipLevel, uint32_t layer, int32_t values[4])
{
	const auto& information = GetFormatInformation(format);
	const auto offset = GetFormatPixelOffset(information, i, j, k, image->getWidth(), image->getHeight(), image->getDepth(), image->getArrayLayers(), mipLevel, layer);
	auto& functions = deviceState->imageFunctions[format];
	if (!functions.SetPixelInt32)
	{
		functions.SetPixelInt32 = reinterpret_cast<decltype(functions.SetPixelInt32)>(CompileSetPixelInt32(deviceState->jit, &information));
	}

	functions.SetPixelInt32(image->getDataPtr(offset, information.TotalSize), values);
}

void SetPixel(DeviceState* deviceState, VkFormat format, Image* image, int32_t i, int32_t j, int32_t k, uint32_t mipLevel, uint32_t layer, uint32_t values[4])
{
	const auto& information = GetFormatInformation(format);
	const auto offset = GetFormatPixelOffset(information, i, j, k, image->getWidth(), image->getHeight(), image->getDepth(), image->getArrayLayers(), mipLevel, layer);
	auto& functions = deviceState->imageFunctions[format];
	if (!functions.SetPixelUInt32)
	{
		functions.SetPixelUInt32 = reinterpret_cast<decltype(functions.SetPixelUInt32)>(CompileSetPixelUInt32(deviceState->jit, &information));
	}

	functions.SetPixelUInt32(image->getDataPtr(offset, information.TotalSize), values);
}

//
// template<typename OutputType>
// void SampleImage(const FormatInformation& format, const Image* image, float u, float v, float w, float q, uint32_t a, VkFilter filter, VkSamplerMipmapMode mipmapMode, VkSamplerAddressMode addressMode, uint64_t output[4])
// {
// 	auto pixels = SampleImage<OutputType>(image, u, v, w, q, a, filter, mipmapMode, addressMode);
// 	ConvertPixelsToTemp(format, &pixels.x, output);
// }
//
// template<typename ReturnType>
// ReturnType SampleImage(const Image* image, float u, float v, float w, float q, uint32_t a, VkFilter filter, VkSamplerMipmapMode mipmapMode, VkSamplerAddressMode addressMode)
// {
// 	// TODO: Optimise for 1D/2D
// 	const auto& format = GetFormatInformation(image->getFormat());
// 	
// 	if (mipmapMode != VK_SAMPLER_MIPMAP_MODE_NEAREST)
// 	{
// 		FATAL_ERROR();
// 	}
//
// 	if (q != 0)
// 	{
// 		FATAL_ERROR();
// 	}
//
// 	if (a != 0)
// 	{
// 		FATAL_ERROR();
// 	}
//
// 	switch (filter)
// 	{
// 	case VK_FILTER_NEAREST:
// 		{
// 			constexpr auto shift = 0.0f; // 0.0 for conventional, 0.5 for corner-sampled
// 			
// 			auto i = static_cast<int32_t>(std::floor(u + shift));
// 			auto j = static_cast<int32_t>(std::floor(v + shift));
// 			auto k = static_cast<int32_t>(std::floor(w + shift));
//
// 			i = wrap(i, image->getWidth(), addressMode);
// 			j = wrap(j, image->getHeight(), addressMode);
// 			k = wrap(k, image->getDepth(), addressMode);
// 			
// 			return GetPixel<ReturnType>(format, image, i, j, k);
// 		}
// 		
// 	case VK_FILTER_LINEAR:
// 		{
// 			constexpr auto shift = 0.5f; // 0.5 for conventional, 0.0 for corner-sampled
// 			
// 			auto i0 = static_cast<int32_t>(std::floor(u - shift));
// 			auto j0 = static_cast<int32_t>(std::floor(v - shift));
// 			auto k0 = static_cast<int32_t>(std::floor(w - shift));
// 			
// 			const auto i1 = wrap(i0 + 1, image->getWidth(), addressMode);
// 			const auto j1 = wrap(j0 + 1, image->getHeight(), addressMode);
// 			const auto k1 = wrap(k0 + 1, image->getDepth(), addressMode);
//
// 			i0 = wrap(i0, image->getWidth(), addressMode);
// 			j0 = wrap(j0, image->getHeight(), addressMode);
// 			k0 = wrap(k0, image->getDepth(), addressMode);
// 			
// 			const auto alpha = frac(u - shift);
// 			const auto beta = frac(v - shift);
// 			const auto gamma = frac(w - shift);
// 			
// 			const auto i0j0k0 = GetPixel<ReturnType>(format, image, i0, j0, k0);
// 			const auto i0j0k1 = GetPixel<ReturnType>(format, image, i0, j0, k1);
// 			const auto i0j1k0 = GetPixel<ReturnType>(format, image, i0, j1, k0);
// 			const auto i0j1k1 = GetPixel<ReturnType>(format, image, i0, j1, k1);
// 			const auto i1j0k0 = GetPixel<ReturnType>(format, image, i1, j0, k0);
// 			const auto i1j0k1 = GetPixel<ReturnType>(format, image, i1, j0, k1);
// 			const auto i1j1k0 = GetPixel<ReturnType>(format, image, i1, j1, k0);
// 			const auto i1j1k1 = GetPixel<ReturnType>(format, image, i1, j1, k1);
//
// 			const auto ij0k0 = lerp(i0j0k0, i1j0k0, alpha);
// 			const auto ij0k1 = lerp(i0j0k1, i1j0k1, alpha);
// 			const auto ij1k0 = lerp(i0j1k0, i1j1k0, alpha);
// 			const auto ij1k1 = lerp(i0j1k1, i1j1k1, alpha);
//
// 			const auto ijk0 = lerp(ij0k0, ij1k0, beta);
// 			const auto ijk1 = lerp(ij0k1, ij1k1, beta);
//
// 			return lerp(ijk0, ijk1, gamma);
// 		}
// 		
// 	case VK_FILTER_CUBIC_IMG:
// 		FATAL_ERROR();
// 		break;
// 		
// 	default:
// 		FATAL_ERROR();
// 	}
// 	
// 	FATAL_ERROR();
// }
//
// template void SampleImage<glm::f32vec4>(const FormatInformation& format, const Image* image, float u, float v, float w, float q, uint32_t a, VkFilter filter, VkSamplerMipmapMode mipmapMode, VkSamplerAddressMode addressMode, uint64_t output[4]);
// template glm::f32vec4 SampleImage(const Image* image, float u, float v, float w, float q, uint32_t a, VkFilter filter, VkSamplerMipmapMode mipmapMode, VkSamplerAddressMode addressMode);
//
// template void SampleImage<glm::f64vec4>(const FormatInformation& format, const Image* image, float u, float v, float w, float q, uint32_t a, VkFilter filter, VkSamplerMipmapMode mipmapMode, VkSamplerAddressMode addressMode, uint64_t output[4]);
// template glm::f64vec4 SampleImage(const Image* image, float u, float v, float w, float q, uint32_t a, VkFilter filter, VkSamplerMipmapMode mipmapMode, VkSamplerAddressMode addressMode);
//
// template void SampleImage<glm::i32vec4>(const FormatInformation& format, const Image* image, float u, float v, float w, float q, uint32_t a, VkFilter filter, VkSamplerMipmapMode mipmapMode, VkSamplerAddressMode addressMode, uint64_t output[4]);
// template glm::i32vec4 SampleImage(const Image* image, float u, float v, float w, float q, uint32_t a, VkFilter filter, VkSamplerMipmapMode mipmapMode, VkSamplerAddressMode addressMode);
//
// template void SampleImage<glm::i64vec4>(const FormatInformation& format, const Image* image, float u, float v, float w, float q, uint32_t a, VkFilter filter, VkSamplerMipmapMode mipmapMode, VkSamplerAddressMode addressMode, uint64_t output[4]);
// template glm::i64vec4 SampleImage(const Image* image, float u, float v, float w, float q, uint32_t a, VkFilter filter, VkSamplerMipmapMode mipmapMode, VkSamplerAddressMode addressMode);
//
// template void SampleImage<glm::u32vec4>(const FormatInformation& format, const Image* image, float u, float v, float w, float q, uint32_t a, VkFilter filter, VkSamplerMipmapMode mipmapMode, VkSamplerAddressMode addressMode, uint64_t output[4]);
// template glm::u32vec4 SampleImage(const Image* image, float u, float v, float w, float q, uint32_t a, VkFilter filter, VkSamplerMipmapMode mipmapMode, VkSamplerAddressMode addressMode);
//
// template void SampleImage<glm::u64vec4>(const FormatInformation& format, const Image* image, float u, float v, float w, float q, uint32_t a, VkFilter filter, VkSamplerMipmapMode mipmapMode, VkSamplerAddressMode addressMode, uint64_t output[4]);
// template glm::u64vec4 SampleImage(const Image* image, float u, float v, float w, float q, uint32_t a, VkFilter filter, VkSamplerMipmapMode mipmapMode, VkSamplerAddressMode addressMode);
