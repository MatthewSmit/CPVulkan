#include "PipelineLayout.h"

#include <cassert>

VkResult PipelineLayout::Create(const VkPipelineLayoutCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkPipelineLayout* pPipelineLayout)
{
	assert(pCreateInfo->sType == VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO);

	auto pipelineLayout = Allocate<PipelineLayout>(pAllocator, VK_SYSTEM_ALLOCATION_SCOPE_DEVICE);

	auto next = pCreateInfo->pNext;
	while (next)
	{
		const auto type = static_cast<const VkBaseInStructure*>(next)->sType;
		switch (type)
		{
		default:
			next = static_cast<const VkBaseInStructure*>(next)->pNext;
			break;
		}
	}

	if (pCreateInfo->flags)
	{
		FATAL_ERROR();
	}

	// TODO

	*pPipelineLayout = reinterpret_cast<VkPipelineLayout>(pipelineLayout);
	return VK_SUCCESS;
}
