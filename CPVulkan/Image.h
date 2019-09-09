#pragma once
#include "Base.h"

class Image final : public VulkanBase
{
public:
	VkResult BindMemory(VkDeviceMemory memory, uint64_t memoryOffset);
	void GetMemoryRequirements(VkMemoryRequirements* pMemoryRequirements);

	static VkResult Create(const VkImageCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkImage* pImage);

	uint32_t getWidth() const { return extent.width; }
	uint32_t getHeight() const { return extent.height; }
	uint32_t getDepth() const { return extent.depth; }
	
	void* getData() const { return data; }

private:
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
	void* data{};
};