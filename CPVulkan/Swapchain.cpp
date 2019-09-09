#include "Swapchain.h"

#include "Formats.h"
#include "Image.h"

#include <vulkan/vk_icd.h>

#include <cassert>

Swapchain::~Swapchain()
{
	assert(data == nullptr);
	assert(images.empty());
}

VkResult Swapchain::GetImages(uint32_t* pSwapchainImageCount, VkImage* vkImage)
{
	return HandleEnumeration<VkImage, Image*>(pSwapchainImageCount, vkImage, images, [](Image* image)
	{
		return reinterpret_cast<VkImage>(image);
	});
}

VkResult Swapchain::AcquireNextImage(uint64_t timeout, VkSemaphore semaphore, VkFence fence, uint32_t* pImageIndex)
{
	// TODO:
	*pImageIndex = 0;
	return VK_SUCCESS;
}

void Swapchain::DeleteImages(const VkAllocationCallbacks* pAllocator)
{
	for (auto image : images)
	{
		Free(image, pAllocator);
	}
	images.clear();
	FreeSized(data, pAllocator);
	data = nullptr;
}

VkResult Swapchain::Present(uint32_t pImageIndex)
{
	const auto image = images[pImageIndex];

	const auto dc = GetDC(reinterpret_cast<VkIcdSurfaceWin32*>(surface)->hwnd);
	if (dc == nullptr)
	{
		FATAL_ERROR();
	}

	RECT rect;
	GetClientRect(reinterpret_cast<VkIcdSurfaceWin32*>(surface)->hwnd, &rect);

	const auto width = rect.right - rect.left;
	const auto height = rect.bottom - rect.top;

	if (StretchDIBits(dc,
	                  0, 0, width, height,
	                  0, 0, image->getWidth(), image->getHeight(),
	                  image->getData(), &bmi, 0, SRCCOPY) == 0)
	{
		FATAL_ERROR();
	}
	
	if (ReleaseDC(reinterpret_cast<VkIcdSurfaceWin32*>(surface)->hwnd, dc) != 1)
	{
		FATAL_ERROR();
	}
	
	if (width != image->getWidth() || height != image->getHeight())
	{
		return VK_ERROR_OUT_OF_DATE_KHR;
	}

	return VK_SUCCESS;
}

VkResult Swapchain::Create(const VkSwapchainCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSwapchainKHR* pSwapchain)
{
	assert(pCreateInfo->sType == VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR);

	auto swapchain = Allocate<Swapchain>(pAllocator);

	auto next = pCreateInfo->pNext;
	while (next)
	{
		const auto type = *static_cast<const VkStructureType*>(next);
		switch (type)
		{
		default:
			FATAL_ERROR();
		}
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
		reinterpret_cast<VkSwapchainKHR>(swapchain),
	};
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
	swapchain->data = AllocateSized(pAllocator, dataSize);

	for (auto i = 0u; i < pCreateInfo->minImageCount; i++)
	{	
		VkImage image;
		const auto result = Image::Create(&imageCreate, pAllocator, &image);
		if (result != VK_SUCCESS)
		{
			FATAL_ERROR();
		}
		reinterpret_cast<Image*>(image)->BindMemory(reinterpret_cast<VkDeviceMemory>(swapchain->data), dataStride * i);
		swapchain->images.push_back(reinterpret_cast<Image*>(image));
	}
	
	if (pCreateInfo->oldSwapchain)
	{
		FATAL_ERROR();
	}

	swapchain->bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	swapchain->bmi.bmiHeader.biWidth = pCreateInfo->imageExtent.width;
	swapchain->bmi.bmiHeader.biHeight = pCreateInfo->imageExtent.height;
	swapchain->bmi.bmiHeader.biPlanes = 1;
	swapchain->bmi.bmiHeader.biBitCount = 32;
	swapchain->bmi.bmiHeader.biCompression = BI_RGB;
	swapchain->bmi.bmiHeader.biSizeImage = 0;
	swapchain->bmi.bmiHeader.biXPelsPerMeter = 0;
	swapchain->bmi.bmiHeader.biYPelsPerMeter = 0;
	swapchain->bmi.bmiHeader.biClrUsed = 0;
	swapchain->bmi.bmiHeader.biClrImportant = 0;

	*pSwapchain = reinterpret_cast<VkSwapchainKHR>(swapchain);
	return VK_SUCCESS;
}
