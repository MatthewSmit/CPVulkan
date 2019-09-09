#include "Queue.h"

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
			auto type = *static_cast<const VkStructureType*>(next);
			switch (type)
			{
			default:
				FATAL_ERROR();
			}
		}

		if (submit.signalSemaphoreCount)
		{
			FATAL_ERROR();
		}

		for (auto j = 0u; j < submit.waitSemaphoreCount; j++)
		{
			auto result = reinterpret_cast<Semaphore*>(submit.pWaitSemaphores[j])->Wait();
			if (result != VK_SUCCESS)
			{
				FATAL_ERROR();
			}
		}

		for (auto j = 0u; j < submit.commandBufferCount; j++)
		{
			const auto result = reinterpret_cast<CommandBuffer*>(submit.pCommandBuffers[j])->Submit();
			if (result != VK_SUCCESS)
			{
				FATAL_ERROR();
			}
		}
	}

	if (fence)
	{
		reinterpret_cast<Fence*>(fence)->Signal();
	}

	return VK_SUCCESS;
}

VkResult Queue::WaitIdle()
{
	// TODO
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
		const auto result = reinterpret_cast<Semaphore*>(pPresentInfo->pWaitSemaphores[j])->Wait();
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
		auto result = reinterpret_cast<Swapchain*>(pPresentInfo->pSwapchains[i])->Present(pPresentInfo->pImageIndices[i]);
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
	auto queue = Allocate<Queue>(pAllocator);
	queue->flags = vkDeviceQueueCreateInfo.flags;

	return queue;
}
