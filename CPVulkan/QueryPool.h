#pragma once
#include "Base.h"

class QueryPool final
{
public:
	QueryPool() = default;
	QueryPool(const QueryPool&) = delete;
	QueryPool(QueryPool&&) = delete;
	~QueryPool();

	QueryPool& operator=(const QueryPool&) = delete;
	QueryPool&& operator=(const QueryPool&&) = delete;

	void OnDelete(const VkAllocationCallbacks*)
	{
	}

	void Reset(uint32_t firstQuery, uint32_t queryCount);
	
	void SetValue(uint32_t query, uint64_t value);
	
	VkResult GetResults(uint32_t firstQuery, uint32_t queryCount, size_t dataSize, void* pData, uint64_t stride, VkQueryResultFlags flags);

	static VkResult Create(const VkQueryPoolCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkQueryPool* pQueryPool);

private:
	VkQueryType queryType{};
	uint32_t queryCount{};
	VkQueryPipelineStatisticFlags pipelineStatistics{};

	std::vector<bool> avaliablity{};

	union
	{
		uint64_t* u64;
	} values;
};