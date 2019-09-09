#pragma once
#include "Base.h"

class Image;

class ImageView final : public VulkanBase
{
public:
	static VkResult Create(const VkImageViewCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkImageView* pView);
	
	Image* getImage() const { return image; }

private:
	Image* image;
	VkImageViewType viewType;
	VkFormat format;
	VkComponentMapping components;
	VkImageSubresourceRange subresourceRange;
};