#include "QueryPool.h"

#include "Device.h"

#include <cassert>

VkResult QueryPool::Create(const VkQueryPoolCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkQueryPool* pQueryPool)
{
	assert(pCreateInfo->sType == VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO);

	auto queryPool = Allocate<QueryPool>(pAllocator, VK_SYSTEM_ALLOCATION_SCOPE_DEVICE);
	if (!queryPool)
	{
		return VK_ERROR_OUT_OF_HOST_MEMORY;
	}

	auto next = static_cast<const VkBaseInStructure*>(pCreateInfo->pNext);
	while (next)
	{
		const auto type = next->sType;
		switch (type)
		{
		default:
			break;
		}
		next = next->pNext;
	}

	if (pCreateInfo->flags)
	{
		FATAL_ERROR();
	}
	
	queryPool->queryType = pCreateInfo->queryType;
	queryPool->queryCount = pCreateInfo->queryCount;
	queryPool->pipelineStatistics = pCreateInfo->pipelineStatistics;

	WrapVulkan(queryPool, pQueryPool);
	return VK_SUCCESS;
}

VkResult Device::CreateQueryPool(const VkQueryPoolCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkQueryPool* pQueryPool)
{
	return QueryPool::Create(pCreateInfo, pAllocator, pQueryPool);
}

void Device::DestroyQueryPool(VkQueryPool queryPool, const VkAllocationCallbacks* pAllocator)
{
	if (queryPool)
	{
		Free(UnwrapVulkan<QueryPool>(queryPool), pAllocator);
	}
}

VkResult Device::GetQueryPoolResults(VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount, size_t dataSize, void* pData, VkDeviceSize stride, VkQueryResultFlags flags)
{
	FATAL_ERROR();
}
