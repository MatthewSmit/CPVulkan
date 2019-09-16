#pragma once
#include "Base.h"

class Semaphore final : public VulkanBase
{
public:
	~Semaphore() override;

	void Signal();
	VkResult Reset();
	VkResult Wait(uint64_t timeout);
	
	static VkResult Create(const VkSemaphoreCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSemaphore* pSemaphore);

	[[nodiscard]] void* getHandle() const { return handle; }
	
private:
	void* handle{};
};