#include "PipelineCache.h"

#include "Device.h"

#include <cassert>

VkResult PipelineCache::Create(const VkPipelineCacheCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkPipelineCache* pPipelineCache)
{
	assert(pCreateInfo->sType == VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO);
	assert(pCreateInfo->pNext == nullptr);
	assert(pCreateInfo->flags == 0);

	const auto pipelineCache = Allocate<PipelineCache>(pAllocator, VK_SYSTEM_ALLOCATION_SCOPE_DEVICE);

	// TODO

	WrapVulkan(pipelineCache, pPipelineCache);
	return VK_SUCCESS;
}

VkResult Device::CreatePipelineCache(const VkPipelineCacheCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkPipelineCache* pPipelineCache)
{
	return PipelineCache::Create(pCreateInfo, pAllocator, pPipelineCache);
}

void Device::DestroyPipelineCache(VkPipelineCache pipelineCache, const VkAllocationCallbacks* pAllocator)
{
	Free(UnwrapVulkan<PipelineCache>(pipelineCache), pAllocator);
}
