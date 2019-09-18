#include "PipelineLayout.h"

#include <cassert>

VkResult PipelineLayout::Create(const VkPipelineLayoutCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkPipelineLayout* pPipelineLayout)
{
	assert(pCreateInfo->sType == VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO);
	assert(pCreateInfo->flags == 0);

	auto pipelineLayout = Allocate<PipelineLayout>(pAllocator, VK_SYSTEM_ALLOCATION_SCOPE_DEVICE);

	auto next = pCreateInfo->pNext;
	while (next)
	{
		const auto type = static_cast<const VkBaseInStructure*>(next)->sType;
		switch (type)
		{
		}
		next = static_cast<const VkBaseInStructure*>(next)->pNext;
	}

	// TODO

	WrapVulkan(pipelineLayout, pPipelineLayout);
	return VK_SUCCESS;
}
