#include "Platform.h"

#include "Util.h"

#define NOMINMAX
#include <Windows.h>

static float performanceToNanoseconds;

void Platform::Initialise()
{
	static auto initialised = false;
	if (initialised)
	{
		return;
	}
	initialised = true;
	
	_CrtSetDbgFlag(_CRTDBG_CHECK_ALWAYS_DF);

	SYSTEM_INFO systemInfo{};
	GetSystemInfo(&systemInfo);

	if (systemInfo.dwPageSize != 4096)
	{
		FATAL_ERROR();
	}

	LARGE_INTEGER frequency;
	const auto result = QueryPerformanceFrequency(&frequency);
	assert(result != 0);

	performanceToNanoseconds = 1e9f / frequency.QuadPart;
}

bool Platform::SupportsSparse()
{
	return false;
}

uint64_t Platform::GetMemorySize()
{
	MEMORYSTATUSEX status{sizeof(MEMORYSTATUSEX)};
	const auto result = GlobalMemoryStatusEx(&status);
	assert(result != 0);
	return status.ullTotalPhys;
}

uint64_t Platform::GetTimestamp()
{
	LARGE_INTEGER timestamp;
	const auto result = QueryPerformanceCounter(&timestamp);
	assert(result != 0);
	return timestamp.QuadPart;
}

float Platform::GetTimestampPeriod()
{
	return performanceToNanoseconds;
}

#if defined(VK_EXT_calibrated_timestamps)
VkTimeDomainEXT Platform::GetTimeDomain()
{
	return VK_TIME_DOMAIN_QUERY_PERFORMANCE_COUNTER_EXT;
}
#endif

#undef CreateMutex
void* Platform::CreateMutex(bool initialState, bool manualReset)
{
	const auto event = CreateEventW(nullptr, manualReset, initialState, nullptr);
	if (event == nullptr)
	{
		FATAL_ERROR();
	}
	return event;
}

void* Platform::CreateSemaphoreExport(bool initialState, bool manualReset, const void* exportSemaphore)
{
	// TODO: dwAccess?
	const auto createInfo = reinterpret_cast<const VkExportSemaphoreWin32HandleInfoKHR*>(exportSemaphore);
	const auto event = CreateEventW(const_cast<LPSECURITY_ATTRIBUTES>(createInfo->pAttributes), manualReset, initialState, createInfo->name);
	if (event == nullptr)
	{
		FATAL_ERROR();
	}
	return event;
}

void Platform::CloseMutex(void* mutex)
{
	CloseHandle(mutex);
}

void Platform::SignalMutex(void* mutex)
{
	SetEvent(mutex);
}

void Platform::ResetMutex(void* mutex)
{
	ResetEvent(mutex);
}

bool Platform::Wait(void* mutex, uint64_t timeout)
{
	const auto milliseconds = static_cast<DWORD>(timeout == UINT64_MAX ? INFINITE : std::min(timeout / 1000000, static_cast<uint64_t>(INFINITE)));
	const auto result = WaitForSingleObject(mutex, milliseconds);
	if (result == WAIT_OBJECT_0)
	{
		return true;
	}
	if (result == WAIT_TIMEOUT)
	{
		return false;
	}

	FATAL_ERROR();
}

bool Platform::WaitMultiple(const std::vector<void*>& mutexes, bool waitAll, uint64_t timeout)
{
	const auto milliseconds = static_cast<DWORD>(timeout == UINT64_MAX ? INFINITE : std::min(timeout / 1000000, static_cast<uint64_t>(INFINITE)));
	const auto result = WaitForMultipleObjects(mutexes.size(), mutexes.data(), waitAll, milliseconds);
	if (result == WAIT_OBJECT_0)
	{
		return true;
	}
	if (result == WAIT_TIMEOUT)
	{
		return false;
	}

	FATAL_ERROR();
}

bool Platform::GetMutexStatus(void* mutex)
{
	const auto result = WaitForSingleObject(mutex, 0);
	if (result == WAIT_OBJECT_0)
	{
		return true;
	}
	if (result == WAIT_TIMEOUT)
	{
		return false;
	}
	
	FATAL_ERROR();
}

void* Platform::AlignedMalloc(size_t size, size_t alignment)
{
	return _aligned_malloc(size, alignment);
}

void Platform::AlignedFree(void* ptr)
{
	_aligned_free(ptr);
}