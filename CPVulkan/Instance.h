#pragma once
#include "Base.h"

#include "Extensions.h"

class PhysicalDevice;

class Instance final
{
public:
	Instance(const VkAllocationCallbacks* pAllocator);
	Instance(const Instance&) = delete;
	Instance(Instance&&) = delete;
	~Instance() = default;

	Instance& operator=(const Instance&) = delete;
	Instance&& operator=(const Instance&&) = delete;

	void OnDelete(const VkAllocationCallbacks* pAllocator);

	VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL GetProcAddress(const char* pName) const;

	VKAPI_ATTR void VKAPI_CALL DestroyInstance(const VkAllocationCallbacks* pAllocator);

	VKAPI_ATTR VkResult VKAPI_CALL EnumeratePhysicalDevices(uint32_t* pPhysicalDeviceCount, VkPhysicalDevice* pPhysicalDevices) const;

	VKAPI_ATTR VkResult VKAPI_CALL EnumeratePhysicalDeviceGroups(uint32_t* pPhysicalDeviceGroupCount, VkPhysicalDeviceGroupProperties* pPhysicalDeviceGroupProperties) const;
	
	VKAPI_ATTR void VKAPI_PTR DestroySurface(VkSurfaceKHR surface, const VkAllocationCallbacks* pAllocator);
	
	VKAPI_ATTR VkResult VKAPI_PTR CreateDisplayPlaneSurface(const VkDisplaySurfaceCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSurfaceKHR* pSurface) { FATAL_ERROR(); } 

	VKAPI_ATTR VkResult VKAPI_PTR CreateDebugReportCallback(const VkDebugReportCallbackCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugReportCallbackEXT* pCallback) { FATAL_ERROR(); } 
	VKAPI_ATTR void VKAPI_PTR DestroyDebugReportCallback(VkDebugReportCallbackEXT callback, const VkAllocationCallbacks* pAllocator) { FATAL_ERROR(); } 
	VKAPI_ATTR void VKAPI_PTR DebugReportMessage(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t object, size_t location, int32_t messageCode, const char* pLayerPrefix, const char* pMessage) { FATAL_ERROR(); } 

	VKAPI_ATTR VkResult VKAPI_PTR CreateDebugUtilsMessenger(const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pMessenger) { FATAL_ERROR(); } 
	VKAPI_ATTR void VKAPI_PTR DestroyDebugUtilsMessenger(VkDebugUtilsMessengerEXT messenger, const VkAllocationCallbacks* pAllocator) { FATAL_ERROR(); } 
	VKAPI_ATTR void VKAPI_PTR SubmitDebugUtilsMessage(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageTypes, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData) { FATAL_ERROR(); } 

	VKAPI_ATTR VkResult VKAPI_PTR CreateHeadlessSurface(const VkHeadlessSurfaceCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSurfaceKHR* pSurface) { FATAL_ERROR(); }

#if defined(VK_USE_PLATFORM_WIN32_KHR)
	VKAPI_ATTR VkResult VKAPI_PTR CreateWin32Surface(const VkWin32SurfaceCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSurfaceKHR* pSurface);
#endif

#if defined(VK_USE_PLATFORM_XCB_KHR)
    VKAPI_ATTR VkResult VKAPI_CALL CreateXcbSurface(const VkXcbSurfaceCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSurfaceKHR* pSurface);
#endif

#if defined(VK_USE_PLATFORM_XLIB_KHR)
    VKAPI_ATTR VkResult VKAPI_CALL CreateXlibSurface(const VkXlibSurfaceCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSurfaceKHR* pSurface);
#endif

	static VkResult Create(const VkInstanceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkInstance* pInstance);

	int getVersion() const { return version; }
	const ExtensionGroup& getEnabledExtensions() const { return enabledExtensions; }

private:
	int version = VK_API_VERSION_1_0;
	VkDebugUtilsMessengerCreateInfoEXT debug{};

	PhysicalDevice* physicalDevice{};
	ExtensionGroup enabledExtensions{};
};

VKAPI_ATTR VkResult VKAPI_CALL CreateInstance(const VkInstanceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkInstance* pInstance);
VKAPI_ATTR VkResult VKAPI_CALL EnumerateInstanceExtensionProperties(const char* pLayerName, uint32_t* pPropertyCount, VkExtensionProperties* pProperties);
VKAPI_ATTR VkResult VKAPI_CALL EnumerateInstanceLayerProperties(uint32_t* pPropertyCount, VkLayerProperties* pProperties);
VKAPI_ATTR VkResult VKAPI_CALL EnumerateInstanceVersion(uint32_t* pApiVersion);