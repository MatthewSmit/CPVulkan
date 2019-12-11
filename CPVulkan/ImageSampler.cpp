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
		return (v % size + size) % size;
		
	case VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT:
		{
			const auto twoSize = 2 * size;
			const auto n = (v % twoSize + twoSize) % twoSize - size;
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
T frac(T value)
{
	T result{};
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

float GetDepthPixel(DeviceState* deviceState, VkFormat format, const Image* image, int32_t i, int32_t j, int32_t k, uint32_t mipLevel, uint32_t layer)
{
	const auto& information = GetFormatInformation(format);
	const auto offset = GetImagePixelOffset(image->getImageSize(), i, j, k, mipLevel, layer);
	auto functions = deviceState->getImageFunctions(format);
	if (!functions->GetPixelDepth)
	{
		functions->GetPixelDepth = reinterpret_cast<decltype(functions->GetPixelDepth)>(CompileGetPixelDepth(deviceState->jit, &information));
	}

	return functions->GetPixelDepth(image->getDataPtr(offset, information.TotalSize));
}

uint8_t GetStencilPixel(DeviceState* deviceState, VkFormat format, const Image* image, int32_t i, int32_t j, int32_t k, uint32_t mipLevel, uint32_t layer)
{
	const auto& information = GetFormatInformation(format);
	const auto offset = GetImagePixelOffset(image->getImageSize(), i, j, k, mipLevel, layer);
	auto functions = deviceState->getImageFunctions(format);
	if (!functions->GetPixelStencil)
	{
		functions->GetPixelStencil = reinterpret_cast<decltype(functions->GetPixelStencil)>(CompileGetPixelStencil(deviceState->jit, &information));
	}

	return functions->GetPixelStencil(image->getDataPtr(offset, information.TotalSize));
}

template<>
glm::fvec4 GetPixel(DeviceState* deviceState, VkFormat format, gsl::span<uint8_t> data, glm::uvec1 range, glm::ivec1 coordinates, glm::fvec4 borderColour)
{
	if (coordinates.x < 0 || coordinates.x >= range.x)
	{
		return borderColour;
	}
	
	const auto& information = GetFormatInformation(format);
	const auto offset = GetImagePixelOffset(GetImageSize(information, range.x, 1, 1, 1, 1), coordinates.x, 0, 0, 0, 0);
	auto functions = deviceState->getImageFunctions(format);
	
	if (!functions->GetPixelF32)
	{
		functions->GetPixelF32 = reinterpret_cast<decltype(functions->GetPixelF32)>(CompileGetPixelF32(deviceState->jit, &information));
	}

	float values[4]{};
	functions->GetPixelF32(data.subspan(offset, information.TotalSize).data(), values);
	return glm::fvec4(values[0], values[1], values[2], values[3]);
}

template<>
glm::fvec4 GetPixel(DeviceState* deviceState, VkFormat format, gsl::span<uint8_t> data, glm::uvec2 range, glm::ivec2 coordinates, glm::fvec4 borderColour)
{
	if (coordinates.x < 0 || coordinates.x >= range.x || coordinates.y < 0 || coordinates.y >= range.y)
	{
		return borderColour;
	}

	const auto& information = GetFormatInformation(format);
	auto functions = deviceState->getImageFunctions(format);
	if (information.Type == FormatType::Compressed)
	{
		const auto subX = coordinates.x % information.Compressed.BlockWidth;
		const auto subY = coordinates.y % information.Compressed.BlockHeight;
		coordinates.x /= information.Compressed.BlockWidth;
		coordinates.y /= information.Compressed.BlockHeight;
		const auto offset = GetImagePixelOffset(GetImageSize(information, range.x, range.y, 1, 1, 1), coordinates.x, coordinates.y, 0, 0, 0);

		if (!functions->GetPixelF32C)
		{
			functions->GetPixelF32C = reinterpret_cast<decltype(functions->GetPixelF32C)>(CompileGetPixelF32(deviceState->jit, &information));
		}

		float values[4]{};
		functions->GetPixelF32C(data.subspan(offset, information.TotalSize).data(), values, subX, subY);
		return glm::fvec4(values[0], values[1], values[2], values[3]);
	}
	else
	{
		const auto offset = GetImagePixelOffset(GetImageSize(information, range.x, range.y, 1, 1, 1), coordinates.x, coordinates.y, 0, 0, 0);

		if (!functions->GetPixelF32)
		{
			functions->GetPixelF32 = reinterpret_cast<decltype(functions->GetPixelF32)>(CompileGetPixelF32(deviceState->jit, &information));
		}

		float values[4]{};
		functions->GetPixelF32(data.subspan(offset, information.TotalSize).data(), values);
		return glm::fvec4(values[0], values[1], values[2], values[3]);
	}
}

template<>
glm::fvec4 GetPixel(DeviceState* deviceState, VkFormat format, gsl::span<uint8_t> data, glm::uvec3 range, glm::ivec3 coordinates, glm::fvec4 borderColour)
{
	if (coordinates.x < 0 || coordinates.x >= range.x || coordinates.y < 0 || coordinates.y >= range.y || coordinates.z < 0 || coordinates.z >= range.z)
	{
		return borderColour;
	}

	const auto& information = GetFormatInformation(format);
	const auto offset = GetImagePixelOffset(GetImageSize(information, range.x, range.y, range.z, 1, 1), coordinates.x, coordinates.y, coordinates.z, 0, 0);
	auto functions = deviceState->getImageFunctions(format);
	
	if (!functions->GetPixelF32)
	{
		functions->GetPixelF32 = reinterpret_cast<decltype(functions->GetPixelF32)>(CompileGetPixelF32(deviceState->jit, &information));
	}

	float values[4]{};
	functions->GetPixelF32(data.subspan(offset, information.TotalSize).data(), values);
	return glm::fvec4(values[0], values[1], values[2], values[3]);
}

template<>
glm::ivec4 GetPixel(DeviceState* deviceState, VkFormat format, gsl::span<uint8_t> data, glm::uvec1 range, glm::ivec1 coordinates, glm::ivec4 borderColour)
{
	if (coordinates.x < 0 || coordinates.x >= range.x)
	{
		return borderColour;
	}

	const auto& information = GetFormatInformation(format);
	const auto offset = GetImagePixelOffset(GetImageSize(information, range.x, 1, 1, 1, 1), coordinates.x, 0, 0, 0, 0);
	auto functions = deviceState->getImageFunctions(format);

	if (!functions->GetPixelI32)
	{
		functions->GetPixelI32 = reinterpret_cast<decltype(functions->GetPixelI32)>(CompileGetPixelI32(deviceState->jit, &information));
	}

	uint32_t values[4]{};
	functions->GetPixelI32(data.subspan(offset, information.TotalSize).data(), values);
	return glm::ivec4(values[0], values[1], values[2], values[3]);
}

template<>
glm::ivec4 GetPixel(DeviceState* deviceState, VkFormat format, gsl::span<uint8_t> data, glm::uvec2 range, glm::ivec2 coordinates, glm::ivec4 borderColour)
{
	if (coordinates.x < 0 || coordinates.x >= range.x || coordinates.y < 0 || coordinates.y >= range.y)
	{
		return borderColour;
	}

	const auto& information = GetFormatInformation(format);
	const auto offset = GetImagePixelOffset(GetImageSize(information, range.x, range.y, 1, 1, 1), coordinates.x, coordinates.y, 0, 0, 0);
	auto functions = deviceState->getImageFunctions(format);

	if (!functions->GetPixelI32)
	{
		functions->GetPixelI32 = reinterpret_cast<decltype(functions->GetPixelI32)>(CompileGetPixelI32(deviceState->jit, &information));
	}

	uint32_t values[4]{};
	functions->GetPixelI32(data.subspan(offset, information.TotalSize).data(), values);
	return glm::ivec4(values[0], values[1], values[2], values[3]);
}

template<>
glm::ivec4 GetPixel(DeviceState* deviceState, VkFormat format, gsl::span<uint8_t> data, glm::uvec3 range, glm::ivec3 coordinates, glm::ivec4 borderColour)
{
	if (coordinates.x < 0 || coordinates.x >= range.x || coordinates.y < 0 || coordinates.y >= range.y || coordinates.z < 0 || coordinates.z >= range.z)
	{
		return borderColour;
	}

	const auto& information = GetFormatInformation(format);
	const auto offset = GetImagePixelOffset(GetImageSize(information, range.x, range.y, range.z, 1, 1), coordinates.x, coordinates.y, coordinates.z, 0, 0);
	auto functions = deviceState->getImageFunctions(format);

	if (!functions->GetPixelI32)
	{
		functions->GetPixelI32 = reinterpret_cast<decltype(functions->GetPixelI32)>(CompileGetPixelI32(deviceState->jit, &information));
	}

	uint32_t values[4]{};
	functions->GetPixelI32(data.subspan(offset, information.TotalSize).data(), values);
	return glm::ivec4(values[0], values[1], values[2], values[3]);
}

template<>
glm::uvec4 GetPixel(DeviceState* deviceState, VkFormat format, gsl::span<uint8_t> data, glm::uvec1 range, glm::ivec1 coordinates, glm::uvec4 borderColour)
{
	if (coordinates.x < 0 || coordinates.x >= range.x)
	{
		return borderColour;
	}

	const auto& information = GetFormatInformation(format);
	const auto offset = GetImagePixelOffset(GetImageSize(information, range.x, 1, 1, 1, 1), coordinates.x, 0, 0, 0, 0);
	auto functions = deviceState->getImageFunctions(format);

	if (!functions->GetPixelU32)
	{
		functions->GetPixelU32 = reinterpret_cast<decltype(functions->GetPixelU32)>(CompileGetPixelU32(deviceState->jit, &information));
	}

	uint32_t values[4]{};
	functions->GetPixelU32(data.subspan(offset, information.TotalSize).data(), values);
	return glm::uvec4(values[0], values[1], values[2], values[3]);
}

template<>
glm::uvec4 GetPixel(DeviceState* deviceState, VkFormat format, gsl::span<uint8_t> data, glm::uvec2 range, glm::ivec2 coordinates, glm::uvec4 borderColour)
{
	if (coordinates.x < 0 || coordinates.x >= range.x || coordinates.y < 0 || coordinates.y >= range.y)
	{
		return borderColour;
	}

	const auto& information = GetFormatInformation(format);
	const auto offset = GetImagePixelOffset(GetImageSize(information, range.x, range.y, 1, 1, 1), coordinates.x, coordinates.y, 0, 0, 0);
	auto functions = deviceState->getImageFunctions(format);

	if (!functions->GetPixelU32)
	{
		functions->GetPixelU32 = reinterpret_cast<decltype(functions->GetPixelU32)>(CompileGetPixelU32(deviceState->jit, &information));
	}

	uint32_t values[4]{};
	functions->GetPixelU32(data.subspan(offset, information.TotalSize).data(), values);
	return glm::uvec4(values[0], values[1], values[2], values[3]);
}

template<>
glm::uvec4 GetPixel(DeviceState* deviceState, VkFormat format, gsl::span<uint8_t> data, glm::uvec3 range, glm::ivec3 coordinates, glm::uvec4 borderColour)
{
	if (coordinates.x < 0 || coordinates.x >= range.x || coordinates.y < 0 || coordinates.y >= range.y || coordinates.z < 0 || coordinates.z >= range.z)
	{
		return borderColour;
	}

	const auto& information = GetFormatInformation(format);
	const auto offset = GetImagePixelOffset(GetImageSize(information, range.x, range.y, range.z, 1, 1), coordinates.x, coordinates.y, coordinates.z, 0, 0);
	auto functions = deviceState->getImageFunctions(format);

	if (!functions->GetPixelU32)
	{
		functions->GetPixelU32 = reinterpret_cast<decltype(functions->GetPixelU32)>(CompileGetPixelU32(deviceState->jit, &information));
	}

	uint32_t values[4]{};
	functions->GetPixelU32(data.subspan(offset, information.TotalSize).data(), values);
	return glm::uvec4(values[0], values[1], values[2], values[3]);
}

template<typename ResultType, int length>
ResultType GetPixelLinear(DeviceState* deviceState, VkFormat format, gsl::span<uint8_t> data, 
                          glm::vec<length, uint32_t> range, glm::vec<length, int32_t> newCoordinates0, glm::vec<length, int32_t> newCoordinates1, glm::vec<length, float> interpolation, ResultType borderColour)
{
	if constexpr (length == 1)
	{
		const auto i0 = GetPixel<ResultType>(deviceState, format, data, range, newCoordinates0, borderColour);
		const auto i1 = GetPixel<ResultType>(deviceState, format, data, range, newCoordinates1, borderColour);
		return lerp(i0, i1, interpolation.x);
	}
	
	if constexpr (length == 2)
	{
		const auto i0j0 = GetPixel<ResultType>(deviceState, format, data, range, glm::ivec2{newCoordinates0.x, newCoordinates0.y}, borderColour);
		const auto i0j1 = GetPixel<ResultType>(deviceState, format, data, range, glm::ivec2{newCoordinates0.x, newCoordinates1.y}, borderColour);
		const auto i1j0 = GetPixel<ResultType>(deviceState, format, data, range, glm::ivec2{newCoordinates1.x, newCoordinates0.y}, borderColour);
		const auto i1j1 = GetPixel<ResultType>(deviceState, format, data, range, glm::ivec2{newCoordinates1.x, newCoordinates1.y}, borderColour);
	
		const auto ij0 = lerp(i0j0, i1j0, interpolation.x);
		const auto ij1 = lerp(i0j1, i1j1, interpolation.x);
	
		return lerp(ij0, ij1, interpolation.y);
	}
	
	if constexpr (length == 3)
	{
		const auto i0j0k0 = GetPixel<ResultType>(deviceState, format, data, range, glm::ivec3{newCoordinates0.x, newCoordinates0.y, newCoordinates0.z}, borderColour);
		const auto i0j0k1 = GetPixel<ResultType>(deviceState, format, data, range, glm::ivec3{newCoordinates0.x, newCoordinates0.y, newCoordinates1.z}, borderColour);
		const auto i0j1k0 = GetPixel<ResultType>(deviceState, format, data, range, glm::ivec3{newCoordinates0.x, newCoordinates1.y, newCoordinates0.z}, borderColour);
		const auto i0j1k1 = GetPixel<ResultType>(deviceState, format, data, range, glm::ivec3{newCoordinates0.x, newCoordinates1.y, newCoordinates1.z}, borderColour);
		const auto i1j0k0 = GetPixel<ResultType>(deviceState, format, data, range, glm::ivec3{newCoordinates1.x, newCoordinates0.y, newCoordinates0.z}, borderColour);
		const auto i1j0k1 = GetPixel<ResultType>(deviceState, format, data, range, glm::ivec3{newCoordinates1.x, newCoordinates0.y, newCoordinates1.z}, borderColour);
		const auto i1j1k0 = GetPixel<ResultType>(deviceState, format, data, range, glm::ivec3{newCoordinates1.x, newCoordinates1.y, newCoordinates0.z}, borderColour);
		const auto i1j1k1 = GetPixel<ResultType>(deviceState, format, data, range, glm::ivec3{newCoordinates1.x, newCoordinates1.y, newCoordinates1.z}, borderColour);
		
		const auto ij0k0 = lerp(i0j0k0, i1j0k0, interpolation.x);
		const auto ij0k1 = lerp(i0j0k1, i1j0k1, interpolation.x);
		const auto ij1k0 = lerp(i0j1k0, i1j1k0, interpolation.x);
		const auto ij1k1 = lerp(i0j1k1, i1j1k1, interpolation.x);
		
		const auto ijk0 = lerp(ij0k0, ij1k0, interpolation.y);
		const auto ijk1 = lerp(ij0k1, ij1k1, interpolation.y);
		
		return lerp(ijk0, ijk1, interpolation.z);
	}

	FATAL_ERROR();
}

template<typename ResultType, int length, typename Op>
ResultType GetPixelMinMax(DeviceState* deviceState, VkFormat format, gsl::span<uint8_t> data,
                          glm::vec<length, uint32_t> range, glm::vec<length, int32_t> newCoordinates0, glm::vec<length, int32_t> newCoordinates1, ResultType borderColour, Op op)
{
	if constexpr (length == 1)
	{
		const auto i0 = GetPixel<ResultType>(deviceState, format, data, range, newCoordinates0, borderColour);
		const auto i1 = GetPixel<ResultType>(deviceState, format, data, range, newCoordinates1, borderColour);
		return op(i0, i1);
	}
	
	if constexpr (length == 2)
	{
		const auto i0j0 = GetPixel<ResultType>(deviceState, format, data, range, glm::ivec2{newCoordinates0.x, newCoordinates0.y}, borderColour);
		const auto i0j1 = GetPixel<ResultType>(deviceState, format, data, range, glm::ivec2{newCoordinates0.x, newCoordinates1.y}, borderColour);
		const auto i1j0 = GetPixel<ResultType>(deviceState, format, data, range, glm::ivec2{newCoordinates1.x, newCoordinates0.y}, borderColour);
		const auto i1j1 = GetPixel<ResultType>(deviceState, format, data, range, glm::ivec2{newCoordinates1.x, newCoordinates1.y}, borderColour);
	
		const auto ij0 = op(i0j0, i1j0);
		const auto ij1 = op(i0j1, i1j1);

		return op(ij0, ij1);
	}
	
	if constexpr (length == 3)
	{
		const auto i0j0k0 = GetPixel<ResultType>(deviceState, format, data, range, glm::ivec3{newCoordinates0.x, newCoordinates0.y, newCoordinates0.z}, borderColour);
		const auto i0j0k1 = GetPixel<ResultType>(deviceState, format, data, range, glm::ivec3{newCoordinates0.x, newCoordinates0.y, newCoordinates1.z}, borderColour);
		const auto i0j1k0 = GetPixel<ResultType>(deviceState, format, data, range, glm::ivec3{newCoordinates0.x, newCoordinates1.y, newCoordinates0.z}, borderColour);
		const auto i0j1k1 = GetPixel<ResultType>(deviceState, format, data, range, glm::ivec3{newCoordinates0.x, newCoordinates1.y, newCoordinates1.z}, borderColour);
		const auto i1j0k0 = GetPixel<ResultType>(deviceState, format, data, range, glm::ivec3{newCoordinates1.x, newCoordinates0.y, newCoordinates0.z}, borderColour);
		const auto i1j0k1 = GetPixel<ResultType>(deviceState, format, data, range, glm::ivec3{newCoordinates1.x, newCoordinates0.y, newCoordinates1.z}, borderColour);
		const auto i1j1k0 = GetPixel<ResultType>(deviceState, format, data, range, glm::ivec3{newCoordinates1.x, newCoordinates1.y, newCoordinates0.z}, borderColour);
		const auto i1j1k1 = GetPixel<ResultType>(deviceState, format, data, range, glm::ivec3{newCoordinates1.x, newCoordinates1.y, newCoordinates1.z}, borderColour);
		
		const auto ij0k0 = op(i0j0k0, i1j0k0);
		const auto ij0k1 = op(i0j0k1, i1j0k1);
		const auto ij1k0 = op(i0j1k0, i1j1k0);
		const auto ij1k1 = op(i0j1k1, i1j1k1);
		
		const auto ijk0 = op(ij0k0, ij1k0);
		const auto ijk1 = op(ij0k1, ij1k1);

		return op(ijk0, ijk1);
	}

	FATAL_ERROR();
}

template<typename ResultType, typename RangeType, typename CoordinateType>
ResultType SampleImageOfLevel(DeviceState* deviceState, const FormatInformation& format, gsl::span<uint8_t> data, RangeType range, CoordinateType coordinates, VkFilter filter,
                              const VkSamplerAddressMode addressMode[3], bool compareEnable, VkCompareOp compareOperation, VkBorderColor borderColour, bool unnormalisedCoordinates, VkSamplerReductionModeEXT reductionMode)
{
	static_assert(std::is_same<typename CoordinateType::value_type, float>::value);

	static ResultType borderColourConversion[6]
	{
		ResultType{0, 0, 0, 0},
		ResultType{0, 0, 0, 0},
		ResultType{0, 0, 0, 1},
		ResultType{0, 0, 0, 1},
		ResultType{1, 1, 1, 1},
		ResultType{1, 1, 1, 1},
	};
	
	using IntCoordinateType = glm::vec<CoordinateType::length(), int32_t>;

	if (compareEnable)
	{
		TODO_ERROR();
	}

	if (unnormalisedCoordinates)
	{
		TODO_ERROR();
	}

	switch (filter)
	{
	case VK_FILTER_NEAREST:
		{
			constexpr auto shift = 0.0f; // 0.0 for conventional, 0.5 for corner-sampled

			IntCoordinateType newCoordinates;
			if (format.Type == FormatType::Compressed)
			{
				if constexpr (CoordinateType::length() == 2)
				{
					uint32_t realRange[]
					{
						range.x * format.Compressed.BlockWidth,
						range.y * format.Compressed.BlockHeight
					};
					for (auto i = 0; i < IntCoordinateType::length(); i++)
					{
						newCoordinates[i] = static_cast<int32_t>(std::floor(coordinates[i] * realRange[i] + shift));
						newCoordinates[i] = wrap(newCoordinates[i], realRange[i], addressMode[i]);
					}
				}
				else
				{
					FATAL_ERROR();
				}
			}
			else
			{
				for (auto i = 0; i < IntCoordinateType::length(); i++)
				{
					newCoordinates[i] = static_cast<int32_t>(std::floor(coordinates[i] * range[i] + shift));
					newCoordinates[i] = wrap(newCoordinates[i], range[i], addressMode[i]);
				}
			}
			
			return GetPixel<ResultType>(deviceState, format.Format, data, range, newCoordinates, borderColourConversion[borderColour]);
		}
		
	case VK_FILTER_LINEAR:
		{
			if (format.Type == FormatType::Compressed)
			{
				TODO_ERROR();
			}

			constexpr auto shift = 0.5f; // 0.5 for conventional, 0.0 for corner-sampled

			IntCoordinateType newCoordinates0{};
			IntCoordinateType newCoordinates1{};

			for (auto i = 0; i < CoordinateType::length(); i++)
			{
				newCoordinates0[i] = static_cast<int32_t>(std::floor(coordinates[i] * range[i] - shift));

				newCoordinates1[i] = wrap(newCoordinates0[i] + 1, range[i], addressMode[i]);
				newCoordinates0[i] = wrap(newCoordinates0[i], range[i], addressMode[i]);
			}

			switch (reductionMode)
			{
			case VK_SAMPLER_REDUCTION_MODE_WEIGHTED_AVERAGE_EXT:
				{
					const auto interpolation = frac(coordinates * static_cast<CoordinateType>(range) - shift);
					return GetPixelLinear<ResultType, IntCoordinateType::length()>(deviceState, format.Format, data, range, newCoordinates0, newCoordinates1, interpolation, borderColourConversion[borderColour]);
				}

			case VK_SAMPLER_REDUCTION_MODE_MIN_EXT:
				{
					ResultType (*op)(ResultType const& a, ResultType const& b) = glm::min;
					return GetPixelMinMax<ResultType, IntCoordinateType::length()>(deviceState, format.Format, data, range, newCoordinates0, newCoordinates1, borderColourConversion[borderColour], op);
				}
				
			case VK_SAMPLER_REDUCTION_MODE_MAX_EXT:
				{
					ResultType (*op)(ResultType const& a, ResultType const& b) = glm::max;
					return GetPixelMinMax<ResultType, IntCoordinateType::length()>(deviceState, format.Format, data, range, newCoordinates0, newCoordinates1, borderColourConversion[borderColour], op);
				}
				
			default:
				FATAL_ERROR();
			}
		}
		
	case VK_FILTER_CUBIC_IMG:
		TODO_ERROR();
		
	default:
		FATAL_ERROR();
	}
}

template<typename ResultType, typename RangeType, typename CoordinateType>
ResultType SampleImage(DeviceState* deviceState, VkFormat format, gsl::span<uint8_t> data[MAX_MIP_LEVELS], RangeType range[MAX_MIP_LEVELS], uint32_t baseLevel, uint32_t levels, CoordinateType coordinates,
                       float lod, VkFilter magFilter, VkFilter minFilter, VkSamplerMipmapMode mipmapMode, VkSamplerAddressMode addressModeU, VkSamplerAddressMode addressModeV, VkSamplerAddressMode addressModeW,
                       bool anisotropyEnable, bool compareEnable, VkCompareOp compareOperation, VkBorderColor borderColour, bool unnormalisedCoordinates, VkSamplerReductionModeEXT reductionMode)
{
	static_assert(std::is_same<typename CoordinateType::value_type, float>::value);

	const auto& information = GetFormatInformation(format);

	const VkSamplerAddressMode addressMode[]
	{
		addressModeU,
		addressModeV,
		addressModeW,
	};

	if (anisotropyEnable)
	{
		TODO_ERROR();
	}

	if (compareEnable)
	{
		TODO_ERROR();
	}

	if (unnormalisedCoordinates)
	{
		TODO_ERROR();
	}

	ResultType result;
	if (lod <= 0)
	{
		result = SampleImageOfLevel<ResultType, RangeType, CoordinateType>(deviceState, information, data[0], range[0], coordinates, magFilter, addressMode,
		                                                                   compareEnable, compareOperation, borderColour, unnormalisedCoordinates, reductionMode);
	}
	else
	{
		const auto maxLevel = static_cast<float>(levels - 1);
		const auto mipLevel = std::clamp(lod, 0.0f, maxLevel);

		if (mipmapMode == VK_SAMPLER_MIPMAP_MODE_NEAREST)
		{
			const auto realMipLevel = static_cast<uint32_t>(std::ceil(mipLevel + 0.5f)) - 1;
			result = SampleImageOfLevel<ResultType, RangeType, CoordinateType>(deviceState, information, data[realMipLevel], range[realMipLevel], coordinates, minFilter, addressMode,
			                                                                   compareEnable, compareOperation, borderColour, unnormalisedCoordinates, reductionMode);
		}
		else
		{
			const auto mipLevel1 = static_cast<uint32_t>(floor(mipLevel));
			const auto delta = mipLevel - mipLevel1;
			const auto mipLevel2 = mipLevel1 + 1;

			const auto pixel1 = SampleImageOfLevel<ResultType, RangeType, CoordinateType>(deviceState, information, data[mipLevel1], range[mipLevel1], coordinates, minFilter, addressMode,
			                                                                              compareEnable, compareOperation, borderColour, unnormalisedCoordinates, reductionMode);;

			if (delta == 0)
			{
				result = pixel1;
			}
			else
			{
				const auto pixel2 = SampleImageOfLevel<ResultType, RangeType, CoordinateType>(deviceState, information, data[mipLevel2], range[mipLevel2], coordinates, minFilter, addressMode,
				                                                                              compareEnable, compareOperation, borderColour, unnormalisedCoordinates, reductionMode);;

				result = lerp(pixel1, pixel2, delta);
			}
		}
	}

	if (!(information.Channels & VK_COLOR_COMPONENT_R_BIT))
	{
		result.r = 0;
	}

	if (!(information.Channels & VK_COLOR_COMPONENT_G_BIT))
	{
		result.g = 0;
	}

	if (!(information.Channels & VK_COLOR_COMPONENT_B_BIT))
	{
		result.b = 0;
	}

	if (!(information.Channels & VK_COLOR_COMPONENT_A_BIT))
	{
		result.a = 1;
	}

	return result;
}

template<typename ResultType, typename RangeType, typename CoordinateType>
ResultType SampleImage(DeviceState* deviceState, VkFormat format, gsl::span<uint8_t> data, RangeType range, CoordinateType coordinates, VkFilter filter)
{
	return SampleImage<ResultType>(deviceState, format, &data, &range, 0, 1, coordinates, 1, filter, filter, VK_SAMPLER_MIPMAP_MODE_NEAREST,
	                               VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, false, false, VK_COMPARE_OP_NEVER,
	                               VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK, false, VK_SAMPLER_REDUCTION_MODE_WEIGHTED_AVERAGE_EXT);
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

template<typename ResultType, typename RangeType, typename CoordinateType>
ResultType SampleImage(DeviceState* deviceState, VkFormat format, gsl::span<uint8_t> data[MAX_MIP_LEVELS], RangeType range[MAX_MIP_LEVELS], uint32_t baseLevel, uint32_t levels, CoordinateType coordinates, float lod, Sampler* sampler)
{
	// TODO: 15.9.1. Wrapping Operation
	// Cube images ignore wrap modes
	if (sampler->getFlags() != 0)
	{
		TODO_ERROR();
	}
	
	return SampleImage<ResultType>(deviceState, format, data, range, baseLevel, levels, coordinates, lod, sampler->getMagFilter(), sampler->getMinFilter(), sampler->getMipmapMode(),
	                               sampler->getAddressModeU(), sampler->getAddressModeV(), sampler->getAddressModeW(), sampler->getAnisotropyEnable(), sampler->getCompareEnable(), 
	                               sampler->getCompareOp(), sampler->getBorderColour(), sampler->getUnnormalisedCoordinates(), sampler->getReductionMode());
}

template glm::fvec4 SampleImage(DeviceState* deviceState, VkFormat format, gsl::span<uint8_t> data[MAX_MIP_LEVELS], glm::uvec1 range[MAX_MIP_LEVELS], uint32_t baseLevel, uint32_t levels, glm::fvec1 coordinates, float lod, Sampler* sampler);
template glm::fvec4 SampleImage(DeviceState* deviceState, VkFormat format, gsl::span<uint8_t> data[MAX_MIP_LEVELS], glm::uvec2 range[MAX_MIP_LEVELS], uint32_t baseLevel, uint32_t levels, glm::fvec2 coordinates, float lod, Sampler* sampler);
template glm::fvec4 SampleImage(DeviceState* deviceState, VkFormat format, gsl::span<uint8_t> data[MAX_MIP_LEVELS], glm::uvec3 range[MAX_MIP_LEVELS], uint32_t baseLevel, uint32_t levels, glm::fvec3 coordinates, float lod, Sampler* sampler);

template glm::ivec4 SampleImage(DeviceState* deviceState, VkFormat format, gsl::span<uint8_t> data[MAX_MIP_LEVELS], glm::uvec1 range[MAX_MIP_LEVELS], uint32_t baseLevel, uint32_t levels, glm::fvec1 coordinates, float lod, Sampler* sampler);
template glm::ivec4 SampleImage(DeviceState* deviceState, VkFormat format, gsl::span<uint8_t> data[MAX_MIP_LEVELS], glm::uvec2 range[MAX_MIP_LEVELS], uint32_t baseLevel, uint32_t levels, glm::fvec2 coordinates, float lod, Sampler* sampler);
template glm::ivec4 SampleImage(DeviceState* deviceState, VkFormat format, gsl::span<uint8_t> data[MAX_MIP_LEVELS], glm::uvec3 range[MAX_MIP_LEVELS], uint32_t baseLevel, uint32_t levels, glm::fvec3 coordinates, float lod, Sampler* sampler);

template glm::uvec4 SampleImage(DeviceState* deviceState, VkFormat format, gsl::span<uint8_t> data[MAX_MIP_LEVELS], glm::uvec1 range[MAX_MIP_LEVELS], uint32_t baseLevel, uint32_t levels, glm::fvec1 coordinates, float lod, Sampler* sampler);
template glm::uvec4 SampleImage(DeviceState* deviceState, VkFormat format, gsl::span<uint8_t> data[MAX_MIP_LEVELS], glm::uvec2 range[MAX_MIP_LEVELS], uint32_t baseLevel, uint32_t levels, glm::fvec2 coordinates, float lod, Sampler* sampler);
template glm::uvec4 SampleImage(DeviceState* deviceState, VkFormat format, gsl::span<uint8_t> data[MAX_MIP_LEVELS], glm::uvec3 range[MAX_MIP_LEVELS], uint32_t baseLevel, uint32_t levels, glm::fvec3 coordinates, float lod, Sampler* sampler);


void SetPixel(DeviceState* deviceState, VkFormat format, Image* image, int32_t i, int32_t j, int32_t k, uint32_t mipLevel, uint32_t layer, VkClearDepthStencilValue value)
{
	const auto& information = GetFormatInformation(format);
	const auto offset = GetImagePixelOffset(image->getImageSize(), i, j, k, mipLevel, layer);
	auto functions = deviceState->getImageFunctions(format);
	if (!functions->SetPixelDepthStencil)
	{
		functions->SetPixelDepthStencil = reinterpret_cast<decltype(functions->SetPixelDepthStencil)>(CompileSetPixelDepthStencil(deviceState->jit, &information));
	}

	functions->SetPixelDepthStencil(image->getDataPtr(offset, information.TotalSize), value.depth, value.stencil);
}

void SetPixel(DeviceState* deviceState, VkFormat format, Image* image, int32_t i, int32_t j, int32_t k, uint32_t mipLevel, uint32_t layer, VkClearColorValue value)
{
	switch (GetFormatInformation(format).Base)
	{
	case BaseType::UNorm:
	case BaseType::SNorm:
	case BaseType::UScaled:
	case BaseType::SScaled:
	case BaseType::UFloat:
	case BaseType::SFloat:
	case BaseType::SRGB:
		SetPixel(deviceState, format, image, i, j, k, mipLevel, layer, value.float32);
		break;

	case BaseType::UInt:
		SetPixel(deviceState, format, image, i, j, k, mipLevel, layer, value.uint32);
		break;
		
	case BaseType::SInt:
		SetPixel(deviceState, format, image, i, j, k, mipLevel, layer, value.int32);
		break;
		
	default:
		FATAL_ERROR();
	}
}

void SetPixel(DeviceState* deviceState, VkFormat format, Image* image, int32_t i, int32_t j, int32_t k, uint32_t mipLevel, uint32_t layer, float values[4])
{
	const auto& information = GetFormatInformation(format);
	const auto offset = GetImagePixelOffset(image->getImageSize(), i, j, k, mipLevel, layer);
	auto functions = deviceState->getImageFunctions(format);
	if (!functions->SetPixelF32)
	{
		functions->SetPixelF32 = reinterpret_cast<decltype(functions->SetPixelF32)>(CompileSetPixelF32(deviceState->jit, &information));
	}

	functions->SetPixelF32(image->getDataPtr(offset, information.TotalSize), values);
}

void SetPixel(DeviceState* deviceState, VkFormat format, Image* image, int32_t i, int32_t j, int32_t k, uint32_t mipLevel, uint32_t layer, int32_t values[4])
{
	const auto& information = GetFormatInformation(format);
	const auto offset = GetImagePixelOffset(image->getImageSize(), i, j, k, mipLevel, layer);
	auto functions = deviceState->getImageFunctions(format);
	if (!functions->SetPixelI32)
	{
		functions->SetPixelI32 = reinterpret_cast<decltype(functions->SetPixelI32)>(CompileSetPixelI32(deviceState->jit, &information));
	}

	functions->SetPixelI32(image->getDataPtr(offset, information.TotalSize), values);
}

void SetPixel(DeviceState* deviceState, VkFormat format, Image* image, int32_t i, int32_t j, int32_t k, uint32_t mipLevel, uint32_t layer, uint32_t values[4])
{
	const auto& information = GetFormatInformation(format);
	const auto offset = GetImagePixelOffset(image->getImageSize(), i, j, k, mipLevel, layer);
	auto functions = deviceState->getImageFunctions(format);
	if (!functions->SetPixelU32)
	{
		functions->SetPixelU32 = reinterpret_cast<decltype(functions->SetPixelU32)>(CompileSetPixelU32(deviceState->jit, &information));
	}

	functions->SetPixelU32(image->getDataPtr(offset, information.TotalSize), values);
}