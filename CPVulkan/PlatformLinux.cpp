#include "Base.h"

#include <unistd.h>

void Platform::Initialise()
{
    if (getpagesize() != 4096)
    {
        FATAL_ERROR();
    }
}

void* Platform::CreateMutex(bool initialState)
{
    auto mutex = new pthread_mutex_t();
    pthread_mutex_init(mutex, nullptr);

    if (initialState)
    {
        SetMutex(mutex);
    }

    return mutex;
}

void Platform::CloseMutex(void* mutex)
{
    pthread_mutex_destroy((pthread_mutex_t*)mutex);
}

void Platform::SetMutex(void* mutex)
{
    pthread_mutex_lock((pthread_mutex_t*)mutex);
}

void Platform::ReleaseMutex(void* mutex)
{
    pthread_mutex_unlock((pthread_mutex_t*)mutex);
}

VkResult Platform::Wait(void* mutex, uint64_t timeout)
{
    int result;
    if (timeout == std::numeric_limits<uint64_t>::max())
    {
        result = pthread_mutex_lock((pthread_mutex_t*)mutex);
    }
    else
    {
        timespec timeoutTime{};
        clock_gettime(CLOCK_REALTIME, &timeoutTime);
        timeoutTime.tv_nsec += timeout;

        result = pthread_mutex_timedlock((pthread_mutex_t*)mutex, &timeoutTime);
    }

    if (result == ETIMEDOUT)
    {
        return VK_TIMEOUT;
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
    FATAL_ERROR();
}

void* Platform::AlignedMalloc(size_t size, size_t alignment)
{
    return std::aligned_alloc(alignment, size);
}

void Platform::AlignedFree(void* ptr)
{
    free(ptr);
}