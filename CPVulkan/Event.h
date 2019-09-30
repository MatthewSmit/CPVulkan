#pragma once
#include "Base.h"

class Event final
{
public:
	Event() = default;
	Event(const Event&) = delete;
	Event(Event&&) = delete;
	~Event();

	Event& operator=(const Event&) = delete;
	Event&& operator=(const Event&&) = delete;

	void OnDelete(const VkAllocationCallbacks*)
	{
	}

	VkResult Signal();
	VkResult Reset();
	VkResult Wait(uint64_t timeout);

	static VkResult Create(const VkEventCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkEvent* pEvent);

	[[nodiscard]] VkResult getStatus() const;
	[[nodiscard]] void* getHandle() const { return handle; }

private:
	void* handle{};
};