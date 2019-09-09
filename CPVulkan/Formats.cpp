#include "Formats.h"

#include <unordered_map>

static constexpr FormatInformation MakeFormatInformation(VkFormat format, bool isBlockCompressed, bool isMultiPlane)
{
	const auto features = static_cast<VkFormatFeatureFlags>(VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT |
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
	return FormatInformation
	{
		format,
		features | (isMultiPlane ? VK_FORMAT_FEATURE_DISJOINT_BIT : 0),
		features | (isMultiPlane ? VK_FORMAT_FEATURE_DISJOINT_BIT : 0),
		isBlockCompressed ? static_cast<VkFormatFeatureFlags>(0) : features,
	};
}

static constexpr FormatInformation MakeFormatInformation(VkFormat format, bool isBlockCompressed, bool isMultiPlane, uint32_t totalSize)
{
	const auto features = static_cast<VkFormatFeatureFlags>(VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT |
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
	return FormatInformation
	{
		format,
		features | (isMultiPlane ? VK_FORMAT_FEATURE_DISJOINT_BIT : 0),
		features | (isMultiPlane ? VK_FORMAT_FEATURE_DISJOINT_BIT : 0),
		isBlockCompressed ? static_cast<VkFormatFeatureFlags>(0) : features,
		totalSize,
	};
}

static constexpr FormatInformation MakeFormatInformation(VkFormat format, bool isBlockCompressed, bool isMultiPlane, uint32_t totalSize, uint32_t elementSize, BaseType baseType,
                                                         uint32_t redOffset, uint32_t greenOffset, uint32_t blueOffset, uint32_t alphaOffset)
{
	const auto features = static_cast<VkFormatFeatureFlags>(VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT |
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
	return FormatInformation
	{
		format,
		features | (isMultiPlane ? VK_FORMAT_FEATURE_DISJOINT_BIT : 0),
		features | (isMultiPlane ? VK_FORMAT_FEATURE_DISJOINT_BIT : 0),
		isBlockCompressed ? static_cast<VkFormatFeatureFlags>(0) : features,
		totalSize,
		elementSize,
		baseType,
		redOffset,
		greenOffset,
		blueOffset,
		alphaOffset,
	};
}

static constexpr FormatInformation formatInformation[]
{
	MakeFormatInformation(VK_FORMAT_UNDEFINED, false, false, 0),
	MakeFormatInformation(VK_FORMAT_R4G4_UNORM_PACK8, false, false, 1),
	MakeFormatInformation(VK_FORMAT_R4G4B4A4_UNORM_PACK16, false, false, 2),
	MakeFormatInformation(VK_FORMAT_B4G4R4A4_UNORM_PACK16, false, false, 2),
	MakeFormatInformation(VK_FORMAT_R5G6B5_UNORM_PACK16, false, false, 2),
	MakeFormatInformation(VK_FORMAT_B5G6R5_UNORM_PACK16, false, false, 2),
	MakeFormatInformation(VK_FORMAT_R5G5B5A1_UNORM_PACK16, false, false, 2),
	MakeFormatInformation(VK_FORMAT_B5G5R5A1_UNORM_PACK16, false, false, 2),
	MakeFormatInformation(VK_FORMAT_A1R5G5B5_UNORM_PACK16, false, false, 2),
	MakeFormatInformation(VK_FORMAT_R8_UNORM, false, false, 1),
	MakeFormatInformation(VK_FORMAT_R8_SNORM, false, false, 1),
	MakeFormatInformation(VK_FORMAT_R8_USCALED, false, false, 1),
	MakeFormatInformation(VK_FORMAT_R8_SSCALED, false, false, 1),
	MakeFormatInformation(VK_FORMAT_R8_UINT, false, false, 1),
	MakeFormatInformation(VK_FORMAT_R8_SINT, false, false, 1),
	MakeFormatInformation(VK_FORMAT_R8_SRGB, false, false, 1),
	MakeFormatInformation(VK_FORMAT_R8G8_UNORM, false, false, 2),
	MakeFormatInformation(VK_FORMAT_R8G8_SNORM, false, false, 2),
	MakeFormatInformation(VK_FORMAT_R8G8_USCALED, false, false, 2),
	MakeFormatInformation(VK_FORMAT_R8G8_SSCALED, false, false, 2),
	MakeFormatInformation(VK_FORMAT_R8G8_UINT, false, false, 2),
	MakeFormatInformation(VK_FORMAT_R8G8_SINT, false, false, 2),
	MakeFormatInformation(VK_FORMAT_R8G8_SRGB, false, false, 2),
	MakeFormatInformation(VK_FORMAT_R8G8B8_UNORM, false, false, 3),
	MakeFormatInformation(VK_FORMAT_R8G8B8_SNORM, false, false, 3),
	MakeFormatInformation(VK_FORMAT_R8G8B8_USCALED, false, false, 3),
	MakeFormatInformation(VK_FORMAT_R8G8B8_SSCALED, false, false, 3),
	MakeFormatInformation(VK_FORMAT_R8G8B8_UINT, false, false, 3),
	MakeFormatInformation(VK_FORMAT_R8G8B8_SINT, false, false, 3),
	MakeFormatInformation(VK_FORMAT_R8G8B8_SRGB, false, false, 3),
	MakeFormatInformation(VK_FORMAT_B8G8R8_UNORM, false, false, 3),
	MakeFormatInformation(VK_FORMAT_B8G8R8_SNORM, false, false, 3),
	MakeFormatInformation(VK_FORMAT_B8G8R8_USCALED, false, false, 3),
	MakeFormatInformation(VK_FORMAT_B8G8R8_SSCALED, false, false, 3),
	MakeFormatInformation(VK_FORMAT_B8G8R8_UINT, false, false, 3),
	MakeFormatInformation(VK_FORMAT_B8G8R8_SINT, false, false, 3),
	MakeFormatInformation(VK_FORMAT_B8G8R8_SRGB, false, false, 3),
	MakeFormatInformation(VK_FORMAT_R8G8B8A8_UNORM, false, false, 4, 1, BaseType::UNorm, 0, 1, 2, 3),
	MakeFormatInformation(VK_FORMAT_R8G8B8A8_SNORM, false, false, 4),
	MakeFormatInformation(VK_FORMAT_R8G8B8A8_USCALED, false, false, 4),
	MakeFormatInformation(VK_FORMAT_R8G8B8A8_SSCALED, false, false, 4),
	MakeFormatInformation(VK_FORMAT_R8G8B8A8_UINT, false, false, 4),
	MakeFormatInformation(VK_FORMAT_R8G8B8A8_SINT, false, false, 4),
	MakeFormatInformation(VK_FORMAT_R8G8B8A8_SRGB, false, false, 4),
	MakeFormatInformation(VK_FORMAT_B8G8R8A8_UNORM, false, false, 4),
	MakeFormatInformation(VK_FORMAT_B8G8R8A8_SNORM, false, false, 4),
	MakeFormatInformation(VK_FORMAT_B8G8R8A8_USCALED, false, false, 4),
	MakeFormatInformation(VK_FORMAT_B8G8R8A8_SSCALED, false, false, 4),
	MakeFormatInformation(VK_FORMAT_B8G8R8A8_UINT, false, false, 4),
	MakeFormatInformation(VK_FORMAT_B8G8R8A8_SINT, false, false, 4),
	MakeFormatInformation(VK_FORMAT_B8G8R8A8_SRGB, false, false, 4),
	MakeFormatInformation(VK_FORMAT_A8B8G8R8_UNORM_PACK32, false, false, 4),
	MakeFormatInformation(VK_FORMAT_A8B8G8R8_SNORM_PACK32, false, false, 4),
	MakeFormatInformation(VK_FORMAT_A8B8G8R8_USCALED_PACK32, false, false, 4),
	MakeFormatInformation(VK_FORMAT_A8B8G8R8_SSCALED_PACK32, false, false, 4),
	MakeFormatInformation(VK_FORMAT_A8B8G8R8_UINT_PACK32, false, false, 4),
	MakeFormatInformation(VK_FORMAT_A8B8G8R8_SINT_PACK32, false, false, 4),
	MakeFormatInformation(VK_FORMAT_A8B8G8R8_SRGB_PACK32, false, false, 4),
	MakeFormatInformation(VK_FORMAT_A2R10G10B10_UNORM_PACK32, false, false, 4),
	MakeFormatInformation(VK_FORMAT_A2R10G10B10_SNORM_PACK32, false, false, 4),
	MakeFormatInformation(VK_FORMAT_A2R10G10B10_USCALED_PACK32, false, false, 4),
	MakeFormatInformation(VK_FORMAT_A2R10G10B10_SSCALED_PACK32, false, false, 4),
	MakeFormatInformation(VK_FORMAT_A2R10G10B10_UINT_PACK32, false, false, 4),
	MakeFormatInformation(VK_FORMAT_A2R10G10B10_SINT_PACK32, false, false, 4),
	MakeFormatInformation(VK_FORMAT_A2B10G10R10_UNORM_PACK32, false, false, 4),
	MakeFormatInformation(VK_FORMAT_A2B10G10R10_SNORM_PACK32, false, false, 4),
	MakeFormatInformation(VK_FORMAT_A2B10G10R10_USCALED_PACK32, false, false, 4),
	MakeFormatInformation(VK_FORMAT_A2B10G10R10_SSCALED_PACK32, false, false, 4),
	MakeFormatInformation(VK_FORMAT_A2B10G10R10_UINT_PACK32, false, false, 4),
	MakeFormatInformation(VK_FORMAT_A2B10G10R10_SINT_PACK32, false, false, 4),
	MakeFormatInformation(VK_FORMAT_R16_UNORM, false, false, 2),
	MakeFormatInformation(VK_FORMAT_R16_SNORM, false, false, 2),
	MakeFormatInformation(VK_FORMAT_R16_USCALED, false, false, 2),
	MakeFormatInformation(VK_FORMAT_R16_SSCALED, false, false, 2),
	MakeFormatInformation(VK_FORMAT_R16_UINT, false, false, 2),
	MakeFormatInformation(VK_FORMAT_R16_SINT, false, false, 2),
	MakeFormatInformation(VK_FORMAT_R16_SFLOAT, false, false, 2),
	MakeFormatInformation(VK_FORMAT_R16G16_UNORM, false, false),
	MakeFormatInformation(VK_FORMAT_R16G16_SNORM, false, false),
	MakeFormatInformation(VK_FORMAT_R16G16_USCALED, false, false),
	MakeFormatInformation(VK_FORMAT_R16G16_SSCALED, false, false),
	MakeFormatInformation(VK_FORMAT_R16G16_UINT, false, false),
	MakeFormatInformation(VK_FORMAT_R16G16_SINT, false, false),
	MakeFormatInformation(VK_FORMAT_R16G16_SFLOAT, false, false),
	MakeFormatInformation(VK_FORMAT_R16G16B16_UNORM, false, false),
	MakeFormatInformation(VK_FORMAT_R16G16B16_SNORM, false, false),
	MakeFormatInformation(VK_FORMAT_R16G16B16_USCALED, false, false),
	MakeFormatInformation(VK_FORMAT_R16G16B16_SSCALED, false, false),
	MakeFormatInformation(VK_FORMAT_R16G16B16_UINT, false, false),
	MakeFormatInformation(VK_FORMAT_R16G16B16_SINT, false, false),
	MakeFormatInformation(VK_FORMAT_R16G16B16_SFLOAT, false, false),
	MakeFormatInformation(VK_FORMAT_R16G16B16A16_UNORM, false, false),
	MakeFormatInformation(VK_FORMAT_R16G16B16A16_SNORM, false, false),
	MakeFormatInformation(VK_FORMAT_R16G16B16A16_USCALED, false, false),
	MakeFormatInformation(VK_FORMAT_R16G16B16A16_SSCALED, false, false),
	MakeFormatInformation(VK_FORMAT_R16G16B16A16_UINT, false, false),
	MakeFormatInformation(VK_FORMAT_R16G16B16A16_SINT, false, false),
	MakeFormatInformation(VK_FORMAT_R16G16B16A16_SFLOAT, false, false),
	MakeFormatInformation(VK_FORMAT_R32_UINT, false, false),
	MakeFormatInformation(VK_FORMAT_R32_SINT, false, false),
	MakeFormatInformation(VK_FORMAT_R32_SFLOAT, false, false),
	MakeFormatInformation(VK_FORMAT_R32G32_UINT, false, false),
	MakeFormatInformation(VK_FORMAT_R32G32_SINT, false, false),
	MakeFormatInformation(VK_FORMAT_R32G32_SFLOAT, false, false),
	MakeFormatInformation(VK_FORMAT_R32G32B32_UINT, false, false),
	MakeFormatInformation(VK_FORMAT_R32G32B32_SINT, false, false),
	MakeFormatInformation(VK_FORMAT_R32G32B32_SFLOAT, false, false),
	MakeFormatInformation(VK_FORMAT_R32G32B32A32_UINT, false, false),
	MakeFormatInformation(VK_FORMAT_R32G32B32A32_SINT, false, false),
	MakeFormatInformation(VK_FORMAT_R32G32B32A32_SFLOAT, false, false),
	MakeFormatInformation(VK_FORMAT_R64_UINT, false, false),
	MakeFormatInformation(VK_FORMAT_R64_SINT, false, false),
	MakeFormatInformation(VK_FORMAT_R64_SFLOAT, false, false),
	MakeFormatInformation(VK_FORMAT_R64G64_UINT, false, false),
	MakeFormatInformation(VK_FORMAT_R64G64_SINT, false, false),
	MakeFormatInformation(VK_FORMAT_R64G64_SFLOAT, false, false),
	MakeFormatInformation(VK_FORMAT_R64G64B64_UINT, false, false),
	MakeFormatInformation(VK_FORMAT_R64G64B64_SINT, false, false),
	MakeFormatInformation(VK_FORMAT_R64G64B64_SFLOAT, false, false),
	MakeFormatInformation(VK_FORMAT_R64G64B64A64_UINT, false, false),
	MakeFormatInformation(VK_FORMAT_R64G64B64A64_SINT, false, false),
	MakeFormatInformation(VK_FORMAT_R64G64B64A64_SFLOAT, false, false),
	MakeFormatInformation(VK_FORMAT_B10G11R11_UFLOAT_PACK32, false, false),
	MakeFormatInformation(VK_FORMAT_E5B9G9R9_UFLOAT_PACK32, false, false),
	MakeFormatInformation(VK_FORMAT_D16_UNORM, false, false, 2, 2, BaseType::UNorm, 0, -1, -1, -1),
	MakeFormatInformation(VK_FORMAT_X8_D24_UNORM_PACK32, false, false, 4),
	MakeFormatInformation(VK_FORMAT_D32_SFLOAT, false, false, 4),
	MakeFormatInformation(VK_FORMAT_S8_UINT, false, false, 1),
	MakeFormatInformation(VK_FORMAT_D16_UNORM_S8_UINT, false, false),
	MakeFormatInformation(VK_FORMAT_D24_UNORM_S8_UINT, false, false),
	MakeFormatInformation(VK_FORMAT_D32_SFLOAT_S8_UINT, false, false),
	MakeFormatInformation(VK_FORMAT_BC1_RGB_UNORM_BLOCK, true, false),
	MakeFormatInformation(VK_FORMAT_BC1_RGB_SRGB_BLOCK, true, false),
	MakeFormatInformation(VK_FORMAT_BC1_RGBA_UNORM_BLOCK, true, false),
	MakeFormatInformation(VK_FORMAT_BC1_RGBA_SRGB_BLOCK, true, false),
	MakeFormatInformation(VK_FORMAT_BC2_UNORM_BLOCK, true, false),
	MakeFormatInformation(VK_FORMAT_BC2_SRGB_BLOCK, true, false),
	MakeFormatInformation(VK_FORMAT_BC3_UNORM_BLOCK, true, false),
	MakeFormatInformation(VK_FORMAT_BC3_SRGB_BLOCK, true, false),
	MakeFormatInformation(VK_FORMAT_BC4_UNORM_BLOCK, true, false),
	MakeFormatInformation(VK_FORMAT_BC4_SNORM_BLOCK, true, false),
	MakeFormatInformation(VK_FORMAT_BC5_UNORM_BLOCK, true, false),
	MakeFormatInformation(VK_FORMAT_BC5_SNORM_BLOCK, true, false),
	MakeFormatInformation(VK_FORMAT_BC6H_UFLOAT_BLOCK, true, false),
	MakeFormatInformation(VK_FORMAT_BC6H_SFLOAT_BLOCK, true, false),
	MakeFormatInformation(VK_FORMAT_BC7_UNORM_BLOCK, true, false),
	MakeFormatInformation(VK_FORMAT_BC7_SRGB_BLOCK, true, false),
	MakeFormatInformation(VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK, true, false),
	MakeFormatInformation(VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK, true, false),
	MakeFormatInformation(VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK, true, false),
	MakeFormatInformation(VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK, true, false),
	MakeFormatInformation(VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK, true, false),
	MakeFormatInformation(VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK, true, false),
	MakeFormatInformation(VK_FORMAT_EAC_R11_UNORM_BLOCK, true, false),
	MakeFormatInformation(VK_FORMAT_EAC_R11_SNORM_BLOCK, true, false),
	MakeFormatInformation(VK_FORMAT_EAC_R11G11_UNORM_BLOCK, true, false),
	MakeFormatInformation(VK_FORMAT_EAC_R11G11_SNORM_BLOCK, true, false),
	MakeFormatInformation(VK_FORMAT_ASTC_4x4_UNORM_BLOCK, true, false),
	MakeFormatInformation(VK_FORMAT_ASTC_4x4_SRGB_BLOCK, true, false),
	MakeFormatInformation(VK_FORMAT_ASTC_5x4_UNORM_BLOCK, true, false),
	MakeFormatInformation(VK_FORMAT_ASTC_5x4_SRGB_BLOCK, true, false),
	MakeFormatInformation(VK_FORMAT_ASTC_5x5_UNORM_BLOCK, true, false),
	MakeFormatInformation(VK_FORMAT_ASTC_5x5_SRGB_BLOCK, true, false),
	MakeFormatInformation(VK_FORMAT_ASTC_6x5_UNORM_BLOCK, true, false),
	MakeFormatInformation(VK_FORMAT_ASTC_6x5_SRGB_BLOCK, true, false),
	MakeFormatInformation(VK_FORMAT_ASTC_6x6_UNORM_BLOCK, true, false),
	MakeFormatInformation(VK_FORMAT_ASTC_6x6_SRGB_BLOCK, true, false),
	MakeFormatInformation(VK_FORMAT_ASTC_8x5_UNORM_BLOCK, true, false),
	MakeFormatInformation(VK_FORMAT_ASTC_8x5_SRGB_BLOCK, true, false),
	MakeFormatInformation(VK_FORMAT_ASTC_8x6_UNORM_BLOCK, true, false),
	MakeFormatInformation(VK_FORMAT_ASTC_8x6_SRGB_BLOCK, true, false),
	MakeFormatInformation(VK_FORMAT_ASTC_8x8_UNORM_BLOCK, true, false),
	MakeFormatInformation(VK_FORMAT_ASTC_8x8_SRGB_BLOCK, true, false),
	MakeFormatInformation(VK_FORMAT_ASTC_10x5_UNORM_BLOCK, true, false),
	MakeFormatInformation(VK_FORMAT_ASTC_10x5_SRGB_BLOCK, true, false),
	MakeFormatInformation(VK_FORMAT_ASTC_10x6_UNORM_BLOCK, true, false),
	MakeFormatInformation(VK_FORMAT_ASTC_10x6_SRGB_BLOCK, true, false),
	MakeFormatInformation(VK_FORMAT_ASTC_10x8_UNORM_BLOCK, true, false),
	MakeFormatInformation(VK_FORMAT_ASTC_10x8_SRGB_BLOCK, true, false),
	MakeFormatInformation(VK_FORMAT_ASTC_10x10_UNORM_BLOCK, true, false),
	MakeFormatInformation(VK_FORMAT_ASTC_10x10_SRGB_BLOCK, true, false),
	MakeFormatInformation(VK_FORMAT_ASTC_12x10_UNORM_BLOCK, true, false),
	MakeFormatInformation(VK_FORMAT_ASTC_12x10_SRGB_BLOCK, true, false),
	MakeFormatInformation(VK_FORMAT_ASTC_12x12_UNORM_BLOCK, true, false),
	MakeFormatInformation(VK_FORMAT_ASTC_12x12_SRGB_BLOCK, true, false),
};

static std::unordered_map<VkFormat, FormatInformation> extraFormatInformation
{
	{VK_FORMAT_G8B8G8R8_422_UNORM, MakeFormatInformation(VK_FORMAT_G8B8G8R8_422_UNORM, false, false)},
	{VK_FORMAT_B8G8R8G8_422_UNORM, MakeFormatInformation(VK_FORMAT_B8G8R8G8_422_UNORM, false, false)},
	{VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM, MakeFormatInformation(VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM, false, true)},
	{VK_FORMAT_G8_B8R8_2PLANE_420_UNORM, MakeFormatInformation(VK_FORMAT_G8_B8R8_2PLANE_420_UNORM, false, true)},
	{VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM, MakeFormatInformation(VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM, false, true)},
	{VK_FORMAT_G8_B8R8_2PLANE_422_UNORM, MakeFormatInformation(VK_FORMAT_G8_B8R8_2PLANE_422_UNORM, false, true)},
	{VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM, MakeFormatInformation(VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM, false, true)},
	{VK_FORMAT_R10X6_UNORM_PACK16, MakeFormatInformation(VK_FORMAT_R10X6_UNORM_PACK16, false, false)},
	{VK_FORMAT_R10X6G10X6_UNORM_2PACK16, MakeFormatInformation(VK_FORMAT_R10X6G10X6_UNORM_2PACK16, false, false)},
	{VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16, MakeFormatInformation(VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16, false, false)},
	{VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16, MakeFormatInformation(VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16, false, false)},
	{VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16, MakeFormatInformation(VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16, false, false)},
	{VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16, MakeFormatInformation(VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16, false, true)},
	{VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16, MakeFormatInformation(VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16, false, true)},
	{VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16, MakeFormatInformation(VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16, false, true)},
	{VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16, MakeFormatInformation(VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16, false, true)},
	{VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16, MakeFormatInformation(VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16, false, true)},
	{VK_FORMAT_R12X4_UNORM_PACK16, MakeFormatInformation(VK_FORMAT_R12X4_UNORM_PACK16, false, false)},
	{VK_FORMAT_R12X4G12X4_UNORM_2PACK16, MakeFormatInformation(VK_FORMAT_R12X4G12X4_UNORM_2PACK16, false, false)},
	{VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16, MakeFormatInformation(VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16, false, false)},
	{VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16, MakeFormatInformation(VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16, false, false)},
	{VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16, MakeFormatInformation(VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16, false, false)},
	{VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16, MakeFormatInformation(VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16, false, true)},
	{VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16, MakeFormatInformation(VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16, false, true)},
	{VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16, MakeFormatInformation(VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16, false, true)},
	{VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16, MakeFormatInformation(VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16, false, true)},
	{VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16, MakeFormatInformation(VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16, false, true)},
	{VK_FORMAT_G16B16G16R16_422_UNORM, MakeFormatInformation(VK_FORMAT_G16B16G16R16_422_UNORM, false, false)},
	{VK_FORMAT_B16G16R16G16_422_UNORM, MakeFormatInformation(VK_FORMAT_B16G16R16G16_422_UNORM, false, false)},
	{VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM, MakeFormatInformation(VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM, false, true)},
	{VK_FORMAT_G16_B16R16_2PLANE_420_UNORM, MakeFormatInformation(VK_FORMAT_G16_B16R16_2PLANE_420_UNORM, false, true)},
	{VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM, MakeFormatInformation(VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM, false, true)},
	{VK_FORMAT_G16_B16R16_2PLANE_422_UNORM, MakeFormatInformation(VK_FORMAT_G16_B16R16_2PLANE_422_UNORM, false, true)},
	{VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM, MakeFormatInformation(VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM, false, true)},
	{VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG, MakeFormatInformation(VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG, false, false)},
	{VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG, MakeFormatInformation(VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG, false, false)},
	{VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG, MakeFormatInformation(VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG, false, false)},
	{VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG, MakeFormatInformation(VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG, false, false)},
	{VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG, MakeFormatInformation(VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG, false, false)},
	{VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG, MakeFormatInformation(VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG, false, false)},
	{VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG, MakeFormatInformation(VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG, false, false)},
	{VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG, MakeFormatInformation(VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG, false, false)},
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
