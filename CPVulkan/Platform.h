#pragma once
#include "Base.h"

namespace Platform
{
	void Initialise();

	bool SupportsSparse();

	void* CreateMutex(bool initialState, bool manualReset);
	void CloseMutex(void* mutex) noexcept;
	void SignalMutex(void* mutex);
	void ResetMutex(void* mutex);
	bool Wait(void* mutex, uint64_t timeout);
	bool WaitMultiple(const std::vector<void*>& mutexes, bool waitAll, uint64_t timeout);
	bool GetMutexStatus(void* mutex);

	void* AlignedMalloc(size_t size, size_t alignment);
	void AlignedFree(void* ptr) noexcept;
}
