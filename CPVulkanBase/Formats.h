#pragma once
#include "Base.h"

enum class FormatType
{
	Normal,
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
	uint32_t RedOffset;
	uint32_t GreenOffset;
	uint32_t BlueOffset;
	uint32_t AlphaOffset;
	uint32_t RedBits;
	uint32_t GreenBits;
	uint32_t BlueBits;
	uint32_t AlphaBits;
};

CP_DLL_EXPORT const FormatInformation& GetFormatInformation(VkFormat format);

CP_DLL_EXPORT uint64_t GetFormatSize(const FormatInformation& format, uint32_t width, uint32_t height, uint32_t depth, uint32_t arrayLayers, uint32_t mipLevels);
CP_DLL_EXPORT void GetFormatStrides(const FormatInformation& format, uint64_t& offset, uint64_t& planeStride, uint64_t& lineStride, uint32_t mipLevel, uint32_t width, uint32_t height, uint32_t depth, uint32_t arrayLayers);
CP_DLL_EXPORT void GetFormatLineSize(const FormatInformation& format, uint64_t& start, uint64_t& size, uint32_t x, uint32_t width);
CP_DLL_EXPORT uint64_t GetFormatMipmapOffset(VkFormat format, uint32_t& width, uint32_t& height, uint32_t& depth, uint32_t arrayLayers, uint32_t mipLevel);
CP_DLL_EXPORT uint64_t GetFormatMipmapOffset(const FormatInformation& format, uint32_t& width, uint32_t& height, uint32_t& depth, uint32_t arrayLayers, uint32_t mipLevel);
CP_DLL_EXPORT uint64_t GetFormatPixelOffset(const FormatInformation& format, uint32_t i, uint32_t j, uint32_t k, uint32_t width, uint32_t height, uint32_t depth, uint32_t arrayLayers, uint32_t mipLevel, uint32_t layer);
CP_DLL_EXPORT void* GetFormatPixelOffset(const FormatInformation& format, gsl::span<uint8_t> data, uint32_t i, uint32_t j, uint32_t k, uint32_t width, uint32_t height, uint32_t depth, uint32_t arrayLayers, uint32_t mipLevel, uint32_t layer);
CP_DLL_EXPORT uint32_t GetFormatHeight(const FormatInformation& format, uint32_t height);

CP_DLL_EXPORT bool NeedsYCBCRConversion(VkFormat format);