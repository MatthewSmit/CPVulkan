#pragma once
#include "Base.h"

class PipelineCache final : public VulkanBase
{
public:
	~PipelineCache() override = default;
	
	static VkResult Create(const VkPipelineCacheCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkPipelineCache* pPipelineCache);
};