#include "ImageView.h"

#include <cassert>

VkResult ImageView::Create(const VkImageViewCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkImageView* pView)
{
	assert(pCreateInfo->sType == VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO);

	auto imageView = Allocate<ImageView>(pAllocator);

	auto next = pCreateInfo->pNext;
	while (next)
	{
		const auto type = *static_cast<const VkStructureType*>(next);
		switch (type)
		{
		default:
			FATAL_ERROR();
		}
	}

	if (pCreateInfo->flags)
	{
		FATAL_ERROR();
	}
	
	imageView->image = reinterpret_cast<Image*>(pCreateInfo->image);
	imageView->viewType = pCreateInfo->viewType;
	imageView->format = pCreateInfo->format;
	imageView->components = pCreateInfo->components;
	imageView->subresourceRange = pCreateInfo->subresourceRange;

	*pView = reinterpret_cast<VkImageView>(imageView);
	return VK_SUCCESS;
}
