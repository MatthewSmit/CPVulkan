#include "Base.h"

#define NOMINMAX
#include <Windows.h>

struct MutexWrapper
{
	explicit MutexWrapper(HANDLE event):
		event{event}
	{
	}

	HANDLE event{};
	uint32_t lock{};
};

void Platform::Initialise()
{
	SYSTEM_INFO systemInfo{};
	GetSystemInfo(&systemInfo);

	if (systemInfo.dwPageSize != 4096)
	{
		FATAL_ERROR();
	}
}

#undef CreateMutex
void* Platform::CreateMutex(bool initialState)
{
	const auto event = CreateEventW(nullptr, true, initialState, nullptr);
	if (event == nullptr)
	{
		FATAL_ERROR();
	}
	return new MutexWrapper(event);
}

void Platform::CloseMutex(void* mutex)
{
	const auto mutexWrapper = static_cast<MutexWrapper*>(mutex);
	CloseHandle(mutexWrapper->event);
	delete mutexWrapper;
}

void Platform::SetMutex(void* mutex)
{
	const auto mutexWrapper = static_cast<MutexWrapper*>(mutex);
	// if (InterlockedExchange(&mutexWrapper->lock, 1) != 0)
	// {
	// 	while (InterlockedExchange(&mutexWrapper->lock, -1) != 0)
	// 	{
	// 		if (WAIT_OBJECT_0 != WaitForSingleObject(mutexWrapper->event, INFINITE))
	// 		{
	// 			FATAL_ERROR();
	// 		}
	// 	}
	// }
	 
	if (InterlockedCompareExchange(&mutexWrapper->lock, 1, 0) != 0)
	{
		// Has Lock
	}
}

void Platform::ReleaseMutex(void* mutex)
{
	const auto mutexWrapper = static_cast<MutexWrapper*>(mutex);
	const auto lock = static_cast<int32_t>(InterlockedExchange(&mutexWrapper->lock, 0));
	if (lock != 0)
	{
		if (lock < 0)
		{
			SetEvent(mutexWrapper->event);
		}
	}
}

VkResult Platform::Wait(void* mutex, uint64_t timeout)
{
	const auto mutexWrapper = static_cast<MutexWrapper*>(mutex);
	const auto milliseconds = static_cast<DWORD>(timeout == UINT64_MAX ? INFINITE : std::min(timeout / 1000000, static_cast<uint64_t>(INFINITE)));
	if (InterlockedExchange(&mutexWrapper->lock, 1) != 0)
	{
		while (InterlockedExchange(&mutexWrapper->lock, -1) != 0)
		{
			const auto result = WaitForSingleObject(mutexWrapper->event, milliseconds);
			if (result == WAIT_OBJECT_0)
			{
				return VK_SUCCESS;
			}
			if (result == WAIT_TIMEOUT)
			{
				return VK_TIMEOUT;
			}

			FATAL_ERROR();
		}
	}
	
	return VK_SUCCESS;
}

VkResult Platform::WaitMultiple(const std::vector<void*>& mutexes, bool waitAll, uint64_t timeout)
{
	if (mutexes.size() == 1)
	{
		return Wait(mutexes[0], timeout);
	}
	
	FATAL_ERROR();
}

VkResult Platform::GetMutexStatus(void* mutex)
{
	const auto mutexWrapper = static_cast<MutexWrapper*>(mutex);
	if (mutexWrapper->lock != 0)
	{
		return VK_EVENT_SET;
	}

	return VK_EVENT_RESET;
}

void* Platform::AlignedMalloc(size_t size, size_t alignment)
{
    return _aligned_malloc(size, alignment);
}

void Platform::AlignedFree(void* ptr)
{
    _aligned_free(ptr);
}