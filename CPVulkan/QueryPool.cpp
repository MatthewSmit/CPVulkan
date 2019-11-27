#include "QueryPool.h"

#include "Device.h"

#include <cassert>

QueryPool::~QueryPool()
{
	switch (queryType)
	{
	case VK_QUERY_TYPE_OCCLUSION:
	case VK_QUERY_TYPE_TIMESTAMP:
	case VK_QUERY_TYPE_TRANSFORM_FEEDBACK_STREAM_EXT:
	case VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_NV:
	case VK_QUERY_TYPE_PERFORMANCE_QUERY_INTEL:
		delete[] values.u64;
		break;
		
	case VK_QUERY_TYPE_PIPELINE_STATISTICS: FATAL_ERROR();

	default: FATAL_ERROR();
	}
}

void Device::DestroyQueryPool(VkQueryPool queryPool, const VkAllocationCallbacks* pAllocator)
{
	if (queryPool)
	{
		Free(UnwrapVulkan<QueryPool>(queryPool), pAllocator);
	}
}

void QueryPool::Reset(uint32_t firstQuery, uint32_t queryCount)
{
	for (auto i = 0u; i < queryCount; i++)
	{
		avaliablity[i + firstQuery] = false;
		values.u64[i + firstQuery] = 0;
	}
}

void Device::ResetQueryPool(VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount)
{
	UnwrapVulkan<QueryPool>(queryPool)->Reset(firstQuery, queryCount);
}

void QueryPool::SetValue(uint32_t query, uint64_t timestamp)
{
	assert(queryType != VK_QUERY_TYPE_PIPELINE_STATISTICS);
	values.u64[query] = timestamp;
	avaliablity[query] = true;
}

VkResult QueryPool::GetResults(uint32_t firstQuery, uint32_t queryCount, size_t dataSize, void* pData, uint64_t stride, VkQueryResultFlags flags)
{
	if (queryType == VK_QUERY_TYPE_PIPELINE_STATISTICS)
	{
		FATAL_ERROR();
	}

	auto result = VK_SUCCESS;
	
	for (auto i = 0u; i < queryCount; i++)
	{
		const auto dataPointer = static_cast<uint8_t*>(pData) + i * stride;

		auto valid = true;
		auto value = values.u64[firstQuery + i];
		if (!avaliablity[firstQuery + i])
		{
			if (flags & VK_QUERY_RESULT_WAIT_BIT)
			{
				FATAL_ERROR();
			}

			result = VK_NOT_READY;
			if (flags & VK_QUERY_RESULT_PARTIAL_BIT)
			{
				value = values.u64[firstQuery + i];
			}
			else
			{
				valid = false;
			}
		}

		if (valid)
		{
			if (flags & VK_QUERY_RESULT_64_BIT)
			{
				*reinterpret_cast<uint64_t*>(dataPointer) = value;
			}
			else
			{
				*reinterpret_cast<uint32_t*>(dataPointer) = static_cast<uint32_t>(value);
			}
		}

		if (flags & VK_QUERY_RESULT_WITH_AVAILABILITY_BIT)
		{
			if (flags & VK_QUERY_RESULT_64_BIT)
			{
				*reinterpret_cast<uint64_t*>(dataPointer + 8) = avaliablity[firstQuery + i];
			}
			else
			{
				*reinterpret_cast<uint32_t*>(dataPointer + 4) = avaliablity[firstQuery + i];
			}
		}
	}

	return result;
}

VkResult Device::GetQueryPoolResults(VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount, size_t dataSize, void* pData, VkDeviceSize stride, VkQueryResultFlags flags)
{
	return UnwrapVulkan<QueryPool>(queryPool)->GetResults(firstQuery, queryCount, dataSize, pData, stride, flags);
}

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

	switch (queryPool->queryType)
	{
	case VK_QUERY_TYPE_OCCLUSION:
	case VK_QUERY_TYPE_TIMESTAMP:
	case VK_QUERY_TYPE_TRANSFORM_FEEDBACK_STREAM_EXT:
	case VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_NV:
	case VK_QUERY_TYPE_PERFORMANCE_QUERY_INTEL:
		queryPool->values.u64 = new uint64_t[queryPool->queryCount];
		break;

	case VK_QUERY_TYPE_PIPELINE_STATISTICS: FATAL_ERROR();

	default: FATAL_ERROR();
	}
	
	queryPool->avaliablity.resize(queryPool->queryCount);

	WrapVulkan(queryPool, pQueryPool);
	return VK_SUCCESS;
}

VkResult Device::CreateQueryPool(const VkQueryPoolCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkQueryPool* pQueryPool)
{
	return QueryPool::Create(pCreateInfo, pAllocator, pQueryPool);
}
