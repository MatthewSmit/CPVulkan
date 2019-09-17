#pragma once
#include "Base.h"

class QueryPool final
{
public:
	QueryPool() = default;
	QueryPool(const QueryPool&) = delete;
	QueryPool(QueryPool&&) = delete;
	~QueryPool() = default;

	QueryPool& operator=(const QueryPool&) = delete;
	QueryPool&& operator=(const QueryPool&&) = delete;

	static VkResult Create(const VkQueryPoolCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkQueryPool* pQueryPool);

private:
	VkQueryType queryType{};
	uint32_t queryCount{};
	VkQueryPipelineStatisticFlags pipelineStatistics{};
};