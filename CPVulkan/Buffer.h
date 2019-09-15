#pragma once
#include "Base.h"

class Buffer final : public VulkanBase
{
public:
	~Buffer() override = default;

	VkResult BindMemory(VkDeviceMemory memory, uint64_t memoryOffset);
	void GetMemoryRequirements(VkMemoryRequirements* pMemoryRequirements);

	static VkResult Create(const VkBufferCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkBuffer* pBuffer);

	[[nodiscard]] uint8_t* getData() const { return data; }

private:
	VkBufferUsageFlags usage{};

	uint8_t* data{};
	uint64_t size{};
};
