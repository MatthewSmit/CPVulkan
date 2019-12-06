#include "DescriptorSetLayout.h"

#include "Device.h"
#include "Util.h"

#include <cassert>

VkResult DescriptorSetLayout::Create(const VkDescriptorSetLayoutCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDescriptorSetLayout* pSetLayout)
{
	assert(pCreateInfo->sType == VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO);

	auto descriptorSetLayout = Allocate<DescriptorSetLayout>(pAllocator, VK_SYSTEM_ALLOCATION_SCOPE_DEVICE);
	if (!descriptorSetLayout)
	{
		return VK_ERROR_OUT_OF_HOST_MEMORY;
	}

	auto next = static_cast<const VkBaseInStructure*>(pCreateInfo->pNext);
	while (next)
	{
		const auto type = next->sType;
		switch (type)
		{
		case VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT:
			FATAL_ERROR();
		}
		next = next->pNext;
	}

	if (pCreateInfo->flags)
	{
		FATAL_ERROR();
	}

	descriptorSetLayout->bindings = ArrayToVector(pCreateInfo->bindingCount, pCreateInfo->pBindings);

	WrapVulkan(descriptorSetLayout, pSetLayout);
	return VK_SUCCESS;
}

VkResult Device::CreateDescriptorSetLayout(const VkDescriptorSetLayoutCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDescriptorSetLayout* pSetLayout)
{
	return DescriptorSetLayout::Create(pCreateInfo, pAllocator, pSetLayout);
}

void Device::DestroyDescriptorSetLayout(VkDescriptorSetLayout descriptorSetLayout, const VkAllocationCallbacks* pAllocator)
{
	if (descriptorSetLayout)
	{
		Free(UnwrapVulkan<DescriptorSetLayout>(descriptorSetLayout), pAllocator);
	}
}
