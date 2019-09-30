#include "Swapchain.h"

#include "Device.h"
#include "Fence.h"
#include "Formats.h"
#include "Image.h"
#include "Semaphore.h"
#include "Util.h"

#include <cassert>

Swapchain::~Swapchain()
{
	assert(data == nullptr);
	assert(images.empty());
}

void Swapchain::OnDelete(const VkAllocationCallbacks* pAllocator)
{
	for (auto image : images)
	{
		Free(image, pAllocator);
	}
	images.clear();
	FreeSized(data, pAllocator);
	data = nullptr;
}

VkResult Swapchain::GetImages(uint32_t* pSwapchainImageCount, VkImage* vkImage) const
{
	return HandleEnumeration<VkImage, Image*>(pSwapchainImageCount, vkImage, images, [](Image* image)
	{
		VkImage result;
		WrapVulkan(image, &result);
		return result;
	});
}

VkResult Swapchain::AcquireNextImage(uint64_t timeout, VkSemaphore semaphore, VkFence fence, uint32_t* pImageIndex)
{
	*pImageIndex = 0;
	if (semaphore)
	{
		UnwrapVulkan<Semaphore>(semaphore)->Signal();
	}
	if (fence)
	{
		UnwrapVulkan<Fence>(fence)->Signal();
	}
	return VK_SUCCESS;
}

VkResult Swapchain::Create(const VkSwapchainCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSwapchainKHR* pSwapchain)
{
	assert(pCreateInfo->sType == VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR);

	auto swapchain = Allocate<Swapchain>(pAllocator, VK_SYSTEM_ALLOCATION_SCOPE_DEVICE);
	if (!swapchain)
	{
		return VK_ERROR_OUT_OF_HOST_MEMORY;
	}

	auto next = pCreateInfo->pNext;
	while (next)
	{
		const auto type = static_cast<const VkBaseInStructure*>(next)->sType;
		switch (type)
		{
		case VK_STRUCTURE_TYPE_DEVICE_GROUP_SWAPCHAIN_CREATE_INFO_KHR:
			FATAL_ERROR();

		case VK_STRUCTURE_TYPE_IMAGE_FORMAT_LIST_CREATE_INFO_KHR:
			FATAL_ERROR();

		case VK_STRUCTURE_TYPE_SURFACE_FULL_SCREEN_EXCLUSIVE_INFO_EXT:
			FATAL_ERROR();

		case VK_STRUCTURE_TYPE_SURFACE_FULL_SCREEN_EXCLUSIVE_WIN32_INFO_EXT:
			FATAL_ERROR();

		case VK_STRUCTURE_TYPE_SWAPCHAIN_COUNTER_CREATE_INFO_EXT:
			FATAL_ERROR();
			
		case VK_STRUCTURE_TYPE_SWAPCHAIN_DISPLAY_NATIVE_HDR_CREATE_INFO_AMD:
			FATAL_ERROR();
		}
		next = static_cast<const VkBaseInStructure*>(next)->pNext;
	}

	if (pCreateInfo->flags)
	{
		FATAL_ERROR();
	}

	swapchain->surface = pCreateInfo->surface;
	
	VkImageSwapchainCreateInfoKHR imageSwapchain
	{
		VK_STRUCTURE_TYPE_IMAGE_SWAPCHAIN_CREATE_INFO_KHR,
		nullptr,
		VK_NULL_HANDLE,
	};
	WrapVulkan(swapchain, &imageSwapchain.swapchain);
	VkImageCreateInfo imageCreate
	{
		VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		&imageSwapchain,
		0,
		VK_IMAGE_TYPE_2D,
		pCreateInfo->imageFormat,
		{
			pCreateInfo->imageExtent.width,
			pCreateInfo->imageExtent.height,
			1
		},
		1,
		pCreateInfo->imageArrayLayers,
		VK_SAMPLE_COUNT_1_BIT,
		VK_IMAGE_TILING_OPTIMAL,
		pCreateInfo->imageUsage,
		pCreateInfo->imageSharingMode,
		pCreateInfo->queueFamilyIndexCount,
		pCreateInfo->pQueueFamilyIndices,
		VK_IMAGE_LAYOUT_UNDEFINED,
	};

	swapchain->imageColorSpace = pCreateInfo->imageColorSpace;

	const auto dataStride = pCreateInfo->imageExtent.width * pCreateInfo->imageExtent.height * pCreateInfo->imageArrayLayers * GetFormatInformation(pCreateInfo->imageFormat).TotalSize;
	const auto dataSize = dataStride * pCreateInfo->minImageCount;
	swapchain->data = AllocateSized(pAllocator, dataSize, VK_SYSTEM_ALLOCATION_SCOPE_DEVICE);

	for (auto i = 0u; i < pCreateInfo->minImageCount; i++)
	{	
		VkImage image;
		const auto result = Image::Create(&imageCreate, pAllocator, &image);
		if (result != VK_SUCCESS)
		{
			Free(swapchain, pAllocator);
			return result;
		}

		VkDeviceMemory deviceMemory;
		WrapVulkan(swapchain->data, &deviceMemory);
		UnwrapVulkan<Image>(image)->BindMemory(deviceMemory, dataStride * i);
		swapchain->images.push_back(UnwrapVulkan<Image>(image));
	}
	
	if (pCreateInfo->oldSwapchain)
	{
		FATAL_ERROR();
	}

	WrapVulkan(swapchain, pSwapchain);
	return VK_SUCCESS;
}

VkResult Device::CreateSwapchain(const VkSwapchainCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSwapchainKHR* pSwapchain)
{
	return Swapchain::Create(pCreateInfo, pAllocator, pSwapchain);
}

void Device::DestroySwapchain(VkSwapchainKHR swapchain, const VkAllocationCallbacks* pAllocator)
{
	if (swapchain)
	{
		Free(UnwrapVulkan<Swapchain>(swapchain), pAllocator);
	}
}

VkResult Device::GetSwapchainImages(VkSwapchainKHR swapchain, uint32_t* pSwapchainImageCount, VkImage* pSwapchainImages)
{
	return UnwrapVulkan<Swapchain>(swapchain)->GetImages(pSwapchainImageCount, pSwapchainImages);
}

VkResult Device::AcquireNextImage(VkSwapchainKHR swapchain, uint64_t timeout, VkSemaphore semaphore, VkFence fence, uint32_t* pImageIndex)
{
	return UnwrapVulkan<Swapchain>(swapchain)->AcquireNextImage(timeout, semaphore, fence, pImageIndex);
}
