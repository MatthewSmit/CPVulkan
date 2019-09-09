#pragma once
#include "Base.h"

class Semaphore final : public VulkanBase
{
public:
	~Semaphore() override;

	VkResult Wait();
	
	static VkResult Create(const VkSemaphoreCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSemaphore* pSemaphore);

private:
	HANDLE handle{};
};