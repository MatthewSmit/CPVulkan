#include "Image.h"

#include "Device.h"
#include "Formats.h"

#include <cassert>

VkResult Image::BindMemory(VkDeviceMemory memory, uint64_t memoryOffset)
{
	// TODO: Bounds check
	data = reinterpret_cast<DeviceMemory*>(memory)->Data + memoryOffset;
	return VK_SUCCESS;
}

VkResult Device::BindImageMemory(VkImage image, VkDeviceMemory memory, VkDeviceSize memoryOffset)
{
	return reinterpret_cast<Image*>(image)->BindMemory(memory, memoryOffset);
}

void Image::GetMemoryRequirements(VkMemoryRequirements* pMemoryRequirements) const
{
	pMemoryRequirements->size = size;
	pMemoryRequirements->alignment = 16;
	pMemoryRequirements->memoryTypeBits = 1;
}

void Device::GetImageMemoryRequirements(VkImage image, VkMemoryRequirements* pMemoryRequirements)
{
	reinterpret_cast<Image*>(image)->GetMemoryRequirements(pMemoryRequirements);
}

VkResult Image::Create(const VkImageCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkImage* pImage)
{
	assert(pCreateInfo->sType == VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO);

	auto image = Allocate<Image>(pAllocator, VK_SYSTEM_ALLOCATION_SCOPE_DEVICE);

	auto next = pCreateInfo->pNext;
	while (next)
	{
		const auto type = static_cast<const VkBaseInStructure*>(next)->sType;
		switch (type)
		{
		case VK_STRUCTURE_TYPE_DEDICATED_ALLOCATION_IMAGE_CREATE_INFO_NV:
			FATAL_ERROR();

		case VK_STRUCTURE_TYPE_EXTERNAL_FORMAT_ANDROID:
			FATAL_ERROR();

		case VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO:
			FATAL_ERROR();

		case VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO_NV:
			FATAL_ERROR();

		case VK_STRUCTURE_TYPE_IMAGE_DRM_FORMAT_MODIFIER_EXPLICIT_CREATE_INFO_EXT:
			FATAL_ERROR();

		case VK_STRUCTURE_TYPE_IMAGE_DRM_FORMAT_MODIFIER_LIST_CREATE_INFO_EXT:
			FATAL_ERROR();

		case VK_STRUCTURE_TYPE_IMAGE_FORMAT_LIST_CREATE_INFO_KHR:
			FATAL_ERROR();

		case VK_STRUCTURE_TYPE_IMAGE_STENCIL_USAGE_CREATE_INFO_EXT:
			FATAL_ERROR();

		case VK_STRUCTURE_TYPE_IMAGE_SWAPCHAIN_CREATE_INFO_KHR:
			break;
		}
		next = static_cast<const VkBaseInStructure*>(next)->pNext;
	}

	image->flags = pCreateInfo->flags;
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

VkResult Device::CreateImage(const VkImageCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkImage* pImage)
{
	return Image::Create(pCreateInfo, pAllocator, pImage);
}

void Device::DestroyImage(VkImage image, const VkAllocationCallbacks* pAllocator)
{
	Free(reinterpret_cast<Image*>(image), pAllocator);
}
