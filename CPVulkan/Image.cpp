#include "Image.h"

#include "Device.h"
#include "Formats.h"
#include "Util.h"

#include <cassert>

void Device::DestroyImage(VkImage image, const VkAllocationCallbacks* pAllocator)
{
	Free(UnwrapVulkan<Image>(image), pAllocator);
}

VkResult Image::BindMemory(VkDeviceMemory memory, uint64_t memoryOffset)
{
	// TODO: Bounds check
	data = UnwrapVulkan<DeviceMemory>(memory)->Data + memoryOffset;
	return VK_SUCCESS;
}

VkResult Device::BindImageMemory(VkImage image, VkDeviceMemory memory, VkDeviceSize memoryOffset)
{
	return UnwrapVulkan<Image>(image)->BindMemory(memory, memoryOffset);
}

void Image::GetMemoryRequirements(VkMemoryRequirements* pMemoryRequirements) const
{
	pMemoryRequirements->size = size;
	pMemoryRequirements->alignment = 16;
	pMemoryRequirements->memoryTypeBits = 1;
}

void Image::GetSubresourceLayout(const VkImageSubresource* pSubresource, VkSubresourceLayout* pLayout)
{
	if (tiling == VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT)
	{
		FATAL_ERROR();
	}
	else if (tiling != VK_IMAGE_TILING_LINEAR)
	{
		FATAL_ERROR();
	}

	auto const& formatInformation = GetFormatInformation(format);
	if (formatInformation.Type != FormatType::Normal)
	{
		FATAL_ERROR();
	}

	if (pSubresource->aspectMask != VK_IMAGE_ASPECT_COLOR_BIT)
	{
		FATAL_ERROR();
	}

	if (pSubresource->arrayLayer != 0)
	{
		FATAL_ERROR();
	}

	if (pSubresource->mipLevel != 0)
	{
		FATAL_ERROR();
	}

	pLayout->offset = 0;
	pLayout->rowPitch = getWidth() * VkDeviceSize{formatInformation.TotalSize};
	pLayout->depthPitch = getHeight() * pLayout->rowPitch;
	pLayout->arrayPitch = getDepth() * pLayout->depthPitch;
	pLayout->size = arrayLayers * pLayout->arrayPitch;
}

void Device::GetImageMemoryRequirements(VkImage image, VkMemoryRequirements* pMemoryRequirements)
{
	UnwrapVulkan<Image>(image)->GetMemoryRequirements(pMemoryRequirements);
}

void Device::GetImageSubresourceLayout(VkImage image, const VkImageSubresource* pSubresource, VkSubresourceLayout* pLayout)
{
	UnwrapVulkan<Image>(image)->GetSubresourceLayout(pSubresource, pLayout);
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

	WrapVulkan(image, pImage);
	return VK_SUCCESS;
}

VkResult Device::CreateImage(const VkImageCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkImage* pImage)
{
	return Image::Create(pCreateInfo, pAllocator, pImage);
}
