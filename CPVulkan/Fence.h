#pragma once
#include "Base.h"

class Fence final
{
public:
	Fence() = default;
	Fence(const Fence&) = delete;
	Fence(Fence&&) = delete;
	~Fence();

	Fence& operator=(const Fence&) = delete;
	Fence&& operator=(const Fence&&) = delete;

	void Signal();
	VkResult Reset();
	VkResult Wait(uint64_t timeout);

	static VkResult Create(const VkFenceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkFence* pFence);

	[[nodiscard]] void* getHandle() const { return handle; }

private:
	void* handle{};
};