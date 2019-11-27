#pragma once
#include "Base.h"

#include <vector>

namespace Platform
{
	void Initialise();

	bool SupportsSparse();

	uint64_t GetMemorySize();
	uint64_t GetTimestamp();
	float GetTimestampPeriod();
#if defined(VK_EXT_calibrated_timestamps)
	VkTimeDomainEXT GetTimeDomain();
#endif

	void* CreateMutex(bool initialState, bool manualReset);
	void* CreateSemaphoreExport(bool initialState, bool manualReset, const void* exportSemaphore);
	void CloseMutex(void* mutex);
	void SignalMutex(void* mutex);
	void ResetMutex(void* mutex);
	bool Wait(void* mutex, uint64_t timeout);
	bool WaitMultiple(const std::vector<void*>& mutexes, bool waitAll, uint64_t timeout);
	bool GetMutexStatus(void* mutex);

	void* AlignedMalloc(size_t size, size_t alignment);
	void AlignedFree(void* ptr);
}
