#pragma once
#include "Base.h"

class Image;

class Swapchain final : public VulkanBase
{
public:
	~Swapchain() override;

	VkResult GetImages(uint32_t* pSwapchainImageCount, VkImage* vkImage);

	VkResult AcquireNextImage(uint64_t timeout, VkSemaphore semaphore, VkFence fence, uint32_t* pImageIndex);

	void DeleteImages(const VkAllocationCallbacks* pAllocator);
	
	VkResult Present(uint32_t pImageIndex);

	static VkResult Create(const VkSwapchainCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSwapchainKHR* pSwapchain);

private:
	DeviceMemory* data{};
	VkSurfaceKHR surface{};
	VkColorSpaceKHR imageColorSpace{};
	std::vector<Image*> images{};
	BITMAPINFO bmi{};
};
