#pragma once
#include "Base.h"

class Image;

class ImageView final
{
public:
	ImageView() = default;
	ImageView(const ImageView&) = delete;
	ImageView(ImageView&&) = delete;
	~ImageView() = default;

	ImageView& operator=(const ImageView&) = delete;
	ImageView&& operator=(const ImageView&&) = delete;

	static VkResult Create(const VkImageViewCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkImageView* pView);
	
	Image* getImage() const { return image; }

private:
	Image* image;
	VkImageViewType viewType;
	VkFormat format;
	VkComponentMapping components;
	VkImageSubresourceRange subresourceRange;
};