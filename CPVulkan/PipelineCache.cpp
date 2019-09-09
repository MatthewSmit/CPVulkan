#include "PipelineCache.h"

#include <cassert>

VkResult PipelineCache::Create(const VkPipelineCacheCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkPipelineCache* pPipelineCache)
{
	assert(pCreateInfo->sType == VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO);

	auto pipelineCache = Allocate<PipelineCache>(pAllocator);

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

	*pPipelineCache = reinterpret_cast<VkPipelineCache>(pipelineCache);
	return VK_SUCCESS;
}
