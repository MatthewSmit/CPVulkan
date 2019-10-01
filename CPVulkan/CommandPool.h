#pragma once
#include "Base.h"

#include <vector>

struct DeviceState;

class CommandBuffer;

class CommandPool final
{
public:
	CommandPool() = default;
	CommandPool(const CommandPool&) = delete;
	CommandPool(CommandPool&&) = delete;
	~CommandPool() = default;

	CommandPool& operator=(const CommandPool&) = delete;
	CommandPool&& operator=(const CommandPool&&) = delete;

	void OnDelete(const VkAllocationCallbacks*)
	{
	}

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