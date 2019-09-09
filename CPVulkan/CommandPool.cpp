#include "CommandPool.h"

#include "CommandBuffer.h"

#include <cassert>

VkResult CommandPool::AllocateCommandBuffers(const VkCommandBufferAllocateInfo* pAllocateInfo, VkCommandBuffer* pCommandBuffers)
{
	assert(pAllocateInfo->sType == VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO);

	auto next = pAllocateInfo->pNext;
	while (next)
	{
		auto type = *(VkStructureType*)next;
		switch (type)
		{
		default:
			FATAL_ERROR();
		}
	}

	for (auto i = 0u; i < pAllocateInfo->commandBufferCount; i++)
	{
		pCommandBuffers[i] = reinterpret_cast<VkCommandBuffer>(new CommandBuffer(deviceState, pAllocateInfo->level, flags));
	}

	return VK_SUCCESS;
}

void CommandPool::FreeCommandBuffers(uint32_t commandBufferCount, const VkCommandBuffer* pCommandBuffers)
{
	for (auto i = 0u; i < commandBufferCount; i++)
	{
		delete reinterpret_cast<CommandBuffer*>(pCommandBuffers[i]);
	}
}

VkResult CommandPool::Create(DeviceState* deviceState, const VkCommandPoolCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkCommandPool* pCommandPool)
{
	assert(pCreateInfo->sType == VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO);

	auto commandPool = Allocate<CommandPool>(pAllocator);

	auto next = pCreateInfo->pNext;
	while (next)
	{
		auto type = *(VkStructureType*)next;
		switch (type)
		{
		default:
			FATAL_ERROR();
		}
	}

	commandPool->flags = pCreateInfo->flags;
	commandPool->deviceState = deviceState;

	// TODO: Use queueFamilyIndex
	
	*pCommandPool = reinterpret_cast<VkCommandPool>(commandPool);
	return VK_SUCCESS;
}
