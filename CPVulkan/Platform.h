#pragma once
#include "Base.h"

namespace Platform
{
	void Initialise();

	void* CreateMutex(bool initialState);
	void CloseMutex(void* mutex);
	void SetMutex(void* mutex);
	void ReleaseMutex(void* mutex);
	VkResult Wait(void* mutex, uint64_t timeout);
	VkResult WaitMultiple(const std::vector<void*>& mutexes, bool waitAll, uint64_t timeout);
	VkResult GetMutexStatus(void* mutex);

	void* AlignedMalloc(size_t size, size_t alignment);
    void AlignedFree(void* ptr);
}
