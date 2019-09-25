#include "CommandBuffer.h"
#include "CommandBuffer.Internal.h"

#include "Buffer.h"
#include "DeviceState.h"
#include "Event.h"
#include "Formats.h"
#include "Framebuffer.h"
#include "Image.h"
#include "ImageSampler.h"
#include "ImageView.h"
#include "RenderPass.h"
#include "Util.h"

#include <glm/glm.hpp>

#include <iostream>

class CopyImageCommand final : public Command
{
public:
	CopyImageCommand(Image* srcImage, VkImageLayout srcImageLayout, Image* dstImage, VkImageLayout dstImageLayout, std::vector<VkImageCopy> regions):
		srcImage{srcImage},
		srcImageLayout{srcImageLayout},
		dstImage{dstImage},
		dstImageLayout{dstImageLayout},
		regions{std::move(regions)}
	{
	}

	void Process(DeviceState*) override
	{
		const auto& format = GetFormatInformation(srcImage->getFormat());
		if (srcImage->getFormat() != dstImage->getFormat())
		{
			FATAL_ERROR();
		}
		
		for (const auto& region : regions)
		{
			if (region.srcSubresource.baseArrayLayer != 0)
			{
				FATAL_ERROR();
			}

			if (region.srcSubresource.mipLevel != 0)
			{
				FATAL_ERROR();
			}

			if (region.srcSubresource.layerCount != 1)
			{
				FATAL_ERROR();
			}

			if (region.srcSubresource.aspectMask != VK_IMAGE_ASPECT_COLOR_BIT)
			{
				FATAL_ERROR();
			}

			if (region.dstSubresource.baseArrayLayer != 0)
			{
				FATAL_ERROR();
			}

			if (region.dstSubresource.mipLevel != 0)
			{
				FATAL_ERROR();
			}

			if (region.dstSubresource.layerCount != 1)
			{
				FATAL_ERROR();
			}

			if (region.dstSubresource.aspectMask != VK_IMAGE_ASPECT_COLOR_BIT)
			{
				FATAL_ERROR();
			}

			const auto srcData = srcImage->getData();
			const auto dstData = dstImage->getData();
			const auto srcImageLineSize = size_t{srcImage->getWidth()} * format.TotalSize;
			const auto srcImagePlaneSize = srcImageLineSize * srcImage->getHeight();
			const auto dstImageLineSize = size_t{dstImage->getWidth()} * format.TotalSize;
			const auto dstImagePlaneSize = srcImageLineSize * dstImage->getHeight();
			
			// TODO: Detect when both memories are contiguous
			for (auto z = 0u; z < region.extent.depth; z++)
			{
				const auto srcZ = z + region.srcOffset.z;
				const auto dstZ = z + region.dstOffset.z;
				
				for (auto y = 0u; y < region.extent.height; y++)
				{
					const auto srcY = y + region.srcOffset.y;
					const auto dstY = y + region.dstOffset.y;
					
					const auto srcLineStart = region.srcOffset.x * format.TotalSize;
					const auto dstLineStart = region.dstOffset.x * format.TotalSize;
					const auto lineSize = region.extent.width * format.TotalSize;
					const auto src = srcData +
						static_cast<uint64_t>(srcZ) * srcImagePlaneSize +
						static_cast<uint64_t>(srcY) * srcImageLineSize +
						srcLineStart;
					const auto dst = dstData +
						static_cast<uint64_t>(dstZ) * dstImagePlaneSize +
						static_cast<uint64_t>(dstY) * dstImageLineSize +
						dstLineStart;
					memcpy(dst, src, lineSize);
				}
			}
		}
	}

private:
	Image* srcImage;
	VkImageLayout srcImageLayout;
	Image* dstImage;
	VkImageLayout dstImageLayout;
	std::vector<VkImageCopy> regions;
};

class BlitImageCommand final : public Command
{
public:
	BlitImageCommand(Image* srcImage, VkImageLayout srcImageLayout, Image* dstImage, VkImageLayout dstImageLayout, std::vector<VkImageBlit> regions, VkFilter filter):
		srcImage{srcImage},
		srcImageLayout{srcImageLayout},
		dstImage{dstImage},
		dstImageLayout{dstImageLayout},
		regions{std::move(regions)},
		filter{filter}
	{
	}

	void Process(DeviceState*) override
	{
		const auto& dstFormat = GetFormatInformation(dstImage->getFormat());
		for (const auto& region : regions)
		{
			if (region.srcSubresource.aspectMask != VK_IMAGE_ASPECT_COLOR_BIT)
			{
				FATAL_ERROR();
			}

			if (region.dstSubresource.aspectMask != VK_IMAGE_ASPECT_COLOR_BIT)
			{
				FATAL_ERROR();
			}

			if (region.srcSubresource.mipLevel != 0)
			{
				FATAL_ERROR();
			}

			if (region.dstSubresource.mipLevel != 0)
			{
				FATAL_ERROR();
			}

			if (region.srcSubresource.baseArrayLayer != 0)
			{
				FATAL_ERROR();
			}

			if (region.dstSubresource.baseArrayLayer != 0)
			{
				FATAL_ERROR();
			}

			if (region.srcSubresource.layerCount != 1)
			{
				FATAL_ERROR();
			}

			if (region.dstSubresource.layerCount != 1)
			{
				FATAL_ERROR();
			}

			const auto dstWidth = region.dstOffsets[1].x - region.dstOffsets[0].x;
			const auto dstHeight = region.dstOffsets[1].y - region.dstOffsets[0].y;
			const auto dstDepth = region.dstOffsets[1].z - region.dstOffsets[0].z;
			constexpr auto currentArray = 0;
			for (auto z = 0; z < dstDepth; z++)
			{
				for (auto y = 0; y < dstHeight; y++)
				{
					for (auto x = 0; x < dstWidth; x++)
					{
						auto u = (x + 0.5f - region.dstOffsets[0].x) * (float(region.srcOffsets[1].x - region.srcOffsets[0].x) / (region.dstOffsets[1].x - region.dstOffsets[0].x)) + region.srcOffsets[0].x;
						auto v = (y + 0.5f - region.dstOffsets[0].y) * (float(region.srcOffsets[1].y - region.srcOffsets[0].y) / (region.dstOffsets[1].y - region.dstOffsets[0].y)) + region.srcOffsets[0].y;
						auto w = (z + 0.5f - region.dstOffsets[0].z) * (float(region.srcOffsets[1].z - region.srcOffsets[0].z) / (region.dstOffsets[1].z - region.dstOffsets[0].z)) + region.srcOffsets[0].z;
						auto q = region.srcSubresource.mipLevel;
						auto a = currentArray - region.dstSubresource.baseArrayLayer + region.srcSubresource.baseArrayLayer;

						// TODO: Use appropriate data type depending on source/dest formats
						auto pixels = SampleImage<glm::vec4>(srcImage, u, v, w, q, a, filter, VK_SAMPLER_MIPMAP_MODE_NEAREST, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
						uint64_t tmp[4];
						Convert(dstFormat, &pixels.x, tmp);
						SetPixel(dstFormat, dstImage, x + region.dstOffsets[0].x, y + region.dstOffsets[0].y, z + region.dstOffsets[0].z, tmp);
					}
				}
			}
		}
	}

private:
	Image* srcImage;
	VkImageLayout srcImageLayout;
	Image* dstImage;
	VkImageLayout dstImageLayout;
	std::vector<VkImageBlit> regions;
	VkFilter filter;
};

class CopyImageToBufferCommand final : public Command
{
public:
	CopyImageToBufferCommand(Image* srcImage, VkImageLayout srcImageLayout, Buffer* dstBuffer, std::vector<VkBufferImageCopy> regions):
		srcImage{srcImage},
		srcImageLayout{srcImageLayout},
		dstBuffer{dstBuffer},
		regions{std::move(regions)}
	{
	}

	void Process(DeviceState*) override
	{
		const auto& format = GetFormatInformation(srcImage->getFormat());
		
		for (const auto& region : regions)
		{
			if (region.imageSubresource.baseArrayLayer)
			{
				FATAL_ERROR();
			}

			if (region.imageSubresource.mipLevel)
			{
				FATAL_ERROR();
			}

			if (region.imageSubresource.layerCount != 1)
			{
				FATAL_ERROR();
			}

			if (region.imageSubresource.aspectMask != VK_IMAGE_ASPECT_COLOR_BIT)
			{
				FATAL_ERROR();
			}

			const auto bufferOffset = region.bufferOffset;
			const auto data = srcImage->getData();
			const auto imageLineSize = size_t{srcImage->getWidth()} * format.TotalSize;
			const auto imagePlaneSize = imageLineSize * srcImage->getHeight();
			const auto bufferLineSize = size_t{region.bufferRowLength == 0 ? region.imageExtent.width : region.bufferRowLength} * format.TotalSize;
			const auto bufferPlaneSize = size_t{region.bufferImageHeight == 0 ? region.imageExtent.height : region.bufferImageHeight} * bufferLineSize;
			
			// TODO: Detect when both memories are contiguous
			for (auto z = region.imageOffset.z; z < region.imageOffset.z + static_cast<int32_t>(region.imageExtent.depth); z++)
			{
				for (auto y = region.imageOffset.y; y < region.imageOffset.y + static_cast<int32_t>(region.imageExtent.height); y++)
				{
					const auto lineStart = region.imageOffset.x * format.TotalSize;
					const auto lineSize = region.imageExtent.width * format.TotalSize;
					const auto src = data +
						static_cast<uint64_t>(z) * imagePlaneSize +
						static_cast<uint64_t>(y) * imageLineSize +
						lineStart;
					const auto dst = dstBuffer->getDataPtr(bufferOffset + z * bufferPlaneSize + y * bufferLineSize + lineStart, lineSize);
					memcpy(dst, src, lineSize);
				}
			}
		}
	}

private:
	Image* srcImage;
	VkImageLayout srcImageLayout;
	Buffer* dstBuffer;
	std::vector<VkBufferImageCopy> regions;
};

class SetEventCommand final : public Command
{
public:
	SetEventCommand(Event* event, VkPipelineStageFlags stageMask):
		event{event},
		stageMask{stageMask}
	{
	}

	void Process(DeviceState*) override
	{
		event->Signal();
	}
	
private:
	Event* event;
	VkPipelineStageFlags stageMask;
};

class ResetEventCommand final : public Command
{
public:
	ResetEventCommand(Event* event, VkPipelineStageFlags stageMask):
		event{event},
		stageMask{stageMask}
	{
	}

	void Process(DeviceState*) override
	{
		event->Reset();
	}
	
private:
	Event* event;
	VkPipelineStageFlags stageMask;
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

class ExecuteCommandsCommand final : public Command
{
public:
	ExecuteCommandsCommand(std::vector<CommandBuffer*> commands):
		commands{std::move(commands)}
	{
	}

	void Process(DeviceState* deviceState) override
	{
		for (const auto commandBuffer : commands)
		{
			for (const auto& command : commandBuffer->commands)
			{
				command->Process(deviceState);
			}
		}
	}

private:
	std::vector<CommandBuffer*> commands;
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
		const auto type = static_cast<const VkBaseInStructure*>(next)->sType;
		switch (type)
		{
		case VK_STRUCTURE_TYPE_DEVICE_GROUP_COMMAND_BUFFER_BEGIN_INFO:
			FATAL_ERROR();
		}
		next = static_cast<const VkBaseInStructure*>(next)->pNext;
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
		assert(pBeginInfo->pInheritanceInfo->sType == VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO);

		next = pBeginInfo->pInheritanceInfo->pNext;
		while (next)
		{
			const auto type = static_cast<const VkBaseInStructure*>(next)->sType;
			switch (type)
			{
			case VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_CONDITIONAL_RENDERING_INFO_EXT:
				FATAL_ERROR();
			}
			next = static_cast<const VkBaseInStructure*>(next)->pNext;
		}
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

	ForceReset();

	return VK_SUCCESS;
}

void CommandBuffer::CopyImage(VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageCopy* pRegions)
{
	assert(state == State::Recording);
	commands.push_back(std::make_unique<CopyImageCommand>(UnwrapVulkan<Image>(srcImage), srcImageLayout, UnwrapVulkan<Image>(dstImage), dstImageLayout, ArrayToVector(regionCount, pRegions)));
}

void CommandBuffer::BlitImage(VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageBlit* pRegions, VkFilter filter)
{
	assert(state == State::Recording);
	commands.push_back(std::make_unique<BlitImageCommand>(UnwrapVulkan<Image>(srcImage), srcImageLayout, UnwrapVulkan<Image>(dstImage), dstImageLayout, ArrayToVector(regionCount, pRegions), filter));
}

void CommandBuffer::CopyImageToBuffer(VkImage srcImage, VkImageLayout srcImageLayout, VkBuffer dstBuffer, uint32_t regionCount, const VkBufferImageCopy* pRegions)
{
	assert(state == State::Recording);
	commands.push_back(std::make_unique<CopyImageToBufferCommand>(UnwrapVulkan<Image>(srcImage), srcImageLayout, UnwrapVulkan<Buffer>(dstBuffer), ArrayToVector(regionCount, pRegions)));
}

void CommandBuffer::SetEvent(VkEvent event, VkPipelineStageFlags stageMask)
{
	assert(state == State::Recording);
	commands.push_back(std::make_unique<SetEventCommand>(UnwrapVulkan<Event>(event), stageMask));
}

void CommandBuffer::ResetEvent(VkEvent event, VkPipelineStageFlags stageMask)
{
	assert(state == State::Recording);
	commands.push_back(std::make_unique<ResetEventCommand>(UnwrapVulkan<Event>(event), stageMask));
}

void CommandBuffer::WaitEvents(uint32_t eventCount, const VkEvent* pEvents, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask,
                               uint32_t memoryBarrierCount, const VkMemoryBarrier* pMemoryBarriers, uint32_t bufferMemoryBarrierCount, const VkBufferMemoryBarrier* pBufferMemoryBarriers,
                               uint32_t imageMemoryBarrierCount, const VkImageMemoryBarrier* pImageMemoryBarriers)
{
	assert(state == State::Recording);
	std::cout << "Waiting for events is not supported" << std::endl;
	// TODO
}

void CommandBuffer::PipelineBarrier(VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask,
                                    VkDependencyFlags dependencyFlags, uint32_t memoryBarrierCount, const VkMemoryBarrier* pMemoryBarriers,
                                    uint32_t bufferMemoryBarrierCount, const VkBufferMemoryBarrier* pBufferMemoryBarriers,
                                    uint32_t imageMemoryBarrierCount, const VkImageMemoryBarrier* pImageMemoryBarriers)
{
	assert(state == State::Recording);
	// TODO
}

void CommandBuffer::BeginRenderPass(const VkRenderPassBeginInfo* pRenderPassBegin, VkSubpassContents contents)
{
	assert(state == State::Recording);
	assert(pRenderPassBegin->sType == VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO);

	auto next = pRenderPassBegin->pNext;
	while (next)
	{
		const auto type = static_cast<const VkBaseInStructure*>(next)->sType;
		switch (type)
		{
		case VK_STRUCTURE_TYPE_DEVICE_GROUP_RENDER_PASS_BEGIN_INFO:
			FATAL_ERROR();
			
		case VK_STRUCTURE_TYPE_RENDER_PASS_ATTACHMENT_BEGIN_INFO_KHR:
			FATAL_ERROR();
			
		case VK_STRUCTURE_TYPE_RENDER_PASS_SAMPLE_LOCATIONS_BEGIN_INFO_EXT:
			FATAL_ERROR();
		}
		next = static_cast<const VkBaseInStructure*>(next)->pNext;
	}

	std::vector<VkClearValue> clearValues(pRenderPassBegin->clearValueCount);
	memcpy(clearValues.data(), pRenderPassBegin->pClearValues, sizeof(VkClearValue) * pRenderPassBegin->clearValueCount);
	commands.push_back(std::make_unique<BeginRenderPassCommand>(UnwrapVulkan<RenderPass>(pRenderPassBegin->renderPass), 
	                                                            UnwrapVulkan<Framebuffer>(pRenderPassBegin->framebuffer),
	                                                            pRenderPassBegin->renderArea,
	                                                            clearValues));
}

void CommandBuffer::EndRenderPass()
{
	assert(state == State::Recording);
	commands.push_back(std::make_unique<EndRenderPassCommand>());
}

void CommandBuffer::ExecuteCommands(uint32_t commandBufferCount, const VkCommandBuffer* pCommandBuffers)
{
	assert(state == State::Recording);
	commands.push_back(std::make_unique<ExecuteCommandsCommand>(ArrayToVector<CommandBuffer*, VkCommandBuffer>(commandBufferCount, pCommandBuffers, [](VkCommandBuffer commandBuffer)
	{
		return UnwrapVulkan<CommandBuffer>(commandBuffer);
	})));
}

void CommandBuffer::ForceReset()
{
	if (state == State::Pending)
	{
		FATAL_ERROR();
	}

	commands.clear();

	state = State::Initial;
}

VkResult CommandBuffer::Submit()
{
	for (const auto& command : commands)
	{
		command->Process(deviceState);
	}
	return VK_SUCCESS;
}
