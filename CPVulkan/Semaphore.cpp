#include "Semaphore.h"

#include <Windows.h>

#include <cassert>

Semaphore::~Semaphore()
{
	CloseHandle(handle);
}

VkResult Semaphore::Wait()
{
	auto result = WaitForSingleObject(handle, INFINITE);
	if (result == WAIT_FAILED)
	{
		FATAL_ERROR();
	}

	if (result == WAIT_TIMEOUT)
	{
		return VK_TIMEOUT;
	}

	return VK_SUCCESS;
}

VkResult Semaphore::Create(const VkSemaphoreCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSemaphore* pSemaphore)
{
	assert(pCreateInfo->sType == VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO);
	assert(pCreateInfo->flags == 0);

	auto semaphore = Allocate<Semaphore>(pAllocator);

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

	semaphore->handle = CreateMutexW(nullptr, false, nullptr);
	if (semaphore->handle == nullptr)
	{
		FATAL_ERROR();
	}

	*pSemaphore = reinterpret_cast<VkSemaphore>(semaphore);
	return VK_SUCCESS;
}
