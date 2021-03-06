#pragma once
#include "Base.h"

#include <Formats.h>

class Image final
{
public:
	Image() = default;
	Image(const Image&) = delete;
	Image(Image&&) = delete;
	~Image() = default;

	Image& operator=(const Image&) = delete;
	Image&& operator=(const Image&&) = delete;

	void OnDelete(const VkAllocationCallbacks*)
	{
	}

	VkResult BindMemory(VkDeviceMemory memory, uint64_t memoryOffset);
	VkResult BindSwapchainMemory(Swapchain* swapchain, uint32_t imageIndex);
	
	void GetMemoryRequirements(VkMemoryRequirements* pMemoryRequirements) const;
	void GetSubresourceLayout(const VkImageSubresource* pSubresource, VkSubresourceLayout* pLayout);

	static VkResult Create(const VkImageCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkImage* pImage);

	[[nodiscard]] VkFormat getFormat() const { return format; }
	[[nodiscard]] uint32_t getWidth() const { return extent.width; }
	[[nodiscard]] uint32_t getHeight() const { return extent.height; }
	[[nodiscard]] uint32_t getDepth() const { return extent.depth; }
	[[nodiscard]] uint32_t getArrayLayers() const { return arrayLayers; }
	[[nodiscard]] VkSampleCountFlagBits getSamples() const { return samples; }
	[[nodiscard]] uint32_t getMipLevels() const { return mipLevels; }
	[[nodiscard]] const ImageSize& getImageSize() const { return imageSize; }

	[[nodiscard]] gsl::span<uint8_t> getData() const
	{
		return data;
	}

	[[nodiscard]] gsl::span<uint8_t> getData(uint64_t offset, uint64_t size) const
	{
		return data.subspan(offset, size);
	}

	[[nodiscard]] uint8_t* getDataPtr(uint64_t offset, uint64_t size) const
	{
		return getData(offset, size).data();
	}

private:
	VkImageCreateFlags flags{};
	VkImageType imageType{};
	VkFormat format{};
	VkExtent3D extent{};
	uint32_t mipLevels{};
	uint32_t arrayLayers{};
	VkSampleCountFlagBits samples{};
	VkImageTiling tiling{};
	VkImageUsageFlags usage{};
	VkImageLayout layout{};

	gsl::span<uint8_t> data{};
	ImageSize imageSize{};
};
