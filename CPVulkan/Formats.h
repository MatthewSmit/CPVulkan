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

void GetFormatStrides(const FormatInformation& format, uint64_t& offset, uint64_t& planeStride, uint64_t& lineStride, uint32_t mipLevel, uint64_t width, uint64_t height);
void GetFormatLineSize(const FormatInformation& format, uint64_t& start, uint64_t& size, uint64_t x, uint64_t width);

bool NeedsYCBCRConversion(VkFormat format);