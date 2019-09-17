#include "Event.h"

#include "Device.h"
#include "Platform.h"

#include <cassert>

Event::~Event()
{
	Platform::CloseMutex(handle);
}

void Event::Signal()
{
	Platform::SetMutex(handle);
}

VkResult Event::Reset()
{
	Platform::ReleaseMutex(handle);
	return VK_SUCCESS;
}

VkResult Event::Wait(uint64_t timeout)
{
	return Platform::Wait(handle, timeout);
}

VkResult Event::Create(const VkEventCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkEvent* pEvent)
{
	assert(pCreateInfo->sType == VK_STRUCTURE_TYPE_EVENT_CREATE_INFO);

	auto event = Allocate<Event>(pAllocator, VK_SYSTEM_ALLOCATION_SCOPE_DEVICE);

	auto next = pCreateInfo->pNext;
	while (next)
	{
		const auto type = static_cast<const VkBaseInStructure*>(next)->sType;
		switch (type)
		{
		}
		next = static_cast<const VkBaseInStructure*>(next)->pNext;
	}

	if (pCreateInfo->flags)
	{
		FATAL_ERROR();
	}

	event->handle = Platform::CreateMutex(false);

	WrapVulkan(event, pEvent);
	return VK_SUCCESS;
}

VkResult Event::getStatus() const
{
	return Platform::GetMutexStatus(handle);
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

VkResult Device::ResetEvent(VkEvent event)
{
	return UnwrapVulkan<Event>(event)->Reset();
}
