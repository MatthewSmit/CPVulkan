#pragma once
#include "Base.h"

class Fence final : public VulkanBase
{
public:
	~Fence() override;
	
	void Signal();

	static VkResult Create(const VkFenceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkFence* pFence);

	HANDLE getHandle() const { return handle; }

private:
	HANDLE handle{};
};