#include "Fence.h"

#include "Device.h"
#include "Platform.h"
#include "Util.h"

#if defined(VK_KHR_external_semaphore_win32)
#include <Windows.h>
#undef CreateMutex
#endif

#include <cassert>

Fence::~Fence()
{
	Platform::CloseMutex(handle);
}

VkResult Fence::Signal()
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

VkResult Fence::Reset()
{
	if (temporaryHandle)
	{
		Platform::ResetMutex(temporaryHandle);
		temporaryHandle = nullptr;
	}
	else
	{
		Platform::ResetMutex(handle);
	}
	return VK_SUCCESS;
}

VkResult Fence::Wait(uint64_t timeout)
{
	if (temporaryHandle)
	{
		const auto result = Platform::Wait(temporaryHandle, timeout);
		temporaryHandle = nullptr;
		return result ? VK_SUCCESS : VK_TIMEOUT;
	}

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

	const void* exportFence = nullptr;

	auto next = static_cast<const VkBaseInStructure*>(pCreateInfo->pNext);
	while (next)
	{
		const auto type = next->sType;
		switch (type)
		{
#if defined(VK_KHR_external_fence)
		case VK_STRUCTURE_TYPE_EXPORT_FENCE_CREATE_INFO:
			{
				const auto createInfo = reinterpret_cast<const VkExportFenceCreateInfo*>(next);
#if defined(VK_KHR_external_fence_win32)
				if (createInfo->handleTypes == VK_EXTERNAL_FENCE_HANDLE_TYPE_OPAQUE_WIN32_BIT)
				{
					break;
				}
#endif
				TODO_ERROR();
			}
#endif

#if defined(VK_KHR_external_fence_win32)
		case VK_STRUCTURE_TYPE_EXPORT_FENCE_WIN32_HANDLE_INFO_KHR:
			exportFence = next;
			break;
#endif

		default:
			break;
		}
		next = next->pNext;
	}

	if (exportFence)
	{
		fence->handle = Platform::CreateFenceExport(pCreateInfo->flags & VK_FENCE_CREATE_SIGNALED_BIT, true, exportFence);
	}
	else
	{
		fence->handle = Platform::CreateMutex(pCreateInfo->flags & VK_FENCE_CREATE_SIGNALED_BIT, true);
	}


	WrapVulkan(fence, pFence);
	return VK_SUCCESS;
}

#if defined(VK_KHR_external_fence_win32)
VkResult Fence::ImportFenceWin32(VkFenceImportFlags flags, VkExternalFenceHandleTypeFlagBits handleType, HANDLE handle, LPCWSTR)
{
	if (handleType == VK_EXTERNAL_FENCE_HANDLE_TYPE_OPAQUE_WIN32_BIT)
	{
		if (flags & VK_FENCE_IMPORT_TEMPORARY_BIT)
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

VkResult Fence::getFenceWin32(VkExternalFenceHandleTypeFlagBits handleType, HANDLE* pHandle) const
{
	if (handleType == VK_EXTERNAL_FENCE_HANDLE_TYPE_OPAQUE_WIN32_BIT)
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
	if (fence)
	{
		Free(UnwrapVulkan<Fence>(fence), pAllocator);
	}
}

VkResult Device::ResetFences(uint32_t fenceCount, const VkFence* pFences)
{
	for (auto i = 0u; i < fenceCount; i++)
	{
		if (UnwrapVulkan<Fence>(pFences[i])->Reset() != VK_SUCCESS)
		{
			TODO_ERROR();
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

VkResult Device::ImportFenceFd(const VkImportFenceFdInfoKHR* pImportFenceFdInfo)
{
	TODO_ERROR();
}

VkResult Device::GetFenceFd(const VkFenceGetFdInfoKHR* pGetFdInfo, int* pFd)
{
	TODO_ERROR();
}

#if defined(VK_KHR_external_fence_win32)
VkResult Device::ImportFenceWin32Handle(const VkImportFenceWin32HandleInfoKHR* pImportFenceWin32HandleInfo)
{
	assert(pImportFenceWin32HandleInfo->sType == VK_STRUCTURE_TYPE_IMPORT_FENCE_WIN32_HANDLE_INFO_KHR);
	return UnwrapVulkan<Fence>(pImportFenceWin32HandleInfo->fence)->ImportFenceWin32(pImportFenceWin32HandleInfo->flags,
	                                                                                 pImportFenceWin32HandleInfo->handleType, 
	                                                                                 pImportFenceWin32HandleInfo->handle, 
	                                                                                 pImportFenceWin32HandleInfo->name);
}

VkResult Device::GetFenceWin32Handle(const VkFenceGetWin32HandleInfoKHR* pGetWin32HandleInfo, HANDLE* pHandle)
{
	assert(pGetWin32HandleInfo->sType == VK_STRUCTURE_TYPE_FENCE_GET_WIN32_HANDLE_INFO_KHR);
	return UnwrapVulkan<Fence>(pGetWin32HandleInfo->fence)->getFenceWin32(pGetWin32HandleInfo->handleType, pHandle);
}
#endif