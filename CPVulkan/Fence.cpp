#include "Fence.h"

#include "Device.h"
#include "Platform.h"
#include "Util.h"

#include <cassert>

Fence::~Fence()
{
	Platform::CloseMutex(handle);
}

VkResult Fence::Signal()
{
	Platform::SignalMutex(handle);
	return VK_SUCCESS;
}

VkResult Fence::Reset()
{
	Platform::ResetMutex(handle);
	return VK_SUCCESS;
}

VkResult Fence::Wait(uint64_t timeout)
{
	return Platform::Wait(handle, timeout) ? VK_SUCCESS : VK_TIMEOUT;
}

VkResult Fence::Create(const VkFenceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkFence* pFence)
{
	assert(pCreateInfo->sType == VK_STRUCTURE_TYPE_FENCE_CREATE_INFO);

	const auto fence = Allocate<Fence>(pAllocator, VK_SYSTEM_ALLOCATION_SCOPE_DEVICE);
	if (!fence)
	{
		return VK_ERROR_OUT_OF_HOST_MEMORY;
	}

	auto next = pCreateInfo->pNext;
	while (next)
	{
		const auto type = static_cast<const VkBaseInStructure*>(next)->sType;
		switch (type)
		{
		case VK_STRUCTURE_TYPE_EXPORT_FENCE_CREATE_INFO:
			FATAL_ERROR();
			
		case VK_STRUCTURE_TYPE_EXPORT_FENCE_WIN32_HANDLE_INFO_KHR:
			FATAL_ERROR();
		}
		next = static_cast<const VkBaseInStructure*>(next)->pNext;
	}

	fence->handle = Platform::CreateMutex(pCreateInfo->flags & VK_FENCE_CREATE_SIGNALED_BIT, true);

	WrapVulkan(fence, pFence);
	return VK_SUCCESS;
}

VkResult Fence::getStatus() const
{
	return Platform::GetMutexStatus(handle) ? VK_SUCCESS : VK_NOT_READY;
}

VkResult Device::CreateFence(const VkFenceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkFence* pFence)
{
	return Fence::Create(pCreateInfo, pAllocator, pFence);
}

void Device::DestroyFence(VkFence fence, const VkAllocationCallbacks* pAllocator)
{
	Free(UnwrapVulkan<Fence>(fence), pAllocator);
}

VkResult Device::ResetFences(uint32_t fenceCount, const VkFence* pFences)
{
	for (auto i = 0u; i < fenceCount; i++)
	{
		if (UnwrapVulkan<Fence>(pFences[i])->Reset() != VK_SUCCESS)
		{
			FATAL_ERROR();
		}
	}
	return VK_SUCCESS;
}

VkResult Device::GetFenceStatus(VkFence fence) const
{
	return UnwrapVulkan<Fence>(fence)->getStatus();
}

VkResult Device::WaitForFences(uint32_t fenceCount, const VkFence* pFences, VkBool32 waitAll, uint64_t timeout)
{
	return Platform::WaitMultiple(ArrayToVector<void*, VkFence>(fenceCount, pFences, [](VkFence fence)
	{
		return UnwrapVulkan<Fence>(fence)->getHandle();
	}), waitAll, timeout)
		       ? VK_SUCCESS
		       : VK_TIMEOUT;
}
