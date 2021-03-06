#include "Instance.h"

#include "Device.h"
#include "Extensions.h"
#include "PhysicalDevice.h"
#include "Platform.h"
#include "Util.h"

#include <algorithm>
#include <cassert>
#include <vector>

Instance::Instance(PhysicalDevice* physicalDevice) :
	physicalDevice{physicalDevice}
{
}

void Instance::OnDelete(const VkAllocationCallbacks* pAllocator)
{
	Free(physicalDevice, pAllocator);
	physicalDevice = nullptr;
}

PFN_vkVoidFunction Instance::GetProcAddress(const char* pName) const
{
	return this->enabledExtensions.getFunction(pName, false);
}

void Instance::DestroyInstance(const VkAllocationCallbacks* pAllocator)
{
	Free(this, pAllocator);
}

VkResult Instance::EnumeratePhysicalDevices(uint32_t* pPhysicalDeviceCount, VkPhysicalDevice* pPhysicalDevices) const
{
	if (pPhysicalDevices == nullptr)
	{
		*pPhysicalDeviceCount = 1;
	}
	else
	{
		if (*pPhysicalDeviceCount == 0)
		{
			return VK_INCOMPLETE;
		}

		WrapVulkan(physicalDevice, pPhysicalDevices);
		*pPhysicalDeviceCount = 1;
	}

	return VK_SUCCESS;
}

VkResult Instance::EnumeratePhysicalDeviceGroups(uint32_t* pPhysicalDeviceGroupCount, VkPhysicalDeviceGroupProperties* pPhysicalDeviceGroupProperties) const
{
	if (pPhysicalDeviceGroupProperties)
	{
		if (*pPhysicalDeviceGroupCount == 0)
		{
			return VK_INCOMPLETE;
		}

		assert(pPhysicalDeviceGroupProperties[0].sType == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_GROUP_PROPERTIES);
		pPhysicalDeviceGroupProperties[0].physicalDeviceCount = 1;
		WrapVulkan(physicalDevice, pPhysicalDeviceGroupProperties[0].physicalDevices);
		pPhysicalDeviceGroupProperties[0].subsetAllocation = false;
	}
	
	*pPhysicalDeviceGroupCount = 1;

	return VK_SUCCESS;
}

VkResult Instance::Create(const VkInstanceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkInstance* pInstance)
{
	assert(pCreateInfo->sType == VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO);
	assert(pCreateInfo->flags == 0);

	Platform::Initialise();

	auto physicalDevice = Allocate<PhysicalDevice>(pAllocator, VK_SYSTEM_ALLOCATION_SCOPE_INSTANCE);
	if (!physicalDevice)
	{
		return VK_ERROR_OUT_OF_HOST_MEMORY;
	}

	auto instance = Allocate<Instance>(pAllocator, VK_SYSTEM_ALLOCATION_SCOPE_INSTANCE, physicalDevice);
	if (!instance)
	{
		Free(physicalDevice, pAllocator);
		return VK_ERROR_OUT_OF_HOST_MEMORY;
	}

	physicalDevice->setInstance(instance);

	auto next = pCreateInfo->pNext;
	while (next)
	{
		const auto type = *static_cast<const VkStructureType*>(next);
		switch (type)
		{
		case VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT:
			TODO_ERROR();
			
		case VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT:
			instance->debug = *static_cast<const VkDebugUtilsMessengerCreateInfoEXT*>(next);
			break;
			
		case VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT:
			TODO_ERROR();
			
		case VK_STRUCTURE_TYPE_VALIDATION_FLAGS_EXT:
			TODO_ERROR();
		}
		next = static_cast<const VkBaseInStructure*>(next)->pNext;
	}

	if (pCreateInfo->pApplicationInfo)
	{
		instance->version = pCreateInfo->pApplicationInfo->apiVersion;
		if (instance->version > VULKAN_VERSION)
		{
			instance->version = VULKAN_VERSION;
		}
		else if (instance->version == 0)
		{
			instance->version = VK_API_VERSION_1_0;
		}

		if (pCreateInfo->pApplicationInfo->pNext)
		{
			TODO_ERROR();
		}
	}

	if (pCreateInfo->enabledLayerCount)
	{
		TODO_ERROR();
	}

	std::vector<const char*> enabledExtensions{};
	if (pCreateInfo->enabledExtensionCount)
	{
		for (auto i = 0u; i < pCreateInfo->enabledExtensionCount; i++)
		{
			enabledExtensions.push_back(pCreateInfo->ppEnabledExtensionNames[i]);
		}
	}

	const auto initialExtension = GetInitialExtensions();
	const auto result = initialExtension.MakeInstanceCopy(instance->version, enabledExtensions, &instance->enabledExtensions);

	if (result != VK_SUCCESS)
	{
		Free(instance, pAllocator);
		*pInstance = nullptr;
		return result;
	}

	WrapVulkan(instance, pInstance);
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