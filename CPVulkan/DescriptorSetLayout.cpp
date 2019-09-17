#include "DescriptorSetLayout.h"

#include "Util.h"

#include <cassert>

VkResult DescriptorSetLayout::Create(const VkDescriptorSetLayoutCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDescriptorSetLayout* pSetLayout)
{
	assert(pCreateInfo->sType == VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO);

	auto descriptorSetLayout = Allocate<DescriptorSetLayout>(pAllocator, VK_SYSTEM_ALLOCATION_SCOPE_DEVICE);

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

	descriptorSetLayout->bindings = ArrayToVector(pCreateInfo->bindingCount, pCreateInfo->pBindings);

	WrapVulkan(descriptorSetLayout, pSetLayout);
	return VK_SUCCESS;
}
