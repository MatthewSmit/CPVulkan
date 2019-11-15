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

template<typename T>
T wrap(T v, T size, VkSamplerAddressMode addressMode)
{
	T result;
	for (auto i = 0; i < T::length(); i++)
	{
		result[i] = wrap(v[i], size[i], addressMode);
	}
	return result;
}

template<typename T>
T frac(T value)
{
	T result;
	for (auto i = 0; i < T::length(); i++)
	{
		result[i] = value[i] - std::floor(value[i]);
	}
	return result;
}

template<typename T>
static T lerp(T min, T max, double delta)
{
	return glm::dvec4(min) + glm::dvec4(max - min) * delta;
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
	const auto offset = GetImagePixelOffset(image->getImageSize(), i, j, k, mipLevel, layer);
	auto& functions = deviceState->imageFunctions[format];
	if (!functions.GetPixelDepth)
	{
		functions.GetPixelDepth = reinterpret_cast<decltype(functions.GetPixelDepth)>(CompileGetPixelDepth(deviceState->jit, &information));
	}

	return functions.GetPixelDepth(image->getDataPtr(offset, information.TotalSize));
}

template<>
glm::fvec4 GetPixel(DeviceState* deviceState, VkFormat format, gsl::span<uint8_t> data, glm::uvec1 range, glm::ivec1 coordinates)
{
	const auto& information = GetFormatInformation(format);
	const auto offset = GetImagePixelOffset(GetImageSize(information, range.x, 1, 1, 1, 1), coordinates.x, 0, 0, 0, 0);
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
glm::fvec4 GetPixel(DeviceState* deviceState, VkFormat format, gsl::span<uint8_t> data, glm::uvec2 range, glm::ivec2 coordinates)
{
	const auto& information = GetFormatInformation(format);
	const auto offset = GetImagePixelOffset(GetImageSize(information, range.x, range.y, 1, 1, 1), coordinates.x, coordinates.y, 0, 0, 0);
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
	const auto offset = GetImagePixelOffset(GetImageSize(information, range.x, range.y, range.z, 1, 1), coordinates.x, coordinates.y, coordinates.z, 0, 0);
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
glm::ivec4 GetPixel(DeviceState* deviceState, VkFormat format, gsl::span<uint8_t> data, glm::uvec1 range, glm::ivec1 coordinates)
{
	const auto& information = GetFormatInformation(format);
	const auto offset = GetImagePixelOffset(GetImageSize(information, range.x, 1, 1, 1, 1), coordinates.x, 0, 0, 0, 0);
	auto& functions = deviceState->imageFunctions[format];

	if (!functions.GetPixelI32)
	{
		if (information.Base == BaseType::UInt && information.ElementSize == 4)
		{
			functions.GetPixelI32 = reinterpret_cast<decltype(functions.GetPixelI32)>(CompileGetPixel(deviceState->jit, &information));
		}
		else
		{
			functions.GetPixelI32 = reinterpret_cast<decltype(functions.GetPixelI32)>(CompileGetPixelI32(deviceState->jit, &information));
		}
	}

	uint32_t values[4]{};
	functions.GetPixelI32(data.subspan(offset, information.TotalSize).data(), values);
	return glm::ivec4(values[0], values[1], values[2], values[3]);
}

template<>
glm::ivec4 GetPixel(DeviceState* deviceState, VkFormat format, gsl::span<uint8_t> data, glm::uvec2 range, glm::ivec2 coordinates)
{
	const auto& information = GetFormatInformation(format);
	const auto offset = GetImagePixelOffset(GetImageSize(information, range.x, range.y, 1, 1, 1), coordinates.x, coordinates.y, 0, 0, 0);
	auto& functions = deviceState->imageFunctions[format];

	if (!functions.GetPixelI32)
	{
		if (information.Base == BaseType::UInt && information.ElementSize == 4)
		{
			functions.GetPixelI32 = reinterpret_cast<decltype(functions.GetPixelI32)>(CompileGetPixel(deviceState->jit, &information));
		}
		else
		{
			functions.GetPixelI32 = reinterpret_cast<decltype(functions.GetPixelI32)>(CompileGetPixelI32(deviceState->jit, &information));
		}
	}

	uint32_t values[4]{};
	functions.GetPixelI32(data.subspan(offset, information.TotalSize).data(), values);
	return glm::ivec4(values[0], values[1], values[2], values[3]);
}

template<>
glm::ivec4 GetPixel(DeviceState* deviceState, VkFormat format, gsl::span<uint8_t> data, glm::uvec3 range, glm::ivec3 coordinates)
{
	const auto& information = GetFormatInformation(format);
	const auto offset = GetImagePixelOffset(GetImageSize(information, range.x, range.y, range.z, 1, 1), coordinates.x, coordinates.y, coordinates.z, 0, 0);
	auto& functions = deviceState->imageFunctions[format];

	if (!functions.GetPixelI32)
	{
		if (information.Base == BaseType::UInt && information.ElementSize == 4)
		{
			functions.GetPixelI32 = reinterpret_cast<decltype(functions.GetPixelI32)>(CompileGetPixel(deviceState->jit, &information));
		}
		else
		{
			functions.GetPixelI32 = reinterpret_cast<decltype(functions.GetPixelI32)>(CompileGetPixelI32(deviceState->jit, &information));
		}
	}

	uint32_t values[4]{};
	functions.GetPixelI32(data.subspan(offset, information.TotalSize).data(), values);
	return glm::ivec4(values[0], values[1], values[2], values[3]);
}

template<>
glm::uvec4 GetPixel(DeviceState* deviceState, VkFormat format, gsl::span<uint8_t> data, glm::uvec1 range, glm::ivec1 coordinates)
{
	const auto& information = GetFormatInformation(format);
	const auto offset = GetImagePixelOffset(GetImageSize(information, range.x, 1, 1, 1, 1), coordinates.x, 0, 0, 0, 0);
	auto& functions = deviceState->imageFunctions[format];

	if (!functions.GetPixelU32)
	{
		if (information.Base == BaseType::UInt && information.ElementSize == 4)
		{
			functions.GetPixelU32 = reinterpret_cast<decltype(functions.GetPixelU32)>(CompileGetPixel(deviceState->jit, &information));
		}
		else
		{
			functions.GetPixelU32 = reinterpret_cast<decltype(functions.GetPixelU32)>(CompileGetPixelU32(deviceState->jit, &information));
		}
	}

	uint32_t values[4]{};
	functions.GetPixelU32(data.subspan(offset, information.TotalSize).data(), values);
	return glm::uvec4(values[0], values[1], values[2], values[3]);
}

template<>
glm::uvec4 GetPixel(DeviceState* deviceState, VkFormat format, gsl::span<uint8_t> data, glm::uvec2 range, glm::ivec2 coordinates)
{
	const auto& information = GetFormatInformation(format);
	const auto offset = GetImagePixelOffset(GetImageSize(information, range.x, range.y, 1, 1, 1), coordinates.x, coordinates.y, 0, 0, 0);
	auto& functions = deviceState->imageFunctions[format];

	if (!functions.GetPixelU32)
	{
		if (information.Base == BaseType::UInt && information.ElementSize == 4)
		{
			functions.GetPixelU32 = reinterpret_cast<decltype(functions.GetPixelU32)>(CompileGetPixel(deviceState->jit, &information));
		}
		else
		{
			functions.GetPixelU32 = reinterpret_cast<decltype(functions.GetPixelU32)>(CompileGetPixelU32(deviceState->jit, &information));
		}
	}

	uint32_t values[4]{};
	functions.GetPixelU32(data.subspan(offset, information.TotalSize).data(), values);
	return glm::uvec4(values[0], values[1], values[2], values[3]);
}

template<>
glm::uvec4 GetPixel(DeviceState* deviceState, VkFormat format, gsl::span<uint8_t> data, glm::uvec3 range, glm::ivec3 coordinates)
{
	const auto& information = GetFormatInformation(format);
	const auto offset = GetImagePixelOffset(GetImageSize(information, range.x, range.y, range.z, 1, 1), coordinates.x, coordinates.y, coordinates.z, 0, 0);
	auto& functions = deviceState->imageFunctions[format];

	if (!functions.GetPixelU32)
	{
		if (information.Base == BaseType::UInt && information.ElementSize == 4)
		{
			functions.GetPixelU32 = reinterpret_cast<decltype(functions.GetPixelU32)>(CompileGetPixel(deviceState->jit, &information));
		}
		else
		{
			functions.GetPixelU32 = reinterpret_cast<decltype(functions.GetPixelU32)>(CompileGetPixelU32(deviceState->jit, &information));
		}
	}

	uint32_t values[4]{};
	functions.GetPixelU32(data.subspan(offset, information.TotalSize).data(), values);
	return glm::uvec4(values[0], values[1], values[2], values[3]);
}

template<typename ResultType, typename CoordinateType>
ResultType GetPixelLinear(DeviceState* deviceState, VkFormat format, gsl::span<uint8_t> data, 
                          glm::vec<CoordinateType::length(), uint32_t> range, CoordinateType newCoordinates0, CoordinateType newCoordinates1, glm::vec<CoordinateType::length(), float> interpolation)
{
	if constexpr (CoordinateType::length() == 1)
	{
		const auto i0 = GetPixel<ResultType>(deviceState, format, data, range, newCoordinates0);
		const auto i1 = GetPixel<ResultType>(deviceState, format, data, range, newCoordinates1);
		return lerp(i0, i1, interpolation.x);
	}

	if constexpr (CoordinateType::length() == 2)
	{
		const auto i0j0 = GetPixel<ResultType>(deviceState, format, data, range, glm::ivec2{newCoordinates0.x, newCoordinates0.y});
		const auto i0j1 = GetPixel<ResultType>(deviceState, format, data, range, glm::ivec2{newCoordinates0.x, newCoordinates1.y});
		const auto i1j0 = GetPixel<ResultType>(deviceState, format, data, range, glm::ivec2{newCoordinates1.x, newCoordinates0.y});
		const auto i1j1 = GetPixel<ResultType>(deviceState, format, data, range, glm::ivec2{newCoordinates1.x, newCoordinates1.y});

		const auto ij0 = lerp(i0j0, i1j0, interpolation.x);
		const auto ij1 = lerp(i0j1, i1j1, interpolation.x);

		return lerp(ij0, ij1, interpolation.y);
	}

	if constexpr (CoordinateType::length() == 3)
	{
		const auto i0j0k0 = GetPixel<ResultType>(deviceState, format, data, range, glm::ivec3{newCoordinates0.x, newCoordinates0.y, newCoordinates0.z});
		const auto i0j0k1 = GetPixel<ResultType>(deviceState, format, data, range, glm::ivec3{newCoordinates0.x, newCoordinates0.y, newCoordinates1.z});
		const auto i0j1k0 = GetPixel<ResultType>(deviceState, format, data, range, glm::ivec3{newCoordinates0.x, newCoordinates1.y, newCoordinates0.z});
		const auto i0j1k1 = GetPixel<ResultType>(deviceState, format, data, range, glm::ivec3{newCoordinates0.x, newCoordinates1.y, newCoordinates1.z});
		const auto i1j0k0 = GetPixel<ResultType>(deviceState, format, data, range, glm::ivec3{newCoordinates1.x, newCoordinates0.y, newCoordinates0.z});
		const auto i1j0k1 = GetPixel<ResultType>(deviceState, format, data, range, glm::ivec3{newCoordinates1.x, newCoordinates0.y, newCoordinates1.z});
		const auto i1j1k0 = GetPixel<ResultType>(deviceState, format, data, range, glm::ivec3{newCoordinates1.x, newCoordinates1.y, newCoordinates0.z});
		const auto i1j1k1 = GetPixel<ResultType>(deviceState, format, data, range, glm::ivec3{newCoordinates1.x, newCoordinates1.y, newCoordinates1.z});
		
		const auto ij0k0 = lerp(i0j0k0, i1j0k0, interpolation.x);
		const auto ij0k1 = lerp(i0j0k1, i1j0k1, interpolation.x);
		const auto ij1k0 = lerp(i0j1k0, i1j1k0, interpolation.x);
		const auto ij1k1 = lerp(i0j1k1, i1j1k1, interpolation.x);
		
		const auto ijk0 = lerp(ij0k0, ij1k0, interpolation.y);
		const auto ijk1 = lerp(ij0k1, ij1k1, interpolation.y);
		
		return lerp(ijk0, ijk1, interpolation.z);
	}
}

template<typename ResultType, typename RangeType, typename CoordinateType>
ResultType SampleImage(DeviceState* deviceState, VkFormat format, gsl::span<uint8_t> data, RangeType range, CoordinateType coordinates, VkFilter filter)
{
	static_assert(std::is_same<typename CoordinateType::value_type, float>::value);
	
	using IntCoordinateType = glm::vec<CoordinateType::length(), int32_t>;
	
	switch (filter)
	{
	case VK_FILTER_NEAREST:
		{
			constexpr auto addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			constexpr auto shift = 0.0f; // 0.0 for conventional, 0.5 for corner-sampled

			IntCoordinateType newCoordinates;
			for (auto i = 0; i < IntCoordinateType::length(); i++)
			{
				newCoordinates[i] = static_cast<int32_t>(std::floor(coordinates[i] * range[i] + shift));
			}
			newCoordinates = wrap<IntCoordinateType>(newCoordinates, range, addressMode);

			return GetPixel<ResultType>(deviceState, format, data, range, newCoordinates);
		}
		
	case VK_FILTER_LINEAR:
		{
			constexpr auto addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			constexpr auto shift = 0.5f; // 0.5 for conventional, 0.0 for corner-sampled

			IntCoordinateType newCoordinates0;
			for (auto i = 0; i < IntCoordinateType::length(); i++)
			{
				newCoordinates0[i] = static_cast<int32_t>(std::floor(coordinates[i] * range[i] - shift));
			}
			IntCoordinateType newCoordinates1 = newCoordinates0 + 1;
			newCoordinates0 = wrap<IntCoordinateType>(newCoordinates0, range, addressMode);
			newCoordinates1 = wrap<IntCoordinateType>(newCoordinates1, range, addressMode);
			
			const auto interpolation = frac(coordinates * static_cast<CoordinateType>(range) - shift);
			return GetPixelLinear<ResultType, IntCoordinateType>(deviceState, format, data, range, newCoordinates0, newCoordinates1, interpolation);
		}
		
	case VK_FILTER_CUBIC_IMG:
		FATAL_ERROR();
		
	default: FATAL_ERROR();
	}
}

template glm::fvec4 SampleImage(DeviceState* deviceState, VkFormat format, gsl::span<uint8_t> data, glm::uvec1 range, glm::fvec1 coordinates, VkFilter filter);
template glm::fvec4 SampleImage(DeviceState* deviceState, VkFormat format, gsl::span<uint8_t> data, glm::uvec2 range, glm::fvec2 coordinates, VkFilter filter);
template glm::fvec4 SampleImage(DeviceState* deviceState, VkFormat format, gsl::span<uint8_t> data, glm::uvec3 range, glm::fvec3 coordinates, VkFilter filter);

template glm::ivec4 SampleImage(DeviceState* deviceState, VkFormat format, gsl::span<uint8_t> data, glm::uvec1 range, glm::fvec1 coordinates, VkFilter filter);
template glm::ivec4 SampleImage(DeviceState* deviceState, VkFormat format, gsl::span<uint8_t> data, glm::uvec2 range, glm::fvec2 coordinates, VkFilter filter);
template glm::ivec4 SampleImage(DeviceState* deviceState, VkFormat format, gsl::span<uint8_t> data, glm::uvec3 range, glm::fvec3 coordinates, VkFilter filter);

template glm::uvec4 SampleImage(DeviceState* deviceState, VkFormat format, gsl::span<uint8_t> data, glm::uvec1 range, glm::fvec1 coordinates, VkFilter filter);
template glm::uvec4 SampleImage(DeviceState* deviceState, VkFormat format, gsl::span<uint8_t> data, glm::uvec2 range, glm::fvec2 coordinates, VkFilter filter);
template glm::uvec4 SampleImage(DeviceState* deviceState, VkFormat format, gsl::span<uint8_t> data, glm::uvec3 range, glm::fvec3 coordinates, VkFilter filter);

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

	glm::ivec2 newCoordinates;
	newCoordinates.x = static_cast<int32_t>(std::floor(coordinates.x * range.x + shift));
	newCoordinates.y = static_cast<int32_t>(std::floor(coordinates.y * range.y + shift));
	
	newCoordinates = wrap<glm::ivec2>(newCoordinates, range, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);

	return GetPixel<glm::fvec4>(deviceState, format, data, range, newCoordinates);

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
	const auto offset = GetImagePixelOffset(image->getImageSize(), i, j, k, mipLevel, layer);
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
	const auto offset = GetImagePixelOffset(image->getImageSize(), i, j, k, mipLevel, layer);
	auto& functions = deviceState->imageFunctions[format];
	if (!functions.SetPixelF32)
	{
		functions.SetPixelF32 = reinterpret_cast<decltype(functions.SetPixelF32)>(CompileSetPixelF32(deviceState->jit, &information));
	}

	functions.SetPixelF32(image->getDataPtr(offset, information.TotalSize), values);
}

void SetPixel(DeviceState* deviceState, VkFormat format, Image* image, int32_t i, int32_t j, int32_t k, uint32_t mipLevel, uint32_t layer, int32_t values[4])
{
	const auto& information = GetFormatInformation(format);
	const auto offset = GetImagePixelOffset(image->getImageSize(), i, j, k, mipLevel, layer);
	auto& functions = deviceState->imageFunctions[format];
	if (!functions.SetPixelI32)
	{
		functions.SetPixelI32 = reinterpret_cast<decltype(functions.SetPixelI32)>(CompileSetPixelI32(deviceState->jit, &information));
	}

	functions.SetPixelI32(image->getDataPtr(offset, information.TotalSize), values);
}

void SetPixel(DeviceState* deviceState, VkFormat format, Image* image, int32_t i, int32_t j, int32_t k, uint32_t mipLevel, uint32_t layer, uint32_t values[4])
{
	const auto& information = GetFormatInformation(format);
	const auto offset = GetImagePixelOffset(image->getImageSize(), i, j, k, mipLevel, layer);
	auto& functions = deviceState->imageFunctions[format];
	if (!functions.SetPixelU32)
	{
		functions.SetPixelU32 = reinterpret_cast<decltype(functions.SetPixelU32)>(CompileSetPixelU32(deviceState->jit, &information));
	}

	functions.SetPixelU32(image->getDataPtr(offset, information.TotalSize), values);
}