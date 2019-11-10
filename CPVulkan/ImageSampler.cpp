#include "ImageSampler.h"

#include "DeviceState.h"
#include "Formats.h"
#include "Image.h"
#include "Sampler.h"

#include <ImageCompiler.h>

#include <glm/glm.hpp>

static int32_t wrap(int32_t v, int32_t size, VkSamplerAddressMode addressMode)
{
	switch (addressMode)
	{
	case VK_SAMPLER_ADDRESS_MODE_REPEAT:
		return v % size;
		
	case VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT:
		{
			const auto n = v % (2 * size) - size;
			return size - 1 - (n >= 0 ? n : -(1 + n));
		}
		
	case VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE:
		return std::clamp(v, 0, size - 1);
		
	case VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER:
		return std::clamp(v, -1, size);
		
	case VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE:
		return std::clamp(v >= 0 ? v : -(1 + v), 0, size - 1);

	default:
		FATAL_ERROR();
	}
}

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

template<>
glm::fvec4 GetPixel(DeviceState* deviceState, VkFormat format, gsl::span<uint8_t> data, glm::uvec2 range, glm::ivec2 coordinates)
{
	const auto& information = GetFormatInformation(format);
	const auto offset = GetFormatPixelOffset(information, coordinates.x, coordinates.y, 0, range.x, range.y, 1, 1, 0, 0);
	auto& functions = deviceState->imageFunctions[format];
	
	if (!functions.GetPixelF32)
	{
		if (information.Base == BaseType::SFloat && information.ElementSize == 4)
		{
			functions.GetPixelF32 = reinterpret_cast<decltype(functions.GetPixelU32)>(CompileGetPixel(deviceState->jit, &information));
		}
		else
		{
			functions.GetPixelF32 = reinterpret_cast<decltype(functions.GetPixelU32)>(CompileGetPixelF32(deviceState->jit, &information));
		}
	}

	float values[4]{};
	functions.GetPixelF32(data.subspan(offset, information.TotalSize).data(), values);
	return glm::fvec4(values[0], values[1], values[2], values[3]);
}

template<>
glm::fvec4 GetPixel(DeviceState* deviceState, VkFormat format, gsl::span<uint8_t> data, glm::uvec3 range, glm::ivec3 coordinates)
{
	const auto& information = GetFormatInformation(format);
	const auto offset = GetFormatPixelOffset(information, coordinates.x, coordinates.y, coordinates.z, range.x, range.y, range.z, 1, 0, 0);
	auto& functions = deviceState->imageFunctions[format];
	
	if (!functions.GetPixelF32)
	{
		if (information.Base == BaseType::SFloat && information.ElementSize == 4)
		{
			functions.GetPixelF32 = reinterpret_cast<decltype(functions.GetPixelU32)>(CompileGetPixel(deviceState->jit, &information));
		}
		else
		{
			functions.GetPixelF32 = reinterpret_cast<decltype(functions.GetPixelU32)>(CompileGetPixelF32(deviceState->jit, &information));
		}
	}

	float values[4]{};
	functions.GetPixelF32(data.subspan(offset, information.TotalSize).data(), values);
	return glm::fvec4(values[0], values[1], values[2], values[3]);
}

template<>
glm::uvec4 GetPixel(DeviceState* deviceState, VkFormat format, gsl::span<uint8_t> data, glm::uvec1 range, glm::ivec1 coordinates)
{
	const auto& information = GetFormatInformation(format);
	const auto offset = GetFormatPixelOffset(information, coordinates.x, 0, 0, range.x, 1, 1, 1, 0, 0);
	auto& functions = deviceState->imageFunctions[format];

	if (!functions.GetPixelU32)
	{
		if (information.Base == BaseType::UInt && information.ElementSize == 4)
		{
			functions.GetPixelU32 = reinterpret_cast<decltype(functions.GetPixelU32)>(CompileGetPixel(deviceState->jit, &information));
		}
		else
		{
			FATAL_ERROR();
		}
	}

	uint32_t values[4]{};
	functions.GetPixelU32(data.subspan(offset, information.TotalSize).data(), values);
	return glm::uvec4(values[0], values[1], values[2], values[3]);
}

template<>
glm::fvec4 SampleImage(DeviceState* deviceState, VkFormat format, gsl::span<uint8_t> data, glm::uvec3 range, glm::fvec3 coordinates)
{
	constexpr auto shift = 0.0f; // 0.0 for conventional, 0.5 for corner-sampled

	auto i = static_cast<int32_t>(std::floor(coordinates.x * range.x + shift));
	auto j = static_cast<int32_t>(std::floor(coordinates.y * range.y + shift));
	auto k = static_cast<int32_t>(std::floor(coordinates.z * range.z + shift));
	
	i = wrap(i, range.x, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
	j = wrap(j, range.y, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
	k = wrap(k, range.z, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);

	return GetPixel<glm::fvec4>(deviceState, format, data, range, glm::ivec3{i, j, k});

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
}

template<>
glm::fvec4 SampleImage(DeviceState* deviceState, VkFormat format, gsl::span<uint8_t> data, glm::uvec2 range, glm::fvec2 coordinates, float lod, Sampler* sampler)
{
	if (lod != 0)
	{
		FATAL_ERROR();
	}
	
	if (sampler->getFlags() != 0)
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

	if (sampler->getAddressModeU() != VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE)
	{
		FATAL_ERROR();
	}

	if (sampler->getAddressModeV() != VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE)
	{
		FATAL_ERROR();
	}

	if (sampler->getAddressModeW() != VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE)
	{
		FATAL_ERROR();
	}

	if (sampler->getMipLodBias() != 0)
	{
		FATAL_ERROR();
	}

	if (sampler->getAnisotropyEnable())
	{
		FATAL_ERROR();
	}

	if (sampler->getMaxAnisotropy() != 1)
	{
		FATAL_ERROR();
	}

	if (sampler->getCompareEnable())
	{
		FATAL_ERROR();
	}

	if (sampler->getCompareOp() != VK_COMPARE_OP_NEVER)
	{
		FATAL_ERROR();
	}

	if (sampler->getMinLod() != 0)
	{
		FATAL_ERROR();
	}

	if (sampler->getMaxLod() != 0)
	{
		FATAL_ERROR();
	}

	if (sampler->getBorderColor() != VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE)
	{
		FATAL_ERROR();
	}

	if (sampler->getUnnormalisedCoordinates())
	{
		FATAL_ERROR();
	}

	constexpr auto shift = 0.0f; // 0.0 for conventional, 0.5 for corner-sampled

	auto i = static_cast<int32_t>(std::floor(coordinates.x * range.x + shift));
	auto j = static_cast<int32_t>(std::floor(coordinates.y * range.y + shift));
	
	i = wrap(i, range.x, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
	j = wrap(j, range.y, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);

	return GetPixel<glm::fvec4>(deviceState, format, data, range, glm::ivec2{i, j});

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