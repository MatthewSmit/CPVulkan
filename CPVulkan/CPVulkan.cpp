﻿#include "Instance.h"

#include "Device.h"
#include "Trampoline.h"

#include <cstring>

#if defined(_MSC_VER)
#define DLL_EXPORT __declspec(dllexport)
#else
#define DLL_EXPORT
#endif

extern "C" DLL_EXPORT VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vk_icdGetInstanceProcAddr(VkInstance instance, const char* pName)
{
	if (strcmp(pName, "vkGetInstanceProcAddr") == 0)
	{
		return GET_TRAMPOLINE(Instance, GetProcAddress);
	}

	if (strcmp(pName, "vkCreateInstance") == 0)
	{
		return reinterpret_cast<PFN_vkVoidFunction>(CreateInstance);
	}

	if (strcmp(pName, "vkEnumerateInstanceExtensionProperties") == 0)
	{
		return reinterpret_cast<PFN_vkVoidFunction>(EnumerateInstanceExtensionProperties);
	}

	if (strcmp(pName, "vkEnumerateInstanceVersion") == 0)
	{
		return reinterpret_cast<PFN_vkVoidFunction>(EnumerateInstanceVersion);
	}

#if defined(VK_KHR_win32_surface)
	if (strcmp(pName, "vkCreateWin32SurfaceKHR") == 0)
	{
		return GET_TRAMPOLINE(Instance, CreateWin32Surface);
	}
#endif

#if defined(VK_KHR_xcb_surface)
	if (strcmp(pName, "vkCreateXcbSurfaceKHR") == 0)
	{
		return GET_TRAMPOLINE(Instance, CreateXcbSurface);
	}
#endif

#if defined(VK_KHR_xlib_surface)
	if (strcmp(pName, "vkCreateXlibSurfaceKHR") == 0)
	{
		return GET_TRAMPOLINE(Instance, CreateXlibSurface);
	}
#endif

	if (strcmp(pName, "vkDestroySurfaceKHR") == 0)
	{
		return GET_TRAMPOLINE(Instance, DestroySurface);
	}

	if (strcmp(pName, "vkCreateSwapchainKHR") == 0)
	{
		return GET_TRAMPOLINE(Device, CreateSwapchain);
	}

	if (strcmp(pName, "vkGetPhysicalDeviceSurfaceSupportKHR") == 0)
	{
		return GET_TRAMPOLINE(PhysicalDevice, GetSurfaceSupport);
	}

	if (strcmp(pName, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR") == 0)
	{
		return GET_TRAMPOLINE(PhysicalDevice, GetSurfaceCapabilities);
	}

	if (strcmp(pName, "vkGetPhysicalDeviceSurfaceFormatsKHR") == 0)
	{
		return GET_TRAMPOLINE(PhysicalDevice, GetSurfaceFormats);
	}

	if (strcmp(pName, "vkGetPhysicalDeviceSurfacePresentModesKHR") == 0)
	{
		return GET_TRAMPOLINE(PhysicalDevice, GetSurfacePresentModes);
	}

	return UnwrapVulkan<Instance>(instance)->GetProcAddress(pName);
}

extern "C" DLL_EXPORT VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vk_icdGetPhysicalDeviceProcAddr(VkInstance, const char*)
{
	return nullptr;
}

extern "C" DLL_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vk_icdNegotiateLoaderICDInterfaceVersion(uint32_t* pSupportedVersion)
{
	constexpr auto LATEST_ICD_VERSION = 5;
	
	if (*pSupportedVersion > LATEST_ICD_VERSION)
	{
		*pSupportedVersion = LATEST_ICD_VERSION;
	}

	return VK_SUCCESS;
}