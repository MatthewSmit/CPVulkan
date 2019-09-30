#include "GlslFunctions.h"

#include "Base.h"
#include "CommandBuffer.Internal.h"
#include "Formats.h"
#include "Image.h"
#include "ImageView.h"
#include "Sampler.h"

#include <Converter.h>

#include <glm/glm.hpp>

template<typename ReturnType, typename CoordinateType>
static void ImageSampleLod(ReturnType* result, VkDescriptorImageInfo* sampledImage, CoordinateType* coordinate, float lod)
{
	auto sampler = UnwrapVulkan<Sampler>(sampledImage->sampler);
	auto imageView = UnwrapVulkan<ImageView>(sampledImage->imageView);

	// Instructions with ExplicitLod in the name determine the LOD used in the sampling operation based on additional coordinates.
	// https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html#textures-level-of-detail-operation 
	
	if (imageView->getViewType() != VK_IMAGE_VIEW_TYPE_2D)
	{
		FATAL_ERROR();
	}
	
	if (imageView->getSubresourceRange().aspectMask != VK_IMAGE_ASPECT_COLOR_BIT)
	{
		FATAL_ERROR();
	}
	
	if (imageView->getComponents().r != VK_COMPONENT_SWIZZLE_R && imageView->getComponents().r != VK_COMPONENT_SWIZZLE_IDENTITY)
	{
		FATAL_ERROR();
	}
	
	if (imageView->getComponents().g != VK_COMPONENT_SWIZZLE_G && imageView->getComponents().g != VK_COMPONENT_SWIZZLE_IDENTITY)
	{
		FATAL_ERROR();
	}
	
	if (imageView->getComponents().b != VK_COMPONENT_SWIZZLE_B && imageView->getComponents().b != VK_COMPONENT_SWIZZLE_IDENTITY)
	{
		FATAL_ERROR();
	}
	
	if (imageView->getComponents().a != VK_COMPONENT_SWIZZLE_A && imageView->getComponents().a != VK_COMPONENT_SWIZZLE_IDENTITY)
	{
		FATAL_ERROR();
	}
	
	if (imageView->getSubresourceRange().baseMipLevel != 0)
	{
		FATAL_ERROR();
	}
	
	if (imageView->getSubresourceRange().levelCount != 1)
	{
		FATAL_ERROR();
	}
	
	if (imageView->getSubresourceRange().baseArrayLayer != 0)
	{
		FATAL_ERROR();
	}
	
	if (imageView->getSubresourceRange().layerCount != 1)
	{
		FATAL_ERROR();
	}
	
	if (sampler->getFlags())
	{
		FATAL_ERROR();
	}
	
	if (sampler->getMagFilter() != VK_FILTER_NEAREST)
	{
		FATAL_ERROR();
	}
	
	if (sampler->getMinFilter() != VK_FILTER_NEAREST)
	{
		FATAL_ERROR();
	}
	
	if (sampler->getMipmapMode() != VK_SAMPLER_MIPMAP_MODE_NEAREST)
	{
		FATAL_ERROR();
	}
	
	if (sampler->getAnisotropyEnable())
	{
		FATAL_ERROR();
	}
	
	if (sampler->getCompareEnable())
	{
		FATAL_ERROR();
	}
	
	if (sampler->getUnnormalisedCoordinates())
	{
		FATAL_ERROR();
	}
	
	if (coordinate->x < 0 || coordinate->x > 1 || coordinate->y < 0 || coordinate->y > 1)
	{
		FATAL_ERROR();
	}
	
	const auto& formatInformation = GetFormatInformation(imageView->getFormat());
	
	const auto x = static_cast<uint32_t>(coordinate->x * imageView->getImage()->getWidth() + 0.5);
	const auto y = static_cast<uint32_t>(coordinate->y * imageView->getImage()->getHeight() + 0.5);
	
	float values[4];
	GetPixel(formatInformation, imageView->getImage(), x, y, 0, values);
	*result = glm::vec4(values[0], values[1], values[2], values[3]);
}

void AddGlslFunctions(SpirvJit* jit)
{
	AddSpirvFunction("_Image_Sample_4_F32_2_F32_Lod", reinterpret_cast<FunctionPointer>(ImageSampleLod<glm::vec4, glm::vec2>));
}
