#include "Formats.h"

#include <unordered_map>

static constexpr VkFormatFeatureFlags GetFeatures(FormatType type)
{
	VkFormatFeatureFlags features = 0;
	switch (type)
	{
	case FormatType::Normal:
	case FormatType::DepthStencil:
	case FormatType::Packed:
	case FormatType::Compressed:
		features |= static_cast<VkFormatFeatureFlags>(VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT |
			VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT |
			VK_FORMAT_FEATURE_STORAGE_IMAGE_ATOMIC_BIT |
			VK_FORMAT_FEATURE_UNIFORM_TEXEL_BUFFER_BIT |
			VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_BIT |
			VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_ATOMIC_BIT |
			VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT |
			VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT |
			VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BLEND_BIT |
			VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT |
			VK_FORMAT_FEATURE_BLIT_SRC_BIT |
			VK_FORMAT_FEATURE_BLIT_DST_BIT |
			VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT |
			VK_FORMAT_FEATURE_TRANSFER_SRC_BIT |
			VK_FORMAT_FEATURE_TRANSFER_DST_BIT |
			VK_FORMAT_FEATURE_MIDPOINT_CHROMA_SAMPLES_BIT |
			VK_FORMAT_FEATURE_SAMPLED_IMAGE_YCBCR_CONVERSION_LINEAR_FILTER_BIT |
			VK_FORMAT_FEATURE_SAMPLED_IMAGE_YCBCR_CONVERSION_SEPARATE_RECONSTRUCTION_FILTER_BIT |
			VK_FORMAT_FEATURE_SAMPLED_IMAGE_YCBCR_CONVERSION_CHROMA_RECONSTRUCTION_EXPLICIT_BIT |
			VK_FORMAT_FEATURE_SAMPLED_IMAGE_YCBCR_CONVERSION_CHROMA_RECONSTRUCTION_EXPLICIT_FORCEABLE_BIT |
			VK_FORMAT_FEATURE_COSITED_CHROMA_SAMPLES_BIT |
			VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_CUBIC_BIT_IMG |
			VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_MINMAX_BIT_EXT |
			VK_FORMAT_FEATURE_FRAGMENT_DENSITY_MAP_BIT_EXT);
		break;
	case FormatType::Planar:
		break;
	case FormatType::PlanarSamplable:
		features |= static_cast<VkFormatFeatureFlags>(VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT | VK_FORMAT_FEATURE_TRANSFER_SRC_BIT | VK_FORMAT_FEATURE_TRANSFER_DST_BIT | VK_FORMAT_FEATURE_COSITED_CHROMA_SAMPLES_BIT);
		break;
	}

	return features;
}

static constexpr FormatInformation MakeFormatInformation(VkFormat format, FormatType type, int32_t channels, uint32_t totalSize)
{
	const auto features = GetFeatures(type);
	return FormatInformation
	{
		format,
		type,
		features,
		features,
		type != FormatType::Normal ? static_cast<VkFormatFeatureFlags>(0) : features,
		static_cast<VkColorComponentFlagBits>(channels),
		totalSize,
	};
}

static constexpr FormatInformation MakeFormatInformation(VkFormat format, FormatType type, int32_t channels, uint32_t totalSize, uint32_t elementSize, BaseType baseType,
                                                         uint32_t redOffset, uint32_t greenOffset, uint32_t blueOffset, uint32_t alphaOffset)
{
	const auto features = GetFeatures(type);
	FormatInformation information
	{
		format,
		type,
		features,
		features,
		type != FormatType::Normal ? static_cast<VkFormatFeatureFlags>(0) : features,
		static_cast<VkColorComponentFlagBits>(channels),
		totalSize,
		elementSize,
		baseType
	};
	information.Normal =
	{
		redOffset,
		greenOffset,
		blueOffset,
		alphaOffset
	};
	return information;
}

static constexpr FormatInformation MakeDepthFormatInformation(VkFormat format, uint32_t totalSize, uint32_t elementSize, BaseType baseType,
                                                              uint32_t depthOffset, uint32_t stencilOffset)
{
	constexpr auto features = GetFeatures(FormatType::DepthStencil);
	FormatInformation information
	{
		format,
		FormatType::DepthStencil,
		features,
		features,
		features,
		static_cast<VkColorComponentFlagBits>(VK_COLOR_COMPONENT_R_BIT),
		totalSize,
		elementSize,
		baseType
	};
	information.DepthStencil =
	{
		depthOffset,
		stencilOffset,
	};
	return information;
}

static constexpr FormatInformation MakeCompressedFormatInformation(VkFormat format, int32_t channels, uint32_t totalSize, BaseType baseType, uint32_t width, uint32_t height)
{
	constexpr auto features = GetFeatures(FormatType::Compressed);
	FormatInformation information
	{
		format,
		FormatType::Compressed,
		features,
		features,
		0,
		static_cast<VkColorComponentFlagBits>(channels),
		totalSize,
		0,
		baseType
	};
	information.Compressed =
	{
		width,
		height,
	};
	return information;
}

static constexpr FormatInformation MakePackedFormatInformation(VkFormat format, int32_t channels,
                                                               uint32_t totalSize, BaseType baseType,
                                                               uint32_t redOffset, uint32_t greenOffset, uint32_t blueOffset, uint32_t alphaOffset,
                                                               uint32_t redBits, uint32_t greenBits, uint32_t blueBits, uint32_t alphaBits)
{
	constexpr auto features = GetFeatures(FormatType::Packed);
	FormatInformation information
	{
		format,
		FormatType::Packed,
		features,
		features,
		features,
		static_cast<VkColorComponentFlagBits>(channels),
		totalSize,
		0,
		baseType,
	};
	information.Packed =
	{
		redOffset,
		greenOffset,
		blueOffset,
		alphaOffset,
		redBits,
		greenBits,
		blueBits,
		alphaBits,
	};
	return information;
}

static constexpr auto COLOUR_NONE = 0;
static constexpr auto COLOUR_R = VK_COLOR_COMPONENT_R_BIT;
static constexpr auto COLOUR_RG = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT;
static constexpr auto COLOUR_RGB = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT;
static constexpr auto COLOUR_RGBA = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

static constexpr FormatInformation formatInformation[]
{
	MakeFormatInformation(VK_FORMAT_UNDEFINED, FormatType::Normal, COLOUR_NONE, 0),
	MakePackedFormatInformation(VK_FORMAT_R4G4_UNORM_PACK8, COLOUR_RG, 1, BaseType::UNorm, 4, 0, 0, 0, 4, 4, 0, 0),
	MakePackedFormatInformation(VK_FORMAT_R4G4B4A4_UNORM_PACK16, COLOUR_RGBA, 2, BaseType::UNorm, 12, 8, 4, 0, 4, 4, 4, 4),
	MakePackedFormatInformation(VK_FORMAT_B4G4R4A4_UNORM_PACK16, COLOUR_RGBA, 2, BaseType::UNorm, 4, 8, 12, 0, 4, 4, 4, 4),
	MakePackedFormatInformation(VK_FORMAT_R5G6B5_UNORM_PACK16, COLOUR_RGB, 2, BaseType::UNorm, 11, 5, 0, 0, 5, 6, 5, 0),
	MakePackedFormatInformation(VK_FORMAT_B5G6R5_UNORM_PACK16, COLOUR_RGB, 2, BaseType::UNorm, 0, 5, 11, 0, 5, 6, 5, 0),
	MakePackedFormatInformation(VK_FORMAT_R5G5B5A1_UNORM_PACK16, COLOUR_RGBA, 2, BaseType::UNorm, 11, 6, 1, 0, 5, 5, 5, 1),
	MakePackedFormatInformation(VK_FORMAT_B5G5R5A1_UNORM_PACK16, COLOUR_RGBA, 2, BaseType::UNorm, 1, 6, 11, 0, 5, 5, 5, 1),
	MakePackedFormatInformation(VK_FORMAT_A1R5G5B5_UNORM_PACK16, COLOUR_RGBA, 2, BaseType::UNorm, 10, 5, 0, 15, 5, 5, 5, 1),
	MakeFormatInformation(VK_FORMAT_R8_UNORM, FormatType::Normal, COLOUR_R, 1, 1, BaseType::UNorm, 0, -1, -1, -1),
	MakeFormatInformation(VK_FORMAT_R8_SNORM, FormatType::Normal, COLOUR_R, 1, 1, BaseType::SNorm, 0, -1, -1, -1),
	MakeFormatInformation(VK_FORMAT_R8_USCALED, FormatType::Normal, COLOUR_R, 1, 1, BaseType::UScaled, 0, -1, -1, -1),
	MakeFormatInformation(VK_FORMAT_R8_SSCALED, FormatType::Normal, COLOUR_R, 1, 1, BaseType::SScaled, 0, -1, -1, -1),
	MakeFormatInformation(VK_FORMAT_R8_UINT, FormatType::Normal, COLOUR_R, 1, 1, BaseType::UInt, 0, -1, -1, -1),
	MakeFormatInformation(VK_FORMAT_R8_SINT, FormatType::Normal, COLOUR_R, 1, 1, BaseType::SInt, 0, -1, -1, -1),
	MakeFormatInformation(VK_FORMAT_R8_SRGB, FormatType::Normal, COLOUR_R, 1, 1, BaseType::SRGB, 0, -1, -1, -1),
	MakeFormatInformation(VK_FORMAT_R8G8_UNORM, FormatType::Normal, COLOUR_RG, 2, 1, BaseType::UNorm, 0, 1, -1, -1),
	MakeFormatInformation(VK_FORMAT_R8G8_SNORM, FormatType::Normal, COLOUR_RG, 2, 1, BaseType::SNorm, 0, 1, -1, -1),
	MakeFormatInformation(VK_FORMAT_R8G8_USCALED, FormatType::Normal, COLOUR_RG, 2, 1, BaseType::UScaled, 0, 1, -1, -1),
	MakeFormatInformation(VK_FORMAT_R8G8_SSCALED, FormatType::Normal, COLOUR_RG, 2, 1, BaseType::SScaled, 0, 1, -1, -1),
	MakeFormatInformation(VK_FORMAT_R8G8_UINT, FormatType::Normal, COLOUR_RG, 2, 1, BaseType::UInt, 0, 1, -1, -1),
	MakeFormatInformation(VK_FORMAT_R8G8_SINT, FormatType::Normal, COLOUR_RG, 2, 1, BaseType::SInt, 0, 1, -1, -1),
	MakeFormatInformation(VK_FORMAT_R8G8_SRGB, FormatType::Normal, COLOUR_RG, 2, 1, BaseType::SRGB, 0, 1, -1, -1),
	MakeFormatInformation(VK_FORMAT_R8G8B8_UNORM, FormatType::Normal, COLOUR_RGB, 3, 1, BaseType::UNorm, 0, 1, 2, -1),
	MakeFormatInformation(VK_FORMAT_R8G8B8_SNORM, FormatType::Normal, COLOUR_RGB, 3, 1, BaseType::SNorm, 0, 1, 2, -1),
	MakeFormatInformation(VK_FORMAT_R8G8B8_USCALED, FormatType::Normal, COLOUR_RGB, 3, 1, BaseType::UScaled, 0, 1, 2, -1),
	MakeFormatInformation(VK_FORMAT_R8G8B8_SSCALED, FormatType::Normal, COLOUR_RGB, 3, 1, BaseType::SScaled, 0, 1, 2, -1),
	MakeFormatInformation(VK_FORMAT_R8G8B8_UINT, FormatType::Normal, COLOUR_RGB, 3, 1, BaseType::UInt, 0, 1, 2, -1),
	MakeFormatInformation(VK_FORMAT_R8G8B8_SINT, FormatType::Normal, COLOUR_RGB, 3, 1, BaseType::SInt, 0, 1, 2, -1),
	MakeFormatInformation(VK_FORMAT_R8G8B8_SRGB, FormatType::Normal, COLOUR_RGB, 3, 1, BaseType::SRGB, 0, 1, 2, -1),
	MakeFormatInformation(VK_FORMAT_B8G8R8_UNORM, FormatType::Normal, COLOUR_RGB, 3, 1, BaseType::UNorm, 2, 1, 0, -1),
	MakeFormatInformation(VK_FORMAT_B8G8R8_SNORM, FormatType::Normal, COLOUR_RGB, 3, 1, BaseType::SNorm, 2, 1, 0, -1),
	MakeFormatInformation(VK_FORMAT_B8G8R8_USCALED, FormatType::Normal, COLOUR_RGB, 3, 1, BaseType::UScaled, 2, 1, 0, -1),
	MakeFormatInformation(VK_FORMAT_B8G8R8_SSCALED, FormatType::Normal, COLOUR_RGB, 3, 1, BaseType::SScaled, 2, 1, 0, -1),
	MakeFormatInformation(VK_FORMAT_B8G8R8_UINT, FormatType::Normal, COLOUR_RGB, 3, 1, BaseType::UInt, 2, 1, 0, -1),
	MakeFormatInformation(VK_FORMAT_B8G8R8_SINT, FormatType::Normal, COLOUR_RGB, 3, 1, BaseType::SInt, 2, 1, 0, -1),
	MakeFormatInformation(VK_FORMAT_B8G8R8_SRGB, FormatType::Normal, COLOUR_RGB, 3, 1, BaseType::SRGB, 2, 1, 0, -1),
	MakeFormatInformation(VK_FORMAT_R8G8B8A8_UNORM, FormatType::Normal, COLOUR_RGBA, 4, 1, BaseType::UNorm, 0, 1, 2, 3),
	MakeFormatInformation(VK_FORMAT_R8G8B8A8_SNORM, FormatType::Normal, COLOUR_RGBA, 4, 1, BaseType::SNorm, 0, 1, 2, 3),
	MakeFormatInformation(VK_FORMAT_R8G8B8A8_USCALED, FormatType::Normal, COLOUR_RGBA, 4, 1, BaseType::UScaled, 0, 1, 2, 3),
	MakeFormatInformation(VK_FORMAT_R8G8B8A8_SSCALED, FormatType::Normal, COLOUR_RGBA, 4, 1, BaseType::SScaled, 0, 1, 2, 3),
	MakeFormatInformation(VK_FORMAT_R8G8B8A8_UINT, FormatType::Normal, COLOUR_RGBA, 4, 1, BaseType::UInt, 0, 1, 2, 3),
	MakeFormatInformation(VK_FORMAT_R8G8B8A8_SINT, FormatType::Normal, COLOUR_RGBA, 4, 1, BaseType::SInt, 0, 1, 2, 3),
	MakeFormatInformation(VK_FORMAT_R8G8B8A8_SRGB, FormatType::Normal, COLOUR_RGBA, 4, 1, BaseType::SRGB, 0, 1, 2, 3),
	MakeFormatInformation(VK_FORMAT_B8G8R8A8_UNORM, FormatType::Normal, COLOUR_RGBA, 4, 1, BaseType::UNorm, 2, 1, 0, 3),
	MakeFormatInformation(VK_FORMAT_B8G8R8A8_SNORM, FormatType::Normal, COLOUR_RGBA, 4, 1, BaseType::SNorm, 2, 1, 0, 3),
	MakeFormatInformation(VK_FORMAT_B8G8R8A8_USCALED, FormatType::Normal, COLOUR_RGBA, 4, 1, BaseType::UScaled, 2, 1, 0, 3),
	MakeFormatInformation(VK_FORMAT_B8G8R8A8_SSCALED, FormatType::Normal, COLOUR_RGBA, 4, 1, BaseType::SScaled, 2, 1, 0, 3),
	MakeFormatInformation(VK_FORMAT_B8G8R8A8_UINT, FormatType::Normal, COLOUR_RGBA, 4, 1, BaseType::UInt, 2, 1, 0, 3),
	MakeFormatInformation(VK_FORMAT_B8G8R8A8_SINT, FormatType::Normal, COLOUR_RGBA, 4, 1, BaseType::SInt, 2, 1, 0, 3),
	MakeFormatInformation(VK_FORMAT_B8G8R8A8_SRGB, FormatType::Normal, COLOUR_RGBA, 4, 1, BaseType::SRGB, 2, 1, 0, 3),
	MakePackedFormatInformation(VK_FORMAT_A8B8G8R8_UNORM_PACK32, COLOUR_RGBA, 4, BaseType::UNorm, 0, 8, 16, 24, 8, 8, 8, 8),
	MakePackedFormatInformation(VK_FORMAT_A8B8G8R8_SNORM_PACK32, COLOUR_RGBA, 4, BaseType::SNorm, 0, 8, 16, 24, 8, 8, 8, 8),
	MakePackedFormatInformation(VK_FORMAT_A8B8G8R8_USCALED_PACK32, COLOUR_RGBA, 4, BaseType::UScaled, 0, 8, 16, 24, 8, 8, 8, 8),
	MakePackedFormatInformation(VK_FORMAT_A8B8G8R8_SSCALED_PACK32, COLOUR_RGBA, 4, BaseType::SScaled, 0, 8, 16, 24, 8, 8, 8, 8),
	MakePackedFormatInformation(VK_FORMAT_A8B8G8R8_UINT_PACK32, COLOUR_RGBA, 4, BaseType::UInt, 0, 8, 16, 24, 8, 8, 8, 8),
	MakePackedFormatInformation(VK_FORMAT_A8B8G8R8_SINT_PACK32, COLOUR_RGBA, 4, BaseType::SInt, 0, 8, 16, 24, 8, 8, 8, 8),
	MakePackedFormatInformation(VK_FORMAT_A8B8G8R8_SRGB_PACK32, COLOUR_RGBA, 4, BaseType::SRGB, 0, 8, 16, 24, 8, 8, 8, 8),
	MakePackedFormatInformation(VK_FORMAT_A2R10G10B10_UNORM_PACK32, COLOUR_RGBA, 4, BaseType::UNorm, 20, 10, 0, 30, 10, 10, 10, 2),
	MakePackedFormatInformation(VK_FORMAT_A2R10G10B10_SNORM_PACK32, COLOUR_RGBA, 4, BaseType::SNorm, 20, 10, 0, 30, 10, 10, 10, 2),
	MakePackedFormatInformation(VK_FORMAT_A2R10G10B10_USCALED_PACK32, COLOUR_RGBA, 4, BaseType::UScaled, 20, 10, 0, 30, 10, 10, 10, 2),
	MakePackedFormatInformation(VK_FORMAT_A2R10G10B10_SSCALED_PACK32, COLOUR_RGBA, 4, BaseType::SScaled, 20, 10, 0, 30, 10, 10, 10, 2),
	MakePackedFormatInformation(VK_FORMAT_A2R10G10B10_UINT_PACK32, COLOUR_RGBA, 4, BaseType::UInt, 20, 10, 0, 30, 10, 10, 10, 2),
	MakePackedFormatInformation(VK_FORMAT_A2R10G10B10_SINT_PACK32, COLOUR_RGBA, 4, BaseType::SInt, 20, 10, 0, 30, 10, 10, 10, 2),
	MakePackedFormatInformation(VK_FORMAT_A2B10G10R10_UNORM_PACK32, COLOUR_RGBA, 4, BaseType::UNorm, 0, 10, 20, 30, 10, 10, 10, 2),
	MakePackedFormatInformation(VK_FORMAT_A2B10G10R10_SNORM_PACK32, COLOUR_RGBA, 4, BaseType::SNorm, 0, 10, 20, 30, 10, 10, 10, 2),
	MakePackedFormatInformation(VK_FORMAT_A2B10G10R10_USCALED_PACK32, COLOUR_RGBA, 4, BaseType::UScaled, 0, 10, 20, 30, 10, 10, 10, 2),
	MakePackedFormatInformation(VK_FORMAT_A2B10G10R10_SSCALED_PACK32, COLOUR_RGBA, 4, BaseType::SScaled, 0, 10, 20, 30, 10, 10, 10, 2),
	MakePackedFormatInformation(VK_FORMAT_A2B10G10R10_UINT_PACK32, COLOUR_RGBA, 4, BaseType::UInt, 0, 10, 20, 30, 10, 10, 10, 2),
	MakePackedFormatInformation(VK_FORMAT_A2B10G10R10_SINT_PACK32, COLOUR_RGBA, 4, BaseType::SInt, 0, 10, 20, 30, 10, 10, 10, 2),
	MakeFormatInformation(VK_FORMAT_R16_UNORM, FormatType::Normal, COLOUR_R, 2, 2, BaseType::UNorm, 0, -1, -1, -1),
	MakeFormatInformation(VK_FORMAT_R16_SNORM, FormatType::Normal, COLOUR_R, 2, 2, BaseType::SNorm, 0, -1, -1, -1),
	MakeFormatInformation(VK_FORMAT_R16_USCALED, FormatType::Normal, COLOUR_R, 2, 2, BaseType::UScaled, 0, -1, -1, -1),
	MakeFormatInformation(VK_FORMAT_R16_SSCALED, FormatType::Normal, COLOUR_R, 2, 2, BaseType::SScaled, 0, -1, -1, -1),
	MakeFormatInformation(VK_FORMAT_R16_UINT, FormatType::Normal, COLOUR_R, 2, 2, BaseType::UInt, 0, -1, -1, -1),
	MakeFormatInformation(VK_FORMAT_R16_SINT, FormatType::Normal, COLOUR_R, 2, 2, BaseType::SInt, 0, -1, -1, -1),
	MakeFormatInformation(VK_FORMAT_R16_SFLOAT, FormatType::Normal, COLOUR_R, 2, 2, BaseType::SFloat, 0, -1, -1, -1),
	MakeFormatInformation(VK_FORMAT_R16G16_UNORM, FormatType::Normal, COLOUR_RG, 4, 2, BaseType::UNorm, 0, 2, -1, -1),
	MakeFormatInformation(VK_FORMAT_R16G16_SNORM, FormatType::Normal, COLOUR_RG, 4, 2, BaseType::SNorm, 0, 2, -1, -1),
	MakeFormatInformation(VK_FORMAT_R16G16_USCALED, FormatType::Normal, COLOUR_RG, 4, 2, BaseType::UScaled, 0, 2, -1, -1),
	MakeFormatInformation(VK_FORMAT_R16G16_SSCALED, FormatType::Normal, COLOUR_RG, 4, 2, BaseType::SScaled, 0, 2, -1, -1),
	MakeFormatInformation(VK_FORMAT_R16G16_UINT, FormatType::Normal, COLOUR_RG, 4, 2, BaseType::UInt, 0, 2, -1, -1),
	MakeFormatInformation(VK_FORMAT_R16G16_SINT, FormatType::Normal, COLOUR_RG, 4, 2, BaseType::SInt, 0, 2, -1, -1),
	MakeFormatInformation(VK_FORMAT_R16G16_SFLOAT, FormatType::Normal, COLOUR_RG, 4, 2, BaseType::SFloat, 0, 2, -1, -1),
	MakeFormatInformation(VK_FORMAT_R16G16B16_UNORM, FormatType::Normal, COLOUR_RGB, 6, 2, BaseType::UNorm, 0, 2, 4, -1),
	MakeFormatInformation(VK_FORMAT_R16G16B16_SNORM, FormatType::Normal, COLOUR_RGB, 6, 2, BaseType::SNorm, 0, 2, 4, -1),
	MakeFormatInformation(VK_FORMAT_R16G16B16_USCALED, FormatType::Normal, COLOUR_RGB, 6, 2, BaseType::UScaled, 0, 2, 4, -1),
	MakeFormatInformation(VK_FORMAT_R16G16B16_SSCALED, FormatType::Normal, COLOUR_RGB, 6, 2, BaseType::SScaled, 0, 2, 4, -1),
	MakeFormatInformation(VK_FORMAT_R16G16B16_UINT, FormatType::Normal, COLOUR_RGB, 6, 2, BaseType::UInt, 0, 2, 4, -1),
	MakeFormatInformation(VK_FORMAT_R16G16B16_SINT, FormatType::Normal, COLOUR_RGB, 6, 2, BaseType::SInt, 0, 2, 4, -1),
	MakeFormatInformation(VK_FORMAT_R16G16B16_SFLOAT, FormatType::Normal, COLOUR_RGB, 6, 2, BaseType::SFloat, 0, 2, 4, -1),
	MakeFormatInformation(VK_FORMAT_R16G16B16A16_UNORM, FormatType::Normal, COLOUR_RGBA, 8, 2, BaseType::UNorm, 0, 2, 4, 6),
	MakeFormatInformation(VK_FORMAT_R16G16B16A16_SNORM, FormatType::Normal, COLOUR_RGBA, 8, 2, BaseType::SNorm, 0, 2, 4, 6),
	MakeFormatInformation(VK_FORMAT_R16G16B16A16_USCALED, FormatType::Normal, COLOUR_RGBA, 8, 2, BaseType::UScaled, 0, 2, 4, 6),
	MakeFormatInformation(VK_FORMAT_R16G16B16A16_SSCALED, FormatType::Normal, COLOUR_RGBA, 8, 2, BaseType::SScaled, 0, 2, 4, 6),
	MakeFormatInformation(VK_FORMAT_R16G16B16A16_UINT, FormatType::Normal, COLOUR_RGBA, 8, 2, BaseType::UInt, 0, 2, 4, 6),
	MakeFormatInformation(VK_FORMAT_R16G16B16A16_SINT, FormatType::Normal, COLOUR_RGBA, 8, 2, BaseType::SInt, 0, 2, 4, 6),
	MakeFormatInformation(VK_FORMAT_R16G16B16A16_SFLOAT, FormatType::Normal, COLOUR_RGBA, 8, 2, BaseType::SFloat, 0, 2, 4, 6),
	MakeFormatInformation(VK_FORMAT_R32_UINT, FormatType::Normal, COLOUR_R, 4, 4, BaseType::UInt, 0, -1, -1, -1),
	MakeFormatInformation(VK_FORMAT_R32_SINT, FormatType::Normal, COLOUR_R, 4, 4, BaseType::SInt, 0, -1, -1, -1),
	MakeFormatInformation(VK_FORMAT_R32_SFLOAT, FormatType::Normal, COLOUR_R, 4, 4, BaseType::SFloat, 0, -1, -1, -1),
	MakeFormatInformation(VK_FORMAT_R32G32_UINT, FormatType::Normal, COLOUR_RG, 8, 4, BaseType::UInt, 0, 4, -1, -1),
	MakeFormatInformation(VK_FORMAT_R32G32_SINT, FormatType::Normal, COLOUR_RG, 8, 4, BaseType::SInt, 0, 4, -1, -1),
	MakeFormatInformation(VK_FORMAT_R32G32_SFLOAT, FormatType::Normal, COLOUR_RG, 8, 4, BaseType::SFloat, 0, 4, -1, -1),
	MakeFormatInformation(VK_FORMAT_R32G32B32_UINT, FormatType::Normal, COLOUR_RGB, 12, 4, BaseType::UInt, 0, 4, 8, -1),
	MakeFormatInformation(VK_FORMAT_R32G32B32_SINT, FormatType::Normal, COLOUR_RGB, 12, 4, BaseType::SInt, 0, 4, 8, -1),
	MakeFormatInformation(VK_FORMAT_R32G32B32_SFLOAT, FormatType::Normal, COLOUR_RGB, 12, 4, BaseType::SFloat, 0, 4, 8, -1),
	MakeFormatInformation(VK_FORMAT_R32G32B32A32_UINT, FormatType::Normal, COLOUR_RGBA, 16, 4, BaseType::UInt, 0, 4, 8, 12),
	MakeFormatInformation(VK_FORMAT_R32G32B32A32_SINT, FormatType::Normal, COLOUR_RGBA, 16, 4, BaseType::SInt, 0, 4, 8, 12),
	MakeFormatInformation(VK_FORMAT_R32G32B32A32_SFLOAT, FormatType::Normal, COLOUR_RGBA, 16, 4, BaseType::SFloat, 0, 4, 8, 12),
	MakeFormatInformation(VK_FORMAT_R64_UINT, FormatType::Normal, COLOUR_R, 8, 8, BaseType::UInt, 0, -1, -1, -1),
	MakeFormatInformation(VK_FORMAT_R64_SINT, FormatType::Normal, COLOUR_R, 8, 8, BaseType::SInt, 0, -1, -1, -1),
	MakeFormatInformation(VK_FORMAT_R64_SFLOAT, FormatType::Normal, COLOUR_R, 8, 8, BaseType::SFloat, 0, -1, -1, -1),
	MakeFormatInformation(VK_FORMAT_R64G64_UINT, FormatType::Normal, COLOUR_RG, 16, 8, BaseType::UInt, 0, 8, -1, -1),
	MakeFormatInformation(VK_FORMAT_R64G64_SINT, FormatType::Normal, COLOUR_RG, 16, 8, BaseType::SInt, 0, 8, -1, -1),
	MakeFormatInformation(VK_FORMAT_R64G64_SFLOAT, FormatType::Normal, COLOUR_RG, 16, 8, BaseType::SFloat, 0, 8, -1, -1),
	MakeFormatInformation(VK_FORMAT_R64G64B64_UINT, FormatType::Normal, COLOUR_RGB, 24, 8, BaseType::UInt, 0, 8, 16, -1),
	MakeFormatInformation(VK_FORMAT_R64G64B64_SINT, FormatType::Normal, COLOUR_RGB, 24, 8, BaseType::SInt, 0, 8, 16, -1),
	MakeFormatInformation(VK_FORMAT_R64G64B64_SFLOAT, FormatType::Normal, COLOUR_RGB, 24, 8, BaseType::SFloat, 0, 8, 16, -1),
	MakeFormatInformation(VK_FORMAT_R64G64B64A64_UINT, FormatType::Normal, COLOUR_RGBA, 32, 8, BaseType::UInt, 0, 8, 16, 24),
	MakeFormatInformation(VK_FORMAT_R64G64B64A64_SINT, FormatType::Normal, COLOUR_RGBA, 32, 8, BaseType::SInt, 0, 8, 16, 24),
	MakeFormatInformation(VK_FORMAT_R64G64B64A64_SFLOAT, FormatType::Normal, COLOUR_RGBA, 32, 8, BaseType::SFloat, 0, 8, 16, 24),
	MakePackedFormatInformation(VK_FORMAT_B10G11R11_UFLOAT_PACK32, COLOUR_RGB, 4, BaseType::UFloat, 0, 11, 22, -1, 11, 11, 10, -1),
	MakePackedFormatInformation(VK_FORMAT_E5B9G9R9_UFLOAT_PACK32, COLOUR_RGB, 4, BaseType::UFloat, 0, 9, 18, 27, 9, 9, 9, 5),
	MakeDepthFormatInformation(VK_FORMAT_D16_UNORM, 2, 2, BaseType::UNorm, 0, -1),
	MakeDepthFormatInformation(VK_FORMAT_X8_D24_UNORM_PACK32, 4, 4, BaseType::UNorm, 0, -1),
	MakeDepthFormatInformation(VK_FORMAT_D32_SFLOAT, 4, 4, BaseType::SFloat, 0, -1),
	MakeDepthFormatInformation(VK_FORMAT_S8_UINT, 1, 1, BaseType::UInt, -1, 0),
	MakeDepthFormatInformation(VK_FORMAT_D16_UNORM_S8_UINT, 3, 2, BaseType::UNorm, 0, 2),
	MakeDepthFormatInformation(VK_FORMAT_D24_UNORM_S8_UINT, 4, 3, BaseType::UNorm, 0, 3),
	MakeDepthFormatInformation(VK_FORMAT_D32_SFLOAT_S8_UINT, 8, 4, BaseType::SFloat, 0, 4),
	MakeCompressedFormatInformation(VK_FORMAT_BC1_RGB_UNORM_BLOCK, COLOUR_RGB, 8, BaseType::UNorm, 4, 4),
	MakeCompressedFormatInformation(VK_FORMAT_BC1_RGB_SRGB_BLOCK, COLOUR_RGB, 8, BaseType::SRGB, 4, 4),
	MakeCompressedFormatInformation(VK_FORMAT_BC1_RGBA_UNORM_BLOCK, COLOUR_RGBA, 8, BaseType::UNorm, 4, 4),
	MakeCompressedFormatInformation(VK_FORMAT_BC1_RGBA_SRGB_BLOCK, COLOUR_RGBA, 8, BaseType::SRGB, 4, 4),
	MakeCompressedFormatInformation(VK_FORMAT_BC2_UNORM_BLOCK, COLOUR_RGBA, 16, BaseType::UNorm, 4, 4),
	MakeCompressedFormatInformation(VK_FORMAT_BC2_SRGB_BLOCK, COLOUR_RGBA, 16, BaseType::SRGB, 4, 4),
	MakeCompressedFormatInformation(VK_FORMAT_BC3_UNORM_BLOCK, COLOUR_RGBA, 16, BaseType::UNorm, 4, 4),
	MakeCompressedFormatInformation(VK_FORMAT_BC3_SRGB_BLOCK, COLOUR_RGBA, 16, BaseType::SRGB, 4, 4),
	MakeCompressedFormatInformation(VK_FORMAT_BC4_UNORM_BLOCK, COLOUR_RGBA, 8, BaseType::UNorm, 4, 4),
	MakeCompressedFormatInformation(VK_FORMAT_BC4_SNORM_BLOCK, COLOUR_RGBA, 8, BaseType::SNorm, 4, 4),
	MakeCompressedFormatInformation(VK_FORMAT_BC5_UNORM_BLOCK, COLOUR_RGBA, 16, BaseType::UNorm, 4, 4),
	MakeCompressedFormatInformation(VK_FORMAT_BC5_SNORM_BLOCK, COLOUR_RGBA, 16, BaseType::SNorm, 4, 4),
	MakeCompressedFormatInformation(VK_FORMAT_BC6H_UFLOAT_BLOCK, COLOUR_RGBA, 16, BaseType::UFloat, 4, 4),
	MakeCompressedFormatInformation(VK_FORMAT_BC6H_SFLOAT_BLOCK, COLOUR_RGBA, 16, BaseType::SFloat, 4, 4),
	MakeCompressedFormatInformation(VK_FORMAT_BC7_UNORM_BLOCK, COLOUR_RGBA, 16, BaseType::UNorm, 4, 4),
	MakeCompressedFormatInformation(VK_FORMAT_BC7_SRGB_BLOCK, COLOUR_RGBA, 16, BaseType::SRGB, 4, 4),
	MakeCompressedFormatInformation(VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK, COLOUR_RGB, 8, BaseType::UNorm, 4, 4),
	MakeCompressedFormatInformation(VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK, COLOUR_RGB, 8, BaseType::SRGB, 4, 4),
	MakeCompressedFormatInformation(VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK, COLOUR_RGBA, 8, BaseType::UNorm, 4, 4),
	MakeCompressedFormatInformation(VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK, COLOUR_RGBA, 8, BaseType::SRGB, 4, 4),
	MakeCompressedFormatInformation(VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK, COLOUR_RGBA, 16, BaseType::UNorm, 4, 4),
	MakeCompressedFormatInformation(VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK, COLOUR_RGBA, 16, BaseType::SRGB, 4, 4),
	MakeCompressedFormatInformation(VK_FORMAT_EAC_R11_UNORM_BLOCK, COLOUR_RGBA, 8, BaseType::UNorm, 4, 4),
	MakeCompressedFormatInformation(VK_FORMAT_EAC_R11_SNORM_BLOCK, COLOUR_RGBA, 8, BaseType::SNorm, 4, 4),
	MakeCompressedFormatInformation(VK_FORMAT_EAC_R11G11_UNORM_BLOCK, COLOUR_RGBA, 16, BaseType::UNorm, 4, 4),
	MakeCompressedFormatInformation(VK_FORMAT_EAC_R11G11_SNORM_BLOCK, COLOUR_RGBA, 16, BaseType::SNorm, 4, 4),
	MakeCompressedFormatInformation(VK_FORMAT_ASTC_4x4_UNORM_BLOCK, COLOUR_RGBA, 16, BaseType::UNorm, 4, 4),
	MakeCompressedFormatInformation(VK_FORMAT_ASTC_4x4_SRGB_BLOCK, COLOUR_RGBA, 16, BaseType::SRGB, 4, 4),
	MakeCompressedFormatInformation(VK_FORMAT_ASTC_5x4_UNORM_BLOCK, COLOUR_RGBA, 16, BaseType::UNorm, 5, 4),
	MakeCompressedFormatInformation(VK_FORMAT_ASTC_5x4_SRGB_BLOCK, COLOUR_RGBA, 16, BaseType::SRGB, 5, 4),
	MakeCompressedFormatInformation(VK_FORMAT_ASTC_5x5_UNORM_BLOCK, COLOUR_RGBA, 16, BaseType::UNorm, 5, 5),
	MakeCompressedFormatInformation(VK_FORMAT_ASTC_5x5_SRGB_BLOCK, COLOUR_RGBA, 16, BaseType::SRGB, 5, 5),
	MakeCompressedFormatInformation(VK_FORMAT_ASTC_6x5_UNORM_BLOCK, COLOUR_RGBA, 16, BaseType::UNorm, 6, 5),
	MakeCompressedFormatInformation(VK_FORMAT_ASTC_6x5_SRGB_BLOCK, COLOUR_RGBA, 16, BaseType::SRGB, 6, 5),
	MakeCompressedFormatInformation(VK_FORMAT_ASTC_6x6_UNORM_BLOCK, COLOUR_RGBA, 16, BaseType::UNorm, 6, 6),
	MakeCompressedFormatInformation(VK_FORMAT_ASTC_6x6_SRGB_BLOCK, COLOUR_RGBA, 16, BaseType::SRGB, 6, 6),
	MakeCompressedFormatInformation(VK_FORMAT_ASTC_8x5_UNORM_BLOCK, COLOUR_RGBA, 16, BaseType::UNorm, 8, 5),
	MakeCompressedFormatInformation(VK_FORMAT_ASTC_8x5_SRGB_BLOCK, COLOUR_RGBA, 16, BaseType::SRGB, 8, 5),
	MakeCompressedFormatInformation(VK_FORMAT_ASTC_8x6_UNORM_BLOCK, COLOUR_RGBA, 16, BaseType::UNorm, 8, 6),
	MakeCompressedFormatInformation(VK_FORMAT_ASTC_8x6_SRGB_BLOCK, COLOUR_RGBA, 16, BaseType::SRGB, 8, 6),
	MakeCompressedFormatInformation(VK_FORMAT_ASTC_8x8_UNORM_BLOCK, COLOUR_RGBA, 16, BaseType::UNorm, 8, 8),
	MakeCompressedFormatInformation(VK_FORMAT_ASTC_8x8_SRGB_BLOCK, COLOUR_RGBA, 16, BaseType::SRGB, 8, 8),
	MakeCompressedFormatInformation(VK_FORMAT_ASTC_10x5_UNORM_BLOCK, COLOUR_RGBA, 16, BaseType::UNorm, 10, 5),
	MakeCompressedFormatInformation(VK_FORMAT_ASTC_10x5_SRGB_BLOCK, COLOUR_RGBA, 16, BaseType::SRGB, 10, 5),
	MakeCompressedFormatInformation(VK_FORMAT_ASTC_10x6_UNORM_BLOCK, COLOUR_RGBA, 16, BaseType::UNorm, 10, 6),
	MakeCompressedFormatInformation(VK_FORMAT_ASTC_10x6_SRGB_BLOCK, COLOUR_RGBA, 16, BaseType::SRGB, 10, 6),
	MakeCompressedFormatInformation(VK_FORMAT_ASTC_10x8_UNORM_BLOCK, COLOUR_RGBA, 16, BaseType::UNorm, 10, 8),
	MakeCompressedFormatInformation(VK_FORMAT_ASTC_10x8_SRGB_BLOCK, COLOUR_RGBA, 16, BaseType::SRGB, 10, 8),
	MakeCompressedFormatInformation(VK_FORMAT_ASTC_10x10_UNORM_BLOCK, COLOUR_RGBA, 16, BaseType::UNorm, 10, 10),
	MakeCompressedFormatInformation(VK_FORMAT_ASTC_10x10_SRGB_BLOCK, COLOUR_RGBA, 16, BaseType::SRGB, 10, 10),
	MakeCompressedFormatInformation(VK_FORMAT_ASTC_12x10_UNORM_BLOCK, COLOUR_RGBA, 16, BaseType::UNorm, 12, 10),
	MakeCompressedFormatInformation(VK_FORMAT_ASTC_12x10_SRGB_BLOCK, COLOUR_RGBA, 16, BaseType::SRGB, 12, 10),
	MakeCompressedFormatInformation(VK_FORMAT_ASTC_12x12_UNORM_BLOCK, COLOUR_RGBA, 16, BaseType::UNorm, 12, 12),
	MakeCompressedFormatInformation(VK_FORMAT_ASTC_12x12_SRGB_BLOCK, COLOUR_RGBA, 16, BaseType::UNorm, 12, 12),
};

static std::unordered_map<VkFormat, FormatInformation> extraFormatInformation
{
	{VK_FORMAT_G8B8G8R8_422_UNORM, MakeFormatInformation(VK_FORMAT_G8B8G8R8_422_UNORM, FormatType::Planar, COLOUR_RGBA, 4)},
	{VK_FORMAT_B8G8R8G8_422_UNORM, MakeFormatInformation(VK_FORMAT_B8G8R8G8_422_UNORM, FormatType::Planar, COLOUR_RGBA, 4)},
	{VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM, MakeFormatInformation(VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM, FormatType::PlanarSamplable, COLOUR_RGB, 2)},
	{VK_FORMAT_G8_B8R8_2PLANE_420_UNORM, MakeFormatInformation(VK_FORMAT_G8_B8R8_2PLANE_420_UNORM, FormatType::PlanarSamplable, COLOUR_RGB, 4)},
	{VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM, MakeFormatInformation(VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM, FormatType::PlanarSamplable, COLOUR_RGB, 2)},
	{VK_FORMAT_G8_B8R8_2PLANE_422_UNORM, MakeFormatInformation(VK_FORMAT_G8_B8R8_2PLANE_422_UNORM, FormatType::PlanarSamplable, COLOUR_RGB, 4)},
	{VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM, MakeFormatInformation(VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM, FormatType::Planar, COLOUR_RGB, 4)},
	{VK_FORMAT_R10X6_UNORM_PACK16, MakeFormatInformation(VK_FORMAT_R10X6_UNORM_PACK16, FormatType::Planar, COLOUR_R, 2)},
	{VK_FORMAT_R10X6G10X6_UNORM_2PACK16, MakeFormatInformation(VK_FORMAT_R10X6G10X6_UNORM_2PACK16, FormatType::Planar, COLOUR_RG, 4)},
	{VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16, MakeFormatInformation(VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16, FormatType::Planar, COLOUR_RGBA, 8)},
	{VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16, MakeFormatInformation(VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16, FormatType::Planar, COLOUR_RGBA, 8)},
	{VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16, MakeFormatInformation(VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16, FormatType::Planar, COLOUR_RGBA, 8)},
	{VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16, MakeFormatInformation(VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16, FormatType::Planar, COLOUR_RGB, 6)},
	{VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16, MakeFormatInformation(VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16, FormatType::Planar, COLOUR_RGB, 6)},
	{VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16, MakeFormatInformation(VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16, FormatType::Planar, COLOUR_RGB, 6)},
	{VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16, MakeFormatInformation(VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16, FormatType::Planar, COLOUR_RGB, 6)},
	{VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16, MakeFormatInformation(VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16, FormatType::Planar, COLOUR_RGB, 6)},
	{VK_FORMAT_R12X4_UNORM_PACK16, MakeFormatInformation(VK_FORMAT_R12X4_UNORM_PACK16, FormatType::Planar, COLOUR_R, 2)},
	{VK_FORMAT_R12X4G12X4_UNORM_2PACK16, MakeFormatInformation(VK_FORMAT_R12X4G12X4_UNORM_2PACK16, FormatType::Planar, COLOUR_RG, 4)},
	{VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16, MakeFormatInformation(VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16, FormatType::Planar, COLOUR_RGBA, 8)},
	{VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16, MakeFormatInformation(VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16, FormatType::Planar, COLOUR_RGBA, 8)},
	{VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16, MakeFormatInformation(VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16, FormatType::Planar, COLOUR_RGBA, 8)},
	{VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16, MakeFormatInformation(VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16, FormatType::Planar, COLOUR_RGB, 6)},
	{VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16, MakeFormatInformation(VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16, FormatType::Planar, COLOUR_RGB, 6)},
	{VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16, MakeFormatInformation(VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16, FormatType::Planar, COLOUR_RGB, 6)},
	{VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16, MakeFormatInformation(VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16, FormatType::Planar, COLOUR_RGB, 6)},
	{VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16, MakeFormatInformation(VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16, FormatType::Planar, COLOUR_RGB, 6)},
	{VK_FORMAT_G16B16G16R16_422_UNORM, MakeFormatInformation(VK_FORMAT_G16B16G16R16_422_UNORM, FormatType::Planar, COLOUR_RGB, 8)},
	{VK_FORMAT_B16G16R16G16_422_UNORM, MakeFormatInformation(VK_FORMAT_B16G16R16G16_422_UNORM, FormatType::Planar, COLOUR_RGB, 8)},
	{VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM, MakeFormatInformation(VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM, FormatType::Planar, COLOUR_RGB, 8)},
	{VK_FORMAT_G16_B16R16_2PLANE_420_UNORM, MakeFormatInformation(VK_FORMAT_G16_B16R16_2PLANE_420_UNORM, FormatType::Planar, COLOUR_RGB, 8)},
	{VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM, MakeFormatInformation(VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM, FormatType::Planar, COLOUR_RGB, 8)},
	{VK_FORMAT_G16_B16R16_2PLANE_422_UNORM, MakeFormatInformation(VK_FORMAT_G16_B16R16_2PLANE_422_UNORM, FormatType::Planar, COLOUR_RGB, 8)},
	{VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM, MakeFormatInformation(VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM, FormatType::Planar, COLOUR_RGB, 8)},
	{VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG, MakeCompressedFormatInformation(VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG, COLOUR_RGBA, 8, BaseType::UNorm, 8, 4)},
	{VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG, MakeCompressedFormatInformation(VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG, COLOUR_RGBA, 8, BaseType::UNorm, 4, 4)},
	{VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG, MakeCompressedFormatInformation(VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG, COLOUR_RGBA, 8, BaseType::UNorm, 8, 4)},
	{VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG, MakeCompressedFormatInformation(VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG, COLOUR_RGBA, 8, BaseType::UNorm, 4, 4)},
	{VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG, MakeCompressedFormatInformation(VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG, COLOUR_RGBA, 8, BaseType::SRGB, 8, 4)},
	{VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG, MakeCompressedFormatInformation(VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG, COLOUR_RGBA, 8, BaseType::SRGB, 4, 4)},
	{VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG, MakeCompressedFormatInformation(VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG, COLOUR_RGBA, 8, BaseType::SRGB, 8, 4)},
	{VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG, MakeCompressedFormatInformation(VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG, COLOUR_RGBA, 8, BaseType::SRGB, 4, 4)},
};

const FormatInformation& GetFormatInformation(VkFormat format)
{
	const auto iFormat = static_cast<int>(format);
	if (iFormat >= VK_FORMAT_BEGIN_RANGE && iFormat <= VK_FORMAT_END_RANGE)
	{
		return gsl::at(formatInformation, iFormat);
	}

	return extraFormatInformation[format];
}

static ImageSize GetNormalImageSize(const FormatInformation& format, uint32_t width, uint32_t height, uint32_t depth, uint32_t arrayLayers, uint32_t mipLevels)
{
	const auto maxMip = CountMipLevels(width, height, depth);
	assert(maxMip >= mipLevels);

	ImageSize imageSize{};
	imageSize.PixelSize = format.TotalSize;
	imageSize.NumberLayers = arrayLayers;
	imageSize.NumberMipLevels = mipLevels;
	for (auto i = 0u; i < mipLevels; i++)
	{
		auto& level = gsl::at(imageSize.Level, i);
		level.Offset = imageSize.LayerSize;
		level.Width = width;
		level.Height = height;
		level.Depth = depth;
		level.Stride = format.TotalSize * level.Width;
		// level.Stride = ((level.Stride + 3) / 4) * 4;
		level.PlaneSize = level.Stride * level.Height;
		level.LevelSize = level.PlaneSize * level.Depth;
		imageSize.LayerSize += level.LevelSize;

		width = std::max(width / 2, 1u);
		height = std::max(height / 2, 1u);
		depth = std::max(depth / 2, 1u);
	}

	imageSize.TotalSize = imageSize.LayerSize * arrayLayers;
	return imageSize;
}

static ImageSize GetCompressedImageSize(const FormatInformation& format, uint32_t width, uint32_t height, uint32_t depth, uint32_t arrayLayers, uint32_t mipLevels)
{
	const auto maxMip = CountMipLevels(width, height, depth);
	assert(maxMip >= mipLevels);

	const auto blockWidth = format.Compressed.BlockWidth;
	const auto blockHeight = format.Compressed.BlockHeight;

	ImageSize imageSize{};
	imageSize.PixelSize = format.TotalSize;
	imageSize.NumberLayers = arrayLayers;
	imageSize.NumberMipLevels = mipLevels;
	
	for (auto i = 0u; i < mipLevels; i++)
	{
		auto& level = gsl::at(imageSize.Level, i);
		level.Offset = imageSize.LayerSize;
		level.Width = (width + blockWidth - 1) / blockWidth;
		level.Height = (height + blockHeight - 1) / blockHeight;
		level.Depth = depth;
		level.Stride = format.TotalSize * level.Width;
		level.Stride = ((level.Stride + 3) / 4) * 4;
		level.PlaneSize = level.Stride * level.Height;
		level.LevelSize = level.PlaneSize * level.Depth;
		imageSize.LayerSize += level.LevelSize;

		width = std::max(width / 2, 1u);
		height = std::max(height / 2, 1u);
		depth = std::max(depth / 2, 1u);
	}

	imageSize.TotalSize = imageSize.LayerSize * arrayLayers;
	return imageSize;
}

ImageSize GetImageSize(const FormatInformation& format, uint32_t width, uint32_t height, uint32_t depth, uint32_t arrayLayers, uint32_t mipLevels)
{
	assert(width > 0);
	assert(height > 0);
	assert(depth > 0);
	assert(arrayLayers > 0);
	assert(mipLevels > 0);

	switch (format.Type)
	{
	case FormatType::Normal:
	case FormatType::Packed:
	case FormatType::DepthStencil:
		return GetNormalImageSize(format, width, height, depth, arrayLayers, mipLevels);
		
	case FormatType::Compressed:
		return GetCompressedImageSize(format, width, height, depth, arrayLayers, mipLevels);
		
	case FormatType::Planar:
		FATAL_ERROR();
		
	case FormatType::PlanarSamplable:
		FATAL_ERROR();
		
	default:
		assert(false);
		exit(1);
	}
}

uint64_t GetImagePixelOffset(const ImageSize& imageSize, int32_t i, int32_t j, int32_t k, uint32_t level, uint32_t layer)
{
	const auto& mipLevel = gsl::at(imageSize.Level, level);
	return imageSize.LayerSize * layer + mipLevel.Offset + k * mipLevel.PlaneSize + j * mipLevel.Stride + i * imageSize.PixelSize;
}

bool NeedsYCBCRConversion(VkFormat format)
{
	return format == VK_FORMAT_G8B8G8R8_422_UNORM ||
		format == VK_FORMAT_B8G8R8G8_422_UNORM ||
		format == VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM ||
		format == VK_FORMAT_G8_B8R8_2PLANE_420_UNORM ||
		format == VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM ||
		format == VK_FORMAT_G8_B8R8_2PLANE_422_UNORM ||
		format == VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM ||
		format == VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16 ||
		format == VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16 ||
		format == VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16 ||
		format == VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16 ||
		format == VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16 ||
		format == VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16 ||
		format == VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16 ||
		format == VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16 ||
		format == VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16 ||
		format == VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16 ||
		format == VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16 ||
		format == VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16 ||
		format == VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16 ||
		format == VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16 ||
		format == VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16 ||
		format == VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16 ||
		format == VK_FORMAT_G16B16G16R16_422_UNORM ||
		format == VK_FORMAT_B16G16R16G16_422_UNORM ||
		format == VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM ||
		format == VK_FORMAT_G16_B16R16_2PLANE_420_UNORM ||
		format == VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM ||
		format == VK_FORMAT_G16_B16R16_2PLANE_422_UNORM ||
		format == VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM;
}
