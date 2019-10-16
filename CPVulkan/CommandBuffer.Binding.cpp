#include "CommandBuffer.h"

#include "DeviceState.h"

class Pipeline;
class PipelineLayout;

class BindPipelineCommand final : public Command
{
public:
	BindPipelineCommand(int index, Pipeline* pipeline):
		index{index},
		pipeline{pipeline}
	{
	}

	void Process(DeviceState* deviceState) override
	{
		deviceState->pipeline[index] = pipeline;
	}

private:
	int index;
	Pipeline* pipeline;
};

class SetViewportCommand final : public Command
{
public:
	SetViewportCommand(uint32_t firstViewport, std::vector<VkViewport> viewports):
		firstViewport{firstViewport},
		viewports{std::move(viewports)}
	{
	}

	void Process(DeviceState* deviceState) override
	{
		for (auto i = 0u; i < viewports.size(); i++)
		{
			deviceState->viewports[i + firstViewport] = viewports[i];
		}
	}

private:
	uint32_t firstViewport;
	std::vector<VkViewport> viewports;
};

class SetScissorCommand final : public Command
{
public:
	SetScissorCommand(uint32_t firstScissor, std::vector<VkRect2D> scissors):
		firstScissor{firstScissor},
		scissors{std::move(scissors)}
	{
	}

	void Process(DeviceState* deviceState) override
	{
		for (auto i = 0u; i < scissors.size(); i++)
		{
			deviceState->scissors[i + firstScissor] = scissors[i];
		}
	}

private:
	uint32_t firstScissor;
	std::vector<VkRect2D> scissors;
};

class BindDescriptorSetsCommand final : public Command
{
public:
	BindDescriptorSetsCommand(int pipelineIndex, PipelineLayout* pipelineLayout, uint32_t firstSet, std::vector<DescriptorSet*> descriptorSets, std::vector<uint32_t> dynamicOffsets):
		pipelineIndex{pipelineIndex},
		pipelineLayout{pipelineLayout},
		firstSet{firstSet},
		descriptorSets{std::move(descriptorSets)},
		dynamicOffsets{std::move(dynamicOffsets)}
	{
	}

	void Process(DeviceState* deviceState) override
	{
		for (auto i = 0u; i < descriptorSets.size(); i++)
		{
			deviceState->descriptorSets[i + firstSet][pipelineIndex] = descriptorSets[i];
		}

		if (!dynamicOffsets.empty())
		{
			FATAL_ERROR();
		}
	}

private:
	int pipelineIndex;
	PipelineLayout* pipelineLayout;
	uint32_t firstSet;
	std::vector<DescriptorSet*> descriptorSets;
	std::vector<uint32_t> dynamicOffsets;
};

class BindVertexBuffersCommand final : public Command
{
public:
	BindVertexBuffersCommand(uint32_t firstBinding, std::vector<Buffer*> buffers, std::vector<VkDeviceSize> bufferOffsets):
		firstBinding{firstBinding},
		buffers{std::move(buffers)},
		bufferOffsets{std::move(bufferOffsets)}
	{
	}

	void Process(DeviceState* deviceState) override
	{
		for (auto i = 0u; i < buffers.size(); i++)
		{
			deviceState->vertexBinding[i + firstBinding] = buffers[i];
			deviceState->vertexBindingOffset[i + firstBinding] = bufferOffsets[i];
		}
	}

private:
	uint32_t firstBinding;
	std::vector<Buffer*> buffers;
	std::vector<VkDeviceSize> bufferOffsets;
};

void CommandBuffer::BindPipeline(VkPipelineBindPoint pipelineBindPoint, VkPipeline pipeline)
{
	assert(state == State::Recording);
	if (pipelineBindPoint != VK_PIPELINE_BIND_POINT_GRAPHICS && pipelineBindPoint != VK_PIPELINE_BIND_POINT_COMPUTE)
	{
		FATAL_ERROR();
	}
		
	commands.push_back(std::make_unique<BindPipelineCommand>(static_cast<int>(pipelineBindPoint), UnwrapVulkan<Pipeline>(pipeline)));
}

void CommandBuffer::SetViewport(uint32_t firstViewport, uint32_t viewportCount, const VkViewport* pViewports)
{
	assert(state == State::Recording);
	std::vector<VkViewport> viewports(viewportCount);
	memcpy(viewports.data(), pViewports, sizeof(VkViewport) * viewportCount);
	commands.push_back(std::make_unique<SetViewportCommand>(firstViewport, viewports));
}

void CommandBuffer::SetScissor(uint32_t firstScissor, uint32_t scissorCount, const VkRect2D* pScissors)
{
	assert(state == State::Recording);
	std::vector<VkRect2D> scissors(scissorCount);
	memcpy(scissors.data(), pScissors, sizeof(VkRect2D) * scissorCount);
	commands.push_back(std::make_unique<SetScissorCommand>(firstScissor, scissors));
}

void CommandBuffer::BindDescriptorSets(VkPipelineBindPoint pipelineBindPoint, VkPipelineLayout layout, uint32_t firstSet, uint32_t descriptorSetCount, const VkDescriptorSet* pDescriptorSets, uint32_t dynamicOffsetCount, const uint32_t* pDynamicOffsets)
{
	assert(state == State::Recording);
	if (pipelineBindPoint != VK_PIPELINE_BIND_POINT_GRAPHICS && pipelineBindPoint != VK_PIPELINE_BIND_POINT_COMPUTE)
	{
		FATAL_ERROR();
	}

	std::vector<DescriptorSet*> descriptorSets(descriptorSetCount);
	for (auto i = 0u; i < descriptorSetCount; i++)
	{
		descriptorSets[i] = UnwrapVulkan<DescriptorSet>(pDescriptorSets[i]);
	}

	std::vector<uint32_t> dynamicOffsets(dynamicOffsetCount);
	memcpy(dynamicOffsets.data(), pDynamicOffsets, sizeof(uint32_t) * dynamicOffsetCount);

	commands.push_back(std::make_unique<BindDescriptorSetsCommand>(static_cast<int>(pipelineBindPoint), UnwrapVulkan<PipelineLayout>(layout), firstSet, descriptorSets, dynamicOffsets));
}

void CommandBuffer::BindVertexBuffers(uint32_t firstBinding, uint32_t bindingCount, const VkBuffer* pBuffers, const VkDeviceSize* pOffsets)
{
	assert(state == State::Recording);
	std::vector<Buffer*> buffers(bindingCount);
	std::vector<VkDeviceSize> bufferOffsets(bindingCount);

	for (auto i = 0u; i < bindingCount; i++)
	{
		buffers[i] = UnwrapVulkan<Buffer>(pBuffers[i]);
		bufferOffsets[i] = pOffsets[i];
	}
	
	commands.push_back(std::make_unique<BindVertexBuffersCommand>(firstBinding, buffers, bufferOffsets));
}
	