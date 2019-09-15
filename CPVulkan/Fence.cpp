#include "Fence.h"

#include <Windows.h>

#include <cassert>

Fence::~Fence()
{
	CloseHandle(handle);
}

void Fence::Signal()
{
	if (!ReleaseMutex(handle))
	{
		FATAL_ERROR();
	}
}

VkResult Fence::Create(const VkFenceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkFence* pFence)
{
	assert(pCreateInfo->sType == VK_STRUCTURE_TYPE_FENCE_CREATE_INFO);

	auto fence = Allocate<Fence>(pAllocator);

	auto next = pCreateInfo->pNext;
	while (next)
	{
		auto type = *static_cast<const VkStructureType*>(next);
		switch (type)
		{
		default:
			FATAL_ERROR();
		}
	}

	fence->handle = CreateMutexW(nullptr, !(pCreateInfo->flags & VK_FENCE_CREATE_SIGNALED_BIT), nullptr);
	if (fence->handle == nullptr)
	{
		FATAL_ERROR();
	}

	*pFence = reinterpret_cast<VkFence>(fence);
	return VK_SUCCESS;
}
