#include "Queue.h"

#include "Buffer.h"
#include "CommandBuffer.h"
#include "Fence.h"
#include "Semaphore.h"
#include "Swapchain.h"

#include <cassert>

VkResult Queue::Submit(uint32_t submitCount, const VkSubmitInfo* pSubmits, VkFence fence)
{
	for (auto i = 0u; i < submitCount; i++)
	{
		const auto& submit = pSubmits[i];
		assert(submit.sType == VK_STRUCTURE_TYPE_SUBMIT_INFO);

		auto next = submit.pNext;
		while (next)
		{
			const auto type = static_cast<const VkBaseInStructure*>(next)->sType;
			switch (type)
			{
			case VK_STRUCTURE_TYPE_D3D12_FENCE_SUBMIT_INFO_KHR:
				FATAL_ERROR();

			case VK_STRUCTURE_TYPE_DEVICE_GROUP_SUBMIT_INFO:
				FATAL_ERROR();

			case VK_STRUCTURE_TYPE_PROTECTED_SUBMIT_INFO:
				FATAL_ERROR();

			case VK_STRUCTURE_TYPE_WIN32_KEYED_MUTEX_ACQUIRE_RELEASE_INFO_KHR:
				FATAL_ERROR();

			case VK_STRUCTURE_TYPE_WIN32_KEYED_MUTEX_ACQUIRE_RELEASE_INFO_NV:
				FATAL_ERROR();
			}
			next = static_cast<const VkBaseInStructure*>(next)->pNext;
		}

		if (submit.signalSemaphoreCount)
		{
			FATAL_ERROR();
		}

		for (auto j = 0u; j < submit.waitSemaphoreCount; j++)
		{
			const auto result = UnwrapVulkan<Semaphore>(submit.pWaitSemaphores[j])->Wait(std::numeric_limits<uint64_t>::max());
			if (result != VK_SUCCESS)
			{
				FATAL_ERROR();
			}
		}

		for (auto j = 0u; j < submit.commandBufferCount; j++)
		{
			const auto result = UnwrapVulkan<CommandBuffer>(submit.pCommandBuffers[j])->Submit();
			if (result != VK_SUCCESS)
			{
				FATAL_ERROR();
			}
		}
	}

	if (fence)
	{
		UnwrapVulkan<Fence>(fence)->Signal();
	}

	return VK_SUCCESS;
}

VkResult Queue::WaitIdle()
{
	// TODO
	return VK_SUCCESS;
}

VkResult Queue::BindSparse(uint32_t bindInfoCount, const VkBindSparseInfo* pBindInfo, VkFence fence)
{
	for (auto i = 0u; i < bindInfoCount; i++)
	{
		const auto& bindInfo = pBindInfo[i];
		assert(bindInfo.sType == VK_STRUCTURE_TYPE_BIND_SPARSE_INFO);

		auto next = bindInfo.pNext;
		while (next)
		{
			const auto type = static_cast<const VkBaseInStructure*>(next)->sType;
			switch (type)
			{
			case VK_STRUCTURE_TYPE_DEVICE_GROUP_BIND_SPARSE_INFO:
				FATAL_ERROR();
			}
			next = static_cast<const VkBaseInStructure*>(next)->pNext;
		}

		if (bindInfo.waitSemaphoreCount)
		{
			FATAL_ERROR();
		}

		for (auto j = 0u; j < bindInfo.bufferBindCount; j++)
		{
			const auto& bufferBind = bindInfo.pBufferBinds[j];
			UnwrapVulkan<SparseBuffer>(bufferBind.buffer)->SparseBindMemory(bufferBind.bindCount, bufferBind.pBinds);
		}

		if (bindInfo.imageOpaqueBindCount)
		{
			FATAL_ERROR();
		}

		if (bindInfo.imageBindCount)
		{
			FATAL_ERROR();
		}

		if (bindInfo.signalSemaphoreCount)
		{
			FATAL_ERROR();
		}
	}
	
	if (fence)
	{
		UnwrapVulkan<Fence>(fence)->Signal();
	}
	return VK_SUCCESS;
}

VkResult Queue::Present(const VkPresentInfoKHR* pPresentInfo)
{
	assert(pPresentInfo->sType == VK_STRUCTURE_TYPE_PRESENT_INFO_KHR);

	auto next = pPresentInfo->pNext;
	while (next)
	{
		auto type = *static_cast<const VkStructureType*>(next);
		switch (type)
		{
		default:
			FATAL_ERROR();
		}
	}

	for (auto j = 0u; j < pPresentInfo->waitSemaphoreCount; j++)
	{
		const auto result = UnwrapVulkan<Semaphore>(pPresentInfo->pWaitSemaphores[j])->Wait(std::numeric_limits<uint64_t>::max());
		if (result != VK_SUCCESS)
		{
			FATAL_ERROR();
		}
	}

	if (pPresentInfo->swapchainCount != 1)
	{
		FATAL_ERROR();
	}
	
	for (auto i = 0u; i < pPresentInfo->swapchainCount; i++)
	{
		auto result = UnwrapVulkan<Swapchain>(pPresentInfo->pSwapchains[i])->Present(pPresentInfo->pImageIndices[i]);
		if (pPresentInfo->pResults)
		{
			pPresentInfo->pResults[i] = result;
		}

		if (result != VK_SUCCESS)
		{
			FATAL_ERROR();
		}
	}

	return VK_SUCCESS;
}

Queue* Queue::Create(const VkDeviceQueueCreateInfo& vkDeviceQueueCreateInfo, const VkAllocationCallbacks* pAllocator)
{
	auto queue = Allocate<Queue>(pAllocator, VK_SYSTEM_ALLOCATION_SCOPE_DEVICE);
	queue->flags = vkDeviceQueueCreateInfo.flags;

	return queue;
}
