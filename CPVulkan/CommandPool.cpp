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
		}
		next = static_cast<const VkBaseInStructure*>(next)->pNext;
	}

	for (auto i = 0u; i < pAllocateInfo->commandBufferCount; i++)
	{
		auto commandBuffer = Allocate<CommandBuffer>(nullptr, VK_SYSTEM_ALLOCATION_SCOPE_DEVICE, deviceState, pAllocateInfo->level, flags);
		if (!commandBuffer)
		{
			FATAL_ERROR();
			// return VK_ERROR_OUT_OF_HOST_MEMORY;
		}
		commandBuffers.push_back(commandBuffer);
		WrapVulkan(commandBuffer, &pCommandBuffers[i]);
	}

	return VK_SUCCESS;
}

void CommandPool::FreeCommandBuffers(uint32_t commandBufferCount, const VkCommandBuffer* pCommandBuffers)
{
	for (auto i = 0u; i < commandBufferCount; i++)
	{
		auto commandBuffer = UnwrapVulkan<CommandBuffer>(pCommandBuffers[i]);
		auto commandBufferPtr = std::find(commandBuffers.begin(), commandBuffers.end(), commandBuffer);
		if (commandBufferPtr == commandBuffers.end())
		{
			FATAL_ERROR();
		}
		commandBuffers.erase(commandBufferPtr);
		Free(commandBuffer, nullptr);
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

	const auto commandPool = Allocate<CommandPool>(pAllocator, VK_SYSTEM_ALLOCATION_SCOPE_DEVICE);
	if (!commandPool)
	{
		return VK_ERROR_OUT_OF_HOST_MEMORY;
	}

	auto next = pCreateInfo->pNext;
	while (next)
	{
		const auto type = static_cast<const VkBaseInStructure*>(next)->sType;
		switch (type)
		{
		}
		next = static_cast<const VkBaseInStructure*>(next)->pNext;
	}

	commandPool->flags = pCreateInfo->flags;
	commandPool->deviceState = deviceState;

	// TODO: Use queueFamilyIndex

	WrapVulkan(commandPool, pCommandPool);
	return VK_SUCCESS;
}

VkResult Device::CreateCommandPool(const VkCommandPoolCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkCommandPool* pCommandPool)
{
	return CommandPool::Create(state.get(), pCreateInfo, pAllocator, pCommandPool);
}

void Device::DestroyCommandPool(VkCommandPool commandPool, const VkAllocationCallbacks* pAllocator)
{
	Free(UnwrapVulkan<CommandPool>(commandPool), pAllocator);
}

VkResult Device::ResetCommandPool(VkCommandPool commandPool, VkCommandPoolResetFlags flags)
{
	return UnwrapVulkan<CommandPool>(commandPool)->Reset(flags);
}

VkResult Device::AllocateCommandBuffers(const VkCommandBufferAllocateInfo* pAllocateInfo, VkCommandBuffer* pCommandBuffers)
{
	return UnwrapVulkan<CommandPool>(pAllocateInfo->commandPool)->AllocateCommandBuffers(pAllocateInfo, pCommandBuffers);
}

void Device::FreeCommandBuffers(VkCommandPool commandPool, uint32_t commandBufferCount, const VkCommandBuffer* pCommandBuffers)
{
	UnwrapVulkan<CommandPool>(commandPool)->FreeCommandBuffers(commandBufferCount, pCommandBuffers);
}

void Device::TrimCommandPool(VkCommandPool commandPool, VkCommandPoolTrimFlags flags)
{
	UnwrapVulkan<CommandPool>(commandPool)->Trim(flags);
}