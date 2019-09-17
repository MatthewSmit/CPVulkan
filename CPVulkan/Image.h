#pragma once
#include "Base.h"

class Image final : public VulkanBase
{
public:
	VkResult BindMemory(VkDeviceMemory memory, uint64_t memoryOffset);
	void GetMemoryRequirements(VkMemoryRequirements* pMemoryRequirements) const;

	static VkResult Create(const VkImageCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkImage* pImage);

	[[nodiscard]] VkFormat getFormat() const { return format; }
	[[nodiscard]] uint32_t getWidth() const { return extent.width; }
	[[nodiscard]] uint32_t getHeight() const { return extent.height; }
	[[nodiscard]] uint32_t getDepth() const { return extent.depth; }
	
	[[nodiscard]] uint8_t* getData() const { return data; }

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
	
	uint64_t size{};
    uint8_t* data{};
};