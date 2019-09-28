#include "ImageSampler.h"

#include "Formats.h"
#include "Image.h"

#include <glm/glm.hpp>

static float frac(float value)
{
	return value - floor(value);
}

template<typename T>
static T lerp(T min, T max, float delta)
{
	return min + (max - min) * delta;
}

template<>
glm::ivec4 lerp(glm::ivec4 min, glm::ivec4 max, float delta)
{
	return glm::dvec4(min) + glm::dvec4(max - min) * double(delta);
}

static int32_t Wrap(int32_t v, int32_t size, VkSamplerAddressMode addressMode)
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
	}
	
	FATAL_ERROR();
}


template<>
void ConvertPixelsFromTemp<uint8_t>(const uint64_t input[4], float output[4])
{
	output[0] = static_cast<float>(static_cast<uint8_t>(input[0])) / std::numeric_limits<uint8_t>::max();
	output[1] = static_cast<float>(static_cast<uint8_t>(input[1])) / std::numeric_limits<uint8_t>::max();
	output[2] = static_cast<float>(static_cast<uint8_t>(input[2])) / std::numeric_limits<uint8_t>::max();
	output[3] = static_cast<float>(static_cast<uint8_t>(input[3])) / std::numeric_limits<uint8_t>::max();
}

template<>
void ConvertPixelsFromTemp<uint16_t>(const uint64_t input[4], float output[4])
{
	output[0] = static_cast<float>(static_cast<uint16_t>(input[0])) / std::numeric_limits<uint16_t>::max();
	output[1] = static_cast<float>(static_cast<uint16_t>(input[1])) / std::numeric_limits<uint16_t>::max();
	output[2] = static_cast<float>(static_cast<uint16_t>(input[2])) / std::numeric_limits<uint16_t>::max();
	output[3] = static_cast<float>(static_cast<uint16_t>(input[3])) / std::numeric_limits<uint16_t>::max();
}

template<>
void ConvertPixelsFromTemp<uint8_t>(const uint64_t input[4], int32_t output[4])
{
	output[0] = static_cast<uint8_t>(input[0]);
	output[1] = static_cast<uint8_t>(input[1]);
	output[2] = static_cast<uint8_t>(input[2]);
	output[3] = static_cast<uint8_t>(input[3]);
}

template<typename OutputType>
void ConvertPixelsFromTemp(const FormatInformation& format, const uint64_t input[4], OutputType output[])
{
	switch (format.Base)
	{
	case BaseType::UNorm:
	case BaseType::UInt:
		switch (format.ElementSize)
		{
		case 1:
			ConvertPixelsFromTemp<uint8_t, OutputType>(input, output);
			return;
		}
		break;

	case BaseType::SNorm: break;
	case BaseType::UScaled: break;
	case BaseType::SScaled: break;
	case BaseType::SInt: break;
	case BaseType::UFloat: break;
	case BaseType::SFloat: break;
	case BaseType::SRGB: break;
	}

	FATAL_ERROR();
}


template<>
void ConvertPixelsToTemp<float, uint8_t>(const float input[4], uint64_t output[4])
{
	output[0] = static_cast<uint64_t>(input[0] * static_cast<uint64_t>(std::numeric_limits<uint8_t>::max()));
	output[1] = static_cast<uint64_t>(input[1] * static_cast<uint64_t>(std::numeric_limits<uint8_t>::max()));
	output[2] = static_cast<uint64_t>(input[2] * static_cast<uint64_t>(std::numeric_limits<uint8_t>::max()));
	output[3] = static_cast<uint64_t>(input[3] * static_cast<uint64_t>(std::numeric_limits<uint8_t>::max()));
}

template<>
void ConvertPixelsToTemp<float, float>(const float input[4], uint64_t output[4])
{
	output[0] = *reinterpret_cast<const uint64_t*>(&input[0]);
	output[1] = *reinterpret_cast<const uint64_t*>(&input[1]);
	output[2] = *reinterpret_cast<const uint64_t*>(&input[2]);
	output[3] = *reinterpret_cast<const uint64_t*>(&input[3]);
}

template<>
void ConvertPixelsToTemp<int32_t, uint8_t>(const int32_t input[4], uint64_t output[4])
{
	output[0] = static_cast<uint8_t>(input[0]);
	output[1] = static_cast<uint8_t>(input[1]);
	output[2] = static_cast<uint8_t>(input[2]);
	output[3] = static_cast<uint8_t>(input[3]);
}

template<>
void ConvertPixelsToTemp<int32_t, float>(const int32_t input[4], uint64_t output[4])
{
	FATAL_ERROR();
}

template<typename InputType>
void ConvertPixelsToTemp(const FormatInformation& format, const InputType input[4], uint64_t output[4])
{
	switch (format.Base)
	{
	case BaseType::UNorm:
	case BaseType::UInt:
		switch (format.ElementSize)
		{
		case 1:
			ConvertPixelsToTemp<InputType, uint8_t>(input, output);
			return;
		}
		break;
		
	case BaseType::SNorm: break;
	case BaseType::UScaled: break;
	case BaseType::SScaled: break;
	case BaseType::SInt: break;
	case BaseType::UFloat: break;
		
	case BaseType::SFloat:
		switch (format.ElementSize)
		{
		case 4:
			ConvertPixelsToTemp<InputType, float>(input, output);
			return;
		}
		break;
		
	case BaseType::SRGB: break;
	}
	
	FATAL_ERROR();
}

template void ConvertPixelsToTemp(const FormatInformation& format, const float input[4], uint64_t output[4]);


template<typename Size>
void GetPixel(const FormatInformation& format, const void* data, uint32_t i, uint32_t j, uint32_t k, uint32_t width, uint32_t height, uint32_t depth, uint64_t values[4])
{
	static_assert(std::numeric_limits<Size>::is_integer);
	static_assert(!std::numeric_limits<Size>::is_signed);

	if (format.ElementSize != sizeof(Size))
	{
		FATAL_ERROR();
	}

	const auto pixel = reinterpret_cast<const Size*>(GetFormatPixelOffset(format, data, i, j, k, width, height, depth, 0));
	if (format.RedOffset != -1) values[0] = static_cast<uint64_t>(pixel[format.RedOffset]);
	if (format.GreenOffset != -1) values[1] = static_cast<uint64_t>(pixel[format.GreenOffset]);
	if (format.BlueOffset != -1) values[2] = static_cast<uint64_t>(pixel[format.BlueOffset]);
	if (format.AlphaOffset != -1) values[3] = static_cast<uint64_t>(pixel[format.AlphaOffset]);
}

template<typename OutputType>
void GetPixel(const FormatInformation& format, const Image* image, int32_t i, int32_t j, int32_t k, OutputType output[4])
{
	// TODO: Border colour
	assert(i >= 0 && j >= 0 && k >= 0 && i < image->getWidth() && j < image->getHeight() && k < image->getDepth());

	uint64_t rawValues[4]{};
	if (format.Base == BaseType::UNorm || format.Base == BaseType::UInt)
	{
		if (format.ElementSize == 1)
		{
			GetPixel<uint8_t>(format, image->getDataPtr(0, 1), i, j, k, image->getWidth(), image->getHeight(), image->getDepth(), rawValues);
		}
		else if (format.ElementSize == 2)
		{
			GetPixel<uint16_t>(format, image->getDataPtr(0, 1), i, j, k, image->getWidth(), image->getHeight(), image->getDepth(), rawValues);
		}
		else
		{
			FATAL_ERROR();
		}
	}
	else
	{
		FATAL_ERROR();
	}
	
	ConvertPixelsFromTemp<OutputType>(format, rawValues, output);
}

template<typename OutputType>
OutputType GetPixel(const FormatInformation& format, const Image* image, int32_t i, int32_t j, int32_t k)
{
	static_assert(OutputType::length() == 4);
	OutputType result{};
	GetPixel<typename OutputType::value_type>(format, image, i, j, k, &result.x);
	return result;
}


template<typename Size>
void SetPixel(const FormatInformation& format, void* data, uint32_t i, uint32_t j, uint32_t k, uint32_t width, uint32_t height, uint32_t depth, uint32_t mipLevel, const uint64_t values[4])
{
	static_assert(std::numeric_limits<Size>::is_integer);
	static_assert(!std::numeric_limits<Size>::is_signed);
	
	if (format.ElementSize != sizeof(Size))
	{
		FATAL_ERROR();
	}

	const auto pixel = reinterpret_cast<Size*>(GetFormatPixelOffset(format, data, i, j, k, width, height, depth, mipLevel));
	if (format.RedOffset != -1) pixel[format.RedOffset] = static_cast<Size>(values[0]);
	if (format.GreenOffset != -1) pixel[format.GreenOffset] = static_cast<Size>(values[1]);
	if (format.BlueOffset != -1) pixel[format.BlueOffset] = static_cast<Size>(values[2]);
	if (format.AlphaOffset != -1) pixel[format.AlphaOffset] = static_cast<Size>(values[3]);
}

template<typename Size>
static void SetPackedPixel(const FormatInformation& format, void* data, uint32_t i, uint32_t j, uint32_t k, uint32_t width, uint32_t height, uint32_t depth, uint32_t mipLevel, uint64_t values[4])
{
	static_assert(std::numeric_limits<Size>::is_integer);
	static_assert(!std::numeric_limits<Size>::is_signed);

	const auto pixel = reinterpret_cast<Size*>(GetFormatPixelOffset(format, data, i, j, k, width, height, depth, mipLevel));
	*pixel = static_cast<Size>(values[0]);
}

void SetPixel(const FormatInformation& format, Image* image, uint32_t i, uint32_t j, uint32_t k, uint32_t mipLevel, uint64_t values[4])
{
	const auto width = image->getWidth();
	const auto height = image->getHeight();
	const auto depth = image->getDepth();
	
	assert(i < width);
	assert(j < height);
	assert(k < depth);
	
	switch (format.Base)
	{
	case BaseType::UScaled:
	case BaseType::SScaled:
		FATAL_ERROR();
	
	case BaseType::SRGB:
		FATAL_ERROR();
	}
	
	if (format.ElementSize == 0 && format.TotalSize == 1)
	{
		SetPackedPixel<uint8_t>(format, image->getDataPtr(0, 1), i, j, k, width, height, depth, mipLevel, values);
	}
	else if (format.ElementSize == 0 && format.TotalSize == 2)
	{
		SetPackedPixel<uint16_t>(format, image->getDataPtr(0, 1), i, j, k, width, height, depth, mipLevel, values);
	}
	else if (format.ElementSize == 0 && format.TotalSize == 4)
	{
		SetPackedPixel<uint32_t>(format, image->getDataPtr(0, 1), i, j, k, width, height, depth, mipLevel, values);
	}
	else if (format.ElementSize == 1)
	{
		SetPixel<uint8_t>(format, image->getDataPtr(0, 1), i, j, k, width, height, depth, mipLevel, values);
	}
	else if (format.ElementSize == 2)
	{
		SetPixel<uint16_t>(format, image->getDataPtr(0, 1), i, j, k, width, height, depth, mipLevel, values);
	}
	else if (format.ElementSize == 4)
	{
		SetPixel<uint32_t>(format, image->getDataPtr(0, 1), i, j, k, width, height, depth, mipLevel, values);
	}
	else
	{
		FATAL_ERROR();
	}
}


template<typename OutputType>
void SampleImage(const FormatInformation& format, const Image* image, float u, float v, float w, float q, float a, VkFilter filter, VkSamplerMipmapMode mipmapMode, VkSamplerAddressMode addressMode, uint64_t output[4])
{
	auto pixels = SampleImage<OutputType>(image, u, v, w, q, a, filter, mipmapMode, addressMode);
	ConvertPixelsToTemp(format, &pixels.x, output);
}

template<typename ReturnType>
ReturnType SampleImage(const Image* image, float u, float v, float w, float q, float a, VkFilter filter, VkSamplerMipmapMode mipmapMode, VkSamplerAddressMode addressMode)
{
	// TODO: Optimise for 1D/2D
	const auto& format = GetFormatInformation(image->getFormat());
	
	if (mipmapMode != VK_SAMPLER_MIPMAP_MODE_NEAREST)
	{
		FATAL_ERROR();
	}

	if (q != 0)
	{
		FATAL_ERROR();
	}

	if (a != 0)
	{
		FATAL_ERROR();
	}

	switch (filter)
	{
	case VK_FILTER_NEAREST:
		{
			constexpr auto shift = 0.0f; // 0.0 for conventional, 0.5 for corner-sampled
			
			auto i = static_cast<int32_t>(std::floor(u + shift));
			auto j = static_cast<int32_t>(std::floor(v + shift));
			auto k = static_cast<int32_t>(std::floor(w + shift));

			i = Wrap(i, image->getWidth(), addressMode);
			j = Wrap(j, image->getHeight(), addressMode);
			k = Wrap(k, image->getDepth(), addressMode);
			
			return GetPixel<ReturnType>(format, image, i, j, k);
		}
		
	case VK_FILTER_LINEAR:
		{
			constexpr auto shift = 0.5f; // 0.5 for conventional, 0.0 for corner-sampled
			
			auto i0 = static_cast<int32_t>(std::floor(u - shift));
			auto j0 = static_cast<int32_t>(std::floor(v - shift));
			auto k0 = static_cast<int32_t>(std::floor(w - shift));
			
			const auto i1 = Wrap(i0 + 1, image->getWidth(), addressMode);
			const auto j1 = Wrap(j0 + 1, image->getHeight(), addressMode);
			const auto k1 = Wrap(k0 + 1, image->getDepth(), addressMode);

			i0 = Wrap(i0, image->getWidth(), addressMode);
			j0 = Wrap(j0, image->getHeight(), addressMode);
			k0 = Wrap(k0, image->getDepth(), addressMode);
			
			const auto alpha = frac(u - shift);
			const auto beta = frac(v - shift);
			const auto gamma = frac(w - shift);
			
			const auto i0j0k0 = GetPixel<ReturnType>(format, image, i0, j0, k0);
			const auto i0j0k1 = GetPixel<ReturnType>(format, image, i0, j0, k1);
			const auto i0j1k0 = GetPixel<ReturnType>(format, image, i0, j1, k0);
			const auto i0j1k1 = GetPixel<ReturnType>(format, image, i0, j1, k1);
			const auto i1j0k0 = GetPixel<ReturnType>(format, image, i1, j0, k0);
			const auto i1j0k1 = GetPixel<ReturnType>(format, image, i1, j0, k1);
			const auto i1j1k0 = GetPixel<ReturnType>(format, image, i1, j1, k0);
			const auto i1j1k1 = GetPixel<ReturnType>(format, image, i1, j1, k1);

			const auto ij0k0 = lerp(i0j0k0, i1j0k0, alpha);
			const auto ij0k1 = lerp(i0j0k1, i1j0k1, alpha);
			const auto ij1k0 = lerp(i0j1k0, i1j1k0, alpha);
			const auto ij1k1 = lerp(i0j1k1, i1j1k1, alpha);

			const auto ijk0 = lerp(ij0k0, ij1k0, beta);
			const auto ijk1 = lerp(ij0k1, ij1k1, beta);

			return lerp(ijk0, ijk1, gamma);
		}
		
	case VK_FILTER_CUBIC_IMG:
		FATAL_ERROR();
		break;
		
	default:
		FATAL_ERROR();
	}
	
	FATAL_ERROR();
}

template void SampleImage<glm::vec4>(const FormatInformation& format, const Image* image, float u, float v, float w, float q, float a, VkFilter filter, VkSamplerMipmapMode mipmapMode, VkSamplerAddressMode addressMode, uint64_t output[4]);
template glm::vec4 SampleImage(const Image* image, float u, float v, float w, float q, float a, VkFilter filter, VkSamplerMipmapMode mipmapMode, VkSamplerAddressMode addressMode);

template void SampleImage<glm::ivec4>(const FormatInformation& format, const Image* image, float u, float v, float w, float q, float a, VkFilter filter, VkSamplerMipmapMode mipmapMode, VkSamplerAddressMode addressMode, uint64_t output[4]);
template glm::ivec4 SampleImage(const Image* image, float u, float v, float w, float q, float a, VkFilter filter, VkSamplerMipmapMode mipmapMode, VkSamplerAddressMode addressMode);
