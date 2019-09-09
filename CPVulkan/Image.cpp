#include "Image.h"

#include "Formats.h"

#include <cassert>

VkResult Image::BindMemory(VkDeviceMemory memory, uint64_t memoryOffset)
{
	// TODO: Bounds check
	data = reinterpret_cast<DeviceMemory*>(memory)->Data + memoryOffset;
	return VK_SUCCESS;
}

void Image::GetMemoryRequirements(VkMemoryRequirements* pMemoryRequirements)
{
	pMemoryRequirements->size = size;
	pMemoryRequirements->alignment = 16;
	pMemoryRequirements->memoryTypeBits = 1;
}

VkResult Image::Create(const VkImageCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkImage* pImage)
{
	assert(pCreateInfo->sType == VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO);

	auto image = Allocate<Image>(pAllocator);

	auto next = pCreateInfo->pNext;
	while (next)
	{
		const auto type = *static_cast<const VkStructureType*>(next);
		switch (type)
		{
		case VK_STRUCTURE_TYPE_IMAGE_SWAPCHAIN_CREATE_INFO_KHR:
			{
				const auto createInfo = reinterpret_cast<const VkImageSwapchainCreateInfoKHR*>(next);
				next = createInfo->pNext;
				break;
			}
		default:
			FATAL_ERROR();
		}
	}

	if (pCreateInfo->flags)
	{
		FATAL_ERROR();
	}

	image->imageType = pCreateInfo->imageType;
	image->format = pCreateInfo->format;
	image->extent = pCreateInfo->extent;
	image->mipLevels = pCreateInfo->mipLevels;
	image->arrayLayers = pCreateInfo->arrayLayers;
	image->samples = pCreateInfo->samples;
	image->tiling = pCreateInfo->tiling;
	image->usage = pCreateInfo->usage;
	image->layout = pCreateInfo->initialLayout;

	if (pCreateInfo->sharingMode != VK_SHARING_MODE_EXCLUSIVE)
	{
		FATAL_ERROR();
	}

	image->size = image->extent.width * image->extent.height * image->extent.depth * image->arrayLayers * GetFormatInformation(image->format).TotalSize;
	if (image->size == 0)
	{
		FATAL_ERROR();
	}
	if (image->mipLevels > 1)
	{
		FATAL_ERROR();
	}
	
	*pImage = reinterpret_cast<VkImage>(image);
	return VK_SUCCESS;
}
