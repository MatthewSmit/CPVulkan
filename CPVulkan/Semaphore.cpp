#include "Semaphore.h"

#include "Device.h"
#include "Platform.h"

#if defined(VK_KHR_external_semaphore_win32)
#include <Windows.h>
#undef CreateMutex
#endif

#include <cassert>

Semaphore::~Semaphore()
{
	Platform::CloseMutex(handle);
}

VkResult Semaphore::Signal()
{
	if (temporaryHandle)
	{
		Platform::SignalMutex(temporaryHandle);
	}
	else
	{
		Platform::SignalMutex(handle);
	}
	return VK_SUCCESS;
}

VkResult Semaphore::Reset()
{
	if (temporaryHandle)
	{
		Platform::ResetMutex(temporaryHandle);
	}
	else
	{
		Platform::ResetMutex(handle);
	}
	return VK_SUCCESS;
}

VkResult Semaphore::Wait(uint64_t timeout)
{
	if (temporaryHandle)
	{
		const auto result = Platform::Wait(temporaryHandle, timeout);
		temporaryHandle = nullptr;
		return result ? VK_SUCCESS : VK_TIMEOUT;
	}

	return Platform::Wait(handle, timeout) ? VK_SUCCESS : VK_TIMEOUT;
}

VkResult Semaphore::Create(const VkSemaphoreCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSemaphore* pSemaphore)
{
	assert(pCreateInfo->sType == VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO);
	assert(pCreateInfo->flags == 0);

	const auto semaphore = Allocate<Semaphore>(pAllocator, VK_SYSTEM_ALLOCATION_SCOPE_DEVICE);
	if (!semaphore)
	{
		return VK_ERROR_OUT_OF_HOST_MEMORY;
	}

	const void* exportSemaphore = nullptr;

	auto next = static_cast<const VkBaseInStructure*>(pCreateInfo->pNext);
	while (next)
	{
		const auto type = next->sType;
		switch (type)
		{
#if defined(VK_KHR_external_semaphore)
		case VK_STRUCTURE_TYPE_EXPORT_SEMAPHORE_CREATE_INFO:
			{
				const auto createInfo = reinterpret_cast<const VkExportSemaphoreCreateInfo*>(next);
#if defined(VK_KHR_external_semaphore_win32)
				if (createInfo->handleTypes == VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT)
				{
					break;
				}
#endif
				TODO_ERROR();
			}
#endif

#if defined(VK_KHR_external_semaphore_win32)
		case VK_STRUCTURE_TYPE_EXPORT_SEMAPHORE_WIN32_HANDLE_INFO_KHR:
			exportSemaphore = next;
			break;
#endif

		default:
			break;
		}
		next = next->pNext;
	}

	if (exportSemaphore)
	{
		semaphore->handle = Platform::CreateSemaphoreExport(false, false, exportSemaphore);
	}
	else
	{
		semaphore->handle = Platform::CreateMutex(false, false);
	}

	WrapVulkan(semaphore, pSemaphore);
	return VK_SUCCESS;
}

#if defined(VK_KHR_external_semaphore_win32)
VkResult Semaphore::ImportSemaphoreWin32(VkSemaphoreImportFlags flags, VkExternalSemaphoreHandleTypeFlagBits handleType, HANDLE handle, LPCWSTR)
{
	if (handleType == VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT)
	{
		if (flags & VK_SEMAPHORE_IMPORT_TEMPORARY_BIT)
		{
			temporaryHandle = handle;
		}
		else
		{
			Platform::CloseMutex(this->handle);
			const auto result = DuplicateHandle(GetCurrentProcess(), handle, GetCurrentProcess(), &this->handle, 0, false, DUPLICATE_SAME_ACCESS);
			if (!result)
			{
				TODO_ERROR();
			}
		}

		return VK_SUCCESS;
	}
	
	TODO_ERROR();
}

VkResult Semaphore::getSemaphoreWin32(VkExternalSemaphoreHandleTypeFlagBits handleType, HANDLE* pHandle) const
{
	if (handleType == VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT)
	{
		const auto result = DuplicateHandle(GetCurrentProcess(), handle, GetCurrentProcess(), pHandle, 0, false, DUPLICATE_SAME_ACCESS);
		if (!result)
		{
			TODO_ERROR();
		}
		return VK_SUCCESS;
	}

	TODO_ERROR();
}
#endif

#undef CreateSemaphore

VkResult Device::CreateSemaphore(const VkSemaphoreCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSemaphore* pSemaphore)
{
	return Semaphore::Create(pCreateInfo, pAllocator, pSemaphore);
}

void Device::DestroySemaphore(VkSemaphore semaphore, const VkAllocationCallbacks* pAllocator)
{
	if (semaphore)
	{
		Free(UnwrapVulkan<Semaphore>(semaphore), pAllocator);
	}
}

#if defined(VK_KHR_external_semaphore_fd)
VkResult Device::ImportSemaphoreFd(const VkImportSemaphoreFdInfoKHR* pImportSemaphoreFdInfo)
{
	TODO_ERROR();
}

VkResult Device::GetSemaphoreFd(const VkSemaphoreGetFdInfoKHR* pGetFdInfo, int* pFd)
{
	TODO_ERROR();
}
#endif

#if defined(VK_KHR_external_semaphore_win32)
VkResult Device::ImportSemaphoreWin32Handle(const VkImportSemaphoreWin32HandleInfoKHR* pImportSemaphoreWin32HandleInfo)
{
	assert(pImportSemaphoreWin32HandleInfo->sType == VK_STRUCTURE_TYPE_IMPORT_SEMAPHORE_WIN32_HANDLE_INFO_KHR);
	return UnwrapVulkan<Semaphore>(pImportSemaphoreWin32HandleInfo->semaphore)->ImportSemaphoreWin32(pImportSemaphoreWin32HandleInfo->flags, 
	                                                                                                 pImportSemaphoreWin32HandleInfo->handleType, 
	                                                                                                 pImportSemaphoreWin32HandleInfo->handle, 
	                                                                                                 pImportSemaphoreWin32HandleInfo->name);
}

VkResult Device::GetSemaphoreWin32Handle(const VkSemaphoreGetWin32HandleInfoKHR* pGetWin32HandleInfo, HANDLE* pHandle)
{
	assert(pGetWin32HandleInfo->sType == VK_STRUCTURE_TYPE_SEMAPHORE_GET_WIN32_HANDLE_INFO_KHR);
	return UnwrapVulkan<Semaphore>(pGetWin32HandleInfo->semaphore)->getSemaphoreWin32(pGetWin32HandleInfo->handleType, pHandle);
}
#endif