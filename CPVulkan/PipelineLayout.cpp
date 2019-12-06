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

	pipelineLayout->descriptorSetLayouts = std::vector<DescriptorSetLayout*>(pCreateInfo->setLayoutCount);
	for (auto i = 0u; i < pCreateInfo->setLayoutCount; i++)
	{
		pipelineLayout->descriptorSetLayouts[i] = UnwrapVulkan<DescriptorSetLayout>(pCreateInfo->pSetLayouts[i]);
	}

	pipelineLayout->pushConstantRanges = std::vector<VkPushConstantRange>(pCreateInfo->pushConstantRangeCount);
	for (auto i = 0u; i < pCreateInfo->pushConstantRangeCount; i++)
	{
		pipelineLayout->pushConstantRanges[i] = pCreateInfo->pPushConstantRanges[i];
	}
	
	WrapVulkan(pipelineLayout, pPipelineLayout);
	return VK_SUCCESS;
}

VkResult Device::CreatePipelineLayout(const VkPipelineLayoutCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkPipelineLayout* pPipelineLayout)
{
	return PipelineLayout::Create(pCreateInfo, pAllocator, pPipelineLayout);
}

void Device::DestroyPipelineLayout(VkPipelineLayout pipelineLayout, const VkAllocationCallbacks* pAllocator)
{
	if (pipelineLayout)
	{
		Free(UnwrapVulkan<PipelineLayout>(pipelineLayout), pAllocator);
	}
}
