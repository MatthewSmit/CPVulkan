#include "QueryPool.h"

#include "Device.h"

#include <cassert>

VkResult QueryPool::Create(const VkQueryPoolCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkQueryPool* pQueryPool)
{
	assert(pCreateInfo->sType == VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO);

	auto queryPool = Allocate<QueryPool>(pAllocator, VK_SYSTEM_ALLOCATION_SCOPE_DEVICE);

	auto next = pCreateInfo->pNext;
	while (next)
	{
		const auto type = static_cast<const VkBaseInStructure*>(next)->sType;
		switch (type)
		{
		}
		next = static_cast<const VkBaseInStructure*>(next)->pNext;
	}

	if (pCreateInfo->flags)
	{
		FATAL_ERROR();
	}
	
	queryPool->queryType = pCreateInfo->queryType;
	queryPool->queryCount = pCreateInfo->queryCount;
	queryPool->pipelineStatistics = pCreateInfo->pipelineStatistics;
	
	*pQueryPool = reinterpret_cast<VkQueryPool>(queryPool);
	return VK_SUCCESS;
}

VkResult Device::CreateQueryPool(const VkQueryPoolCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkQueryPool* pQueryPool)
{
	return QueryPool::Create(pCreateInfo, pAllocator, pQueryPool);
}

void Device::DestroyQueryPool(VkQueryPool queryPool, const VkAllocationCallbacks* pAllocator)
{
	Free(reinterpret_cast<QueryPool*>(queryPool), pAllocator);
}