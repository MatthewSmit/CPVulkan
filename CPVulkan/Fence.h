#pragma once
#include "Base.h"

class Fence final
{
public:
	Fence() = default;
	Fence(const Fence&) = delete;
	Fence(Fence&&) = delete;
	~Fence();

	Fence& operator=(const Fence&) = delete;
	Fence&& operator=(const Fence&&) = delete;

	void OnDelete(const VkAllocationCallbacks*)
	{
	}

	VkResult Signal();
	VkResult Reset();
	VkResult Wait(uint64_t timeout);

#if defined(VK_KHR_external_fence_win32)
	VkResult ImportFenceWin32(VkFenceImportFlags flags, VkExternalFenceHandleTypeFlagBits handleType, HANDLE handle, LPCWSTR name);
#endif

	static VkResult Create(const VkFenceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkFence* pFence);

#if defined(VK_KHR_external_fence_win32)
	[[nodiscard]] VkResult getFenceWin32(VkExternalFenceHandleTypeFlagBits handleType, HANDLE* pHandle) const;
#endif

	[[nodiscard]] VkResult getStatus() const;
	[[nodiscard]] void* getHandle() const { return handle; }

private:
	void* handle{};
	void* temporaryHandle{};
};