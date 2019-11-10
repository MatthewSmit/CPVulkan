#pragma once
#include "Base.h"

#include <vector>

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

	void OnDelete(const VkAllocationCallbacks* pAllocator);

	VkResult GetImages(uint32_t* pSwapchainImageCount, VkImage* vkImage) const;

	VkResult AcquireNextImage(uint64_t timeout, VkSemaphore semaphore, VkFence fence, uint32_t* pImageIndex);
	
	VkResult Present(uint32_t pImageIndex);

	static VkResult Create(const VkSwapchainCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSwapchainKHR* pSwapchain);

	[[nodiscard]] Image* getImage(uint32_t index) const noexcept { return gsl::at(images, index); }

private:
	DeviceMemory* data{};
	VkSurfaceKHR surface{};
	VkColorSpaceKHR imageColorSpace{};
	std::vector<Image*> images{};
};
