#pragma once
#include "Base.h"

enum class BaseType
{
	Unknown,
	UNorm,
};

struct FormatInformation
{
	VkFormat Format;
	VkFormatFeatureFlags LinearTilingFeatures;
	VkFormatFeatureFlags OptimalTilingFeatures;
	VkFormatFeatureFlags BufferFeatures;
	uint32_t TotalSize;
	uint32_t ElementSize;
	BaseType BaseType;
	uint32_t RedOffset;
	uint32_t GreenOffset;
	uint32_t BlueOffset;
	uint32_t AlphaOffset;
};

const FormatInformation& GetFormatInformation(VkFormat format);