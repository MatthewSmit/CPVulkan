#include "PipelineLayout.h"

#include <cassert>

VkResult PipelineLayout::Create(const VkPipelineLayoutCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkPipelineLayout* pPipelineLayout)
{
	assert(pCreateInfo->sType == VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO);

	auto pipelineLayout = Allocate<PipelineLayout>(pAllocator);

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

	// TODO

	*pPipelineLayout = reinterpret_cast<VkPipelineLayout>(pipelineLayout);
	return VK_SUCCESS;
}
