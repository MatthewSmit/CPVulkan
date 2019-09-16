#include "Swapchain.h"

#include "Image.h"

#include <Windows.h>
#include <vulkan/vk_icd.h>

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
	
	if (ReleaseDC(reinterpret_cast<VkIcdSurfaceWin32*>(surface)->hwnd, dc) != 1)
	{
		FATAL_ERROR();
	}
	
	if (width != imageWidth || height != imageHeight)
	{
		return VK_ERROR_OUT_OF_DATE_KHR;
	}

	return VK_SUCCESS;
}
