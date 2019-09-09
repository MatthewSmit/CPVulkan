#include "Instance.h"

#include "Device.h"
#include "Extensions.h"
#include "PhysicalDevice.h"

#include <vulkan/vk_icd.h>
#include <vulkan/vk_layer.h>

#include <algorithm>
#include <cassert>
#include <exception>
#include <vector>

Instance::Instance() :
	physicalDevice{std::make_unique<PhysicalDevice>(this)}
{
}

Instance::~Instance() = default;

PFN_vkVoidFunction Instance::GetProcAddress(const char* pName)
{
	return this->enabledExtensions.getFunction(pName, false);
}

void Instance::DestroyInstance(const VkAllocationCallbacks* pAllocator)
{
	Free(this, pAllocator);
}

VkResult Instance::EnumeratePhysicalDevices(uint32_t* pPhysicalDeviceCount, VkPhysicalDevice* pPhysicalDevices)
{
	return HandleEnumeration(pPhysicalDeviceCount, pPhysicalDevices, std::vector<VkPhysicalDevice>
	{
		reinterpret_cast<VkPhysicalDevice>(physicalDevice.get())
	});
}

VkResult Instance::EnumeratePhysicalDeviceGroups(uint32_t* pPhysicalDeviceGroupCount, VkPhysicalDeviceGroupProperties* pPhysicalDeviceGroupProperties)
{
	if (pPhysicalDeviceGroupProperties)
	{
		if (pPhysicalDeviceGroupCount == 0)
		{
			return VK_INCOMPLETE;
		}

		assert(pPhysicalDeviceGroupProperties[0].sType == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_GROUP_PROPERTIES);
		assert(pPhysicalDeviceGroupProperties[0].pNext == nullptr);
		pPhysicalDeviceGroupProperties[0].physicalDeviceCount = 1;
		pPhysicalDeviceGroupProperties[0].physicalDevices[0] = reinterpret_cast<VkPhysicalDevice>(physicalDevice.get());
		pPhysicalDeviceGroupProperties[0].subsetAllocation = false;

		*pPhysicalDeviceGroupCount = 1;
	}
	else
	{
		*pPhysicalDeviceGroupCount = 1;
	}

	return VK_SUCCESS;
}

void Instance::DestroySurface(VkSurfaceKHR surface, const VkAllocationCallbacks* pAllocator)
{
	Free(reinterpret_cast<VkIcdSurfaceWin32*>(surface), pAllocator);
}

VkResult Instance::CreateWin32Surface(const VkWin32SurfaceCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSurfaceKHR* pSurface)
{
	assert(pCreateInfo->sType == VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR);
	assert(pCreateInfo->pNext == nullptr);
	assert(pCreateInfo->flags == 0);
	
	auto surface = Allocate<VkIcdSurfaceWin32>(pAllocator);
	surface->base.platform = VK_ICD_WSI_PLATFORM_WIN32;
	surface->hinstance = pCreateInfo->hinstance;
	surface->hwnd = pCreateInfo->hwnd;
	*pSurface = reinterpret_cast<VkSurfaceKHR>(surface);
	return VK_SUCCESS;
}

VkResult Instance::Create(const VkInstanceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkInstance* pInstance)
{
	assert(pCreateInfo->sType == VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO);
	assert(pCreateInfo->flags == 0);

	auto instance = Allocate<Instance>(pAllocator);

	auto next = pCreateInfo->pNext;
	while (next)
	{
		auto type = *(VkStructureType*)next;
		switch (type)
		{
		case VK_STRUCTURE_TYPE_LOADER_INSTANCE_CREATE_INFO:
			next = ((VkLayerInstanceCreateInfo*)next)->pNext;
			break;

		case VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT:
			instance->debug = *(VkDebugUtilsMessengerCreateInfoEXT*)next;
			next = ((VkDebugUtilsMessengerCreateInfoEXT*)next)->pNext;
			break;

		default:
			FATAL_ERROR();
		}
	}

	if (pCreateInfo->pApplicationInfo)
	{
		instance->version = pCreateInfo->pApplicationInfo->apiVersion;
		if (instance->version > LATEST_VERSION)
		{
			instance->version = LATEST_VERSION;
		}

		if (pCreateInfo->pApplicationInfo->pNext)
		{
			FATAL_ERROR();
		}
	}

	if (pCreateInfo->enabledLayerCount)
	{
		FATAL_ERROR();
	}

	std::vector<const char*> enabledExtensions{};
	if (pCreateInfo->enabledExtensionCount)
	{
		for (auto i = 0u; i < pCreateInfo->enabledExtensionCount; i++)
		{
			enabledExtensions.push_back(pCreateInfo->ppEnabledExtensionNames[i]);
		}
	}

	auto initialExtension = GetInitialExtensions();
	auto result = initialExtension.MakeInstanceCopy(instance->version, enabledExtensions, &instance->enabledExtensions);

	if (result != VK_SUCCESS)
	{
		Free(instance, pAllocator);
		return result;
	}

	*pInstance = reinterpret_cast<VkInstance>(instance);
	return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL CreateInstance(const VkInstanceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkInstance* pInstance)
{
	return Instance::Create(pCreateInfo, pAllocator, pInstance);
}

VKAPI_ATTR VkResult VKAPI_CALL EnumerateInstanceExtensionProperties(const char* pLayerName, uint32_t* pPropertyCount, VkExtensionProperties* pProperties)
{
	assert(pLayerName == nullptr);

	if (pProperties)
	{
		const auto toCopy = std::min(*pPropertyCount, GetInitialExtensions().getNumberInstanceExtensions());
		GetInitialExtensions().FillExtensionProperties(pProperties, toCopy, false);
		
		if (toCopy < GetInitialExtensions().getNumberInstanceExtensions())
		{
			return VK_INCOMPLETE;
		}

		*pPropertyCount = GetInitialExtensions().getNumberInstanceExtensions();
	}
	else
	{
		*pPropertyCount = GetInitialExtensions().getNumberInstanceExtensions();
	}

	return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL EnumerateInstanceLayerProperties(uint32_t* pPropertyCount, VkLayerProperties* pProperties)
{
	*pPropertyCount = 0;
	return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL EnumerateInstanceVersion(uint32_t* pApiVersion)
{
	*pApiVersion = VK_API_VERSION_1_1;
	return VK_SUCCESS;
}