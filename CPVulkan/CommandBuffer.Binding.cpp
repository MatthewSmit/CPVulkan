#include "CommandBuffer.h"

#include "Buffer.h"
#include "DebugHelper.h"
#include "DescriptorSet.h"
#include "DeviceState.h"

#include <fstream>

class Pipeline;
class PipelineLayout;

class BindPipelineCommand final : public Command
{
public:
	BindPipelineCommand(VkPipelineBindPoint bindPoint, Pipeline* pipeline):
		bindPoint{bindPoint},
		pipeline{pipeline}
	{
	}

	~BindPipelineCommand() override = default;

#if CV_DEBUG_LEVEL > 0
	void DebugOutput(DeviceState* deviceState) override
	{
		*deviceState->debugOutput << "BindPipeline: Binding pipeline for " << bindPoint << " to " << pipeline << std::endl;
	}
#endif

	void Process(DeviceState* deviceState) override
	{
		gsl::at(deviceState->pipelineState, bindPoint).pipeline = pipeline;
	}

private:
	VkPipelineBindPoint bindPoint;
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

	~SetViewportCommand() override = default;

#if CV_DEBUG_LEVEL > 0
	void DebugOutput(DeviceState* deviceState) override
	{
		*deviceState->debugOutput << "SetViewport: Setting viewports starting at " << firstViewport << " with " << viewports << std::endl;
	}
#endif

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

	~SetScissorCommand() override = default;

#if CV_DEBUG_LEVEL > 0
	void DebugOutput(DeviceState* deviceState) override
	{
		*deviceState->debugOutput << "SetScissor: Setting scissor regions starting at " << firstScissor << " with " << scissors << std::endl;
	}
#endif

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
	BindDescriptorSetsCommand(VkPipelineBindPoint bindPoint, PipelineLayout* pipelineLayout, uint32_t firstSet, std::vector<DescriptorSet*> descriptorSets, std::vector<uint32_t> dynamicOffsets):
		bindPoint{bindPoint},
		pipelineLayout{pipelineLayout},
		firstSet{firstSet},
		descriptorSets{std::move(descriptorSets)},
		dynamicOffsets{std::move(dynamicOffsets)}
	{
	}

	~BindDescriptorSetsCommand() override = default;

#if CV_DEBUG_LEVEL > 0
	void DebugOutput(DeviceState* deviceState) override
	{
		*deviceState->debugOutput << "BindPipeline: Binding pipeline for " << bindPoint <<
			" starting at " << firstSet <<
			" with " << pipelineLayout <<
			" to " << descriptorSets <<
			" at " << dynamicOffsets << std::endl;
	}
#endif

	void Process(DeviceState* deviceState) override
	{
		auto& pipelineState = deviceState->pipelineState[bindPoint];
		for (auto i = 0u, dynamic = 0u; i < descriptorSets.size(); i++)
		{
			const auto& descriptorSet = pipelineState.descriptorSets[i + firstSet] = descriptorSets[i];
			for (auto j = 0u; j < descriptorSet->getNumberBindings(); j++)
			{
				VkDescriptorType descriptorType;
				Descriptor* value;
				descriptorSet->getBinding(j, descriptorType, value);
				if (descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC || descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC)
				{
					assert(dynamic < dynamicOffsets.size());
					pipelineState.descriptorSetDynamicOffset[i + firstSet][j] = dynamicOffsets[dynamic];
					dynamic++;
				}
			}
		}
	}

private:
	VkPipelineBindPoint bindPoint;
	PipelineLayout* pipelineLayout;
	uint32_t firstSet;
	std::vector<DescriptorSet*> descriptorSets;
	std::vector<uint32_t> dynamicOffsets;
};

class BindIndexBufferCommand final : public Command
{
public:
	BindIndexBufferCommand(Buffer* buffer, uint64_t offset, VkIndexType indexType) :
		buffer{buffer},
		offset{offset},
		indexType{indexType}
	{
	}

	~BindIndexBufferCommand() override = default;

#if CV_DEBUG_LEVEL > 0
	void DebugOutput(DeviceState* deviceState) override
	{
		*deviceState->debugOutput << "BindIndexBuffer: Binding index buffer " <<
			" to " << buffer <<
			" at " << offset << 
			" of type " << indexType << 
			std::endl;
	}
#endif

	void Process(DeviceState* deviceState) override
	{
		deviceState->indexBinding = buffer;
		deviceState->indexBindingOffset = offset;
		switch (indexType)
		{
		case VK_INDEX_TYPE_UINT8_EXT:
			deviceState->indexBindingStride = 1;
			break;

		case VK_INDEX_TYPE_UINT16:
			deviceState->indexBindingStride = 2;
			break;

		case VK_INDEX_TYPE_UINT32:
			deviceState->indexBindingStride = 4;
			break;

		default:
			FATAL_ERROR();
		}
	}

private:
	Buffer* buffer;
	uint64_t offset;
	VkIndexType indexType;
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

	~BindVertexBuffersCommand() override = default;

#if CV_DEBUG_LEVEL > 0
	void DebugOutput(DeviceState* deviceState) override
	{
		*deviceState->debugOutput << "BindVertexBuffers: Binding vertex buffers " <<
			" starting at " << firstBinding <<
			" to " << buffers <<
			" at " << bufferOffsets <<
			std::endl;
	}
#endif

	void Process(DeviceState* deviceState) override
	{
		for (auto i = 0u; i < buffers.size(); i++)
		{
			deviceState->vertexBinding[i + firstBinding] = buffers[i];
			deviceState->graphicsPipelineState.vertexBinding[i + firstBinding] = buffers[i]->getDataPtr(bufferOffsets[i], 0);
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
	commands.push_back(std::make_unique<BindPipelineCommand>(pipelineBindPoint, UnwrapVulkan<Pipeline>(pipeline)));
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
	std::vector<DescriptorSet*> descriptorSets(descriptorSetCount);
	for (auto i = 0u; i < descriptorSetCount; i++)
	{
		descriptorSets[i] = UnwrapVulkan<DescriptorSet>(pDescriptorSets[i]);
	}

	std::vector<uint32_t> dynamicOffsets(dynamicOffsetCount);
	memcpy(dynamicOffsets.data(), pDynamicOffsets, sizeof(uint32_t) * dynamicOffsetCount);

	commands.push_back(std::make_unique<BindDescriptorSetsCommand>(pipelineBindPoint, UnwrapVulkan<PipelineLayout>(layout), firstSet, descriptorSets, dynamicOffsets));
}

void CommandBuffer::BindIndexBuffer(VkBuffer buffer, VkDeviceSize offset, VkIndexType indexType)
{
	assert(state == State::Recording);
	commands.push_back(std::make_unique<BindIndexBufferCommand>(UnwrapVulkan<Buffer>(buffer), offset, indexType));
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
	