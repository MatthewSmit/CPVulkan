#include "CommandBuffer.h"
#include "CommandBuffer.Internal.h"

#include "Buffer.h"
#include "DeviceState.h"
#include "Platform.h"
#include "QueryPool.h"

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

	~BeginPoolCommand() override = default;

#if CV_DEBUG_LEVEL > 0
	void DebugOutput(DeviceState* deviceState) override
	{
		*deviceState->debugOutput << "BeginPool" << std::endl;
	}
#endif

	void Process(DeviceState* deviceState) override
	{
		TODO_ERROR();
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

	~EndPoolCommand() override = default;

#if CV_DEBUG_LEVEL > 0
	void DebugOutput(DeviceState* deviceState) override
	{
		*deviceState->debugOutput << "EndPool" << std::endl;
	}
#endif

	void Process(DeviceState* deviceState) override
	{
		TODO_ERROR();
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

	~ResetQueryPoolCommand() override = default;

#if CV_DEBUG_LEVEL > 0
	void DebugOutput(DeviceState* deviceState) override
	{
		*deviceState->debugOutput << "ResetQueryPool" << std::endl;
	}
#endif

	void Process(DeviceState*) override
	{
		queryPool->Reset(firstQuery, queryCount);
	}

private:
	QueryPool* queryPool;
	uint32_t firstQuery;
	uint32_t queryCount;
};

class WriteTimestampCommand final : public Command
{
public:
	WriteTimestampCommand(VkPipelineStageFlagBits pipelineStage, QueryPool* queryPool, uint32_t query) :
		pipelineStage{pipelineStage},
		queryPool{queryPool},
		query{query}
	{
	}

	~WriteTimestampCommand() override = default;

#if CV_DEBUG_LEVEL > 0
	void DebugOutput(DeviceState* deviceState) override
	{
		*deviceState->debugOutput << "WriteTimestamp: writing timestamp" <<
			" for stage " << pipelineStage <<
			" to query pool " << queryPool <<
			" with index " << query <<
			std::endl;
	}
#endif

	void Process(DeviceState*) override
	{
		queryPool->SetValue(query, Platform::GetTimestamp());
	}

private:
	VkPipelineStageFlagBits pipelineStage;
	QueryPool* queryPool;
	uint32_t query;
};

class CopyQueryPoolResultsCommand final : public Command
{
public:
	CopyQueryPoolResultsCommand(QueryPool* queryPool, uint32_t firstQuery, uint32_t queryCount, Buffer* dstBuffer, uint64_t dstOffset, uint64_t stride, VkQueryResultFlags flags) :
		queryPool{queryPool},
		firstQuery{firstQuery},
		queryCount{queryCount},
		dstBuffer{dstBuffer},
		dstOffset{dstOffset},
		stride{stride},
		flags{flags}
	{
	}

	~CopyQueryPoolResultsCommand() override = default;

#if CV_DEBUG_LEVEL > 0
	void DebugOutput(DeviceState* deviceState) override
	{
		*deviceState->debugOutput << "CopyQueryPoolResults" << std::endl;
	}
#endif

	void Process(DeviceState*) override
	{
		const auto size = stride * queryCount;
		queryPool->GetResults(firstQuery, queryCount, size, dstBuffer->getDataPtr(dstOffset, size), stride, flags);
	}

private:
	QueryPool* queryPool;
	uint32_t firstQuery;
	uint32_t queryCount;
	Buffer* dstBuffer;
	uint64_t dstOffset;
	uint64_t stride;
	VkQueryResultFlags flags;
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

void CommandBuffer::WriteTimestamp(VkPipelineStageFlagBits pipelineStage, VkQueryPool queryPool, uint32_t query)
{
	assert(state == State::Recording);
	commands.push_back(std::make_unique<WriteTimestampCommand>(pipelineStage, UnwrapVulkan<QueryPool>(queryPool), query));
}

void CommandBuffer::CopyQueryPoolResults(VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount, VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize stride, VkQueryResultFlags flags)
{
	assert(state == State::Recording);
	commands.push_back(std::make_unique<CopyQueryPoolResultsCommand>(UnwrapVulkan<QueryPool>(queryPool), firstQuery, queryCount, UnwrapVulkan<Buffer>(dstBuffer), dstOffset, stride, flags));
}