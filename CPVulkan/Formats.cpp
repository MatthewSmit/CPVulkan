#include "Formats.h"

#include <unordered_map>

static constexpr VkFormatFeatureFlags GetFeatures(FormatType type)
{
	VkFormatFeatureFlags features = 0;
	switch (type)
	{
	case FormatType::Normal:
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
	default:
		FATAL_ERROR();
	}

	return features;
}

static constexpr FormatInformation MakeFormatInformation(VkFormat format, FormatType type)
{
	const auto features = GetFeatures(type);
	return FormatInformation
	{
		format,
		type,
		features,
		features,
		type != FormatType::Normal ? static_cast<VkFormatFeatureFlags>(0) : features,
	};
}

static constexpr FormatInformation MakeFormatInformation(VkFormat format, FormatType type, uint32_t totalSize)
{
	const auto features = GetFeatures(type);
	return FormatInformation
	{
		format,
		type,
		features,
		features,
		type != FormatType::Normal ? static_cast<VkFormatFeatureFlags>(0) : features,
		totalSize,
	};
}

static constexpr FormatInformation MakeFormatInformation(VkFormat format, FormatType type, uint32_t totalSize, uint32_t elementSize, BaseType baseType,
                                                         uint32_t redOffset, uint32_t greenOffset, uint32_t blueOffset, uint32_t alphaOffset)
{
	const auto features = GetFeatures(type);
	return FormatInformation
	{
		format,
		type,
		features,
		features,
		type != FormatType::Normal ? static_cast<VkFormatFeatureFlags>(0) : features,
		totalSize,
		elementSize,
		baseType,
		redOffset,
		greenOffset,
		blueOffset,
		alphaOffset,
		0,
		0,
		0,
		0,
	};
}

static constexpr FormatInformation MakeFormatInformation(VkFormat format, FormatType type,
                                                         uint32_t totalSize, uint32_t elementSize, BaseType baseType,
                                                         uint32_t redOffset, uint32_t greenOffset, uint32_t blueOffset, uint32_t alphaOffset,
                                                         uint32_t redPack, uint32_t greenPack, uint32_t bluePack, uint32_t alphaPack)
{
	const auto features = GetFeatures(type);
	return FormatInformation
	{
		format,
		type,
		features,
		features,
		type != FormatType::Normal ? static_cast<VkFormatFeatureFlags>(0) : features,
		totalSize,
		elementSize,
		baseType,
		redOffset,
		greenOffset,
		blueOffset,
		alphaOffset,
		redPack,
		greenPack,
		bluePack,
		alphaPack,
	};
}

static constexpr FormatInformation formatInformation[]
{
	MakeFormatInformation(VK_FORMAT_UNDEFINED, FormatType::Normal, 0),
	MakeFormatInformation(VK_FORMAT_R4G4_UNORM_PACK8, FormatType::Normal, 1, 0, BaseType::UNorm, 4, 0, 0, 0, 4, 4, 0, 0),
	MakeFormatInformation(VK_FORMAT_R4G4B4A4_UNORM_PACK16, FormatType::Normal, 2, 0, BaseType::UNorm, 12, 8, 4, 0, 4, 4, 4, 4),
	MakeFormatInformation(VK_FORMAT_B4G4R4A4_UNORM_PACK16, FormatType::Normal, 2, 0, BaseType::UNorm, 4, 8, 12, 0, 4, 4, 4, 4),
	MakeFormatInformation(VK_FORMAT_R5G6B5_UNORM_PACK16, FormatType::Normal, 2, 0, BaseType::UNorm, 11, 5, 0, 0, 5, 6, 5, 0),
	MakeFormatInformation(VK_FORMAT_B5G6R5_UNORM_PACK16, FormatType::Normal, 2, 0, BaseType::UNorm, 0, 5, 11, 0, 5, 6, 5, 0),
	MakeFormatInformation(VK_FORMAT_R5G5B5A1_UNORM_PACK16, FormatType::Normal, 2, 0, BaseType::UNorm, 11, 6, 1, 0, 5, 5, 5, 1),
	MakeFormatInformation(VK_FORMAT_B5G5R5A1_UNORM_PACK16, FormatType::Normal, 2, 0, BaseType::UNorm, 1, 6, 11, 0, 5, 5, 5, 1),
	MakeFormatInformation(VK_FORMAT_A1R5G5B5_UNORM_PACK16, FormatType::Normal, 2, 0, BaseType::UNorm, 10, 5, 0, 15, 5, 5, 5, 1),
	MakeFormatInformation(VK_FORMAT_R8_UNORM, FormatType::Normal, 1, 1, BaseType::UNorm, 0, -1, -1, -1),
	MakeFormatInformation(VK_FORMAT_R8_SNORM, FormatType::Normal, 1, 1, BaseType::SNorm, 0, -1, -1, -1),
	MakeFormatInformation(VK_FORMAT_R8_USCALED, FormatType::Normal, 1, 1, BaseType::UScaled, 0, -1, -1, -1),
	MakeFormatInformation(VK_FORMAT_R8_SSCALED, FormatType::Normal, 1, 1, BaseType::SScaled, 0, -1, -1, -1),
	MakeFormatInformation(VK_FORMAT_R8_UINT, FormatType::Normal, 1, 1, BaseType::UInt, 0, -1, -1, -1),
	MakeFormatInformation(VK_FORMAT_R8_SINT, FormatType::Normal, 1, 1, BaseType::SInt, 0, -1, -1, -1),
	MakeFormatInformation(VK_FORMAT_R8_SRGB, FormatType::Normal, 1, 1, BaseType::SRGB, 0, -1, -1, -1),
	MakeFormatInformation(VK_FORMAT_R8G8_UNORM, FormatType::Normal, 2, 1, BaseType::UNorm, 0, 1, -1, -1),
	MakeFormatInformation(VK_FORMAT_R8G8_SNORM, FormatType::Normal, 2, 1, BaseType::SNorm, 0, 1, -1, -1),
	MakeFormatInformation(VK_FORMAT_R8G8_USCALED, FormatType::Normal, 2, 1, BaseType::UScaled, 0, 1, -1, -1),
	MakeFormatInformation(VK_FORMAT_R8G8_SSCALED, FormatType::Normal, 2, 1, BaseType::SScaled, 0, 1, -1, -1),
	MakeFormatInformation(VK_FORMAT_R8G8_UINT, FormatType::Normal, 2, 1, BaseType::UInt, 0, 1, -1, -1),
	MakeFormatInformation(VK_FORMAT_R8G8_SINT, FormatType::Normal, 2, 1, BaseType::SInt, 0, 1, -1, -1),
	MakeFormatInformation(VK_FORMAT_R8G8_SRGB, FormatType::Normal, 2, 1, BaseType::SRGB, 0, 1, -1, -1),
	MakeFormatInformation(VK_FORMAT_R8G8B8_UNORM, FormatType::Normal, 3, 1, BaseType::UNorm, 0, 1, 2, -1),
	MakeFormatInformation(VK_FORMAT_R8G8B8_SNORM, FormatType::Normal, 3, 1, BaseType::SNorm, 0, 1, 2, -1),
	MakeFormatInformation(VK_FORMAT_R8G8B8_USCALED, FormatType::Normal, 3, 1, BaseType::UScaled, 0, 1, 2, -1),
	MakeFormatInformation(VK_FORMAT_R8G8B8_SSCALED, FormatType::Normal, 3, 1, BaseType::SScaled, 0, 1, 2, -1),
	MakeFormatInformation(VK_FORMAT_R8G8B8_UINT, FormatType::Normal, 3, 1, BaseType::UInt, 0, 1, 2, -1),
	MakeFormatInformation(VK_FORMAT_R8G8B8_SINT, FormatType::Normal, 3, 1, BaseType::SInt, 0, 1, 2, -1),
	MakeFormatInformation(VK_FORMAT_R8G8B8_SRGB, FormatType::Normal, 3, 1, BaseType::SRGB, 0, 1, 2, -1),
	MakeFormatInformation(VK_FORMAT_B8G8R8_UNORM, FormatType::Normal, 3, 1, BaseType::UNorm, 2, 1, 0, -1),
	MakeFormatInformation(VK_FORMAT_B8G8R8_SNORM, FormatType::Normal, 3),
	MakeFormatInformation(VK_FORMAT_B8G8R8_USCALED, FormatType::Normal, 3),
	MakeFormatInformation(VK_FORMAT_B8G8R8_SSCALED, FormatType::Normal, 3),
	MakeFormatInformation(VK_FORMAT_B8G8R8_UINT, FormatType::Normal, 3),
	MakeFormatInformation(VK_FORMAT_B8G8R8_SINT, FormatType::Normal, 3),
	MakeFormatInformation(VK_FORMAT_B8G8R8_SRGB, FormatType::Normal, 3),
	MakeFormatInformation(VK_FORMAT_R8G8B8A8_UNORM, FormatType::Normal, 4, 1, BaseType::UNorm, 0, 1, 2, 3),
	MakeFormatInformation(VK_FORMAT_R8G8B8A8_SNORM, FormatType::Normal, 4),
	MakeFormatInformation(VK_FORMAT_R8G8B8A8_USCALED, FormatType::Normal, 4),
	MakeFormatInformation(VK_FORMAT_R8G8B8A8_SSCALED, FormatType::Normal, 4),
	MakeFormatInformation(VK_FORMAT_R8G8B8A8_UINT, FormatType::Normal, 4),
	MakeFormatInformation(VK_FORMAT_R8G8B8A8_SINT, FormatType::Normal, 4),
	MakeFormatInformation(VK_FORMAT_R8G8B8A8_SRGB, FormatType::Normal, 4),
	MakeFormatInformation(VK_FORMAT_B8G8R8A8_UNORM, FormatType::Normal, 4, 1, BaseType::UNorm, 2, 1, 0, 3),
	MakeFormatInformation(VK_FORMAT_B8G8R8A8_SNORM, FormatType::Normal, 4),
	MakeFormatInformation(VK_FORMAT_B8G8R8A8_USCALED, FormatType::Normal, 4),
	MakeFormatInformation(VK_FORMAT_B8G8R8A8_SSCALED, FormatType::Normal, 4),
	MakeFormatInformation(VK_FORMAT_B8G8R8A8_UINT, FormatType::Normal, 4),
	MakeFormatInformation(VK_FORMAT_B8G8R8A8_SINT, FormatType::Normal, 4),
	MakeFormatInformation(VK_FORMAT_B8G8R8A8_SRGB, FormatType::Normal, 4),
	MakeFormatInformation(VK_FORMAT_A8B8G8R8_UNORM_PACK32, FormatType::Normal, 4),
	MakeFormatInformation(VK_FORMAT_A8B8G8R8_SNORM_PACK32, FormatType::Normal, 4),
	MakeFormatInformation(VK_FORMAT_A8B8G8R8_USCALED_PACK32, FormatType::Normal, 4),
	MakeFormatInformation(VK_FORMAT_A8B8G8R8_SSCALED_PACK32, FormatType::Normal, 4),
	MakeFormatInformation(VK_FORMAT_A8B8G8R8_UINT_PACK32, FormatType::Normal, 4),
	MakeFormatInformation(VK_FORMAT_A8B8G8R8_SINT_PACK32, FormatType::Normal, 4),
	MakeFormatInformation(VK_FORMAT_A8B8G8R8_SRGB_PACK32, FormatType::Normal, 4),
	MakeFormatInformation(VK_FORMAT_A2R10G10B10_UNORM_PACK32, FormatType::Normal, 4),
	MakeFormatInformation(VK_FORMAT_A2R10G10B10_SNORM_PACK32, FormatType::Normal, 4),
	MakeFormatInformation(VK_FORMAT_A2R10G10B10_USCALED_PACK32, FormatType::Normal, 4),
	MakeFormatInformation(VK_FORMAT_A2R10G10B10_SSCALED_PACK32, FormatType::Normal, 4),
	MakeFormatInformation(VK_FORMAT_A2R10G10B10_UINT_PACK32, FormatType::Normal, 4),
	MakeFormatInformation(VK_FORMAT_A2R10G10B10_SINT_PACK32, FormatType::Normal, 4),
	MakeFormatInformation(VK_FORMAT_A2B10G10R10_UNORM_PACK32, FormatType::Normal, 4),
	MakeFormatInformation(VK_FORMAT_A2B10G10R10_SNORM_PACK32, FormatType::Normal, 4),
	MakeFormatInformation(VK_FORMAT_A2B10G10R10_USCALED_PACK32, FormatType::Normal, 4),
	MakeFormatInformation(VK_FORMAT_A2B10G10R10_SSCALED_PACK32, FormatType::Normal, 4),
	MakeFormatInformation(VK_FORMAT_A2B10G10R10_UINT_PACK32, FormatType::Normal, 4),
	MakeFormatInformation(VK_FORMAT_A2B10G10R10_SINT_PACK32, FormatType::Normal, 4),
	MakeFormatInformation(VK_FORMAT_R16_UNORM, FormatType::Normal, 2),
	MakeFormatInformation(VK_FORMAT_R16_SNORM, FormatType::Normal, 2),
	MakeFormatInformation(VK_FORMAT_R16_USCALED, FormatType::Normal, 2),
	MakeFormatInformation(VK_FORMAT_R16_SSCALED, FormatType::Normal, 2),
	MakeFormatInformation(VK_FORMAT_R16_UINT, FormatType::Normal, 2),
	MakeFormatInformation(VK_FORMAT_R16_SINT, FormatType::Normal, 2),
	MakeFormatInformation(VK_FORMAT_R16_SFLOAT, FormatType::Normal, 2),
	MakeFormatInformation(VK_FORMAT_R16G16_UNORM, FormatType::Normal),
	MakeFormatInformation(VK_FORMAT_R16G16_SNORM, FormatType::Normal),
	MakeFormatInformation(VK_FORMAT_R16G16_USCALED, FormatType::Normal),
	MakeFormatInformation(VK_FORMAT_R16G16_SSCALED, FormatType::Normal),
	MakeFormatInformation(VK_FORMAT_R16G16_UINT, FormatType::Normal),
	MakeFormatInformation(VK_FORMAT_R16G16_SINT, FormatType::Normal),
	MakeFormatInformation(VK_FORMAT_R16G16_SFLOAT, FormatType::Normal),
	MakeFormatInformation(VK_FORMAT_R16G16B16_UNORM, FormatType::Normal),
	MakeFormatInformation(VK_FORMAT_R16G16B16_SNORM, FormatType::Normal),
	MakeFormatInformation(VK_FORMAT_R16G16B16_USCALED, FormatType::Normal),
	MakeFormatInformation(VK_FORMAT_R16G16B16_SSCALED, FormatType::Normal),
	MakeFormatInformation(VK_FORMAT_R16G16B16_UINT, FormatType::Normal),
	MakeFormatInformation(VK_FORMAT_R16G16B16_SINT, FormatType::Normal),
	MakeFormatInformation(VK_FORMAT_R16G16B16_SFLOAT, FormatType::Normal),
	MakeFormatInformation(VK_FORMAT_R16G16B16A16_UNORM, FormatType::Normal),
	MakeFormatInformation(VK_FORMAT_R16G16B16A16_SNORM, FormatType::Normal),
	MakeFormatInformation(VK_FORMAT_R16G16B16A16_USCALED, FormatType::Normal),
	MakeFormatInformation(VK_FORMAT_R16G16B16A16_SSCALED, FormatType::Normal),
	MakeFormatInformation(VK_FORMAT_R16G16B16A16_UINT, FormatType::Normal),
	MakeFormatInformation(VK_FORMAT_R16G16B16A16_SINT, FormatType::Normal),
	MakeFormatInformation(VK_FORMAT_R16G16B16A16_SFLOAT, FormatType::Normal),
	MakeFormatInformation(VK_FORMAT_R32_UINT, FormatType::Normal, 4, 4, BaseType::UInt, 0, -1, -1, -1),
	MakeFormatInformation(VK_FORMAT_R32_SINT, FormatType::Normal),
	MakeFormatInformation(VK_FORMAT_R32_SFLOAT, FormatType::Normal),
	MakeFormatInformation(VK_FORMAT_R32G32_UINT, FormatType::Normal),
	MakeFormatInformation(VK_FORMAT_R32G32_SINT, FormatType::Normal),
	MakeFormatInformation(VK_FORMAT_R32G32_SFLOAT, FormatType::Normal),
	MakeFormatInformation(VK_FORMAT_R32G32B32_UINT, FormatType::Normal),
	MakeFormatInformation(VK_FORMAT_R32G32B32_SINT, FormatType::Normal),
	MakeFormatInformation(VK_FORMAT_R32G32B32_SFLOAT, FormatType::Normal),
	MakeFormatInformation(VK_FORMAT_R32G32B32A32_UINT, FormatType::Normal),
	MakeFormatInformation(VK_FORMAT_R32G32B32A32_SINT, FormatType::Normal),
	MakeFormatInformation(VK_FORMAT_R32G32B32A32_SFLOAT, FormatType::Normal),
	MakeFormatInformation(VK_FORMAT_R64_UINT, FormatType::Normal),
	MakeFormatInformation(VK_FORMAT_R64_SINT, FormatType::Normal),
	MakeFormatInformation(VK_FORMAT_R64_SFLOAT, FormatType::Normal),
	MakeFormatInformation(VK_FORMAT_R64G64_UINT, FormatType::Normal),
	MakeFormatInformation(VK_FORMAT_R64G64_SINT, FormatType::Normal),
	MakeFormatInformation(VK_FORMAT_R64G64_SFLOAT, FormatType::Normal),
	MakeFormatInformation(VK_FORMAT_R64G64B64_UINT, FormatType::Normal),
	MakeFormatInformation(VK_FORMAT_R64G64B64_SINT, FormatType::Normal),
	MakeFormatInformation(VK_FORMAT_R64G64B64_SFLOAT, FormatType::Normal),
	MakeFormatInformation(VK_FORMAT_R64G64B64A64_UINT, FormatType::Normal),
	MakeFormatInformation(VK_FORMAT_R64G64B64A64_SINT, FormatType::Normal),
	MakeFormatInformation(VK_FORMAT_R64G64B64A64_SFLOAT, FormatType::Normal),
	MakeFormatInformation(VK_FORMAT_B10G11R11_UFLOAT_PACK32, FormatType::Normal),
	MakeFormatInformation(VK_FORMAT_E5B9G9R9_UFLOAT_PACK32, FormatType::Normal),
	MakeFormatInformation(VK_FORMAT_D16_UNORM, FormatType::Normal, 2, 2, BaseType::UNorm, 0, -1, -1, -1),
	MakeFormatInformation(VK_FORMAT_X8_D24_UNORM_PACK32, FormatType::Normal, 4),
	MakeFormatInformation(VK_FORMAT_D32_SFLOAT, FormatType::Normal, 4),
	MakeFormatInformation(VK_FORMAT_S8_UINT, FormatType::Normal, 1),
	MakeFormatInformation(VK_FORMAT_D16_UNORM_S8_UINT, FormatType::Normal),
	MakeFormatInformation(VK_FORMAT_D24_UNORM_S8_UINT, FormatType::Normal),
	MakeFormatInformation(VK_FORMAT_D32_SFLOAT_S8_UINT, FormatType::Normal),
	MakeFormatInformation(VK_FORMAT_BC1_RGB_UNORM_BLOCK, FormatType::Compressed),
	MakeFormatInformation(VK_FORMAT_BC1_RGB_SRGB_BLOCK, FormatType::Compressed),
	MakeFormatInformation(VK_FORMAT_BC1_RGBA_UNORM_BLOCK, FormatType::Compressed),
	MakeFormatInformation(VK_FORMAT_BC1_RGBA_SRGB_BLOCK, FormatType::Compressed),
	MakeFormatInformation(VK_FORMAT_BC2_UNORM_BLOCK, FormatType::Compressed),
	MakeFormatInformation(VK_FORMAT_BC2_SRGB_BLOCK, FormatType::Compressed),
	MakeFormatInformation(VK_FORMAT_BC3_UNORM_BLOCK, FormatType::Compressed),
	MakeFormatInformation(VK_FORMAT_BC3_SRGB_BLOCK, FormatType::Compressed),
	MakeFormatInformation(VK_FORMAT_BC4_UNORM_BLOCK, FormatType::Compressed),
	MakeFormatInformation(VK_FORMAT_BC4_SNORM_BLOCK, FormatType::Compressed),
	MakeFormatInformation(VK_FORMAT_BC5_UNORM_BLOCK, FormatType::Compressed),
	MakeFormatInformation(VK_FORMAT_BC5_SNORM_BLOCK, FormatType::Compressed),
	MakeFormatInformation(VK_FORMAT_BC6H_UFLOAT_BLOCK, FormatType::Compressed),
	MakeFormatInformation(VK_FORMAT_BC6H_SFLOAT_BLOCK, FormatType::Compressed),
	MakeFormatInformation(VK_FORMAT_BC7_UNORM_BLOCK, FormatType::Compressed),
	MakeFormatInformation(VK_FORMAT_BC7_SRGB_BLOCK, FormatType::Compressed),
	MakeFormatInformation(VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK, FormatType::Compressed),
	MakeFormatInformation(VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK, FormatType::Compressed),
	MakeFormatInformation(VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK, FormatType::Compressed),
	MakeFormatInformation(VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK, FormatType::Compressed),
	MakeFormatInformation(VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK, FormatType::Compressed),
	MakeFormatInformation(VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK, FormatType::Compressed),
	MakeFormatInformation(VK_FORMAT_EAC_R11_UNORM_BLOCK, FormatType::Compressed),
	MakeFormatInformation(VK_FORMAT_EAC_R11_SNORM_BLOCK, FormatType::Compressed),
	MakeFormatInformation(VK_FORMAT_EAC_R11G11_UNORM_BLOCK, FormatType::Compressed),
	MakeFormatInformation(VK_FORMAT_EAC_R11G11_SNORM_BLOCK, FormatType::Compressed),
	MakeFormatInformation(VK_FORMAT_ASTC_4x4_UNORM_BLOCK, FormatType::Compressed),
	MakeFormatInformation(VK_FORMAT_ASTC_4x4_SRGB_BLOCK, FormatType::Compressed),
	MakeFormatInformation(VK_FORMAT_ASTC_5x4_UNORM_BLOCK, FormatType::Compressed),
	MakeFormatInformation(VK_FORMAT_ASTC_5x4_SRGB_BLOCK, FormatType::Compressed),
	MakeFormatInformation(VK_FORMAT_ASTC_5x5_UNORM_BLOCK, FormatType::Compressed),
	MakeFormatInformation(VK_FORMAT_ASTC_5x5_SRGB_BLOCK, FormatType::Compressed),
	MakeFormatInformation(VK_FORMAT_ASTC_6x5_UNORM_BLOCK, FormatType::Compressed),
	MakeFormatInformation(VK_FORMAT_ASTC_6x5_SRGB_BLOCK, FormatType::Compressed),
	MakeFormatInformation(VK_FORMAT_ASTC_6x6_UNORM_BLOCK, FormatType::Compressed),
	MakeFormatInformation(VK_FORMAT_ASTC_6x6_SRGB_BLOCK, FormatType::Compressed),
	MakeFormatInformation(VK_FORMAT_ASTC_8x5_UNORM_BLOCK, FormatType::Compressed),
	MakeFormatInformation(VK_FORMAT_ASTC_8x5_SRGB_BLOCK, FormatType::Compressed),
	MakeFormatInformation(VK_FORMAT_ASTC_8x6_UNORM_BLOCK, FormatType::Compressed),
	MakeFormatInformation(VK_FORMAT_ASTC_8x6_SRGB_BLOCK, FormatType::Compressed),
	MakeFormatInformation(VK_FORMAT_ASTC_8x8_UNORM_BLOCK, FormatType::Compressed),
	MakeFormatInformation(VK_FORMAT_ASTC_8x8_SRGB_BLOCK, FormatType::Compressed),
	MakeFormatInformation(VK_FORMAT_ASTC_10x5_UNORM_BLOCK, FormatType::Compressed),
	MakeFormatInformation(VK_FORMAT_ASTC_10x5_SRGB_BLOCK, FormatType::Compressed),
	MakeFormatInformation(VK_FORMAT_ASTC_10x6_UNORM_BLOCK, FormatType::Compressed),
	MakeFormatInformation(VK_FORMAT_ASTC_10x6_SRGB_BLOCK, FormatType::Compressed),
	MakeFormatInformation(VK_FORMAT_ASTC_10x8_UNORM_BLOCK, FormatType::Compressed),
	MakeFormatInformation(VK_FORMAT_ASTC_10x8_SRGB_BLOCK, FormatType::Compressed),
	MakeFormatInformation(VK_FORMAT_ASTC_10x10_UNORM_BLOCK, FormatType::Compressed),
	MakeFormatInformation(VK_FORMAT_ASTC_10x10_SRGB_BLOCK, FormatType::Compressed),
	MakeFormatInformation(VK_FORMAT_ASTC_12x10_UNORM_BLOCK, FormatType::Compressed),
	MakeFormatInformation(VK_FORMAT_ASTC_12x10_SRGB_BLOCK, FormatType::Compressed),
	MakeFormatInformation(VK_FORMAT_ASTC_12x12_UNORM_BLOCK, FormatType::Compressed),
	MakeFormatInformation(VK_FORMAT_ASTC_12x12_SRGB_BLOCK, FormatType::Compressed),
};

static std::unordered_map<VkFormat, FormatInformation> extraFormatInformation
{
	{VK_FORMAT_G8B8G8R8_422_UNORM, MakeFormatInformation(VK_FORMAT_G8B8G8R8_422_UNORM, FormatType::Planar)},
	{VK_FORMAT_B8G8R8G8_422_UNORM, MakeFormatInformation(VK_FORMAT_B8G8R8G8_422_UNORM, FormatType::Planar)},
	{VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM, MakeFormatInformation(VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM, FormatType::PlanarSamplable)},
	{VK_FORMAT_G8_B8R8_2PLANE_420_UNORM, MakeFormatInformation(VK_FORMAT_G8_B8R8_2PLANE_420_UNORM, FormatType::PlanarSamplable)},
	{VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM, MakeFormatInformation(VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM, FormatType::PlanarSamplable)},
	{VK_FORMAT_G8_B8R8_2PLANE_422_UNORM, MakeFormatInformation(VK_FORMAT_G8_B8R8_2PLANE_422_UNORM, FormatType::PlanarSamplable)},
	{VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM, MakeFormatInformation(VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM, FormatType::Planar)},
	{VK_FORMAT_R10X6_UNORM_PACK16, MakeFormatInformation(VK_FORMAT_R10X6_UNORM_PACK16, FormatType::Planar)},
	{VK_FORMAT_R10X6G10X6_UNORM_2PACK16, MakeFormatInformation(VK_FORMAT_R10X6G10X6_UNORM_2PACK16, FormatType::Planar)},
	{VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16, MakeFormatInformation(VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16, FormatType::Planar)},
	{VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16, MakeFormatInformation(VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16, FormatType::Planar)},
	{VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16, MakeFormatInformation(VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16, FormatType::Planar)},
	{VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16, MakeFormatInformation(VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16, FormatType::Planar)},
	{VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16, MakeFormatInformation(VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16, FormatType::Planar)},
	{VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16, MakeFormatInformation(VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16, FormatType::Planar)},
	{VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16, MakeFormatInformation(VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16, FormatType::Planar)},
	{VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16, MakeFormatInformation(VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16, FormatType::Planar)},
	{VK_FORMAT_R12X4_UNORM_PACK16, MakeFormatInformation(VK_FORMAT_R12X4_UNORM_PACK16, FormatType::Planar)},
	{VK_FORMAT_R12X4G12X4_UNORM_2PACK16, MakeFormatInformation(VK_FORMAT_R12X4G12X4_UNORM_2PACK16, FormatType::Planar)},
	{VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16, MakeFormatInformation(VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16, FormatType::Planar)},
	{VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16, MakeFormatInformation(VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16, FormatType::Planar)},
	{VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16, MakeFormatInformation(VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16, FormatType::Planar)},
	{VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16, MakeFormatInformation(VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16, FormatType::Planar)},
	{VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16, MakeFormatInformation(VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16, FormatType::Planar)},
	{VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16, MakeFormatInformation(VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16, FormatType::Planar)},
	{VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16, MakeFormatInformation(VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16, FormatType::Planar)},
	{VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16, MakeFormatInformation(VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16, FormatType::Planar)},
	{VK_FORMAT_G16B16G16R16_422_UNORM, MakeFormatInformation(VK_FORMAT_G16B16G16R16_422_UNORM, FormatType::Planar)},
	{VK_FORMAT_B16G16R16G16_422_UNORM, MakeFormatInformation(VK_FORMAT_B16G16R16G16_422_UNORM, FormatType::Planar)},
	{VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM, MakeFormatInformation(VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM, FormatType::Planar)},
	{VK_FORMAT_G16_B16R16_2PLANE_420_UNORM, MakeFormatInformation(VK_FORMAT_G16_B16R16_2PLANE_420_UNORM, FormatType::Planar)},
	{VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM, MakeFormatInformation(VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM, FormatType::Planar)},
	{VK_FORMAT_G16_B16R16_2PLANE_422_UNORM, MakeFormatInformation(VK_FORMAT_G16_B16R16_2PLANE_422_UNORM, FormatType::Planar)},
	{VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM, MakeFormatInformation(VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM, FormatType::Planar)},
	{VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG, MakeFormatInformation(VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG, FormatType::Compressed)},
	{VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG, MakeFormatInformation(VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG, FormatType::Compressed)},
	{VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG, MakeFormatInformation(VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG, FormatType::Compressed)},
	{VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG, MakeFormatInformation(VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG, FormatType::Compressed)},
	{VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG, MakeFormatInformation(VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG, FormatType::Compressed)},
	{VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG, MakeFormatInformation(VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG, FormatType::Compressed)},
	{VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG, MakeFormatInformation(VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG, FormatType::Compressed)},
	{VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG, MakeFormatInformation(VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG, FormatType::Compressed)},
};

const FormatInformation& GetFormatInformation(VkFormat format)
{
	const auto iFormat = static_cast<int>(format);
	if (iFormat >= VK_FORMAT_BEGIN_RANGE && iFormat <= VK_FORMAT_END_RANGE)
	{
		return formatInformation[iFormat];
	}

	return extraFormatInformation[format];
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
