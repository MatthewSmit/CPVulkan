#pragma once
#include "Base.h"

struct DeviceState;

class CommandBuffer;

class CommandPool final : public VulkanBase
{
public:
	~CommandPool() override = default;
	
	VkResult AllocateCommandBuffers(const VkCommandBufferAllocateInfo* pAllocateInfo, VkCommandBuffer* pCommandBuffers);
	void FreeCommandBuffers(uint32_t commandBufferCount, const VkCommandBuffer* pCommandBuffers);
	
	VkResult Reset(VkCommandPoolResetFlags flags);
	void Trim(VkCommandPoolTrimFlags flags);

	static VkResult Create(DeviceState* deviceState, const VkCommandPoolCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkCommandPool* pCommandPool);

private:
	DeviceState* deviceState{};
	std::vector<CommandBuffer*> commandBuffers{};
	VkCommandPoolCreateFlags flags{};
};