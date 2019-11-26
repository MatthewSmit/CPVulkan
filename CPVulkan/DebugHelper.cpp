#include "DebugHelper.h"

#include "Image.h"
#include "ImageView.h"

#include <Formats.h>

#if CV_DEBUG_LEVEL >= CV_DEBUG_LOG
std::ostream& operator<<(std::ostream& stream, VkIndexType value)
{
	switch (value)
	{
	case VK_INDEX_TYPE_UINT16: return stream << "U16";
	case VK_INDEX_TYPE_UINT32: return stream << "U32";
	case VK_INDEX_TYPE_NONE_NV: return stream << "None";
	case VK_INDEX_TYPE_UINT8_EXT: return stream << "U8";
	default: FATAL_ERROR();
	}
}

std::ostream& operator<<(std::ostream& stream, VkPipelineBindPoint value)
{
	switch (value)
	{
	case VK_PIPELINE_BIND_POINT_GRAPHICS: return stream << "Graphics";
	case VK_PIPELINE_BIND_POINT_COMPUTE: return stream << "Compute";
	case VK_PIPELINE_BIND_POINT_RAY_TRACING_NV: return stream << "Ray-Tracing";
	default: FATAL_ERROR();
	}
}
 
std::ostream& operator<<(std::ostream& stream, const VkBufferCopy& value)
{
	return stream << "BufferCopy(" <<
		"Src Offset: " << value.srcOffset <<
		", Dst Offset: " << value.dstOffset <<
		", Size: " << value.size <<
		")";
}

std::ostream& operator<<(std::ostream& stream, const VkBufferImageCopy& value)
{
	return stream << "BufferImageCopy(" <<
		"Buffer Offset" << value.bufferOffset <<
		"Buffer Row Length" << value.bufferRowLength <<
		"Buffer Image Height" << value.bufferImageHeight <<
		"Image Subresource" << value.imageSubresource <<
		"Image Offset" << value.imageOffset <<
		"Image Extent" << value.imageExtent <<
		")";
}

std::ostream& operator<<(std::ostream& stream, const VkClearAttachment& value)
{
	return stream << "ClearAttachment(" <<
		"Aspect Mask: " << value.aspectMask <<
		", Colour Attachment: " << value.colorAttachment <<
		", Clear Value: " << value.clearValue <<
		")";
}

std::ostream& operator<<(std::ostream& stream, const VkClearDepthStencilValue& value)
{
	return stream << "ClearValue(" << value.depth << ", " << value.stencil << ")";
}

std::ostream& operator<<(std::ostream& stream, const VkClearRect& value)
{
	return stream << "ClearRect(" <<
		"Base Array Layer: " << value.baseArrayLayer <<
		", Layer Count: " << value.layerCount <<
		", Rectangle: " << value.rect <<
		")";
}

std::ostream& operator<<(std::ostream& stream, const VkClearValue& value)
{
	return stream << "ClearValue()";
}

std::ostream& operator<<(std::ostream& stream, const VkExtent3D& value)
{
	return stream << "Extent3D(" <<
		"Width: " << value.width <<
		", Height: " << value.height <<
		", Depth: " << value.depth <<
		")";
}

std::ostream& operator<<(std::ostream& stream, const VkImageBlit& value)
{
	return stream << "ImageBlit(" <<
		"Src Subresource: " << value.srcSubresource <<
		", Src Offset[0]: " << value.srcOffsets[0] <<
		", Src Offset[1]: " << value.srcOffsets[1] <<
		", Dst Subresource: " << value.dstSubresource <<
		", Dst Offset[0]: " << value.dstOffsets[0] <<
		", Dst Offset[1]: " << value.dstOffsets[1] <<
		")";
}

std::ostream& operator<<(std::ostream& stream, const VkImageCopy& value)
{
	return stream << "ImageCopy(" <<
		"Src Subresource: " << value.srcSubresource <<
		", Src Offset: " << value.srcOffset <<
		", Dst Subresource: " << value.dstSubresource <<
		", Dst Offset: " << value.dstOffset <<
		", Extent: " << value.extent <<
		")";
}

std::ostream& operator<<(std::ostream& stream, const VkImageResolve& value)
{
	return stream << "ImageResolve(" <<
		"Src Subresource: " << value.srcSubresource <<
		", Src Offset: " << value.srcOffset <<
		", Dst Subresource: " << value.dstSubresource <<
		", Dst Offset: " << value.dstOffset <<
		", Extent: " << value.extent <<
		")";
}

std::ostream& operator<<(std::ostream& stream, const VkImageSubresourceLayers& value)
{
	return stream << "ImageSubresourceLayers(" <<
		"Aspect Mask: " << value.aspectMask <<
		", Mip Level: " << value.mipLevel <<
		", Base Array Layer: " << value.baseArrayLayer <<
		", Layer Count: " << value.layerCount <<
		")";
}

std::ostream& operator<<(std::ostream& stream, const VkImageSubresourceRange& value)
{
	return stream << "ImageSubresourceRange(" <<
		"Aspect Mask: " << value.aspectMask <<
		", Base Mip Level: " << value.baseMipLevel <<
		", Mip Level Count: " << value.levelCount <<
		", Base Array Layer: " << value.baseArrayLayer <<
		", Layer Count: " << value.layerCount <<
		")";
}

std::ostream& operator<<(std::ostream& stream, const VkOffset3D& value)
{
	return stream << "Offset3D(" <<
		"X: " << value.x <<
		", Y: " << value.y <<
		", Z: " << value.z <<
		")";
}

std::ostream& operator<<(std::ostream& stream, const VkRect2D& value)
{
	return stream << "Rect2D(" <<
		"X: " << value.offset.x <<
		", Y: " << value.offset.y <<
		", Width: " << value.extent.width <<
		", Height: " << value.extent.height <<
		")";
}

std::ostream& operator<<(std::ostream& stream, const VkViewport& value)
{
	return stream << "Viewport(" <<
		"X: " << value.x <<
		", Y: " << value.y <<
		", Width: " << value.width <<
		", Height: " << value.height <<
		", Min Depth: " << value.minDepth <<
		", Max Depth: " << value.maxDepth <<
		")";
}

std::ostream& operator<<(std::ostream& stream, Buffer* value)
{
	return stream << "Buffer[" << reinterpret_cast<uint64_t>(value) << "]";
}

std::ostream& operator<<(std::ostream& stream, DescriptorSet* value)
{
	return stream << "DescriptorSet[" << reinterpret_cast<uint64_t>(value) << "]";
}

std::ostream& operator<<(std::ostream& stream, Pipeline* value)
{
	return stream << "Pipeline[" << reinterpret_cast<uint64_t>(value) << "]";
}

std::ostream& operator<<(std::ostream& stream, PipelineLayout* value)
{
	return stream << "PipelineLayout[" << reinterpret_cast<uint64_t>(value) << "]";
}

std::string DebugPrint(Image* image, VkClearColorValue value)
{
	switch (GetFormatInformation(image->getFormat()).Base)
	{
	case BaseType::UNorm:
	case BaseType::SNorm:
	case BaseType::UScaled:
	case BaseType::SScaled:
	case BaseType::UFloat:
	case BaseType::SFloat:
	case BaseType::SRGB:
		return std::string("ClearValue(")
			+ std::to_string(value.float32[0]) + ", "
			+ std::to_string(value.float32[1]) + ", "
			+ std::to_string(value.float32[2]) + ", "
			+ std::to_string(value.float32[3]) + ")";
		
	case BaseType::UInt:
		return std::string("ClearValue(")
			+ std::to_string(value.uint32[0]) + ", "
			+ std::to_string(value.uint32[1]) + ", "
			+ std::to_string(value.uint32[2]) + ", "
			+ std::to_string(value.uint32[3]) + ")";
		
	case BaseType::SInt:
		return std::string("ClearValue(")
			+ std::to_string(value.int32[0]) + ", "
			+ std::to_string(value.int32[1]) + ", "
			+ std::to_string(value.int32[2]) + ", "
			+ std::to_string(value.int32[3]) + ")";
		
	default: FATAL_ERROR();
	}
}
#endif

#if CV_DEBUG_LEVEL >= CV_DEBUG_IMAGE
struct DDSPixelFormat
{
	uint32_t Size;
	uint32_t Flags;
	uint32_t FourCC;
	uint32_t RGBBitCount;
	uint32_t RBitMask;
	uint32_t GBitMask;
	uint32_t BBitMask;
	uint32_t ABitMask;
};

static_assert(sizeof(DDSPixelFormat) == 32);

struct DDSHeader
{
	uint32_t Size;
	uint32_t Flags;
	uint32_t Height;
	uint32_t Width;
	uint32_t PitchOrLinearSize;
	uint32_t Depth;
	uint32_t MipMapCount;
	uint32_t Reserved1[11];
	DDSPixelFormat PixelFormat;
	uint32_t Caps;
	uint32_t Caps2;
	uint32_t Caps3;
	uint32_t Caps4;
	uint32_t Reserved2;
};

static_assert(sizeof(DDSHeader) == 124);

enum DXGIFormat
{
	DXGI_FORMAT_UNKNOWN,
	DXGI_FORMAT_R32G32B32A32_TYPELESS,
	DXGI_FORMAT_R32G32B32A32_FLOAT,
	DXGI_FORMAT_R32G32B32A32_UINT,
	DXGI_FORMAT_R32G32B32A32_SINT,
	DXGI_FORMAT_R32G32B32_TYPELESS,
	DXGI_FORMAT_R32G32B32_FLOAT,
	DXGI_FORMAT_R32G32B32_UINT,
	DXGI_FORMAT_R32G32B32_SINT,
	DXGI_FORMAT_R16G16B16A16_TYPELESS,
	DXGI_FORMAT_R16G16B16A16_FLOAT,
	DXGI_FORMAT_R16G16B16A16_UNORM,
	DXGI_FORMAT_R16G16B16A16_UINT,
	DXGI_FORMAT_R16G16B16A16_SNORM,
	DXGI_FORMAT_R16G16B16A16_SINT,
	DXGI_FORMAT_R32G32_TYPELESS,
	DXGI_FORMAT_R32G32_FLOAT,
	DXGI_FORMAT_R32G32_UINT,
	DXGI_FORMAT_R32G32_SINT,
	DXGI_FORMAT_R32G8X24_TYPELESS,
	DXGI_FORMAT_D32_FLOAT_S8X24_UINT,
	DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS,
	DXGI_FORMAT_X32_TYPELESS_G8X24_UINT,
	DXGI_FORMAT_R10G10B10A2_TYPELESS,
	DXGI_FORMAT_R10G10B10A2_UNORM,
	DXGI_FORMAT_R10G10B10A2_UINT,
	DXGI_FORMAT_R11G11B10_FLOAT,
	DXGI_FORMAT_R8G8B8A8_TYPELESS,
	DXGI_FORMAT_R8G8B8A8_UNORM,
	DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
	DXGI_FORMAT_R8G8B8A8_UINT,
	DXGI_FORMAT_R8G8B8A8_SNORM,
	DXGI_FORMAT_R8G8B8A8_SINT,
	DXGI_FORMAT_R16G16_TYPELESS,
	DXGI_FORMAT_R16G16_FLOAT,
	DXGI_FORMAT_R16G16_UNORM,
	DXGI_FORMAT_R16G16_UINT,
	DXGI_FORMAT_R16G16_SNORM,
	DXGI_FORMAT_R16G16_SINT,
	DXGI_FORMAT_R32_TYPELESS,
	DXGI_FORMAT_D32_FLOAT,
	DXGI_FORMAT_R32_FLOAT,
	DXGI_FORMAT_R32_UINT,
	DXGI_FORMAT_R32_SINT,
	DXGI_FORMAT_R24G8_TYPELESS,
	DXGI_FORMAT_D24_UNORM_S8_UINT,
	DXGI_FORMAT_R24_UNORM_X8_TYPELESS,
	DXGI_FORMAT_X24_TYPELESS_G8_UINT,
	DXGI_FORMAT_R8G8_TYPELESS,
	DXGI_FORMAT_R8G8_UNORM,
	DXGI_FORMAT_R8G8_UINT,
	DXGI_FORMAT_R8G8_SNORM,
	DXGI_FORMAT_R8G8_SINT,
	DXGI_FORMAT_R16_TYPELESS,
	DXGI_FORMAT_R16_FLOAT,
	DXGI_FORMAT_D16_UNORM,
	DXGI_FORMAT_R16_UNORM,
	DXGI_FORMAT_R16_UINT,
	DXGI_FORMAT_R16_SNORM,
	DXGI_FORMAT_R16_SINT,
	DXGI_FORMAT_R8_TYPELESS,
	DXGI_FORMAT_R8_UNORM,
	DXGI_FORMAT_R8_UINT,
	DXGI_FORMAT_R8_SNORM,
	DXGI_FORMAT_R8_SINT,
	DXGI_FORMAT_A8_UNORM,
	DXGI_FORMAT_R1_UNORM,
	DXGI_FORMAT_R9G9B9E5_SHAREDEXP,
	DXGI_FORMAT_R8G8_B8G8_UNORM,
	DXGI_FORMAT_G8R8_G8B8_UNORM,
	DXGI_FORMAT_BC1_TYPELESS,
	DXGI_FORMAT_BC1_UNORM,
	DXGI_FORMAT_BC1_UNORM_SRGB,
	DXGI_FORMAT_BC2_TYPELESS,
	DXGI_FORMAT_BC2_UNORM,
	DXGI_FORMAT_BC2_UNORM_SRGB,
	DXGI_FORMAT_BC3_TYPELESS,
	DXGI_FORMAT_BC3_UNORM,
	DXGI_FORMAT_BC3_UNORM_SRGB,
	DXGI_FORMAT_BC4_TYPELESS,
	DXGI_FORMAT_BC4_UNORM,
	DXGI_FORMAT_BC4_SNORM,
	DXGI_FORMAT_BC5_TYPELESS,
	DXGI_FORMAT_BC5_UNORM,
	DXGI_FORMAT_BC5_SNORM,
	DXGI_FORMAT_B5G6R5_UNORM,
	DXGI_FORMAT_B5G5R5A1_UNORM,
	DXGI_FORMAT_B8G8R8A8_UNORM,
	DXGI_FORMAT_B8G8R8X8_UNORM,
	DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM,
	DXGI_FORMAT_B8G8R8A8_TYPELESS,
	DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,
	DXGI_FORMAT_B8G8R8X8_TYPELESS,
	DXGI_FORMAT_B8G8R8X8_UNORM_SRGB,
	DXGI_FORMAT_BC6H_TYPELESS,
	DXGI_FORMAT_BC6H_UF16,
	DXGI_FORMAT_BC6H_SF16,
	DXGI_FORMAT_BC7_TYPELESS,
	DXGI_FORMAT_BC7_UNORM,
	DXGI_FORMAT_BC7_UNORM_SRGB,
	DXGI_FORMAT_AYUV,
	DXGI_FORMAT_Y410,
	DXGI_FORMAT_Y416,
	DXGI_FORMAT_NV12,
	DXGI_FORMAT_P010,
	DXGI_FORMAT_P016,
	DXGI_FORMAT_420_OPAQUE,
	DXGI_FORMAT_YUY2,
	DXGI_FORMAT_Y210,
	DXGI_FORMAT_Y216,
	DXGI_FORMAT_NV11,
	DXGI_FORMAT_AI44,
	DXGI_FORMAT_IA44,
	DXGI_FORMAT_P8,
	DXGI_FORMAT_A8P8,
	DXGI_FORMAT_B4G4R4A4_UNORM,
	DXGI_FORMAT_P208,
	DXGI_FORMAT_V208,
	DXGI_FORMAT_V408,
	DXGI_FORMAT_FORCE_UINT
};

struct DDSHeaderDXT10
{
	DXGIFormat DXGIFormat;
	uint32_t ResourceDimension;
	uint32_t MiscFlag;
	uint32_t ArraySize;
	uint32_t MiscFlags2;
};

static DXGIFormat ConvertFormat(VkFormat format)
{
	switch (format)
	{
	case VK_FORMAT_UNDEFINED: return DXGI_FORMAT_UNKNOWN;
	case VK_FORMAT_R4G4_UNORM_PACK8:
	case VK_FORMAT_R4G4B4A4_UNORM_PACK16:
	case VK_FORMAT_B4G4R4A4_UNORM_PACK16:
	case VK_FORMAT_R5G6B5_UNORM_PACK16:
	case VK_FORMAT_B5G6R5_UNORM_PACK16:
	case VK_FORMAT_R5G5B5A1_UNORM_PACK16:
	case VK_FORMAT_B5G5R5A1_UNORM_PACK16:
	case VK_FORMAT_A1R5G5B5_UNORM_PACK16:
		FATAL_ERROR();

	case VK_FORMAT_R8_UNORM:
	case VK_FORMAT_R8_SNORM:
	case VK_FORMAT_R8_USCALED:
	case VK_FORMAT_R8_SSCALED:
	case VK_FORMAT_R8_UINT:
	case VK_FORMAT_R8_SINT:
	case VK_FORMAT_R8_SRGB:
		FATAL_ERROR();

	case VK_FORMAT_R8G8_UNORM:
	case VK_FORMAT_R8G8_SNORM:
	case VK_FORMAT_R8G8_USCALED:
	case VK_FORMAT_R8G8_SSCALED:
	case VK_FORMAT_R8G8_UINT:
	case VK_FORMAT_R8G8_SINT:
	case VK_FORMAT_R8G8_SRGB:
		FATAL_ERROR();

	case VK_FORMAT_R8G8B8_UNORM:
	case VK_FORMAT_R8G8B8_SNORM:
	case VK_FORMAT_R8G8B8_USCALED:
	case VK_FORMAT_R8G8B8_SSCALED:
	case VK_FORMAT_R8G8B8_UINT:
	case VK_FORMAT_R8G8B8_SINT:
	case VK_FORMAT_R8G8B8_SRGB:
		FATAL_ERROR();

	case VK_FORMAT_B8G8R8_UNORM:
	case VK_FORMAT_B8G8R8_SNORM:
	case VK_FORMAT_B8G8R8_USCALED:
	case VK_FORMAT_B8G8R8_SSCALED:
	case VK_FORMAT_B8G8R8_UINT:
	case VK_FORMAT_B8G8R8_SINT:
	case VK_FORMAT_B8G8R8_SRGB:
		FATAL_ERROR();

	case VK_FORMAT_R8G8B8A8_UNORM: return DXGI_FORMAT_R8G8B8A8_UNORM;
	case VK_FORMAT_R8G8B8A8_SNORM: return DXGI_FORMAT_R8G8B8A8_SNORM;
	case VK_FORMAT_R8G8B8A8_USCALED: FATAL_ERROR();
	case VK_FORMAT_R8G8B8A8_SSCALED: FATAL_ERROR();
	case VK_FORMAT_R8G8B8A8_UINT: return DXGI_FORMAT_R8G8B8A8_UINT;
	case VK_FORMAT_R8G8B8A8_SINT: return DXGI_FORMAT_R8G8B8A8_SINT;
	case VK_FORMAT_R8G8B8A8_SRGB: return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

	case VK_FORMAT_B8G8R8A8_UNORM: return DXGI_FORMAT_B8G8R8A8_UNORM;
	case VK_FORMAT_B8G8R8A8_SNORM: FATAL_ERROR();
	case VK_FORMAT_B8G8R8A8_USCALED: FATAL_ERROR();
	case VK_FORMAT_B8G8R8A8_SSCALED: FATAL_ERROR();
	case VK_FORMAT_B8G8R8A8_UINT: FATAL_ERROR();
	case VK_FORMAT_B8G8R8A8_SINT: FATAL_ERROR();
	case VK_FORMAT_B8G8R8A8_SRGB: return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;

	case VK_FORMAT_A8B8G8R8_UNORM_PACK32:
	case VK_FORMAT_A8B8G8R8_SNORM_PACK32:
	case VK_FORMAT_A8B8G8R8_USCALED_PACK32:
	case VK_FORMAT_A8B8G8R8_SSCALED_PACK32:
	case VK_FORMAT_A8B8G8R8_UINT_PACK32:
	case VK_FORMAT_A8B8G8R8_SINT_PACK32:
	case VK_FORMAT_A8B8G8R8_SRGB_PACK32:
		FATAL_ERROR();

	case VK_FORMAT_A2R10G10B10_UNORM_PACK32:
	case VK_FORMAT_A2R10G10B10_SNORM_PACK32:
	case VK_FORMAT_A2R10G10B10_USCALED_PACK32:
	case VK_FORMAT_A2R10G10B10_SSCALED_PACK32:
	case VK_FORMAT_A2R10G10B10_UINT_PACK32:
	case VK_FORMAT_A2R10G10B10_SINT_PACK32:
	case VK_FORMAT_A2B10G10R10_UNORM_PACK32:
	case VK_FORMAT_A2B10G10R10_SNORM_PACK32:
	case VK_FORMAT_A2B10G10R10_USCALED_PACK32:
	case VK_FORMAT_A2B10G10R10_SSCALED_PACK32:
	case VK_FORMAT_A2B10G10R10_UINT_PACK32:
	case VK_FORMAT_A2B10G10R10_SINT_PACK32:
		FATAL_ERROR();

	case VK_FORMAT_R16_UNORM:
	case VK_FORMAT_R16_SNORM:
	case VK_FORMAT_R16_USCALED:
	case VK_FORMAT_R16_SSCALED:
	case VK_FORMAT_R16_UINT:
	case VK_FORMAT_R16_SINT:
	case VK_FORMAT_R16_SFLOAT:
	case VK_FORMAT_R16G16_UNORM:
	case VK_FORMAT_R16G16_SNORM:
	case VK_FORMAT_R16G16_USCALED:
	case VK_FORMAT_R16G16_SSCALED:
	case VK_FORMAT_R16G16_UINT:
	case VK_FORMAT_R16G16_SINT:
	case VK_FORMAT_R16G16_SFLOAT:
	case VK_FORMAT_R16G16B16_UNORM:
	case VK_FORMAT_R16G16B16_SNORM:
	case VK_FORMAT_R16G16B16_USCALED:
	case VK_FORMAT_R16G16B16_SSCALED:
	case VK_FORMAT_R16G16B16_UINT:
	case VK_FORMAT_R16G16B16_SINT:
	case VK_FORMAT_R16G16B16_SFLOAT:
	case VK_FORMAT_R16G16B16A16_UNORM:
	case VK_FORMAT_R16G16B16A16_SNORM:
	case VK_FORMAT_R16G16B16A16_USCALED:
	case VK_FORMAT_R16G16B16A16_SSCALED:
	case VK_FORMAT_R16G16B16A16_UINT:
	case VK_FORMAT_R16G16B16A16_SINT:
	case VK_FORMAT_R16G16B16A16_SFLOAT:
		FATAL_ERROR();

	case VK_FORMAT_R32_UINT: return DXGI_FORMAT_R32_UINT;
	case VK_FORMAT_R32_SINT: return DXGI_FORMAT_R32_SINT;
	case VK_FORMAT_R32_SFLOAT: return DXGI_FORMAT_R32_FLOAT;
	case VK_FORMAT_R32G32_UINT:	return DXGI_FORMAT_R32G32_UINT;
	case VK_FORMAT_R32G32_SINT:	return DXGI_FORMAT_R32G32_SINT;
	case VK_FORMAT_R32G32_SFLOAT: return DXGI_FORMAT_R32G32_FLOAT;

	case VK_FORMAT_R32G32B32_UINT:
	case VK_FORMAT_R32G32B32_SINT:
	case VK_FORMAT_R32G32B32_SFLOAT:
		FATAL_ERROR();

	case VK_FORMAT_R32G32B32A32_UINT:
	case VK_FORMAT_R32G32B32A32_SINT:
	case VK_FORMAT_R32G32B32A32_SFLOAT:
		FATAL_ERROR();

	case VK_FORMAT_R64_UINT:
	case VK_FORMAT_R64_SINT:
	case VK_FORMAT_R64_SFLOAT:
	case VK_FORMAT_R64G64_UINT:
	case VK_FORMAT_R64G64_SINT:
	case VK_FORMAT_R64G64_SFLOAT:
	case VK_FORMAT_R64G64B64_UINT:
	case VK_FORMAT_R64G64B64_SINT:
	case VK_FORMAT_R64G64B64_SFLOAT:
	case VK_FORMAT_R64G64B64A64_UINT:
	case VK_FORMAT_R64G64B64A64_SINT:
	case VK_FORMAT_R64G64B64A64_SFLOAT:
	case VK_FORMAT_B10G11R11_UFLOAT_PACK32:
	case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32:
		FATAL_ERROR();

	case VK_FORMAT_D16_UNORM: return DXGI_FORMAT_D16_UNORM;
	case VK_FORMAT_X8_D24_UNORM_PACK32: return DXGI_FORMAT_D24_UNORM_S8_UINT;
	case VK_FORMAT_D32_SFLOAT: return DXGI_FORMAT_D32_FLOAT;
	case VK_FORMAT_S8_UINT: return DXGI_FORMAT_R8_UINT;
	case VK_FORMAT_D16_UNORM_S8_UINT: FATAL_ERROR();
	case VK_FORMAT_D24_UNORM_S8_UINT: return DXGI_FORMAT_D24_UNORM_S8_UINT;
	case VK_FORMAT_D32_SFLOAT_S8_UINT: return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;

	case VK_FORMAT_BC1_RGB_UNORM_BLOCK:
	case VK_FORMAT_BC1_RGB_SRGB_BLOCK:
	case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:
	case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:
	case VK_FORMAT_BC2_UNORM_BLOCK:
	case VK_FORMAT_BC2_SRGB_BLOCK:
	case VK_FORMAT_BC3_UNORM_BLOCK:
	case VK_FORMAT_BC3_SRGB_BLOCK:
	case VK_FORMAT_BC4_UNORM_BLOCK:
	case VK_FORMAT_BC4_SNORM_BLOCK:
	case VK_FORMAT_BC5_UNORM_BLOCK:
	case VK_FORMAT_BC5_SNORM_BLOCK:
	case VK_FORMAT_BC6H_UFLOAT_BLOCK:
	case VK_FORMAT_BC6H_SFLOAT_BLOCK:
	case VK_FORMAT_BC7_UNORM_BLOCK:
	case VK_FORMAT_BC7_SRGB_BLOCK:
		FATAL_ERROR();

	case VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK:
	case VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK:
	case VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK:
	case VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK:
	case VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK:
	case VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK:
	case VK_FORMAT_EAC_R11_UNORM_BLOCK:
	case VK_FORMAT_EAC_R11_SNORM_BLOCK:
	case VK_FORMAT_EAC_R11G11_UNORM_BLOCK:
	case VK_FORMAT_EAC_R11G11_SNORM_BLOCK:
		FATAL_ERROR();

	case VK_FORMAT_ASTC_4x4_UNORM_BLOCK:
	case VK_FORMAT_ASTC_4x4_SRGB_BLOCK:
	case VK_FORMAT_ASTC_5x4_UNORM_BLOCK:
	case VK_FORMAT_ASTC_5x4_SRGB_BLOCK:
	case VK_FORMAT_ASTC_5x5_UNORM_BLOCK:
	case VK_FORMAT_ASTC_5x5_SRGB_BLOCK:
	case VK_FORMAT_ASTC_6x5_UNORM_BLOCK:
	case VK_FORMAT_ASTC_6x5_SRGB_BLOCK:
	case VK_FORMAT_ASTC_6x6_UNORM_BLOCK:
	case VK_FORMAT_ASTC_6x6_SRGB_BLOCK:
	case VK_FORMAT_ASTC_8x5_UNORM_BLOCK:
	case VK_FORMAT_ASTC_8x5_SRGB_BLOCK:
	case VK_FORMAT_ASTC_8x6_UNORM_BLOCK:
	case VK_FORMAT_ASTC_8x6_SRGB_BLOCK:
	case VK_FORMAT_ASTC_8x8_UNORM_BLOCK:
	case VK_FORMAT_ASTC_8x8_SRGB_BLOCK:
	case VK_FORMAT_ASTC_10x5_UNORM_BLOCK:
	case VK_FORMAT_ASTC_10x5_SRGB_BLOCK:
	case VK_FORMAT_ASTC_10x6_UNORM_BLOCK:
	case VK_FORMAT_ASTC_10x6_SRGB_BLOCK:
	case VK_FORMAT_ASTC_10x8_UNORM_BLOCK:
	case VK_FORMAT_ASTC_10x8_SRGB_BLOCK:
	case VK_FORMAT_ASTC_10x10_UNORM_BLOCK:
	case VK_FORMAT_ASTC_10x10_SRGB_BLOCK:
	case VK_FORMAT_ASTC_12x10_UNORM_BLOCK:
	case VK_FORMAT_ASTC_12x10_SRGB_BLOCK:
	case VK_FORMAT_ASTC_12x12_UNORM_BLOCK:
	case VK_FORMAT_ASTC_12x12_SRGB_BLOCK:
		FATAL_ERROR();

	case VK_FORMAT_G8B8G8R8_422_UNORM:
	case VK_FORMAT_B8G8R8G8_422_UNORM:
	case VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM:
	case VK_FORMAT_G8_B8R8_2PLANE_420_UNORM:
	case VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM:
	case VK_FORMAT_G8_B8R8_2PLANE_422_UNORM:
	case VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM:
	case VK_FORMAT_R10X6_UNORM_PACK16:
	case VK_FORMAT_R10X6G10X6_UNORM_2PACK16:
	case VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16:
	case VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16:
	case VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16:
	case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16:
	case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16:
	case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16:
	case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16:
	case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16:
	case VK_FORMAT_R12X4_UNORM_PACK16:
	case VK_FORMAT_R12X4G12X4_UNORM_2PACK16:
	case VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16:
	case VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16:
	case VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16:
	case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16:
	case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16:
	case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16:
	case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16:
	case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16:
	case VK_FORMAT_G16B16G16R16_422_UNORM:
	case VK_FORMAT_B16G16R16G16_422_UNORM:
	case VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM:
	case VK_FORMAT_G16_B16R16_2PLANE_420_UNORM:
	case VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM:
	case VK_FORMAT_G16_B16R16_2PLANE_422_UNORM:
	case VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM:
		FATAL_ERROR();

	case VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG:
	case VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG:
	case VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG:
	case VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG:
	case VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG:
	case VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG:
	case VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG:
	case VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG:
		FATAL_ERROR();

	case VK_FORMAT_ASTC_4x4_SFLOAT_BLOCK_EXT:
	case VK_FORMAT_ASTC_5x4_SFLOAT_BLOCK_EXT:
	case VK_FORMAT_ASTC_5x5_SFLOAT_BLOCK_EXT:
	case VK_FORMAT_ASTC_6x5_SFLOAT_BLOCK_EXT:
	case VK_FORMAT_ASTC_6x6_SFLOAT_BLOCK_EXT:
	case VK_FORMAT_ASTC_8x5_SFLOAT_BLOCK_EXT:
	case VK_FORMAT_ASTC_8x6_SFLOAT_BLOCK_EXT:
	case VK_FORMAT_ASTC_8x8_SFLOAT_BLOCK_EXT:
	case VK_FORMAT_ASTC_10x5_SFLOAT_BLOCK_EXT:
	case VK_FORMAT_ASTC_10x6_SFLOAT_BLOCK_EXT:
	case VK_FORMAT_ASTC_10x8_SFLOAT_BLOCK_EXT:
	case VK_FORMAT_ASTC_10x10_SFLOAT_BLOCK_EXT:
	case VK_FORMAT_ASTC_12x10_SFLOAT_BLOCK_EXT:
	case VK_FORMAT_ASTC_12x12_SFLOAT_BLOCK_EXT:
		FATAL_ERROR();
		
	default: FATAL_ERROR();
	}
}

void DumpImage(const std::string& fileName, Image* image, ImageView* imageView)
{
	const auto DDS_MAGIC = "DDS ";
	const auto DX10_MAGIC = "DX10 ";
	const auto format = imageView->getFormat();
	const auto& information = GetFormatInformation(format);
	
	FILE* file;
	auto error = fopen_s(&file, fileName.c_str(), "w");
	assert(error == 0 && file != nullptr);

	fwrite(DDS_MAGIC, 4, 1, file);

	DDSHeader header = {};
	header.Size = sizeof(DDSHeader);
	header.Flags = 0x01 | 0x02 | 0x04 | 0x1000;
	if (information.Type == FormatType::Compressed)
	{
		header.Flags |= 0x80000;
	}
	else
	{
		header.Flags |= 0x08;
	}
	if (image->getDepth() > 1)
	{
		header.Flags |= 0x800000;
	}
	if (image->getMipLevels() > 1)
	{
		header.Flags |= 0x20000;
	}

	header.Height = image->getHeight();
	header.Width = image->getWidth();

	uint64_t start;
	uint64_t size;
	GetFormatLineSize(information, start, size, 0, image->getWidth());
	assert(size <= 0xFFFFFFFF);
	header.PitchOrLinearSize = static_cast<uint32_t>(size);
	header.Depth = image->getDepth();
	header.MipMapCount = image->getMipLevels();

	header.PixelFormat.Size = sizeof(DDSPixelFormat);
	header.PixelFormat.Flags = 0x04;
	header.PixelFormat.FourCC = *reinterpret_cast<const uint32_t*>(DX10_MAGIC);

	header.Caps = 0x1000;
	if (image->getMipLevels() > 1)
	{
		header.Caps |= 0x400000;
	}
	if (image->getMipLevels() > 1 || image->getDepth() > 1 || image->getArrayLayers() > 1)
	{
		header.Caps |= 0x08;
	}

	if (imageView->getViewType() == VK_IMAGE_VIEW_TYPE_CUBE || imageView->getViewType() == VK_IMAGE_VIEW_TYPE_CUBE_ARRAY)
	{
		header.Caps2 |= 0x200 | 0x400 | 0x800 | 0x1000 | 0x2000 | 0x4000 | 0x8000;
	}

	if (image->getDepth() > 1)
	{
		header.Caps2 |= 0x200000;
	}
	
	fwrite(&header, sizeof(header), 1, file);

	DDSHeaderDXT10 header10 = {};
	header10.DXGIFormat = ConvertFormat(format);
	switch (imageView->getViewType())
	{
	case VK_IMAGE_VIEW_TYPE_1D:
	case VK_IMAGE_VIEW_TYPE_1D_ARRAY:
		header10.ResourceDimension = 2;
		break;
		
	case VK_IMAGE_VIEW_TYPE_2D:
	case VK_IMAGE_VIEW_TYPE_2D_ARRAY:
		header10.ResourceDimension = 3;
		break;
		
	case VK_IMAGE_VIEW_TYPE_CUBE:
	case VK_IMAGE_VIEW_TYPE_CUBE_ARRAY:
		header10.ResourceDimension = 3;
		header10.MiscFlag = 0x04;
		break;
		
	case VK_IMAGE_VIEW_TYPE_3D:
		header10.ResourceDimension = 4;
		break;
	}

	header10.ArraySize = image->getArrayLayers();
	
	fwrite(&header10, sizeof(header10), 1, file);

	if (imageView->getViewType() == VK_IMAGE_VIEW_TYPE_CUBE || imageView->getViewType() == VK_IMAGE_VIEW_TYPE_CUBE_ARRAY)
	{
		FATAL_ERROR();
	}
	else
	{
		fwrite(image->getData().data(), image->getData().size(), 1, file);
	}
	
	error = fclose(file);
	assert(error == 0);
}
#endif