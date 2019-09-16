#pragma once
#include "Base.h"

class Event final : public VulkanBase
{
public:
	~Event() override;
	
	void Signal();
	VkResult Reset();
	VkResult Wait(uint64_t timeout);

	static VkResult Create(const VkEventCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkEvent* pEvent);

	[[nodiscard]] VkResult getStatus() const;
	[[nodiscard]] void* getHandle() const { return handle; }

private:
	void* handle{};
};