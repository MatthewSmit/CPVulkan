#pragma once
#include "Base.h"

class Fence final : public VulkanBase
{
public:
	~Fence() override;

	void Signal();
	VkResult Reset();
	VkResult Wait(uint64_t timeout);

	static VkResult Create(const VkFenceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkFence* pFence);

	[[nodiscard]] void* getHandle() const { return handle; }

private:
	void* handle{};
};