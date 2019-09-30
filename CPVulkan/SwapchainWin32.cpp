#include "Swapchain.h"

#include "Image.h"
#include "Instance.h"
#include "PhysicalDevice.h"

#include <Windows.h>
#include <vulkan/vk_icd.h>

VkResult Swapchain::Present(uint32_t pImageIndex)
{
    // TODO: Y flip
	const auto win32Surface = UnwrapVulkan<VkIcdSurfaceWin32>(surface);
	const auto image = images[pImageIndex];

	const auto dc = GetDC(win32Surface->hwnd);
	if (dc == nullptr)
	{
		FATAL_ERROR();
	}

	RECT rect;
	GetClientRect(win32Surface->hwnd, &rect);

	const auto width = rect.right - rect.left;
	const auto height = rect.bottom - rect.top;
	const auto imageWidth = static_cast<LONG>(image->getWidth());
	const auto imageHeight = static_cast<LONG>(image->getHeight());

	BITMAPINFO bmi
	{
		{
			sizeof(BITMAPINFOHEADER),
			imageWidth,
			imageHeight,
			1,
			32,
			BI_RGB
		}
	};

	if (StretchDIBits(dc,
	                  0, 0, width, height,
	                  0, 0, image->getWidth(), image->getHeight(),
	                  image->getData(), &bmi, 0, SRCCOPY) == 0)
	{
		FATAL_ERROR();
	}
	
	if (ReleaseDC(win32Surface->hwnd, dc) != 1)
	{
		FATAL_ERROR();
	}
	
	if (width != imageWidth || height != imageHeight)
	{
		return VK_ERROR_OUT_OF_DATE_KHR;
	}

	return VK_SUCCESS;
}

void Instance::DestroySurface(VkSurfaceKHR surface, const VkAllocationCallbacks* pAllocator)
{
	Free(UnwrapVulkan<VkIcdSurfaceWin32>(surface), pAllocator);
}

VkResult Instance::CreateWin32Surface(const VkWin32SurfaceCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSurfaceKHR* pSurface)
{
	assert(pCreateInfo->sType == VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR);
	assert(pCreateInfo->flags == 0);

	auto surface = Allocate<VkIcdSurfaceWin32>(pAllocator, VK_SYSTEM_ALLOCATION_SCOPE_INSTANCE);
	surface->base.platform = VK_ICD_WSI_PLATFORM_WIN32;
	surface->hinstance = pCreateInfo->hinstance;
	surface->hwnd = pCreateInfo->hwnd;
	
	WrapVulkan(surface, pSurface);
	return VK_SUCCESS;
}

VkResult PhysicalDevice::GetSurfaceCapabilities(VkSurfaceKHR surface, VkSurfaceCapabilitiesKHR* pSurfaceCapabilities) const
{
	const auto win32Surface = UnwrapVulkan<VkIcdSurfaceWin32>(surface);
	RECT rect;
	assert(GetClientRect(win32Surface->hwnd, &rect));

	pSurfaceCapabilities->minImageCount = 1;
	pSurfaceCapabilities->maxImageCount = 0;
	pSurfaceCapabilities->currentExtent.width = rect.right - rect.left;
	pSurfaceCapabilities->currentExtent.height = rect.bottom - rect.top;
	pSurfaceCapabilities->minImageExtent.width = 1;
	pSurfaceCapabilities->minImageExtent.height = 1;
	pSurfaceCapabilities->maxImageExtent.width = 8192;
	pSurfaceCapabilities->maxImageExtent.height = 8192;
	pSurfaceCapabilities->maxImageArrayLayers = 1;
	pSurfaceCapabilities->supportedTransforms = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	pSurfaceCapabilities->currentTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	pSurfaceCapabilities->supportedCompositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	pSurfaceCapabilities->supportedUsageFlags = VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
		VK_IMAGE_USAGE_TRANSFER_DST_BIT |
		VK_IMAGE_USAGE_SAMPLED_BIT |
		VK_IMAGE_USAGE_STORAGE_BIT |
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
		VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT |
		VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT |
		VK_IMAGE_USAGE_SHADING_RATE_IMAGE_BIT_NV |
		VK_IMAGE_USAGE_FRAGMENT_DENSITY_MAP_BIT_EXT;
	return VK_SUCCESS;
}
