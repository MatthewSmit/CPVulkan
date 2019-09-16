#pragma once
#include "Base.h"

class Instance;

class PhysicalDevice final : public VulkanBase
{
public:
	PhysicalDevice(Instance* instance) :
		instance{instance}
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
	VKAPI_ATTR VkResult VKAPI_PTR GetSurfaceCapabilities(VkSurfaceKHR surface, VkSurfaceCapabilitiesKHR* pSurfaceCapabilities);
	VKAPI_ATTR VkResult VKAPI_PTR GetSurfaceFormats(VkSurfaceKHR surface, uint32_t* pSurfaceFormatCount, VkSurfaceFormatKHR* pSurfaceFormats);
	VKAPI_ATTR VkResult VKAPI_PTR GetSurfacePresentModes(VkSurfaceKHR surface, uint32_t* pPresentModeCount, VkPresentModeKHR* pPresentModes);

	VKAPI_ATTR VkResult VKAPI_PTR GetPresentRectangles(VkSurfaceKHR surface, uint32_t* pRectCount, VkRect2D* pRects) { FATAL_ERROR(); }

	VKAPI_ATTR VkResult VKAPI_PTR GetDisplayProperties(uint32_t* pPropertyCount, VkDisplayPropertiesKHR* pProperties) { FATAL_ERROR(); }
	VKAPI_ATTR VkResult VKAPI_PTR GetDisplayPlaneProperties(uint32_t* pPropertyCount, VkDisplayPlanePropertiesKHR* pProperties) { FATAL_ERROR(); }
	VKAPI_ATTR VkResult VKAPI_PTR GetDisplayPlaneSupportedDisplays(uint32_t planeIndex, uint32_t* pDisplayCount, VkDisplayKHR* pDisplays) { FATAL_ERROR(); }
	VKAPI_ATTR VkResult VKAPI_PTR GetDisplayModeProperties(VkDisplayKHR display, uint32_t* pPropertyCount, VkDisplayModePropertiesKHR* pProperties) { FATAL_ERROR(); }
	VKAPI_ATTR VkResult VKAPI_PTR CreateDisplayMode(VkDisplayKHR display, const VkDisplayModeCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDisplayModeKHR* pMode) { FATAL_ERROR(); }
	VKAPI_ATTR VkResult VKAPI_PTR GetDisplayPlaneCapabilities(VkDisplayModeKHR mode, uint32_t planeIndex, VkDisplayPlaneCapabilitiesKHR* pCapabilities) { FATAL_ERROR(); }

	VKAPI_ATTR void VKAPI_PTR GetExternalBufferPropertiesKHR(const VkPhysicalDeviceExternalBufferInfo* pExternalBufferInfo, VkExternalBufferProperties* pExternalBufferProperties) { FATAL_ERROR(); }

	VKAPI_ATTR void VKAPI_PTR GetExternalSemaphorePropertiesKHR(const VkPhysicalDeviceExternalSemaphoreInfo* pExternalSemaphoreInfo, VkExternalSemaphoreProperties* pExternalSemaphoreProperties) { FATAL_ERROR(); }

	VKAPI_ATTR void VKAPI_PTR GetExternalFencePropertiesKHR(const VkPhysicalDeviceExternalFenceInfo* pExternalFenceInfo, VkExternalFenceProperties* pExternalFenceProperties) { FATAL_ERROR(); }

	VKAPI_ATTR VkResult VKAPI_PTR GetSurfaceCapabilities2KHR(const VkPhysicalDeviceSurfaceInfo2KHR* pSurfaceInfo, VkSurfaceCapabilities2KHR* pSurfaceCapabilities) { FATAL_ERROR(); }
	VKAPI_ATTR VkResult VKAPI_PTR GetSurfaceFormats2KHR(const VkPhysicalDeviceSurfaceInfo2KHR* pSurfaceInfo, uint32_t* pSurfaceFormatCount, VkSurfaceFormat2KHR* pSurfaceFormats) { FATAL_ERROR(); }

	VKAPI_ATTR VkResult VKAPI_PTR GetDisplayProperties2KHR(uint32_t* pPropertyCount, VkDisplayProperties2KHR* pProperties) { FATAL_ERROR(); }
	VKAPI_ATTR VkResult VKAPI_PTR GetDisplayPlaneProperties2KHR(uint32_t* pPropertyCount, VkDisplayPlaneProperties2KHR* pProperties) { FATAL_ERROR(); }
	VKAPI_ATTR VkResult VKAPI_PTR GetDisplayModeProperties2KHR(VkDisplayKHR display, uint32_t* pPropertyCount, VkDisplayModeProperties2KHR* pProperties) { FATAL_ERROR(); }
	VKAPI_ATTR VkResult VKAPI_PTR GetDisplayPlaneCapabilities2KHR(const VkDisplayPlaneInfo2KHR* pDisplayPlaneInfo, VkDisplayPlaneCapabilities2KHR* pCapabilities) { FATAL_ERROR(); }

	VKAPI_ATTR VkResult VKAPI_PTR GetExternalImageFormatPropertiesNV(VkFormat format, VkImageType type, VkImageTiling tiling, VkImageUsageFlags usage, VkImageCreateFlags flags, VkExternalMemoryHandleTypeFlagsNV externalHandleType, VkExternalImageFormatPropertiesNV* pExternalImageFormatProperties) { FATAL_ERROR(); }

	VKAPI_ATTR void VKAPI_PTR GetGeneratedCommandsPropertiesNVX(VkDeviceGeneratedCommandsFeaturesNVX* pFeatures, VkDeviceGeneratedCommandsLimitsNVX* pLimits) { FATAL_ERROR(); }

	VKAPI_ATTR VkResult VKAPI_PTR ReleaseDisplayEXT(VkDisplayKHR display) { FATAL_ERROR(); }

	VKAPI_ATTR VkResult VKAPI_PTR GetSurfaceCapabilities2EXT(VkSurfaceKHR surface, VkSurfaceCapabilities2EXT* pSurfaceCapabilities) { FATAL_ERROR(); }

	VKAPI_ATTR void VKAPI_PTR GetMultisamplePropertiesEXT(VkSampleCountFlagBits samples, VkMultisamplePropertiesEXT* pMultisampleProperties) { FATAL_ERROR(); }

	VKAPI_ATTR VkResult VKAPI_PTR GetCalibrateableTimeDomainsEXT(uint32_t* pTimeDomainCount, VkTimeDomainEXT* pTimeDomains) { FATAL_ERROR(); }
	
	VKAPI_ATTR VkResult VKAPI_PTR GetCooperativeMatrixPropertiesNV(uint32_t* pPropertyCount, VkCooperativeMatrixPropertiesNV* pProperties) { FATAL_ERROR(); }

	VKAPI_ATTR VkResult VKAPI_PTR GetSupportedFramebufferMixedSamplesCombinationsNV(uint32_t* pCombinationCount, VkFramebufferMixedSamplesCombinationNV* pCombinations) { FATAL_ERROR(); }

	VKAPI_ATTR VkBool32 VKAPI_PTR GetWin32PresentationSupportKHR(uint32_t queueFamilyIndex) { FATAL_ERROR(); }
								  
	VKAPI_ATTR VkResult VKAPI_PTR GetSurfacePresentModes2EXT(const VkPhysicalDeviceSurfaceInfo2KHR* pSurfaceInfo, uint32_t* pPresentModeCount, VkPresentModeKHR* pPresentModes) { FATAL_ERROR(); }

private:
	Instance* instance;
};
