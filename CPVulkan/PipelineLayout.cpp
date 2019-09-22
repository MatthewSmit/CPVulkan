#include "PipelineLayout.h"

#include "Device.h"

#include <cassert>

VkResult PipelineLayout::Create(const VkPipelineLayoutCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkPipelineLayout* pPipelineLayout)
{
	assert(pCreateInfo->sType == VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO);
	assert(pCreateInfo->flags == 0);

	const auto pipelineLayout = Allocate<PipelineLayout>(pAllocator, VK_SYSTEM_ALLOCATION_SCOPE_DEVICE);
	if (!pipelineLayout)
	{
		return VK_ERROR_OUT_OF_HOST_MEMORY;
	}

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

VkResult Device::CreatePipelineLayout(const VkPipelineLayoutCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkPipelineLayout* pPipelineLayout)
{
	return PipelineLayout::Create(pCreateInfo, pAllocator, pPipelineLayout);
}

void Device::DestroyPipelineLayout(VkPipelineLayout pipelineLayout, const VkAllocationCallbacks* pAllocator)
{
	Free(UnwrapVulkan<PipelineLayout>(pipelineLayout), pAllocator);
}
