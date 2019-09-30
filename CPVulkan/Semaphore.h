#pragma once
#include "Base.h"

class Semaphore final
{
public:
	Semaphore() = default;
	Semaphore(const Semaphore&) = delete;
	Semaphore(Semaphore&&) = delete;
	~Semaphore();

	Semaphore& operator=(const Semaphore&) = delete;
	Semaphore&& operator=(const Semaphore&&) = delete;

	VkResult Signal();
	VkResult Reset();
	VkResult Wait(uint64_t timeout);

#if defined(VK_KHR_external_semaphore_win32)
	VkResult ImportSemaphoreWin32(VkSemaphoreImportFlags flags, VkExternalSemaphoreHandleTypeFlagBits handleType, HANDLE handle, LPCWSTR name);
#endif

	static VkResult Create(const VkSemaphoreCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSemaphore* pSemaphore);

#if defined(VK_KHR_external_semaphore_win32)
	[[nodiscard]] VkResult getSemaphoreWin32(VkExternalSemaphoreHandleTypeFlagBits handleType, HANDLE* pHandle) const;
#endif
	
private:
	void* handle{};
	void* temporaryHandle{};
};