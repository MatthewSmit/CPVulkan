#pragma once
#include "Base.h"

struct DeviceState;

class CommandPool final : public VulkanBase
{
public:
	~CommandPool() override = default;
	
	VkResult AllocateCommandBuffers(const VkCommandBufferAllocateInfo* pAllocateInfo, VkCommandBuffer* pCommandBuffers);
	void FreeCommandBuffers(uint32_t commandBufferCount, const VkCommandBuffer* pCommandBuffers);

	static VkResult Create(DeviceState* deviceState, const VkCommandPoolCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkCommandPool* pCommandPool);

private:
	DeviceState* deviceState{};
	VkCommandPoolCreateFlags flags{};
};