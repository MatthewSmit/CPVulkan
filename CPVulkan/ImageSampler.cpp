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

static int32_t Wrap(int32_t i, int32_t size, VkSamplerAddressMode addressMode)
{
	switch (addressMode)
	{
	case VK_SAMPLER_ADDRESS_MODE_REPEAT:
		return i % size;
		
	case VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT:
		{
			const auto n = i % (2 * size) - size;
			return size - 1 - (n >= 0 ? n : -(1 + n));
		}
		
	case VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE:
		return std::clamp(i, 0, size - 1);
		
	case VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER:
		return std::clamp(i, -1, size);
		
	case VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE:
		return std::clamp(i >= 0 ? i : -(1 + i), 0, size - 1);
	}
	
	FATAL_ERROR();
}

template<typename Size>
static void GetPixel(void* data, const FormatInformation& format, uint32_t i, uint32_t j, uint32_t k, uint32_t width, uint32_t height, uint32_t depth, uint64_t values[4])
{
	static_assert(std::numeric_limits<Size>::is_integer);
	static_assert(!std::numeric_limits<Size>::is_signed);

	if (format.ElementSize != sizeof(Size))
	{
		FATAL_ERROR();
	}

	const auto pixelSize = static_cast<uint64_t>(format.TotalSize);
	const auto stride = width * pixelSize;
	const auto pane = height * stride;

	auto pixel = reinterpret_cast<Size*>(reinterpret_cast<uint8_t*>(data) +
		static_cast<uint64_t>(k) * pane +
		static_cast<uint64_t>(j) * stride +
		static_cast<uint64_t>(i) * pixelSize);

	if (format.RedOffset != -1) values[0] = static_cast<uint64_t>(pixel[format.RedOffset]);
	if (format.GreenOffset != -1) values[1] = static_cast<uint64_t>(pixel[format.GreenOffset]);
	if (format.BlueOffset != -1) values[2] = static_cast<uint64_t>(pixel[format.BlueOffset]);
	if (format.AlphaOffset != -1) values[3] = static_cast<uint64_t>(pixel[format.AlphaOffset]);
}

template<typename InputType, typename ReturnType>
static ReturnType Convert(const uint64_t source[4]);

template<>
inline glm::vec4 Convert<uint8_t>(const uint64_t source[4])
{
	return glm::vec4
	{
		static_cast<float>(static_cast<uint8_t>(source[0])) / std::numeric_limits<uint8_t>::max(),
		static_cast<float>(static_cast<uint8_t>(source[1])) / std::numeric_limits<uint8_t>::max(),
		static_cast<float>(static_cast<uint8_t>(source[2])) / std::numeric_limits<uint8_t>::max(),
		static_cast<float>(static_cast<uint8_t>(source[3])) / std::numeric_limits<uint8_t>::max(),
	};
}

template<>
inline glm::vec4 Convert<uint16_t>(const uint64_t source[4])
{
	return glm::vec4
	{
		static_cast<float>(static_cast<uint16_t>(source[0])) / std::numeric_limits<uint16_t>::max(),
		static_cast<float>(static_cast<uint16_t>(source[1])) / std::numeric_limits<uint16_t>::max(),
		static_cast<float>(static_cast<uint16_t>(source[2])) / std::numeric_limits<uint16_t>::max(),
		static_cast<float>(static_cast<uint16_t>(source[3])) / std::numeric_limits<uint16_t>::max(),
	};
}

template<typename ReturnType>
ReturnType GetPixel(const FormatInformation& format, Image* image, int32_t i, int32_t j, int32_t k)
{
	assert(i >= 0 && j >= 0 && k >= 0 && i < image->getWidth() && j < image->getHeight() && k < image->getDepth());
	
	if (format.Base == BaseType::UNorm)
	{
		if (format.ElementSize == 1)
		{
			uint64_t rawValues[4];
			GetPixel<uint8_t>(image->getData(), format, i, j, k, image->getWidth(), image->getHeight(), image->getDepth(), rawValues);
			return Convert<uint8_t, ReturnType>(rawValues);
		}

		if (format.ElementSize == 2)
		{
			uint64_t rawValues[4];
			GetPixel<uint16_t>(image->getData(), format, i, j, k, image->getWidth(), image->getHeight(), image->getDepth(), rawValues);
			return Convert<uint16_t, ReturnType>(rawValues);
		}

		FATAL_ERROR();
	}

	FATAL_ERROR();
}

template<typename ReturnType>
ReturnType SampleImage(Image* image, float u, float v, float w, float q, float a, VkFilter filter, VkSamplerMipmapMode mipmapMode, VkSamplerAddressMode addressMode)
{
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
		FATAL_ERROR();
		break;
		
	case VK_FILTER_LINEAR:
		{
			constexpr auto shift = 0.5f; // 0.5 for conventional, 0.0 for corner-sampled
			
			auto i0 = static_cast<int32_t>(std::floor(u - shift));
			auto j0 = static_cast<int32_t>(std::floor(v - shift));
			auto k0 = static_cast<int32_t>(std::floor(w - shift));
			
			const auto i1 = Wrap(i0 + 1, image->getWidth(), addressMode);
			const auto j1 = Wrap(i0 + 1, image->getHeight(), addressMode);
			const auto k1 = Wrap(i0 + 1, image->getDepth(), addressMode);

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

template glm::vec4 SampleImage(Image* image, float u, float v, float w, float q, float a, VkFilter filter, VkSamplerMipmapMode mipmapMode, VkSamplerAddressMode addressMode);