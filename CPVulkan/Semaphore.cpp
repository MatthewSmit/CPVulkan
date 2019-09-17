#include "Semaphore.h"

#include "Device.h"
#include "Platform.h"

#include <cassert>

Semaphore::~Semaphore()
{
	Platform::CloseMutex(handle);
}

void Semaphore::Signal()
{
	Platform::ReleaseMutex(handle);
}

VkResult Semaphore::Reset()
{
	Platform::SetMutex(handle);
	return VK_SUCCESS;
}

VkResult Semaphore::Wait(uint64_t timeout)
{
	return Platform::Wait(handle, timeout);
}

VkResult Semaphore::Create(const VkSemaphoreCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSemaphore* pSemaphore)
{
	assert(pCreateInfo->sType == VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO);
	assert(pCreateInfo->flags == 0);

	auto semaphore = Allocate<Semaphore>(pAllocator, VK_SYSTEM_ALLOCATION_SCOPE_DEVICE);

	auto next = pCreateInfo->pNext;
	while (next)
	{
		const auto type = static_cast<const VkBaseInStructure*>(next)->sType;
		switch (type)
		{
		case VK_STRUCTURE_TYPE_EXPORT_SEMAPHORE_CREATE_INFO:
			FATAL_ERROR();

		case VK_STRUCTURE_TYPE_EXPORT_SEMAPHORE_WIN32_HANDLE_INFO_KHR:
			FATAL_ERROR();

		default:
			next = static_cast<const VkBaseInStructure*>(next)->pNext;
			break;
		}
	}

	semaphore->handle = Platform::CreateMutex(false);

	WrapVulkan(semaphore, pSemaphore);
	return VK_SUCCESS;
}

#undef CreateSemaphore

VkResult Device::CreateSemaphore(const VkSemaphoreCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSemaphore* pSemaphore)
{
	return Semaphore::Create(pCreateInfo, pAllocator, pSemaphore);
}

void Device::DestroySemaphore(VkSemaphore semaphore, const VkAllocationCallbacks* pAllocator)
{
	Free(UnwrapVulkan<Semaphore>(semaphore), pAllocator);
}