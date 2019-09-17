#include "Fence.h"

#include "Device.h"
#include "Platform.h"
#include "Util.h"

#include <cassert>

Fence::~Fence()
{
	Platform::CloseMutex(handle);
}

void Fence::Signal()
{
	Platform::ReleaseMutex(handle);
}

VkResult Fence::Reset()
{
	Platform::SetMutex(handle);
	return VK_SUCCESS;
}

VkResult Fence::Wait(uint64_t timeout)
{
	return Platform::Wait(handle, timeout);
}

VkResult Fence::Create(const VkFenceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkFence* pFence)
{
	assert(pCreateInfo->sType == VK_STRUCTURE_TYPE_FENCE_CREATE_INFO);

	auto fence = Allocate<Fence>(pAllocator, VK_SYSTEM_ALLOCATION_SCOPE_DEVICE);

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

		default:
			next = static_cast<const VkBaseInStructure*>(next)->pNext;
			break;
		}
	}

	fence->handle = Platform::CreateMutex(pCreateInfo->flags & VK_FENCE_CREATE_SIGNALED_BIT);

	WrapVulkan(fence, pFence);
	return VK_SUCCESS;
}

VkResult Device::CreateFence(const VkFenceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkFence* pFence)
{
	return Fence::Create(pCreateInfo, pAllocator, pFence);
}

void Device::DestroyFence(VkFence fence, const VkAllocationCallbacks* pAllocator)
{
	Free(UnwrapVulkan<Fence>(fence), pAllocator);
}

VkResult Device::WaitForFences(uint32_t fenceCount, const VkFence* pFences, VkBool32 waitAll, uint64_t timeout)
{
	return Platform::WaitMultiple(ArrayToVector<void*, VkFence>(fenceCount, pFences, [](VkFence fence)
	{
		return UnwrapVulkan<Fence>(fence)->getHandle();
	}), waitAll, timeout);
}
