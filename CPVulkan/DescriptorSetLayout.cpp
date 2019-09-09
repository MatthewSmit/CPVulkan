#include "DescriptorSetLayout.h"

#include <cassert>

VkResult DescriptorSetLayout::Create(const VkDescriptorSetLayoutCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDescriptorSetLayout* pSetLayout)
{
	assert(pCreateInfo->sType == VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO);

	auto descriptorSetLayout = Allocate<DescriptorSetLayout>(pAllocator);

	auto next = pCreateInfo->pNext;
	while (next)
	{
		auto type = *static_cast<const VkStructureType*>(next);
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

	descriptorSetLayout->bindings = std::vector<VkDescriptorSetLayoutBinding>{pCreateInfo->bindingCount};
	for (auto i = 0u; i < pCreateInfo->bindingCount; i++)
	{
		descriptorSetLayout->bindings[i] = pCreateInfo->pBindings[i];
	}

	*pSetLayout = reinterpret_cast<VkDescriptorSetLayout>(descriptorSetLayout);
	return VK_SUCCESS;
}
