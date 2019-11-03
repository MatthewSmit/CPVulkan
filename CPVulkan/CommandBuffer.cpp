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

class CopyBufferCommand final : public Command
{
public:
	CopyBufferCommand(Buffer* srcBuffer, Buffer* dstBuffer, std::vector<VkBufferCopy> regions):
		srcBuffer{srcBuffer},
		dstBuffer{dstBuffer},
		regions{std::move(regions)}
	{
	}

	void Process(DeviceState*) override
	{
		for (const auto& region : regions)
		{
			const auto srcSpan = srcBuffer->getData(region.srcOffset, region.size);
			const auto dstSpan = dstBuffer->getData(region.dstOffset, region.size);
			memcpy(dstSpan.data(), srcSpan.data(), region.size);
		}
	}

private:
	Buffer* srcBuffer;
	Buffer* dstBuffer;
	std::vector<VkBufferCopy> regions;
};

class CopyImageCommand final : public Command
{
public:
	CopyImageCommand(Image* srcImage, Image* dstImage, std::vector<VkImageCopy> regions):
		srcImage{srcImage},
		dstImage{dstImage},
		regions{std::move(regions)}
	{
	}

	void Process(DeviceState*) override
	{
		const auto& srcFormat = GetFormatInformation(srcImage->getFormat());
		const auto& dstFormat = GetFormatInformation(dstImage->getFormat());
		
		for (const auto& region : regions)
		{
			if (region.srcSubresource.aspectMask != VK_IMAGE_ASPECT_COLOR_BIT &&
				(region.srcSubresource.aspectMask != VK_IMAGE_ASPECT_DEPTH_BIT || srcFormat.GreenOffset != -1) &&
				(region.srcSubresource.aspectMask != VK_IMAGE_ASPECT_STENCIL_BIT || srcFormat.GreenOffset != -1))
			{
				FATAL_ERROR();
			}

			if (region.dstSubresource.aspectMask != VK_IMAGE_ASPECT_COLOR_BIT &&
				(region.dstSubresource.aspectMask != VK_IMAGE_ASPECT_DEPTH_BIT || dstFormat.GreenOffset != -1) &&
				(region.dstSubresource.aspectMask != VK_IMAGE_ASPECT_STENCIL_BIT || dstFormat.GreenOffset != -1))
			{
				FATAL_ERROR();
			}

			const auto width = region.extent.width;
			const auto height = region.extent.height;
			const auto depth = region.extent.depth;
			auto srcBaseZ = region.srcOffset.z;
			auto dstBaseZ = region.dstOffset.z;

			if (region.srcSubresource.baseArrayLayer > 0)
			{
				if (srcBaseZ == 0)
				{
					srcBaseZ = region.srcSubresource.baseArrayLayer;
				}
				else
				{
					FATAL_ERROR();
				}
			}

			if (region.dstSubresource.baseArrayLayer > 0)
			{
				if (dstBaseZ == 0)
				{
					dstBaseZ = region.dstSubresource.baseArrayLayer;
				}
				else
				{
					FATAL_ERROR();
				}
			}

			uint64_t srcOffset;
			uint64_t srcPlaneStride;
			uint64_t srcLineStride;
			GetFormatStrides(srcFormat, srcOffset, srcPlaneStride, srcLineStride, region.srcSubresource.mipLevel, srcImage->getWidth(), srcImage->getHeight(), srcImage->getDepth(), srcImage->getArrayLayers());

			uint64_t srcLineStart;
			uint64_t srcLineSize;
			GetFormatLineSize(srcFormat, srcLineStart, srcLineSize, region.srcOffset.x, width);

			uint64_t dstOffset;
			uint64_t dstPlaneStride;
			uint64_t dstLineStride;
			GetFormatStrides(dstFormat, dstOffset, dstPlaneStride, dstLineStride, region.dstSubresource.mipLevel, dstImage->getWidth(), dstImage->getHeight(), dstImage->getDepth(), dstImage->getArrayLayers());

			uint64_t dstLineStart;
			uint64_t dstLineSize;
			GetFormatLineSize(dstFormat, dstLineStart, dstLineSize, region.dstOffset.x, width);

			if (srcLineSize != dstLineSize)
			{
				FATAL_ERROR();
			}
			
			// TODO: Detect when both memories are contiguous
			for (auto z = 0u; z < depth; z++)
			{
				const auto srcZ = z + srcBaseZ;
				const auto dstZ = z + dstBaseZ;
				
				for (auto y = 0u; y < height; y++)
				{
					const auto srcY = y + region.srcOffset.y;
					const auto dstY = y + region.dstOffset.y;
					
					const auto src = srcImage->getData(static_cast<uint64_t>(srcZ) * srcPlaneStride + static_cast<uint64_t>(srcY) * srcLineStride + srcLineStart, srcLineSize);
					const auto dst = dstImage->getData(static_cast<uint64_t>(dstZ) * dstPlaneStride + static_cast<uint64_t>(dstY) * dstLineStride + dstLineStart, dstLineSize);
					
					memcpy(dst.data(), src.data(), dstLineSize);
				}
			}
		}
	}

private:
	Image* srcImage;
	Image* dstImage;
	std::vector<VkImageCopy> regions;
};

class BlitImageCommand final : public Command
{
public:
	BlitImageCommand(Image* srcImage, Image* dstImage, std::vector<VkImageBlit> regions, VkFilter filter):
		srcImage{srcImage},
		dstImage{dstImage},
		regions{std::move(regions)},
		filter{filter}
	{
	}

	void Process(DeviceState*) override
	{
		// const auto& dstFormat = GetFormatInformation(dstImage->getFormat());
		//
		// SampleImageType sampleImage;
		// switch (dstFormat.Base)
		// {
		// case BaseType::UNorm:
		// case BaseType::SNorm:
		// case BaseType::SFloat:
		// 	if (dstFormat.TotalBits() > 32)
		// 	{
		// 		sampleImage = SampleImage<glm::f64vec4>;
		// 	}
		// 	else
		// 	{
		// 		sampleImage = SampleImage<glm::f32vec4>;
		// 	}
		// 	break;
		// 	
		// 	// case BaseType::UScaled: break;
		// 	// case BaseType::SScaled: break;
		// 	 
		// case BaseType::UInt:
		// 	if (dstFormat.TotalBits() > 32)
		// 	{
		// 		sampleImage = SampleImage<glm::u64vec4>;
		// 	}
		// 	else
		// 	{
		// 		sampleImage = SampleImage<glm::u32vec4>;
		// 	}
		// 	break;
		//
		// case BaseType::SInt:
		// 	if (dstFormat.TotalBits() > 32)
		// 	{
		// 		sampleImage = SampleImage<glm::i64vec4>;
		// 	}
		// 	else
		// 	{
		// 		sampleImage = SampleImage<glm::i32vec4>;
		// 	}
		// 	break;
		//
		// 	// case BaseType::UFloat: break;
		// 	// case BaseType::SRGB: break;
		// 	
		// default:
		// 	FATAL_ERROR();
		// }
		//
		// for (const auto& region : regions)
		// {
		// 	if (region.srcSubresource.aspectMask != VK_IMAGE_ASPECT_COLOR_BIT)
		// 	{
		// 		FATAL_ERROR();
		// 	}
		//
		// 	if (region.dstSubresource.aspectMask != VK_IMAGE_ASPECT_COLOR_BIT)
		// 	{
		// 		FATAL_ERROR();
		// 	}
		//
		// 	if (region.srcSubresource.mipLevel != 0)
		// 	{
		// 		FATAL_ERROR();
		// 	}
		//
		// 	if (region.dstSubresource.mipLevel != 0)
		// 	{
		// 		FATAL_ERROR();
		// 	}
		//
		// 	if (region.srcSubresource.baseArrayLayer != 0)
		// 	{
		// 		FATAL_ERROR();
		// 	}
		//
		// 	if (region.dstSubresource.baseArrayLayer != 0)
		// 	{
		// 		FATAL_ERROR();
		// 	}
		//
		// 	if (region.srcSubresource.layerCount != 1)
		// 	{
		// 		FATAL_ERROR();
		// 	}
		//
		// 	if (region.dstSubresource.layerCount != 1)
		// 	{
		// 		FATAL_ERROR();
		// 	}
		//
		// 	// TODO: Handle region.dstOffsets[1] < region.dstOffsets[0]
		// 	auto dstWidth = region.dstOffsets[1].x - region.dstOffsets[0].x;
		// 	auto dstHeight = region.dstOffsets[1].y - region.dstOffsets[0].y;
		// 	auto dstDepth = region.dstOffsets[1].z - region.dstOffsets[0].z;
		//
		// 	auto negativeWidth = false;
		// 	auto negativeHeight = false;
		// 	auto negativeDepth = false;
		//
		// 	if (dstWidth < 0)
		// 	{
		// 		negativeWidth = true;
		// 		dstWidth = -dstWidth;
		// 	}
		//
		// 	if (dstHeight < 0)
		// 	{
		// 		negativeHeight = true;
		// 		dstHeight = -dstHeight;
		// 	}
		//
		// 	if (dstDepth < 0)
		// 	{
		// 		negativeDepth = true;
		// 		dstDepth = -dstDepth;
		// 	}
		//
		// 	uint64_t tmp[4];
		// 	constexpr auto currentArray = 0;
		// 	for (auto z = 0; z < dstDepth; z++)
		// 	{
		// 		for (auto y = 0; y < dstHeight; y++)
		// 		{
		// 			for (auto x = 0; x < dstWidth; x++)
		// 			{
		// 				const auto dstX = negativeWidth ? x + region.dstOffsets[1].x : x + region.dstOffsets[0].x;
		// 				const auto dstY = negativeHeight ? y + region.dstOffsets[1].y : y + region.dstOffsets[0].y;
		// 				const auto dstZ = negativeDepth ? z + region.dstOffsets[1].z : z + region.dstOffsets[0].z;
		// 				
		// 				const auto u = (dstX + 0.5f - region.dstOffsets[0].x) * (float(region.srcOffsets[1].x - region.srcOffsets[0].x) / (region.dstOffsets[1].x - region.dstOffsets[0].x)) + region.srcOffsets[0].x;
		// 				const auto v = (dstY + 0.5f - region.dstOffsets[0].y) * (float(region.srcOffsets[1].y - region.srcOffsets[0].y) / (region.dstOffsets[1].y - region.dstOffsets[0].y)) + region.srcOffsets[0].y;
		// 				const auto w = (dstZ + 0.5f - region.dstOffsets[0].z) * (float(region.srcOffsets[1].z - region.srcOffsets[0].z) / (region.dstOffsets[1].z - region.dstOffsets[0].z)) + region.srcOffsets[0].z;
		// 				const auto q = region.srcSubresource.mipLevel;
		// 				const auto a = currentArray - region.dstSubresource.baseArrayLayer + region.srcSubresource.baseArrayLayer;
		//
		// 				sampleImage(dstFormat, srcImage, u, v, w, q, a, filter, VK_SAMPLER_MIPMAP_MODE_NEAREST, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, tmp);
		// 				
		// 				SetPixel(dstFormat, dstImage, dstX, dstY, dstZ, 0, 0, tmp);
		// 			}
		// 		}
		// 	}
		// }
		FATAL_ERROR();
	}

private:
	Image* srcImage;
	Image* dstImage;
	std::vector<VkImageBlit> regions;
	VkFilter filter;
};

class CopyBufferToImageCommand final : public Command
{
public:
	CopyBufferToImageCommand(Buffer* srcBuffer, Image* dstImage, std::vector<VkBufferImageCopy> regions):
		srcBuffer{srcBuffer},
		dstImage{dstImage},
		regions{std::move(regions)}
	{
	}

	void Process(DeviceState*) override
	{
		const auto& format = GetFormatInformation(dstImage->getFormat());
		
		for (const auto& region : regions)
		{
			if (region.imageSubresource.aspectMask != VK_IMAGE_ASPECT_COLOR_BIT &&
				(region.imageSubresource.aspectMask != VK_IMAGE_ASPECT_DEPTH_BIT || format.GreenOffset != -1) && 
				(region.imageSubresource.aspectMask != VK_IMAGE_ASPECT_STENCIL_BIT || format.GreenOffset != -1))
			{
				FATAL_ERROR();
			}

			const auto width = region.imageExtent.width;
			auto height = region.imageExtent.height;
			auto depth = region.imageExtent.depth;
			auto baseZ = region.imageOffset.z;

			if (region.imageSubresource.baseArrayLayer > 0 || region.imageSubresource.layerCount != 1)
			{
				if (baseZ == 0 && depth == 1)
				{
					baseZ = region.imageSubresource.baseArrayLayer;
				}
				else
				{
					FATAL_ERROR();
				}
			}

			const auto srcOffset = region.bufferOffset;
			auto srcLineStride = (region.bufferRowLength == 0 ? width : region.bufferRowLength) * format.TotalSize;
			if (format.Type == FormatType::Compressed)
			{
				srcLineStride /= format.RedOffset;
			}

			auto srcPlaneStride = (region.bufferImageHeight == 0 ? height : region.bufferImageHeight) * srcLineStride;
			if (format.Type == FormatType::Compressed)
			{
				srcPlaneStride /= format.GreenOffset;
			}

			uint64_t dstOffset;
			uint64_t dstPlaneStride;
			uint64_t dstLineStride;
			GetFormatStrides(format, dstOffset, dstPlaneStride, dstLineStride, region.imageSubresource.mipLevel, dstImage->getWidth(), dstImage->getHeight(), dstImage->getDepth(), dstImage->getArrayLayers());

			uint64_t lineStart;
			uint64_t lineSize;
			GetFormatLineSize(format, lineStart, lineSize, region.imageOffset.x, width);

			height = GetFormatHeight(format, height);
			
			// TODO: Detect when both memories are contiguous
			for (auto z = baseZ; z < baseZ + static_cast<int32_t>(depth); z++)
			{
				for (auto y = region.imageOffset.y; y < region.imageOffset.y + static_cast<int32_t>(height); y++)
				{
					const auto src = srcBuffer->getData(srcOffset + z * srcPlaneStride + y * srcLineStride + lineStart, lineSize);
					const auto dst = dstImage->getData(dstOffset + z * dstPlaneStride + y * dstLineStride + lineStart, lineSize);
					
					memcpy(dst.data(), src.data(), lineSize);
				}
			}
		}
	}

private:
	Buffer* srcBuffer;
	Image* dstImage;
	std::vector<VkBufferImageCopy> regions;
};

class CopyImageToBufferCommand final : public Command
{
public:
	CopyImageToBufferCommand(Image* srcImage, Buffer* dstBuffer, std::vector<VkBufferImageCopy> regions):
		srcImage{srcImage},
		dstBuffer{dstBuffer},
		regions{std::move(regions)}
	{
	}

	void Process(DeviceState*) override
	{
		const auto& format = GetFormatInformation(srcImage->getFormat());
		for (const auto& region : regions)
		{
			if (region.imageSubresource.aspectMask != VK_IMAGE_ASPECT_COLOR_BIT &&
				(region.imageSubresource.aspectMask != VK_IMAGE_ASPECT_DEPTH_BIT || format.GreenOffset != -1) && 
				(region.imageSubresource.aspectMask != VK_IMAGE_ASPECT_STENCIL_BIT || format.GreenOffset != -1))
			{
				FATAL_ERROR();
			}

			const auto width = region.imageExtent.width;
			const auto height = region.imageExtent.height;
			auto depth = region.imageExtent.depth;
			auto baseZ = region.imageOffset.z;

			if (region.imageSubresource.baseArrayLayer > 0 || region.imageSubresource.layerCount != 1)
			{
				if (baseZ == 0 && depth == 1)
				{
					baseZ = region.imageSubresource.baseArrayLayer;
					depth = region.imageSubresource.layerCount;
				}
				else
				{
					FATAL_ERROR();
				}
			}

			uint64_t srcOffset;
			uint64_t srcPlaneStride;
			uint64_t srcLineStride;
			GetFormatStrides(format, srcOffset, srcPlaneStride, srcLineStride, region.imageSubresource.mipLevel, srcImage->getWidth(), srcImage->getHeight(), srcImage->getDepth(), srcImage->getArrayLayers());

			const auto dstOffset = region.bufferOffset;
			auto dstLineStride = (region.bufferRowLength == 0 ? width : region.bufferRowLength) * format.TotalSize;
			if (format.Type == FormatType::Compressed)
			{
				dstLineStride /= format.RedOffset;
			}
			
			auto dstPlaneStride = (region.bufferImageHeight == 0 ? height : region.bufferImageHeight) * dstLineStride;
			if (format.Type == FormatType::Compressed)
			{
				dstPlaneStride /= format.GreenOffset;
			}

			uint64_t lineStart;
			uint64_t lineSize;
			GetFormatLineSize(format, lineStart, lineSize, region.imageOffset.x, width);
			
			// TODO: Detect when both memories are contiguous
			for (auto z = baseZ; z < baseZ + static_cast<int32_t>(depth); z++)
			{
				for (auto y = region.imageOffset.y; y < region.imageOffset.y + static_cast<int32_t>(height); y++)
				{
					const auto src = srcImage->getData(srcOffset + z * srcPlaneStride + y * srcLineStride + lineStart, lineSize);
					const auto dst = dstBuffer->getData(dstOffset + z * dstPlaneStride + y * dstLineStride + lineStart, lineSize);

					memcpy(dst.data(), src.data(), lineSize);
				}
			}
		}
	}

private:
	Image* srcImage;
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

class PushConstantsCommand final : public Command
{
public:
	PushConstantsCommand(VkPipelineLayout layout, VkShaderStageFlags stageFlags, uint32_t offset, uint32_t size, std::unique_ptr<uint8_t[]> values) :
		layout{layout},
		stageFlags{stageFlags},
		offset{offset},
		size{size},
		values{std::move(values)}
	{
	}

	~PushConstantsCommand() override = default;

	void Process(DeviceState* state) override
	{
		assert(offset + size <= MAX_PUSH_CONSTANTS_SIZE);
		memcpy(state->pushConstants + offset, values.get(), size);
	}

private:
	VkPipelineLayout layout;
	VkShaderStageFlags stageFlags;
	uint32_t offset;
	uint32_t size;
	std::unique_ptr<uint8_t[]> values;
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

		for (auto attachmentReference : renderPass->getSubpasses()[0].ColourAttachments)
		{
			if (attachmentReference.attachment != VK_ATTACHMENT_UNUSED)
			{
				auto attachment = renderPass->getAttachments()[attachmentReference.attachment];
				auto imageView = framebuffer->getAttachments()[attachmentReference.attachment];
				ClearImage(deviceState, imageView->getImage(), attachment.format, 0, 1, 0, 1, clearValues[attachmentReference.attachment].color);
			}
		}

		if (renderPass->getSubpasses()[0].DepthStencilAttachment.attachment != VK_ATTACHMENT_UNUSED)
		{
			auto attachmentReference = renderPass->getSubpasses()[0].DepthStencilAttachment;
			auto attachment = renderPass->getAttachments()[attachmentReference.attachment];
			auto imageView = framebuffer->getAttachments()[attachmentReference.attachment];
			auto formatInformation = GetFormatInformation(attachment.format);
			VkImageAspectFlags aspects;
			if (formatInformation.Format == VK_FORMAT_S8_UINT)
			{
				aspects = VK_IMAGE_ASPECT_STENCIL_BIT;
			}
			else if (formatInformation.GreenOffset == -1)
			{
				aspects = VK_IMAGE_ASPECT_DEPTH_BIT;
			}
			else
			{
				aspects = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
			}
			ClearImage(deviceState, imageView->getImage(), attachment.format, 0, 1, 0, 1, aspects, clearValues[attachmentReference.attachment].depthStencil);
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
	explicit ExecuteCommandsCommand(std::vector<CommandBuffer*> commands):
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

		default:
			break;
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

			default:
				break;
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

void CommandBuffer::CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, uint32_t regionCount, const VkBufferCopy* pRegions)
{
	assert(state == State::Recording);
	commands.push_back(std::make_unique<CopyBufferCommand>(UnwrapVulkan<Buffer>(srcBuffer), UnwrapVulkan<Buffer>(dstBuffer), ArrayToVector(regionCount, pRegions)));
}

void CommandBuffer::CopyImage(VkImage srcImage, VkImageLayout, VkImage dstImage, VkImageLayout, uint32_t regionCount, const VkImageCopy* pRegions)
{
	assert(state == State::Recording);
	commands.push_back(std::make_unique<CopyImageCommand>(UnwrapVulkan<Image>(srcImage), UnwrapVulkan<Image>(dstImage), ArrayToVector(regionCount, pRegions)));
}

void CommandBuffer::BlitImage(VkImage srcImage, VkImageLayout, VkImage dstImage, VkImageLayout, uint32_t regionCount, const VkImageBlit* pRegions, VkFilter filter)
{
	assert(state == State::Recording);
	commands.push_back(std::make_unique<BlitImageCommand>(UnwrapVulkan<Image>(srcImage), UnwrapVulkan<Image>(dstImage), ArrayToVector(regionCount, pRegions), filter));
}

void CommandBuffer::CopyBufferToImage(VkBuffer srcBuffer, VkImage dstImage, VkImageLayout, uint32_t regionCount, const VkBufferImageCopy* pRegions)
{
	assert(state == State::Recording);
	commands.push_back(std::make_unique<CopyBufferToImageCommand>(UnwrapVulkan<Buffer>(srcBuffer), UnwrapVulkan<Image>(dstImage), ArrayToVector(regionCount, pRegions)));
}

void CommandBuffer::CopyImageToBuffer(VkImage srcImage, VkImageLayout, VkBuffer dstBuffer, uint32_t regionCount, const VkBufferImageCopy* pRegions)
{
	assert(state == State::Recording);
	commands.push_back(std::make_unique<CopyImageToBufferCommand>(UnwrapVulkan<Image>(srcImage), UnwrapVulkan<Buffer>(dstBuffer), ArrayToVector(regionCount, pRegions)));
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

void CommandBuffer::PushConstants(VkPipelineLayout layout, VkShaderStageFlags stageFlags, uint32_t offset, uint32_t size, const void* pValues)
{
	assert(state == State::Recording);
	std::unique_ptr<uint8_t[]> values(new uint8_t[size]);
	memcpy(values.get(), pValues, size);
	commands.push_back(std::make_unique<PushConstantsCommand>(layout, stageFlags, offset, size, move(values)));
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

		default:
			break;
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
