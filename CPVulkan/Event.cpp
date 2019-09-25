#include "Event.h"

#include "Device.h"
#include "Platform.h"

#include <cassert>

Event::~Event()
{
	Platform::CloseMutex(handle);
}

VkResult Event::Signal()
{
	Platform::SignalMutex(handle);
	return VK_SUCCESS;
}

VkResult Event::Reset()
{
	Platform::ResetMutex(handle);
	return VK_SUCCESS;
}

VkResult Event::Wait(uint64_t timeout)
{
	return Platform::Wait(handle, timeout) ? VK_SUCCESS : VK_TIMEOUT;
}

VkResult Event::Create(const VkEventCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkEvent* pEvent)
{
	assert(pCreateInfo->sType == VK_STRUCTURE_TYPE_EVENT_CREATE_INFO);
	assert(pCreateInfo->flags == 0);

	const auto event = Allocate<Event>(pAllocator, VK_SYSTEM_ALLOCATION_SCOPE_DEVICE);
	if (!event)
	{
		return VK_ERROR_OUT_OF_HOST_MEMORY;
	}

	event->handle = Platform::CreateMutex(false, true);

	WrapVulkan(event, pEvent);
	return VK_SUCCESS;
}

VkResult Event::getStatus() const
{
	return Platform::GetMutexStatus(handle) ? VK_EVENT_SET : VK_EVENT_RESET;
}

#undef CreateEvent

VkResult Device::CreateEvent(const VkEventCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkEvent* pEvent)
{
	return Event::Create(pCreateInfo, pAllocator, pEvent);
}

void Device::DestroyEvent(VkEvent event, const VkAllocationCallbacks* pAllocator)
{
	Free(UnwrapVulkan<Event>(event), pAllocator);
}

VkResult Device::GetEventStatus(VkEvent event)
{
	return UnwrapVulkan<Event>(event)->getStatus();
}

VkResult Device::SetEvent(VkEvent event)
{
	return UnwrapVulkan<Event>(event)->Signal();
}

VkResult Device::ResetEvent(VkEvent event)
{
	return UnwrapVulkan<Event>(event)->Reset();
}
