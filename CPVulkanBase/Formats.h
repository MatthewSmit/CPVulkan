#pragma once
#include "Base.h"

enum class FormatType
{
	Normal,
	Packed,
	Compressed,
	Planar,
	PlanarSamplable,
};

enum class BaseType
{
	Unknown,
	UNorm,
	SNorm,
	UScaled,
	SScaled,
	UInt,
	SInt,
	UFloat,
	SFloat,
	SRGB,
};

constexpr auto INVALID_OFFSET = 0xFFFFFFFF;

struct FormatInformation
{
	VkFormat Format;
	FormatType Type;
	VkFormatFeatureFlags LinearTilingFeatures;
	VkFormatFeatureFlags OptimalTilingFeatures;
	VkFormatFeatureFlags BufferFeatures;
	uint32_t TotalSize;
	uint32_t ElementSize;
	BaseType Base;

	union
	{
		struct
		{
			uint32_t RedOffset;
			uint32_t GreenOffset;
			uint32_t BlueOffset;
			uint32_t AlphaOffset;
		} Normal;

		struct
		{
			uint32_t RedOffset;
			uint32_t GreenOffset;
			uint32_t BlueOffset;
			uint32_t AlphaOffset;
			uint32_t RedBits;
			uint32_t GreenBits;
			uint32_t BlueBits;
			uint32_t AlphaBits;
		} Packed;

		struct
		{
			uint32_t BlockWidth;
			uint32_t BlockHeight;
		} Compressed;
	};
};

static constexpr uint32_t clog2(uint32_t n)
{
	return (n < 2) ? 1 : 1 + clog2(n / 2);
}

static constexpr uint32_t CountMipLevels(uint32_t width, uint32_t height, uint32_t depth)
{
	return clog2(std::max(std::max(width, height), depth));
}

struct ImageSize
{
	uint64_t TotalSize;
	uint64_t LayerSize;
	uint64_t NumberLayers;
	uint64_t NumberMipLevels;

	struct
	{
		uint64_t Offset;
		uint64_t LevelSize;
		uint64_t PlaneSize;
		uint64_t Stride;
		uint64_t Width;
		uint64_t Height;
		uint64_t Depth;
	} Level[CountMipLevels(MAX_IMAGE_DIMENSION_1D, MAX_IMAGE_DIMENSION_2D, MAX_IMAGE_DIMENSION_3D)];

	uint64_t PixelSize;
};

CP_DLL_EXPORT const FormatInformation& GetFormatInformation(VkFormat format);

CP_DLL_EXPORT ImageSize GetImageSize(const FormatInformation& format, uint32_t width, uint32_t height, uint32_t depth, uint32_t arrayLayers, uint32_t mipLevels) noexcept;
CP_DLL_EXPORT uint64_t GetImagePixelOffset(const ImageSize& imageSize, int32_t i, int32_t j, int32_t k, uint32_t level, uint32_t layer) noexcept;

CP_DLL_EXPORT bool IsDepthFormat(VkFormat format) noexcept;
CP_DLL_EXPORT bool IsStencilFormat(VkFormat format) noexcept;
CP_DLL_EXPORT bool NeedsYCBCRConversion(VkFormat format) noexcept;