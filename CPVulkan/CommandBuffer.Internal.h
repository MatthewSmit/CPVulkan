#pragma once
#include "Base.h"

#include "Formats.h"
#include "Image.h"

class Image;

template<typename Size>
static void SetPixel(void* data, const FormatInformation& format, uint32_t x, uint32_t y, uint32_t z, uint32_t width, uint32_t height, uint32_t depth, uint64_t values[4])
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
		static_cast<uint64_t>(z) * pane +
		static_cast<uint64_t>(y) * stride +
		static_cast<uint64_t>(x) * pixelSize);

	if (format.RedOffset != -1) pixel[format.RedOffset] = static_cast<Size>(values[0]);
	if (format.GreenOffset != -1) pixel[format.GreenOffset] = static_cast<Size>(values[1]);
	if (format.BlueOffset != -1) pixel[format.BlueOffset] = static_cast<Size>(values[2]);
	if (format.AlphaOffset != -1) pixel[format.AlphaOffset] = static_cast<Size>(values[3]);
}

template<typename Size>
static void SetPackedPixel(void* data, const FormatInformation& format, uint32_t x, uint32_t y, uint32_t z, uint32_t width, uint32_t height, uint32_t depth, uint64_t values[4])
{
	static_assert(std::numeric_limits<Size>::is_integer);
	static_assert(!std::numeric_limits<Size>::is_signed);

	const auto pixelSize = static_cast<uint64_t>(format.TotalSize);
	const auto stride = width * pixelSize;
	const auto pane = height * stride;

	auto pixel = reinterpret_cast<Size*>(reinterpret_cast<uint8_t*>(data) +
		static_cast<uint64_t>(z) * pane +
		static_cast<uint64_t>(y) * stride +
		static_cast<uint64_t>(x) * pixelSize);

	*pixel = static_cast<Size>(values[0]);
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

static void SetPixel(const FormatInformation& format, Image* image, uint32_t x, uint32_t y, uint32_t z, uint64_t values[4])
{
	const auto width = image->getWidth();
	const auto height = image->getHeight();
	const auto depth = image->getDepth();

	assert(x < width);
	assert(y < height);
	assert(z < depth);

	switch (format.Base)
	{
	case BaseType::UScaled:
	case BaseType::SScaled:
		FATAL_ERROR();

	case BaseType::UFloat:
	case BaseType::SFloat:
		FATAL_ERROR();

	case BaseType::SRGB:
		FATAL_ERROR();
	}

	if (format.ElementSize == 0 && format.TotalSize == 1)
	{
		SetPackedPixel<uint8_t>(image->getData(), format, x, y, z, width, height, depth, values);
	}
	else if (format.ElementSize == 0 && format.TotalSize == 2)
	{
		SetPackedPixel<uint16_t>(image->getData(), format, x, y, z, width, height, depth, values);
	}
	else if (format.ElementSize == 0 && format.TotalSize == 4)
	{
		SetPackedPixel<uint32_t>(image->getData(), format, x, y, z, width, height, depth, values);
	}
	else if (format.ElementSize == 1)
	{
		SetPixel<uint8_t>(image->getData(), format, x, y, z, width, height, depth, values);
	}
	else if (format.ElementSize == 2)
	{
		SetPixel<uint16_t>(image->getData(), format, x, y, z, width, height, depth, values);
	}
	else if (format.ElementSize == 4)
	{
		SetPixel<uint32_t>(image->getData(), format, x, y, z, width, height, depth, values);
	}
	else
	{
		FATAL_ERROR();
	}
}

template<typename InputType, typename OutputType>
static void Convert(const uint64_t source[4], OutputType destination[4]);

template<typename InputType, typename OutputType>
static void Convert(const InputType source[4], uint64_t destination[4]);

template<>
inline void Convert<uint8_t, float>(const uint64_t source[4], float destination[4])
{
	destination[0] = static_cast<float>(static_cast<uint8_t>(source[0])) / std::numeric_limits<uint8_t>::max();
	destination[1] = static_cast<float>(static_cast<uint8_t>(source[1])) / std::numeric_limits<uint8_t>::max();
	destination[2] = static_cast<float>(static_cast<uint8_t>(source[2])) / std::numeric_limits<uint8_t>::max();
	destination[3] = static_cast<float>(static_cast<uint8_t>(source[3])) / std::numeric_limits<uint8_t>::max();
}

template<>
inline void Convert<uint16_t, float>(const uint64_t source[4], float destination[4])
{
	destination[0] = static_cast<float>(static_cast<uint16_t>(source[0])) / std::numeric_limits<uint16_t>::max();
	destination[1] = static_cast<float>(static_cast<uint16_t>(source[1])) / std::numeric_limits<uint16_t>::max();
	destination[2] = static_cast<float>(static_cast<uint16_t>(source[2])) / std::numeric_limits<uint16_t>::max();
	destination[3] = static_cast<float>(static_cast<uint16_t>(source[3])) / std::numeric_limits<uint16_t>::max();
}

template<>
inline void Convert<float, uint8_t>(const float source[4], uint64_t destination[4])
{
	destination[0] = static_cast<uint64_t>(source[0] * static_cast<uint64_t>(std::numeric_limits<uint8_t>::max()));
	destination[1] = static_cast<uint64_t>(source[1] * static_cast<uint64_t>(std::numeric_limits<uint8_t>::max()));
	destination[2] = static_cast<uint64_t>(source[2] * static_cast<uint64_t>(std::numeric_limits<uint8_t>::max()));
	destination[3] = static_cast<uint64_t>(source[3] * static_cast<uint64_t>(std::numeric_limits<uint8_t>::max()));
}

template<typename InputType>
static void Convert(const FormatInformation& format, InputType input[4], uint64_t output[4])
{
	switch (format.Base)
	{
	case BaseType::UNorm:
		switch (format.ElementSize)
		{
		case 1:
			Convert<float, uint8_t>(input, output);
			return;
		}
	case BaseType::SNorm: break;
	case BaseType::UScaled: break;
	case BaseType::SScaled: break;
	case BaseType::UInt: break;
	case BaseType::SInt: break;
	case BaseType::UFloat: break;
	case BaseType::SFloat: break;
	case BaseType::SRGB: break;
	}

	FATAL_ERROR();
}

template<typename T>
static void GetPixel(const FormatInformation& format, Image* image, uint32_t x, uint32_t y, uint32_t z, T values[4])
{
	if (format.Base == BaseType::UNorm)
	{
		if (format.ElementSize == 1)
		{
			uint64_t rawValues[4];
			GetPixel<uint8_t>(image->getData(), format, x, y, z, image->getWidth(), image->getHeight(), image->getDepth(), rawValues);
			Convert<uint8_t, T>(rawValues, values);
		}
		else if (format.ElementSize == 2)
		{
			uint64_t rawValues[4];
			GetPixel<uint16_t>(image->getData(), format, x, y, z, image->getWidth(), image->getHeight(), image->getDepth(), rawValues);
			Convert<uint16_t, T>(rawValues, values);
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
}

void ClearImage(Image* image, VkFormat format, VkClearColorValue colour);
void ClearImage(Image* image, VkFormat format, VkClearDepthStencilValue colour);