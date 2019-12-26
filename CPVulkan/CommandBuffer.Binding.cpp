#include "CommandBuffer.h"
#include "CommandBuffer.Internal.h"

#include "Buffer.h"
#include "DebugHelper.h"
#include "DescriptorSet.h"
#include "DeviceState.h"
#include "PipelineLayout.h"

#include <fstream>

class Pipeline;
class PipelineLayout;

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

	CommonPipelineState& GetPipelineState(DeviceState* deviceState, VkPipelineBindPoint bindPoint)
	{
		switch (bindPoint)
		{
		case VK_PIPELINE_BIND_POINT_GRAPHICS:
			return deviceState->graphicsPipelineState;
			
		case VK_PIPELINE_BIND_POINT_COMPUTE:
			return deviceState->computePipelineState;
			
		case VK_PIPELINE_BIND_POINT_RAY_TRACING_NV:
			return deviceState->rayTracingPipelineState;
			
		default:
			FATAL_ERROR();
		}
	}

	void Process(DeviceState* deviceState) override
	{
		auto& pipelineState = GetPipelineState(deviceState, bindPoint);
		for (auto i = 0u, dynamic = 0u; i < descriptorSets.size(); i++)
		{
			const auto& descriptorSet = pipelineState.descriptorSets[i + firstSet] = descriptorSets[i];
			for (auto j = 0u; j < descriptorSet->getNumberBindings(); j++)
			{
				VkDescriptorType descriptorType;
				const Descriptor* value;
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
		deviceState->graphicsPipelineState.indexBinding = buffer;
		deviceState->graphicsPipelineState.indexBindingOffset = offset;
		switch (indexType)
		{
		case VK_INDEX_TYPE_UINT8_EXT:
			deviceState->graphicsPipelineState.indexBindingStride = 1;
			break;

		case VK_INDEX_TYPE_UINT16:
			deviceState->graphicsPipelineState.indexBindingStride = 2;
			break;

		case VK_INDEX_TYPE_UINT32:
			deviceState->graphicsPipelineState.indexBindingStride = 4;
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
			deviceState->graphicsPipelineState.vertexBinding[i + firstBinding] = buffers[i];
			deviceState->graphicsPipelineState.nativeState.vertexBindingPtr[i + firstBinding] = buffers[i]->getDataPtr(bufferOffsets[i], 0);
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
	commands.push_back(std::make_unique<FunctionCommand>([pipelineBindPoint, pipeline](DeviceState* deviceState)
	{
		switch (pipelineBindPoint)
		{
		case VK_PIPELINE_BIND_POINT_GRAPHICS:
			deviceState->graphicsPipelineState.pipeline = UnwrapVulkan<GraphicsPipeline>(pipeline);
			break;
			
		case VK_PIPELINE_BIND_POINT_COMPUTE:
			deviceState->computePipelineState.pipeline = UnwrapVulkan<ComputePipeline>(pipeline);
			break;

#if defined(VK_NV_ray_tracing)
		case VK_PIPELINE_BIND_POINT_RAY_TRACING_NV:
			TODO_ERROR();
#endif
			
		default:
			FATAL_ERROR();
		}
	}));
}

void CommandBuffer::SetViewport(uint32_t firstViewport, uint32_t viewportCount, const VkViewport* pViewports)
{
	assert(state == State::Recording);
	std::vector<VkViewport> viewports(viewportCount);
	memcpy(viewports.data(), pViewports, sizeof(VkViewport) * viewportCount);
	commands.emplace_back(std::make_unique<FunctionCommand>([firstViewport, viewports](DeviceState* deviceState)
	{
		for (auto i = 0u; i < viewports.size(); i++)
		{
			deviceState->graphicsPipelineState.dynamicState.viewports[i + firstViewport] = viewports[i];
		}
	}));
}

void CommandBuffer::SetScissor(uint32_t firstScissor, uint32_t scissorCount, const VkRect2D* pScissors)
{
	assert(state == State::Recording);
	std::vector<VkRect2D> scissors(scissorCount);
	memcpy(scissors.data(), pScissors, sizeof(VkRect2D) * scissorCount);
	commands.emplace_back(std::make_unique<FunctionCommand>([firstScissor, scissors](DeviceState* deviceState)
	{
		for (auto i = 0u; i < scissors.size(); i++)
		{
			deviceState->graphicsPipelineState.dynamicState.scissors[i + firstScissor] = scissors[i];
		}
	}));
}

void CommandBuffer::SetDepthBounds(float minDepthBounds, float maxDepthBounds)
{
	assert(state == State::Recording);
	commands.emplace_back(std::make_unique<FunctionCommand>([minDepthBounds, maxDepthBounds](DeviceState* deviceState)
	{
		deviceState->graphicsPipelineState.dynamicState.minDepthBounds = minDepthBounds;
		deviceState->graphicsPipelineState.dynamicState.maxDepthBounds = maxDepthBounds;
	}));
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

#if defined(VK_KHR_push_descriptor)
void CommandBuffer::PushDescriptorSet(VkPipelineBindPoint pipelineBindPoint, VkPipelineLayout layout, uint32_t set, uint32_t descriptorWriteCount, const VkWriteDescriptorSet* pDescriptorWrites)
{
	assert(state == State::Recording);

	// TODO: Need to do deep copy
	std::vector<VkWriteDescriptorSet> descriptorWrites(descriptorWriteCount);
	memcpy(descriptorWrites.data(), pDescriptorWrites, sizeof(VkWriteDescriptorSet) * descriptorWriteCount);
	
	commands.push_back(std::make_unique<FunctionCommand>([pipelineBindPoint, layout, set, descriptorWrites](DeviceState* deviceState)
	{
		auto& currentSet = deviceState->pipelineState[pipelineBindPoint].pushDescriptorSets[set];
		if (!currentSet)
		{
			currentSet = new DescriptorSet();
		}
		
		currentSet->Initialise(UnwrapVulkan<PipelineLayout>(layout)->getDescriptorSetLayouts()[0]);
		
		for (const auto& descriptorWrite : descriptorWrites)
		{
			assert(descriptorWrite.sType == VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET);

			auto next = static_cast<const VkBaseInStructure*>(descriptorWrite.pNext);
			while (next)
			{
				const auto type = next->sType;
				switch (type)
				{
				case VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_NV:
					TODO_ERROR();

				case VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_INLINE_UNIFORM_BLOCK_EXT:
					TODO_ERROR();

				default:
					break;
				}
				next = next->pNext;
			}

			currentSet->Update(descriptorWrite);
		}
		deviceState->pipelineState[pipelineBindPoint].descriptorSets[set] = currentSet;
	}));
}
#endif