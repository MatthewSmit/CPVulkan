#include "CommandPool.h"

#include "CommandBuffer.h"
#include "Device.h"

#include <cassert>

VkResult CommandPool::AllocateCommandBuffers(const VkCommandBufferAllocateInfo* pAllocateInfo, VkCommandBuffer* pCommandBuffers)
{
	assert(pAllocateInfo->sType == VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO);

	auto next = pAllocateInfo->pNext;
	while (next)
	{
		const auto type = static_cast<const VkBaseInStructure*>(next)->sType;
		switch (type)
		{
		default:
			next = static_cast<const VkBaseInStructure*>(next)->pNext;
			break;
		}
	}

	for (auto i = 0u; i < pAllocateInfo->commandBufferCount; i++)
	{
		auto commandBuffer = new CommandBuffer(deviceState, pAllocateInfo->level, flags);
		commandBuffers.push_back(commandBuffer);
		pCommandBuffers[i] = reinterpret_cast<VkCommandBuffer>(commandBuffer);
	}

	return VK_SUCCESS;
}

void CommandPool::FreeCommandBuffers(uint32_t commandBufferCount, const VkCommandBuffer* pCommandBuffers)
{
	for (auto i = 0u; i < commandBufferCount; i++)
	{
		auto commandBuffer = reinterpret_cast<CommandBuffer*>(pCommandBuffers[i]);
		auto commandBufferPtr = std::find(commandBuffers.begin(), commandBuffers.end(), commandBuffer);
		if (commandBufferPtr == commandBuffers.end())
		{
			FATAL_ERROR();
		}
		commandBuffers.erase(commandBufferPtr);
		delete commandBuffer;
	}
}

VkResult CommandPool::Reset(VkCommandPoolResetFlags flags)
{
	for (const auto commandBuffer : commandBuffers)
	{
		commandBuffer->ForceReset();
	}
	return VK_SUCCESS;
}

void CommandPool::Trim(VkCommandPoolTrimFlags)
{
	// TODO
}

VkResult CommandPool::Create(DeviceState* deviceState, const VkCommandPoolCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkCommandPool* pCommandPool)
{
	assert(pCreateInfo->sType == VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO);

	auto commandPool = Allocate<CommandPool>(pAllocator, VK_SYSTEM_ALLOCATION_SCOPE_DEVICE);

	auto next = pCreateInfo->pNext;
	while (next)
	{
		const auto type = static_cast<const VkBaseInStructure*>(next)->sType;
		switch (type)
		{
		default:
			next = static_cast<const VkBaseInStructure*>(next)->pNext;
			break;
		}
	}

	commandPool->flags = pCreateInfo->flags;
	commandPool->deviceState = deviceState;

	// TODO: Use queueFamilyIndex
	
	*pCommandPool = reinterpret_cast<VkCommandPool>(commandPool);
	return VK_SUCCESS;
}

VkResult Device::CreateCommandPool(const VkCommandPoolCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkCommandPool* pCommandPool)
{
	return CommandPool::Create(state.get(), pCreateInfo, pAllocator, pCommandPool);
}

void Device::DestroyCommandPool(VkCommandPool commandPool, const VkAllocationCallbacks* pAllocator)
{
	Free(reinterpret_cast<CommandPool*>(commandPool), pAllocator);
}

VkResult Device::ResetCommandPool(VkCommandPool commandPool, VkCommandPoolResetFlags flags)
{
	return reinterpret_cast<CommandPool*>(commandPool)->Reset(flags);
}

VkResult Device::AllocateCommandBuffers(const VkCommandBufferAllocateInfo* pAllocateInfo, VkCommandBuffer* pCommandBuffers)
{
	return reinterpret_cast<CommandPool*>(pAllocateInfo->commandPool)->AllocateCommandBuffers(pAllocateInfo, pCommandBuffers);
}

void Device::FreeCommandBuffers(VkCommandPool commandPool, uint32_t commandBufferCount, const VkCommandBuffer* pCommandBuffers)
{
	reinterpret_cast<CommandPool*>(commandPool)->FreeCommandBuffers(commandBufferCount, pCommandBuffers);
}

void Device::TrimCommandPool(VkCommandPool commandPool, VkCommandPoolTrimFlags flags)
{
	reinterpret_cast<CommandPool*>(commandPool)->Trim(flags);
}