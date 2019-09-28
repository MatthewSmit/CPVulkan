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
	uint32_t RedPack;
	uint32_t GreenPack;
	uint32_t BluePack;
	uint32_t AlphaPack;
};

const FormatInformation& GetFormatInformation(VkFormat format);

uint64_t GetFormatSize(const FormatInformation& format, uint32_t width, uint32_t height, uint32_t depth, uint32_t arrayLayers, uint32_t mipLevels);
void GetFormatStrides(const FormatInformation& format, uint64_t& offset, uint64_t& planeStride, uint64_t& lineStride, uint32_t mipLevel, uint32_t width, uint32_t height, uint32_t depth);
void GetFormatLineSize(const FormatInformation& format, uint64_t& start, uint64_t& size, uint32_t x, uint32_t width);
uint64_t GetFormatMipmapOffset(const FormatInformation& format, uint32_t& width, uint32_t& height, uint32_t& depth, uint32_t mipLevel);
void* GetFormatPixelOffset(const FormatInformation& format, void* data, uint32_t i, uint32_t j, uint32_t k, uint32_t width, uint32_t height, uint32_t depth, uint32_t mipLevel);
const void* GetFormatPixelOffset(const FormatInformation& format, const void* data, uint32_t i, uint32_t j, uint32_t k, uint32_t width, uint32_t height, uint32_t depth, uint32_t mipLevel);

bool NeedsYCBCRConversion(VkFormat format);