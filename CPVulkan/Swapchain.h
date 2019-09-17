#pragma once
#include "Base.h"

struct DeviceMemory;

class Image;

class Swapchain final
{
public:
	Swapchain() = default;
	Swapchain(const Swapchain&) = delete;
	Swapchain(Swapchain&&) = delete;
	~Swapchain();

	Swapchain& operator=(const Swapchain&) = delete;
	Swapchain&& operator=(const Swapchain&&) = delete;

	VkResult GetImages(uint32_t* pSwapchainImageCount, VkImage* vkImage) const;

	VkResult AcquireNextImage(uint64_t timeout, VkSemaphore semaphore, VkFence fence, uint32_t* pImageIndex);

	void DeleteImages(const VkAllocationCallbacks* pAllocator);
	
	VkResult Present(uint32_t pImageIndex);

	static VkResult Create(const VkSwapchainCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSwapchainKHR* pSwapchain);

private:
	DeviceMemory* data{};
	VkSurfaceKHR surface{};
	VkColorSpaceKHR imageColorSpace{};
	std::vector<Image*> images{};
};
