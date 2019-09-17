#pragma once
#include "Base.h"

class Semaphore final
{
public:
	Semaphore() = default;
	Semaphore(const Semaphore&) = delete;
	Semaphore(Semaphore&&) = delete;
	~Semaphore();

	Semaphore& operator=(const Semaphore&) = delete;
	Semaphore&& operator=(const Semaphore&&) = delete;

	void Signal();
	VkResult Reset();
	VkResult Wait(uint64_t timeout);
	
	static VkResult Create(const VkSemaphoreCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSemaphore* pSemaphore);

	[[nodiscard]] void* getHandle() const { return handle; }
	
private:
	void* handle{};
};