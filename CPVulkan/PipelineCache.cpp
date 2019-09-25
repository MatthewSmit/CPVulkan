#include "PipelineCache.h"

#include "Device.h"

#include <cassert>

VkResult PipelineCache::GetData(size_t* pDataSize, void* pData)
{
	FATAL_ERROR();
}

VkResult Device::GetPipelineCacheData(VkPipelineCache pipelineCache, size_t* pDataSize, void* pData)
{
	return UnwrapVulkan<PipelineCache>(pipelineCache)->GetData(pDataSize, pData);
}

VkResult PipelineCache::Create(const VkPipelineCacheCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkPipelineCache* pPipelineCache)
{
	assert(pCreateInfo->sType == VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO);
	assert(pCreateInfo->flags == 0);

	const auto pipelineCache = Allocate<PipelineCache>(pAllocator, VK_SYSTEM_ALLOCATION_SCOPE_DEVICE);
	if (!pipelineCache)
	{
		return VK_ERROR_OUT_OF_HOST_MEMORY;
	}

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

VkResult Device::MergePipelineCaches(VkPipelineCache dstCache, uint32_t srcCacheCount, const VkPipelineCache* pSrcCaches)
{
	FATAL_ERROR();
}
