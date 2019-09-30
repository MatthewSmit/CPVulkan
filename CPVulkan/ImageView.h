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

	void OnDelete(const VkAllocationCallbacks*)
	{
	}

	static VkResult Create(const VkImageViewCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkImageView* pView);
	
	[[nodiscard]] Image* getImage() { return image; }
	[[nodiscard]] const Image* getImage() const { return image; }
	[[nodiscard]] VkImageViewType getViewType() const { return viewType; }
	[[nodiscard]] VkFormat getFormat() const { return format; }
	[[nodiscard]] const VkComponentMapping& getComponents() const { return components; }
	[[nodiscard]] const VkImageSubresourceRange& getSubresourceRange() const { return subresourceRange; }

private:
	Image* image;
	VkImageViewType viewType;
	VkFormat format;
	VkComponentMapping components;
	VkImageSubresourceRange subresourceRange;
};