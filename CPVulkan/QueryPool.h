#pragma once
#include "Base.h"

class QueryPool final : public VulkanBase
{
public:
	~QueryPool() override = default;

	static VkResult Create(const VkQueryPoolCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkQueryPool* pQueryPool);

private:
	VkQueryType queryType{};
	uint32_t queryCount{};
	VkQueryPipelineStatisticFlags pipelineStatistics{};
};