#pragma once
#include "Base.h"

class Instance;

class PhysicalDevice final
{
public:
	PhysicalDevice() = default;
	PhysicalDevice(const PhysicalDevice&) = delete;
	PhysicalDevice(PhysicalDevice&&) = delete;
	~PhysicalDevice() = default;

	PhysicalDevice& operator=(const PhysicalDevice&) = delete;
	PhysicalDevice&& operator=(const PhysicalDevice&&) = delete;

	void OnDelete(const VkAllocationCallbacks*)
	{
	}
	
	VKAPI_ATTR void VKAPI_CALL GetFeatures(VkPhysicalDeviceFeatures* pFeatures);
	VKAPI_ATTR void VKAPI_CALL GetFormatProperties(VkFormat format, VkFormatProperties* pFormatProperties);
	VKAPI_ATTR VkResult VKAPI_CALL GetImageFormatProperties(VkFormat format, VkImageType type, VkImageTiling tiling, VkImageUsageFlags usage, VkImageCreateFlags flags, VkImageFormatProperties* pImageFormatProperties);
	VKAPI_ATTR void VKAPI_CALL GetProperties(VkPhysicalDeviceProperties* pProperties) const;
	VKAPI_ATTR void VKAPI_CALL GetQueueFamilyProperties(uint32_t* pQueueFamilyPropertyCount, VkQueueFamilyProperties* pQueueFamilyProperties);
	VKAPI_ATTR void VKAPI_CALL GetMemoryProperties(VkPhysicalDeviceMemoryProperties* pMemoryProperties);
	VKAPI_ATTR void VKAPI_CALL GetSparseImageFormatProperties(VkFormat format, VkImageType type, VkSampleCountFlagBits samples, VkImageUsageFlags usage, VkImageTiling tiling, uint32_t* pPropertyCount, VkSparseImageFormatProperties* pProperties);
	VKAPI_ATTR void VKAPI_CALL GetFeatures2(VkPhysicalDeviceFeatures2* pFeatures);
	VKAPI_ATTR void VKAPI_CALL GetProperties2(VkPhysicalDeviceProperties2* pProperties);
	VKAPI_ATTR void VKAPI_CALL GetFormatProperties2(VkFormat format, VkFormatProperties2* pFormatProperties);
	VKAPI_ATTR VkResult VKAPI_CALL GetImageFormatProperties2(const VkPhysicalDeviceImageFormatInfo2* pImageFormatInfo, VkImageFormatProperties2* pImageFormatProperties);
	VKAPI_ATTR void VKAPI_CALL GetQueueFamilyProperties2(uint32_t* pQueueFamilyPropertyCount, VkQueueFamilyProperties2* pQueueFamilyProperties);
	VKAPI_ATTR void VKAPI_CALL GetMemoryProperties2(VkPhysicalDeviceMemoryProperties2* pMemoryProperties);
	VKAPI_ATTR void VKAPI_CALL GetSparseImageFormatProperties2(const VkPhysicalDeviceSparseImageFormatInfo2* pFormatInfo, uint32_t* pPropertyCount, VkSparseImageFormatProperties2* pProperties);
	VKAPI_ATTR void VKAPI_CALL GetExternalBufferProperties(const VkPhysicalDeviceExternalBufferInfo* pExternalBufferInfo, VkExternalBufferProperties* pExternalBufferProperties);
	VKAPI_ATTR void VKAPI_CALL GetExternalFenceProperties(const VkPhysicalDeviceExternalFenceInfo* pExternalFenceInfo, VkExternalFenceProperties* pExternalFenceProperties);
	VKAPI_ATTR void VKAPI_CALL GetExternalSemaphoreProperties(const VkPhysicalDeviceExternalSemaphoreInfo* pExternalSemaphoreInfo, VkExternalSemaphoreProperties* pExternalSemaphoreProperties);
	VKAPI_ATTR VkResult VKAPI_CALL CreateDevice(const VkDeviceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDevice* pDevice);
	VKAPI_ATTR VkResult VKAPI_CALL EnumerateDeviceExtensionProperties(const char* pLayerName, uint32_t* pPropertyCount, VkExtensionProperties* pProperties);
	VKAPI_ATTR VkResult VKAPI_CALL EnumerateDeviceLayerProperties(uint32_t* pPropertyCount, VkLayerProperties* pProperties) { FATAL_ERROR(); }
	VKAPI_ATTR VkResult VKAPI_PTR GetSurfaceSupport(uint32_t queueFamilyIndex, VkSurfaceKHR surface, VkBool32* pSupported);
	VKAPI_ATTR VkResult VKAPI_PTR GetSurfaceCapabilities(VkSurfaceKHR surface, VkSurfaceCapabilitiesKHR* pSurfaceCapabilities) const;
	VKAPI_ATTR VkResult VKAPI_PTR GetSurfaceFormats(VkSurfaceKHR surface, uint32_t* pSurfaceFormatCount, VkSurfaceFormatKHR* pSurfaceFormats);
	VKAPI_ATTR VkResult VKAPI_PTR GetSurfacePresentModes(VkSurfaceKHR surface, uint32_t* pPresentModeCount, VkPresentModeKHR* pPresentModes);
	VKAPI_ATTR VkResult VKAPI_PTR GetPresentRectangles(VkSurfaceKHR surface, uint32_t* pRectCount, VkRect2D* pRects) { FATAL_ERROR(); }

#if defined(VK_KHR_display)
	VKAPI_ATTR VkResult VKAPI_PTR GetDisplayProperties(uint32_t* pPropertyCount, VkDisplayPropertiesKHR* pProperties);
	VKAPI_ATTR VkResult VKAPI_PTR GetDisplayPlaneProperties(uint32_t* pPropertyCount, VkDisplayPlanePropertiesKHR* pProperties) { FATAL_ERROR(); }
	VKAPI_ATTR VkResult VKAPI_PTR GetDisplayPlaneSupportedDisplays(uint32_t planeIndex, uint32_t* pDisplayCount, VkDisplayKHR* pDisplays) { FATAL_ERROR(); }
	VKAPI_ATTR VkResult VKAPI_PTR GetDisplayModeProperties(VkDisplayKHR display, uint32_t* pPropertyCount, VkDisplayModePropertiesKHR* pProperties) { FATAL_ERROR(); }
	VKAPI_ATTR VkResult VKAPI_PTR CreateDisplayMode(VkDisplayKHR display, const VkDisplayModeCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDisplayModeKHR* pMode) { FATAL_ERROR(); }
	VKAPI_ATTR VkResult VKAPI_PTR GetDisplayPlaneCapabilities(VkDisplayModeKHR mode, uint32_t planeIndex, VkDisplayPlaneCapabilitiesKHR* pCapabilities) { FATAL_ERROR(); }
#endif

	VKAPI_ATTR VkResult VKAPI_PTR GetSurfaceCapabilities2KHR(const VkPhysicalDeviceSurfaceInfo2KHR* pSurfaceInfo, VkSurfaceCapabilities2KHR* pSurfaceCapabilities);
	VKAPI_ATTR VkResult VKAPI_PTR GetSurfaceFormats2(const VkPhysicalDeviceSurfaceInfo2KHR* pSurfaceInfo, uint32_t* pSurfaceFormatCount, VkSurfaceFormat2KHR* pSurfaceFormats);

	VKAPI_ATTR VkResult VKAPI_PTR GetDisplayProperties2(uint32_t* pPropertyCount, VkDisplayProperties2KHR* pProperties) { FATAL_ERROR(); }
	VKAPI_ATTR VkResult VKAPI_PTR GetDisplayPlaneProperties2(uint32_t* pPropertyCount, VkDisplayPlaneProperties2KHR* pProperties) { FATAL_ERROR(); }
	VKAPI_ATTR VkResult VKAPI_PTR GetDisplayModeProperties2(VkDisplayKHR display, uint32_t* pPropertyCount, VkDisplayModeProperties2KHR* pProperties) { FATAL_ERROR(); }
	VKAPI_ATTR VkResult VKAPI_PTR GetDisplayPlaneCapabilities2(const VkDisplayPlaneInfo2KHR* pDisplayPlaneInfo, VkDisplayPlaneCapabilities2KHR* pCapabilities) { FATAL_ERROR(); }

	VKAPI_ATTR VkResult VKAPI_PTR GetExternalImageFormatPropertiesNV(VkFormat format, VkImageType type, VkImageTiling tiling, VkImageUsageFlags usage, VkImageCreateFlags flags, VkExternalMemoryHandleTypeFlagsNV externalHandleType, VkExternalImageFormatPropertiesNV* pExternalImageFormatProperties) { FATAL_ERROR(); }

	VKAPI_ATTR void VKAPI_PTR GetGeneratedCommandsPropertiesNVX(VkDeviceGeneratedCommandsFeaturesNVX* pFeatures, VkDeviceGeneratedCommandsLimitsNVX* pLimits) { FATAL_ERROR(); }

	VKAPI_ATTR VkResult VKAPI_PTR ReleaseDisplay(VkDisplayKHR display) { FATAL_ERROR(); }

	VKAPI_ATTR VkResult VKAPI_PTR GetSurfaceCapabilities2(VkSurfaceKHR surface, VkSurfaceCapabilities2EXT* pSurfaceCapabilities) { FATAL_ERROR(); }

	VKAPI_ATTR void VKAPI_PTR GetMultisampleProperties(VkSampleCountFlagBits samples, VkMultisamplePropertiesEXT* pMultisampleProperties) { FATAL_ERROR(); }

#if defined(VK_EXT_calibrated_timestamps)
	VKAPI_ATTR VkResult VKAPI_PTR GetCalibrateableTimeDomains(uint32_t* pTimeDomainCount, VkTimeDomainEXT* pTimeDomains);
#endif
	
	VKAPI_ATTR VkResult VKAPI_PTR GetCooperativeMatrixPropertiesNV(uint32_t* pPropertyCount, VkCooperativeMatrixPropertiesNV* pProperties) { FATAL_ERROR(); }

	VKAPI_ATTR VkResult VKAPI_PTR GetSupportedFramebufferMixedSamplesCombinationsNV(uint32_t* pCombinationCount, VkFramebufferMixedSamplesCombinationNV* pCombinations) { FATAL_ERROR(); }

	VKAPI_ATTR VkBool32 VKAPI_PTR GetWin32PresentationSupport(uint32_t queueFamilyIndex) { FATAL_ERROR(); }
								  
	VKAPI_ATTR VkResult VKAPI_PTR GetSurfacePresentModes2(const VkPhysicalDeviceSurfaceInfo2KHR* pSurfaceInfo, uint32_t* pPresentModeCount, VkPresentModeKHR* pPresentModes) { FATAL_ERROR(); }

#if defined(VK_KHR_xcb_surface)
    VKAPI_ATTR VkBool32 VKAPI_PTR GetPhysicalDeviceXcbPresentationSupport(uint32_t queueFamilyIndex, xcb_connection_t* connection, xcb_visualid_t visual_id) { FATAL_ERROR(); }
#endif

#if defined(VK_KHR_xlib_surface)
    VKAPI_ATTR VkBool32 VKAPI_PTR GetPhysicalDeviceXlibPresentationSupport(uint32_t queueFamilyIndex, Display* dpy, VisualID visualID) { FATAL_ERROR(); }
#endif

#if defined(VK_EXT_acquire_xlib_display)
    VKAPI_ATTR VkResult VKAPI_PTR AcquireXlibDisplay(Display* dpy, VkDisplayKHR display) { FATAL_ERROR(); }
    VKAPI_ATTR VkResult VKAPI_PTR GetRandROutputDisplay(Display* dpy, RROutput rrOutput, VkDisplayKHR* pDisplay) { FATAL_ERROR(); }
#endif

	void setInstance(Instance* instance) { this->instance = instance; }

private:
	Instance* instance;
};
