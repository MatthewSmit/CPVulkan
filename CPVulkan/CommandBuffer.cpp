#include "CommandBuffer.h"

#include "DeviceState.h"
#include "Formats.h"
#include "Framebuffer.h"
#include "Image.h"
#include "Pipeline.h"
#include "RenderPass.h"

#include <cassert>

class PipelineLayout;

template <typename Size>
static void SetPixelUNorm(void* data, const FormatInformation& information, uint32_t x, uint32_t y, uint32_t z, uint32_t width, uint32_t height, uint32_t depth, float values[4])
{
	static_assert(std::numeric_limits<Size>::is_integer);
	static_assert(!std::numeric_limits<Size>::is_signed);
	
	if (information.ElementSize != sizeof(Size))
	{
		FATAL_ERROR();
	}

	auto pixel = reinterpret_cast<Size*>(reinterpret_cast<uint8_t*>(data) +
		static_cast<uint64_t>(z) * (width * height * information.TotalSize) +
		static_cast<uint64_t>(y) * (width * information.TotalSize) +
		static_cast<uint64_t>(x) * information.TotalSize);

	if (information.RedOffset != -1) pixel[information.RedOffset] = static_cast<uint32_t>(values[0] * std::numeric_limits<Size>::max());
	if (information.GreenOffset != -1) pixel[information.GreenOffset] = static_cast<uint32_t>(values[1] * std::numeric_limits<Size>::max());
	if (information.BlueOffset != -1) pixel[information.BlueOffset] = static_cast<uint32_t>(values[2] * std::numeric_limits<Size>::max());
	if (information.AlphaOffset != -1) pixel[information.AlphaOffset] = static_cast<uint32_t>(values[3] * std::numeric_limits<Size>::max());
}

static void ClearImage(Image* image, VkFormat format, VkClearColorValue colour)
{
	const auto& information = GetFormatInformation(format);
	for (auto z = 0u; z < image->getDepth(); z++)
	{
		for (auto y = 0u; y < image->getHeight(); y++)
		{
			for (auto x = 0u; x < image->getWidth(); x++)
			{
				if (information.BaseType == BaseType::UNorm)
				{
					if (information.ElementSize == 1)
					{
						SetPixelUNorm<uint8_t>(image->getData(), information, x, y, z, image->getWidth(), image->getHeight(), image->getDepth(), colour.float32);
					}
					else
					{
						FATAL_ERROR();
					}
				}
				else
				{
					FATAL_ERROR();
				}
			}
		}
	}
}

static void ClearImage(Image* image, VkFormat format, VkClearDepthStencilValue colour)
{
	const auto& information = GetFormatInformation(format);
	
	float values[4];
	values[0] = colour.depth;
	values[1] = colour.stencil;
	for (auto z = 0u; z < image->getDepth(); z++)
	{
		for (auto y = 0u; y < image->getHeight(); y++)
		{
			for (auto x = 0u; x < image->getWidth(); x++)
			{
				if (information.BaseType == BaseType::UNorm)
				{
					if (information.ElementSize == 2)
					{
						SetPixelUNorm<uint16_t>(image->getData(), information, x, y, z, image->getWidth(), image->getHeight(), image->getDepth(), values);
					}
					else
					{
						FATAL_ERROR();
					}
				}
				else
				{
					FATAL_ERROR();
				}
			}
		}
	}
}

static void ProcessVertexShader(DeviceState* deviceState, uint32_t vertexCount)
{
	const auto& shaderStage = deviceState->pipeline[0]->getShaderStage(0);
	const auto& inputAssembly = deviceState->pipeline[0]->getInputAssemblyState();
	const auto& vertexInput = deviceState->pipeline[0]->getVertexInputState();
	FATAL_ERROR();
}

static void Draw(DeviceState* deviceState, uint32_t vertexCount)
{
	ProcessVertexShader(deviceState, vertexCount);
	FATAL_ERROR();
}

class Command
{
public:
	virtual ~Command() = default;
	
	virtual void Process(DeviceState* deviceState) = 0;
};

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
	BindDescriptorSetsCommand(int index, PipelineLayout* pipelineLayout, uint32_t firstSet, std::vector<DescriptorSet*> descriptorSets, std::vector<uint32_t> dynamicOffsets):
		index{index},
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
			deviceState->descriptorSets[index][i + firstSet] = descriptorSets[i];
		}

		if (!dynamicOffsets.empty())
		{
			FATAL_ERROR();
		}
	}

private:
	int index;
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

class DrawCommand final : public Command
{
public:
	DrawCommand(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance):
		vertexCount{vertexCount},
		instanceCount{instanceCount},
		firstVertex{firstVertex},
		firstInstance{firstInstance}
	{
	}

	void Process(DeviceState* deviceState) override
	{
		if (instanceCount != 1)
		{
			FATAL_ERROR();
		}

		if (firstVertex != 0)
		{
			FATAL_ERROR();
		}

		if (firstInstance != 0)
		{
			FATAL_ERROR();
		}

		Draw(deviceState, vertexCount);
	}

private:
	uint32_t vertexCount;
	uint32_t instanceCount;
	uint32_t firstVertex;
	uint32_t firstInstance;
};

class BeginRenderPassCommand final : public Command
{
public:
	BeginRenderPassCommand(RenderPass* renderPass, Framebuffer* framebuffer, VkRect2D renderArea, std::vector<VkClearValue> clearValues):
		renderPass{renderPass},
		framebuffer{framebuffer},
		renderArea{renderArea},
		clearValues{std::move(clearValues)}
	{
	}

	void Process(DeviceState* deviceState) override
	{
		if (renderPass->getSubpasses().size() != 1)
		{
			FATAL_ERROR();
		}

		for (auto i = 0u; i < renderPass->getSubpasses()[0].ColourAttachments.size(); i++)
		{
			auto attachmentReference = renderPass->getSubpasses()[0].ColourAttachments[i];
			if (attachmentReference.attachment != VK_ATTACHMENT_UNUSED)
			{
				auto attachment = renderPass->getAttachments()[attachmentReference.attachment];
				auto imageView = framebuffer->getAttachments()[attachmentReference.attachment];
				ClearImage(imageView->getImage(), attachment.format, clearValues[attachmentReference.attachment].color);
			}
		}

		if (renderPass->getSubpasses()[0].DepthStencilAttachment.attachment != VK_ATTACHMENT_UNUSED)
		{
			auto attachmentReference = renderPass->getSubpasses()[0].DepthStencilAttachment;
			auto attachment = renderPass->getAttachments()[attachmentReference.attachment];
			auto imageView = framebuffer->getAttachments()[attachmentReference.attachment];
			ClearImage(imageView->getImage(), attachment.format, clearValues[attachmentReference.attachment].depthStencil);
		}

		deviceState->renderPass = renderPass;
		deviceState->framebuffer = framebuffer;
		deviceState->renderArea = renderArea;
	}

private:
	RenderPass* renderPass;
	Framebuffer* framebuffer;
	VkRect2D renderArea;
	std::vector<VkClearValue> clearValues;
};

class EndRenderPassCommand final : public Command
{
public:
	void Process(DeviceState* deviceState) override
	{
		// TODO
	}
};

CommandBuffer::CommandBuffer(DeviceState* deviceState, VkCommandBufferLevel level, VkCommandPoolCreateFlags poolFlags):
	deviceState{deviceState},
	level{level},
	poolFlags{poolFlags}
{
}

CommandBuffer::~CommandBuffer() = default;

VkResult CommandBuffer::Begin(const VkCommandBufferBeginInfo* pBeginInfo)
{
	assert(pBeginInfo->sType == VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO);

	auto next = pBeginInfo->pNext;
	while (next)
	{
		auto type = *static_cast<const VkStructureType*>(next);
		switch (type)
		{
		default:
			FATAL_ERROR();
		}
	}

	if (poolFlags & VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT)
	{
		Reset(0);
	}
	else
	{
		assert(state == State::Initial);
	}

	if (level == VK_COMMAND_BUFFER_LEVEL_SECONDARY)
	{
		FATAL_ERROR();
	}

	if (pBeginInfo->flags)
	{
		FATAL_ERROR();
	}

	state = State::Recording;
	return VK_SUCCESS;
}

VkResult CommandBuffer::End()
{
	assert(state == State::Recording);

	if (level == VK_COMMAND_BUFFER_LEVEL_PRIMARY)
	{
		// TODO: If commandBuffer is a primary command buffer, there must not be an active render pass instance
	}
	else
	{
		// TODO: If commandBuffer is a secondary command buffer, there must not be an outstanding vkCmdBeginDebugUtilsLabelEXT command recorded to commandBuffer that has not previously been ended by a call to vkCmdEndDebugUtilsLabelEXT.
		// TODO: If commandBuffer is a secondary command buffer, there must not be an outstanding vkCmdDebugMarkerBeginEXT command recorded to commandBuffer that has not previously been ended by a call to vkCmdDebugMarkerEndEXT.
	}

	// TODO: All queries made active during the recording of commandBuffer must have been made inactive
	// TODO: Conditional rendering must not be active

	state = State::Executable;
	return VK_SUCCESS;
}

VkResult CommandBuffer::Reset(VkCommandBufferResetFlags flags)
{
	if (flags)
	{
		FATAL_ERROR();
	}

	if (state == State::Pending)
	{
		FATAL_ERROR();
	}

	commands.clear();

	state = State::Initial;
	return VK_SUCCESS;
}

void CommandBuffer::BindPipeline(VkPipelineBindPoint pipelineBindPoint, VkPipeline pipeline)
{
	assert(state == State::Recording);
	if (pipelineBindPoint != VK_PIPELINE_BIND_POINT_GRAPHICS && pipelineBindPoint != VK_PIPELINE_BIND_POINT_COMPUTE)
	{
		FATAL_ERROR();
	}
	
	commands.push_back(std::make_unique<BindPipelineCommand>(static_cast<int>(pipelineBindPoint), reinterpret_cast<Pipeline*>(pipeline)));
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
		descriptorSets[i] = reinterpret_cast<DescriptorSet*>(pDescriptorSets[i]);
	}

	std::vector<uint32_t> dynamicOffsets(dynamicOffsetCount);
	memcpy(dynamicOffsets.data(), pDynamicOffsets, sizeof(uint32_t) * dynamicOffsetCount);

	commands.push_back(std::make_unique<BindDescriptorSetsCommand>(static_cast<int>(pipelineBindPoint), reinterpret_cast<PipelineLayout*>(layout), firstSet, descriptorSets, dynamicOffsets));
}

void CommandBuffer::BindVertexBuffers(uint32_t firstBinding, uint32_t bindingCount, const VkBuffer* pBuffers, const VkDeviceSize* pOffsets)
{
	assert(state == State::Recording);
	std::vector<Buffer*> buffers(bindingCount);
	std::vector<VkDeviceSize> bufferOffsets(bindingCount);

	for (auto i = 0u; i < bindingCount; i++)
	{
		buffers[i] = reinterpret_cast<Buffer*>(pBuffers[i]);
		bufferOffsets[i] = pOffsets[i];
	}
	
	commands.push_back(std::make_unique<BindVertexBuffersCommand>(firstBinding, buffers, bufferOffsets));
}

void CommandBuffer::Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
{
	assert(state == State::Recording);
	commands.push_back(std::make_unique<DrawCommand>(vertexCount, instanceCount, firstVertex, firstInstance));
}

void CommandBuffer::BeginRenderPass(const VkRenderPassBeginInfo* pRenderPassBegin, VkSubpassContents contents)
{
	assert(state == State::Recording);
	assert(pRenderPassBegin->sType == VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO);

	auto next = pRenderPassBegin->pNext;
	while (next)
	{
		auto type = *static_cast<const VkStructureType*>(next);
		switch (type)
		{
		default:
			FATAL_ERROR();
		}
	}

	std::vector<VkClearValue> clearValues(pRenderPassBegin->clearValueCount);
	memcpy(clearValues.data(), pRenderPassBegin->pClearValues, sizeof(VkClearValue) * pRenderPassBegin->clearValueCount);
	commands.push_back(std::make_unique<BeginRenderPassCommand>(reinterpret_cast<RenderPass*>(pRenderPassBegin->renderPass), 
	                                                            reinterpret_cast<Framebuffer*>(pRenderPassBegin->framebuffer),
	                                                            pRenderPassBegin->renderArea,
	                                                            clearValues));
}

void CommandBuffer::EndRenderPass()
{
	assert(state == State::Recording);
	commands.push_back(std::make_unique<EndRenderPassCommand>());
}

VkResult CommandBuffer::Submit()
{
	for (const auto& command : commands)
	{
		command->Process(deviceState);
	}
	return VK_SUCCESS;
}
