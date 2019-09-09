#pragma once
#include "Base.h"

class Buffer final : public VulkanBase
{
public:
	~Buffer() override = default;

	VkResult BindMemory(VkDeviceMemory memory, uint64_t memoryOffset);
	void GetMemoryRequirements(VkMemoryRequirements* pMemoryRequirements);

	static VkResult Create(const VkBufferCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkBuffer* pBuffer);

private:
	VkBufferUsageFlags usage{};

	void* data{};
	uint64_t size{};
};
