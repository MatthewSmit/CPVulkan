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

		auto next = static_cast<const VkBaseInStructure*>(submit.pNext);
		while (next)
		{
			const auto type = next->sType;
			switch (type)
			{
			case VK_STRUCTURE_TYPE_D3D12_FENCE_SUBMIT_INFO_KHR:
				TODO_ERROR();

			case VK_STRUCTURE_TYPE_DEVICE_GROUP_SUBMIT_INFO:
				// We can just ignore as all device indices must be 0
				break;

			case VK_STRUCTURE_TYPE_PROTECTED_SUBMIT_INFO:
				TODO_ERROR();

			case VK_STRUCTURE_TYPE_WIN32_KEYED_MUTEX_ACQUIRE_RELEASE_INFO_KHR:
				TODO_ERROR();

			case VK_STRUCTURE_TYPE_WIN32_KEYED_MUTEX_ACQUIRE_RELEASE_INFO_NV:
				TODO_ERROR();
			}
			next = next->pNext;
		}

		for (auto j = 0u; j < submit.waitSemaphoreCount; j++)
		{
			const auto result = UnwrapVulkan<Semaphore>(submit.pWaitSemaphores[j])->Wait(std::numeric_limits<uint64_t>::max());
			if (result != VK_SUCCESS)
			{
				TODO_ERROR();
			}
		}

		for (auto j = 0u; j < submit.commandBufferCount; j++)
		{
			const auto result = UnwrapVulkan<CommandBuffer>(submit.pCommandBuffers[j])->Submit();
			if (result != VK_SUCCESS)
			{
				TODO_ERROR();
			}
		}

		for (auto j = 0u; j < submit.signalSemaphoreCount; j++)
		{
			const auto result = UnwrapVulkan<Semaphore>(submit.pSignalSemaphores[j])->Signal();
			if (result != VK_SUCCESS)
			{
				TODO_ERROR();
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
	TODO_ERROR();
}

VkResult Queue::Present(const VkPresentInfoKHR* pPresentInfo)
{
	assert(pPresentInfo->sType == VK_STRUCTURE_TYPE_PRESENT_INFO_KHR);

	auto next = static_cast<const VkBaseInStructure*>(pPresentInfo->pNext);
	while (next)
	{
		const auto type = next->sType;
		switch (type)
		{
		case VK_STRUCTURE_TYPE_DEVICE_GROUP_PRESENT_INFO_KHR:
			// We can just ignore as all device indices must be 0
			break;

		case VK_STRUCTURE_TYPE_DISPLAY_PRESENT_INFO_KHR:
			TODO_ERROR();

		case VK_STRUCTURE_TYPE_PRESENT_FRAME_TOKEN_GGP:
			TODO_ERROR();

		case VK_STRUCTURE_TYPE_PRESENT_REGIONS_KHR:
			TODO_ERROR();
			
		case VK_STRUCTURE_TYPE_PRESENT_TIMES_INFO_GOOGLE:
			TODO_ERROR();
		}
		next = next->pNext;
	}

	for (auto j = 0u; j < pPresentInfo->waitSemaphoreCount; j++)
	{
		const auto result = UnwrapVulkan<Semaphore>(pPresentInfo->pWaitSemaphores[j])->Wait(std::numeric_limits<uint64_t>::max());
		if (result != VK_SUCCESS)
		{
			TODO_ERROR();
		}
	}

	if (pPresentInfo->swapchainCount != 1)
	{
		FATAL_ERROR();
	}
	
	for (auto i = 0u; i < pPresentInfo->swapchainCount; i++)
	{
		const auto result = UnwrapVulkan<Swapchain>(pPresentInfo->pSwapchains[i])->Present(pPresentInfo->pImageIndices[i]);
		if (pPresentInfo->pResults)
		{
			pPresentInfo->pResults[i] = result;
		}

		if (result != VK_SUCCESS)
		{
			TODO_ERROR();
		}
	}

	return VK_SUCCESS;
}

Queue* Queue::Create(const VkDeviceQueueCreateInfo* vkDeviceQueueCreateInfo, const VkAllocationCallbacks* pAllocator)
{
	assert(vkDeviceQueueCreateInfo->sType == VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO);

	auto next = static_cast<const VkBaseInStructure*>(vkDeviceQueueCreateInfo->pNext);
	while (next)
	{
		const auto type = next->sType;
		switch (type)
		{
		case VK_STRUCTURE_TYPE_DEVICE_QUEUE_GLOBAL_PRIORITY_CREATE_INFO_EXT:
			TODO_ERROR();
		}
		next = next->pNext;
	}

	const auto queue = Allocate<Queue>(pAllocator, VK_SYSTEM_ALLOCATION_SCOPE_DEVICE);
	if (!queue)
	{
		return nullptr;
	}
	queue->flags = vkDeviceQueueCreateInfo->flags;

	return queue;
}
