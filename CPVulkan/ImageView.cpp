#include "ImageView.h"

#include "Device.h"

#include <cassert>

VkResult ImageView::Create(const VkImageViewCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkImageView* pView)
{
	assert(pCreateInfo->sType == VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO);

	auto imageView = Allocate<ImageView>(pAllocator, VK_SYSTEM_ALLOCATION_SCOPE_DEVICE);
	if (!imageView)
	{
		return VK_ERROR_OUT_OF_HOST_MEMORY;
	}

	auto next = pCreateInfo->pNext;
	while (next)
	{
		const auto type = static_cast<const VkBaseInStructure*>(next)->sType;
		switch (type)
		{
		case VK_STRUCTURE_TYPE_IMAGE_VIEW_ASTC_DECODE_MODE_EXT:
			TODO_ERROR();

		case VK_STRUCTURE_TYPE_IMAGE_VIEW_USAGE_CREATE_INFO:
			TODO_ERROR();

		case VK_STRUCTURE_TYPE_SAMPLER_YCBCR_CONVERSION_INFO:
			TODO_ERROR();

		default:
			break;
		}
		next = static_cast<const VkBaseInStructure*>(next)->pNext;
	}

	if (pCreateInfo->flags)
	{
		TODO_ERROR();
	}
	
	imageView->image = UnwrapVulkan<Image>(pCreateInfo->image);
	imageView->viewType = pCreateInfo->viewType;
	imageView->format = pCreateInfo->format;
	imageView->components = pCreateInfo->components;
	imageView->subresourceRange = pCreateInfo->subresourceRange;

	WrapVulkan(imageView, pView);
	return VK_SUCCESS;
}

VkResult Device::CreateImageView(const VkImageViewCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkImageView* pView)
{
	return ImageView::Create(pCreateInfo, pAllocator, pView);
}

void Device::DestroyImageView(VkImageView imageView, const VkAllocationCallbacks* pAllocator)
{
	if (imageView)
	{
		Free(UnwrapVulkan<ImageView>(imageView), pAllocator);
	}
}
