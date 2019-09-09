#pragma once
#include "Base.h"

class PipelineLayout final : public VulkanBase
{
public:
	~PipelineLayout() override = default;
	
	static VkResult Create(const VkPipelineLayoutCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkPipelineLayout* pPipelineLayout);
};