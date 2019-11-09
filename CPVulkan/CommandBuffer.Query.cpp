#include "CommandBuffer.h"
#include "CommandBuffer.Internal.h"

#include <fstream>

class BeginPoolCommand final : public Command
{
public:
	BeginPoolCommand(QueryPool* queryPool, uint32_t query, VkQueryControlFlags flags) :
		queryPool{queryPool},
		query{query},
		flags{flags}
	{
	}

#if CV_DEBUG_LEVEL > 0
	void DebugOutput(DeviceState* deviceState) override
	{
		FATAL_ERROR();
	}
#endif

	void Process(DeviceState* deviceState) override
	{
		FATAL_ERROR();
	}
	
private:
	QueryPool* queryPool;
	uint32_t query;
	VkQueryControlFlags flags;
};

class EndPoolCommand final : public Command
{
public:
	EndPoolCommand(QueryPool* queryPool, uint32_t query):
		queryPool{queryPool},
		query{query}
	{
	}

#if CV_DEBUG_LEVEL > 0
	void DebugOutput(DeviceState* deviceState) override
	{
		FATAL_ERROR();
	}
#endif

	void Process(DeviceState* deviceState) override
	{
		FATAL_ERROR();
	}

private:
	QueryPool* queryPool;
	uint32_t query;
};

class ResetQueryPoolCommand final : public Command
{
public:
	ResetQueryPoolCommand(QueryPool* queryPool, uint32_t firstQuery, uint32_t queryCount):
		queryPool{queryPool},
		firstQuery{firstQuery},
		queryCount{queryCount}
	{
	}

#if CV_DEBUG_LEVEL > 0
	void DebugOutput(DeviceState* deviceState) override
	{
		FATAL_ERROR();
	}
#endif

	void Process(DeviceState* deviceState) override
	{
		FATAL_ERROR();
	}

private:
	QueryPool* queryPool;
	uint32_t firstQuery;
	uint32_t queryCount;
};

void CommandBuffer::BeginQuery(VkQueryPool queryPool, uint32_t query, VkQueryControlFlags flags)
{
	assert(state == State::Recording);
	commands.push_back(std::make_unique<BeginPoolCommand>(UnwrapVulkan<QueryPool>(queryPool), query, flags));
}

void CommandBuffer::EndQuery(VkQueryPool queryPool, uint32_t query)
{
	assert(state == State::Recording);
	commands.push_back(std::make_unique<EndPoolCommand>(UnwrapVulkan<QueryPool>(queryPool), query));
}

void CommandBuffer::ResetQueryPool(VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount)
{
	assert(state == State::Recording);
	commands.push_back(std::make_unique<ResetQueryPoolCommand>(UnwrapVulkan<QueryPool>(queryPool), firstQuery, queryCount));
}
