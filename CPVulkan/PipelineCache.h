#pragma once
#include "Base.h"

class PipelineCache final
{
public:
	PipelineCache() = default;
	PipelineCache(const PipelineCache&) = delete;
	PipelineCache(PipelineCache&&) = delete;
	~PipelineCache() = default;

	PipelineCache& operator=(const PipelineCache&) = delete;
	PipelineCache&& operator=(const PipelineCache&&) = delete;

	static VkResult Create(const VkPipelineCacheCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkPipelineCache* pPipelineCache);
};