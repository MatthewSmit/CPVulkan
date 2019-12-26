#include "CommandBuffer.h"
#include "CommandBuffer.Internal.h"

#include "Buffer.h"
#include "DebugHelper.h"
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

#include <fstream>
#include <iostream>

static void RunCommands(DeviceState* deviceState, const std::vector<std::unique_ptr<Command>>& commands)
{
	for (const auto& command : commands)
	{
#if CV_DEBUG_LEVEL > 0
		command->DebugOutput(deviceState);
#endif
		command->Process(deviceState);
	}
}

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

	~BlitImageCommand() override = default;

#if CV_DEBUG_LEVEL > 0
	void DebugOutput(DeviceState* deviceState) override
	{
		*deviceState->debugOutput << "BlitImage: blitting images " <<
			" from " << srcImage <<
			" to " << dstImage <<
			" on regions " << regions <<
			" with filter " << filter <<
			std::endl;
	}
#endif

	void Process(DeviceState* deviceState) override
	{
		assert(srcImage->getSamples() == VK_SAMPLE_COUNT_1_BIT);
		assert(dstImage->getSamples() == VK_SAMPLE_COUNT_1_BIT);
		
		std::function<void(int, int, int, int, int, float, float, float, int, int)> blit;
		const auto information = GetFormatInformation(dstImage->getFormat());
		switch (information.Base)
		{
		case BaseType::UNorm:
		case BaseType::SNorm:
		case BaseType::UScaled:
		case BaseType::SScaled:
		case BaseType::UFloat:
		case BaseType::SFloat:
		case BaseType::SRGB:
			if (information.ElementSize > 4)
			{
				TODO_ERROR();
			}
			else
			{
				blit = [&](int dstX, int dstY, int dstZ, int dstLevel, int dstLayer, float u, float v, float w, int q, int a)
				{
					const auto size = srcImage->getImageSize();
					auto value = SampleImage<glm::fvec4>(deviceState, srcImage->getFormat(), srcImage->getData().subspan(size.LayerSize * a + size.Level[q].Offset, size.Level[q].LevelSize),
					                                     glm::uvec3{size.Level[q].Width, size.Level[q].Height, size.Level[q].Depth},
					                                     glm::fvec3{u / size.Level[q].Width, v / size.Level[q].Height, w / size.Level[q].Depth},
					                                     filter);
					SetPixel(deviceState, dstImage->getFormat(), dstImage, dstX, dstY, dstZ, dstLevel, dstLayer, &value.x);
				};
			}
			break;
			
		case BaseType::UInt:
			if (information.ElementSize > 4)
			{
				TODO_ERROR();
			}
			else
			{
				blit = [&](int dstX, int dstY, int dstZ, int dstLevel, int dstLayer, float u, float v, float w, int q, int a)
				{
					const auto size = srcImage->getImageSize();
					auto value = SampleImage<glm::uvec4>(deviceState, srcImage->getFormat(), srcImage->getData().subspan(size.LayerSize * a + size.Level[q].Offset, size.Level[q].LevelSize),
					                                     glm::uvec3{size.Level[q].Width, size.Level[q].Height, size.Level[q].Depth},
					                                     glm::fvec3{u / size.Level[q].Width, v / size.Level[q].Height, w / size.Level[q].Depth},
					                                     filter);
					SetPixel(deviceState, dstImage->getFormat(), dstImage, dstX, dstY, dstZ, dstLevel, dstLayer, &value.x);
				};
			}
			break;

		case BaseType::SInt:
			if (information.ElementSize > 4)
			{
				TODO_ERROR();
			}
			else
			{
				blit = [&](int dstX, int dstY, int dstZ, int dstLevel, int dstLayer, float u, float v, float w, int q, int a)
				{
					const auto size = srcImage->getImageSize();
					auto value = SampleImage<glm::ivec4>(deviceState, srcImage->getFormat(), srcImage->getData().subspan(size.LayerSize * a + size.Level[q].Offset, size.Level[q].LevelSize),
					                                     glm::uvec3{size.Level[q].Width, size.Level[q].Height, size.Level[q].Depth},
					                                     glm::fvec3{u / size.Level[q].Width, v / size.Level[q].Height, w / size.Level[q].Depth},
					                                     filter);
					SetPixel(deviceState, dstImage->getFormat(), dstImage, dstX, dstY, dstZ, dstLevel, dstLayer, &value.x);
				};
			}
			break;
			
		default:
			FATAL_ERROR();
		}
		
		for (const auto& region : regions)
		{
			assert(region.srcSubresource.aspectMask == region.dstSubresource.aspectMask);
			if (region.srcSubresource.aspectMask == (VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT))
			{
				blit = [&](int dstX, int dstY, int dstZ, int dstLevel, int dstLayer, float u, float v, float w, float q, int a)
				{
					assert(filter == VK_FILTER_NEAREST);
					const VkClearDepthStencilValue depth
					{
						GetDepthPixel(deviceState, srcImage->getFormat(), srcImage, u, v, w, q, a),
						GetStencilPixel(deviceState, srcImage->getFormat(), srcImage, u, v, w, q, a),
					};
					SetPixel(deviceState, dstImage->getFormat(), dstImage, dstX, dstY, dstZ, dstLevel, dstLayer, depth);
				};
			}
			else if (region.srcSubresource.aspectMask == VK_IMAGE_ASPECT_DEPTH_BIT)
			{
				blit = [&](int dstX, int dstY, int dstZ, int dstLevel, int dstLayer, float u, float v, float w, float q, int a)
				{
					assert(filter == VK_FILTER_NEAREST);
					const VkClearDepthStencilValue depth
					{
						GetDepthPixel(deviceState, srcImage->getFormat(), srcImage, u, v, w, q, a),
						information.DepthStencil.StencilOffset == INVALID_OFFSET ? 0u : GetStencilPixel(deviceState, dstImage->getFormat(), dstImage, dstX, dstY, dstZ, dstLevel, dstLayer),
					};
					SetPixel(deviceState, dstImage->getFormat(), dstImage, dstX, dstY, dstZ, dstLevel, dstLayer, depth);
				};
			}
			else if (region.srcSubresource.aspectMask == VK_IMAGE_ASPECT_STENCIL_BIT)
			{
				blit = [&](int dstX, int dstY, int dstZ, int dstLevel, int dstLayer, float u, float v, float w, float q, int a)
				{
					assert(filter == VK_FILTER_NEAREST);
					const VkClearDepthStencilValue depth
					{
						information.DepthStencil.DepthOffset == INVALID_OFFSET ? 0 : GetDepthPixel(deviceState, dstImage->getFormat(), dstImage, dstX, dstY, dstZ, dstLevel, dstLayer),
						GetStencilPixel(deviceState, srcImage->getFormat(), srcImage, u, v, w, q, a),
					};
					SetPixel(deviceState, dstImage->getFormat(), dstImage, dstX, dstY, dstZ, dstLevel, dstLayer, depth);
				};
			}
			else if (region.srcSubresource.aspectMask != VK_IMAGE_ASPECT_COLOR_BIT)
			{
				TODO_ERROR();
			}

			assert(region.srcSubresource.layerCount == region.dstSubresource.layerCount);
		
			auto dstWidth = region.dstOffsets[1].x - region.dstOffsets[0].x;
			auto dstHeight = region.dstOffsets[1].y - region.dstOffsets[0].y;
			auto dstDepth = region.dstOffsets[1].z - region.dstOffsets[0].z;
		
			auto negativeWidth = false;
			auto negativeHeight = false;
			auto negativeDepth = false;
		
			if (dstWidth < 0)
			{
				negativeWidth = true;
				dstWidth = -dstWidth;
			}
		
			if (dstHeight < 0)
			{
				negativeHeight = true;
				dstHeight = -dstHeight;
			}
		
			if (dstDepth < 0)
			{
				negativeDepth = true;
				dstDepth = -dstDepth;
			}

			for (auto layer = 0u; layer < region.srcSubresource.layerCount; layer++)
			{
				for (auto z = 0; z < dstDepth; z++)
				{
					for (auto y = 0; y < dstHeight; y++)
					{
						for (auto x = 0; x < dstWidth; x++)
						{
							const auto dstX = negativeWidth ? x + region.dstOffsets[1].x : x + region.dstOffsets[0].x;
							const auto dstY = negativeHeight ? y + region.dstOffsets[1].y : y + region.dstOffsets[0].y;
							const auto dstZ = negativeDepth ? z + region.dstOffsets[1].z : z + region.dstOffsets[0].z;

							const auto u = (dstX + 0.5f - region.dstOffsets[0].x) * (static_cast<float>(region.srcOffsets[1].x - region.srcOffsets[0].x) / (region.dstOffsets[1].x - region.dstOffsets[0].x)) + region.srcOffsets[0].x;
							const auto v = (dstY + 0.5f - region.dstOffsets[0].y) * (static_cast<float>(region.srcOffsets[1].y - region.srcOffsets[0].y) / (region.dstOffsets[1].y - region.dstOffsets[0].y)) + region.srcOffsets[0].y;
							const auto w = (dstZ + 0.5f - region.dstOffsets[0].z) * (static_cast<float>(region.srcOffsets[1].z - region.srcOffsets[0].z) / (region.dstOffsets[1].z - region.dstOffsets[0].z)) + region.srcOffsets[0].z;
							const auto q = region.srcSubresource.mipLevel;
							const auto a = region.srcSubresource.baseArrayLayer + layer;

							blit(dstX, dstY, dstZ, region.dstSubresource.mipLevel, region.dstSubresource.baseArrayLayer + layer, u, v, w, q, a);
						}
					}
				}
			}
		}
	}

private:
	Image* srcImage;
	Image* dstImage;
	std::vector<VkImageBlit> regions;
	VkFilter filter;
};

class UpdateBufferCommand final : public Command
{
public:
	UpdateBufferCommand(Buffer* dstBuffer, uint64_t dstOffset, uint64_t dataSize, const void* pData) :
		dstBuffer{dstBuffer},
		dstOffset{dstOffset},
		dataSize{dataSize}
	{
		assert(dataSize <= sizeof(data));
		memcpy(data, pData, dataSize);
	}

	~UpdateBufferCommand() override = default;

#if CV_DEBUG_LEVEL > 0
	void DebugOutput(DeviceState* deviceState) override
	{
		*deviceState->debugOutput << "UpdateBuffer: filling" <<
			" buffer " << dstBuffer <<
			" from " << dstOffset <<
			std::endl;
	}
#endif

	void Process(DeviceState*) override
	{
		const auto bufferData = reinterpret_cast<uint32_t*>(dstBuffer->getDataPtr(dstOffset, dataSize));
		memcpy(bufferData, data, dataSize);
	}

private:
	Buffer* dstBuffer;
	uint64_t dstOffset;
	uint64_t dataSize;
	uint8_t data[65536];
};

class FillBufferCommand final : public Command
{
public:
	FillBufferCommand(Buffer* dstBuffer, uint64_t dstOffset, uint64_t size, uint32_t data) :
		dstBuffer{dstBuffer},
		dstOffset{dstOffset},
		size{size},
		data{data}
	{
		if (this->size == VK_WHOLE_SIZE)
		{
			this->size = dstBuffer->getSize() - dstOffset;
		}
	}

	~FillBufferCommand() override = default;

#if CV_DEBUG_LEVEL > 0
	void DebugOutput(DeviceState* deviceState) override
	{
		*deviceState->debugOutput << "FillBuffer: filling" <<
			" buffer " << dstBuffer <<
			" from " << dstOffset <<
			" of size " << size <<
			" with " << data <<
			std::endl;
	}
#endif

	void Process(DeviceState*) override
	{
		const auto bufferData = dstBuffer->getDataPtr(dstOffset, size);
		const auto bufferData32 = reinterpret_cast<uint32_t*>(bufferData);

		const auto words = size / 4;
		for (auto i = 0u; i < words; i++)
		{
			bufferData32[i] = data;
		}

		if (size % 4)
		{
			const auto dataByte = reinterpret_cast<uint8_t*>(&data);
			for (auto i = 0u; i < size % 4; i++)
			{
				bufferData[size / 4 + i] = dataByte[i];
			}
		}
	}

private:
	Buffer* dstBuffer;
	uint64_t dstOffset;
	uint64_t size;
	uint32_t data;
};

class ResolveImageCommand final : public Command
{
public:
	ResolveImageCommand(Image* srcImage, Image* dstImage, std::vector<VkImageResolve> regions) :
		srcImage{srcImage},
		dstImage{dstImage},
		regions{std::move(regions)}
	{
	}

	~ResolveImageCommand() override = default;

#if CV_DEBUG_LEVEL > 0
	void DebugOutput(DeviceState* deviceState) override
	{
		*deviceState->debugOutput << "ResolveImage: resolving images " <<
			" from " << srcImage <<
			" to " << dstImage <<
			" on regions " << regions <<
			std::endl;
	}
#endif

	void Process(DeviceState* deviceState) override
	{
		assert(srcImage->getSamples() > VK_SAMPLE_COUNT_1_BIT);
		assert(dstImage->getSamples() == VK_SAMPLE_COUNT_1_BIT);

		std::function<void(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t)> resolve;
		const auto information = GetFormatInformation(dstImage->getFormat());
		switch (information.Base)
		{
		case BaseType::UNorm:
		case BaseType::SNorm:
		case BaseType::UScaled:
		case BaseType::SScaled:
		case BaseType::UFloat:
		case BaseType::SFloat:
		case BaseType::SRGB:
			if (information.ElementSize > 4)
			{
				TODO_ERROR();
			}
			else
			{
				resolve = [&](uint32_t srcX, uint32_t srcY, uint32_t srcZ, uint32_t srcLevel, uint32_t srcArray,
				              uint32_t dstX, uint32_t dstY, uint32_t dstZ, uint32_t dstLevel, uint32_t dstArray)
				{
					const auto size = srcImage->getImageSize();
					auto value = GetPixel<glm::fvec4>(deviceState, srcImage->getFormat(), srcImage->getData(size.LayerSize * srcArray + size.Level[srcLevel].Offset, size.Level[srcLevel].LevelSize),
					                                  glm::uvec3{size.Level[srcLevel].Width, size.Level[srcLevel].Height, size.Level[srcLevel].Depth},
					                                  glm::ivec3{srcX, srcY, srcZ}, glm::fvec4{});
					SetPixel(deviceState, dstImage->getFormat(), dstImage, dstX, dstY, dstZ, dstLevel, dstArray, &value.x);
				};
			}
			break;
		
		case BaseType::UInt:
			if (information.ElementSize > 4)
			{
				TODO_ERROR();
			}
			else
			{
				resolve = [&](uint32_t srcX, uint32_t srcY, uint32_t srcZ, uint32_t srcLevel, uint32_t srcArray,
				              uint32_t dstX, uint32_t dstY, uint32_t dstZ, uint32_t dstLevel, uint32_t dstArray)
				{
					const auto size = srcImage->getImageSize();
					auto value = GetPixel<glm::uvec4>(deviceState, srcImage->getFormat(), srcImage->getData(size.LayerSize * srcArray + size.Level[srcLevel].Offset, size.Level[srcLevel].LevelSize),
					                                  glm::uvec3{size.Level[srcLevel].Width, size.Level[srcLevel].Height, size.Level[srcLevel].Depth},
					                                  glm::ivec3{srcX, srcY, srcZ}, glm::uvec4{});
					SetPixel(deviceState, dstImage->getFormat(), dstImage, dstX, dstY, dstZ, dstLevel, dstArray, &value.x);
				};
			}
			break;
		
		case BaseType::SInt:
			if (information.ElementSize > 4)
			{
				TODO_ERROR();
			}
			else
			{
				resolve = [&](uint32_t srcX, uint32_t srcY, uint32_t srcZ, uint32_t srcLevel, uint32_t srcArray,
				              uint32_t dstX, uint32_t dstY, uint32_t dstZ, uint32_t dstLevel, uint32_t dstArray)
				{
					const auto size = srcImage->getImageSize();
					auto value = GetPixel<glm::ivec4>(deviceState, srcImage->getFormat(), srcImage->getData(size.LayerSize * srcArray + size.Level[srcLevel].Offset, size.Level[srcLevel].LevelSize),
					                                  glm::uvec3{size.Level[srcLevel].Width, size.Level[srcLevel].Height, size.Level[srcLevel].Depth},
					                                  glm::ivec3{srcX, srcY, srcZ}, glm::ivec4{});
					SetPixel(deviceState, dstImage->getFormat(), dstImage, dstX, dstY, dstZ, dstLevel, dstArray, &value.x);
				};
			}
			break;
		
		default:
			FATAL_ERROR();
		}
		
		for (const auto& region : regions)
		{
			assert(region.srcSubresource.aspectMask == region.dstSubresource.aspectMask);
			assert(region.srcSubresource.aspectMask == VK_IMAGE_ASPECT_COLOR_BIT);
			assert(region.srcSubresource.layerCount == region.dstSubresource.layerCount);
			
			for (auto layer = 0u; layer < region.srcSubresource.layerCount; layer++)
			{
				for (auto z = 0u; z < region.extent.depth; z++)
				{
					for (auto y = 0u; y < region.extent.height; y++)
					{
						for (auto x = 0u; x < region.extent.width; x++)
						{
							const auto srcX = region.srcOffset.x + x;
							const auto srcY = region.srcOffset.y + y;
							const auto srcZ = region.srcOffset.z + z;

							const auto dstX = region.dstOffset.x + x;
							const auto dstY = region.dstOffset.y + y;
							const auto dstZ = region.dstOffset.z + z;

							resolve(srcX, srcY, srcZ, region.srcSubresource.mipLevel, region.srcSubresource.baseArrayLayer + layer,
							        dstX, dstY, dstZ, region.dstSubresource.mipLevel, region.dstSubresource.baseArrayLayer + layer);
						}
					}
				}
			}
		}
	}

private:
	Image* srcImage;
	Image* dstImage;
	std::vector<VkImageResolve> regions;
};

class SetEventCommand final : public Command
{
public:
	SetEventCommand(Event* event, VkPipelineStageFlags stageMask):
		event{event},
		stageMask{stageMask}
	{
	}

	~SetEventCommand() override = default;

#if CV_DEBUG_LEVEL > 0
	void DebugOutput(DeviceState* deviceState) override
	{
		*deviceState->debugOutput << "SetEvent: Setting event  on " << event << std::endl;
	}
#endif

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

	~ResetEventCommand() override = default;

#if CV_DEBUG_LEVEL > 0
	void DebugOutput(DeviceState* deviceState) override
	{
		*deviceState->debugOutput << "ResetEvent: Resetting event  on " << event << std::endl;
	}
#endif

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

#if CV_DEBUG_LEVEL > 0
	void DebugOutput(DeviceState* deviceState) override
	{
		*deviceState->debugOutput << "PushConstants: pushing constant values" <<
			" with " << layout <<
			" and offset " << offset <<
			" and size " << size <<
			std::endl;
	}
#endif

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

	~BeginRenderPassCommand() override = default;

#if CV_DEBUG_LEVEL > 0
	void DebugOutput(DeviceState* deviceState) override
	{
		*deviceState->debugOutput << "BeginRenderPass: Beginning render pass " <<
			" on " << renderPass <<
			" with " << framebuffer <<
			" around " << renderArea <<
			" clearing " << clearValues <<
			std::endl;
	}
#endif

	void Process(DeviceState* deviceState) override
	{
		deviceState->graphicsPipelineState.currentSubpass = &renderPass->getSubpasses()[0];
		deviceState->graphicsPipelineState.currentSubpassIndex = 0;
		deviceState->graphicsPipelineState.currentRenderPass = renderPass;
		deviceState->graphicsPipelineState.currentFramebuffer = framebuffer;
		deviceState->graphicsPipelineState.currentRenderArea = renderArea;

		for (auto attachmentReference : deviceState->graphicsPipelineState.currentSubpass->colourAttachments)
		{
			if (attachmentReference.attachment != VK_ATTACHMENT_UNUSED)
			{
				const auto attachment = renderPass->getAttachments()[attachmentReference.attachment];
				auto imageView = framebuffer->getAttachments()[attachmentReference.attachment];
				if (attachment.loadOp == VK_ATTACHMENT_LOAD_OP_CLEAR)
				{
					ClearImage(deviceState, imageView->getImage(), attachment.format, imageView->getSubresourceRange().baseMipLevel,
					           imageView->getSubresourceRange().levelCount, imageView->getSubresourceRange().baseArrayLayer, imageView->getSubresourceRange().layerCount,
					           clearValues[attachmentReference.attachment].color);
				}
			}
		}

		const auto attachmentReference = deviceState->graphicsPipelineState.currentSubpass->depthStencilAttachment;
		if (attachmentReference.attachment != VK_ATTACHMENT_UNUSED)
		{
			const auto attachment = renderPass->getAttachments()[attachmentReference.attachment];
			const auto imageView = framebuffer->getAttachments()[attachmentReference.attachment];
			const auto formatInformation = GetFormatInformation(attachment.format);
			VkImageAspectFlags aspects;
			if (formatInformation.Format == VK_FORMAT_S8_UINT)
			{
				aspects = VK_IMAGE_ASPECT_STENCIL_BIT;
			}
			else if (formatInformation.DepthStencil.StencilOffset == INVALID_OFFSET)
			{
				aspects = VK_IMAGE_ASPECT_DEPTH_BIT;
			}
			else
			{
				aspects = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
			}
			if (attachment.loadOp == VK_ATTACHMENT_LOAD_OP_CLEAR)
			{
				ClearImage(deviceState, imageView->getImage(), attachment.format, imageView->getSubresourceRange().baseMipLevel,
				           imageView->getSubresourceRange().levelCount, imageView->getSubresourceRange().baseArrayLayer, imageView->getSubresourceRange().layerCount,
				           aspects, clearValues[attachmentReference.attachment].depthStencil);
			}
		}
	}

private:
	RenderPass* renderPass;
	Framebuffer* framebuffer;
	VkRect2D renderArea;
	std::vector<VkClearValue> clearValues;
};

class NextSubpassCommand final : public Command
{
public:
#if CV_DEBUG_LEVEL > 0
	void DebugOutput(DeviceState* deviceState) override
	{
		*deviceState->debugOutput << "NextSubpass: Next subpass" << std::endl;
	}
#endif
	
	void Process(DeviceState* deviceState) override
	{
		deviceState->graphicsPipelineState.currentSubpassIndex += 1;
		deviceState->graphicsPipelineState.currentSubpass = &deviceState->graphicsPipelineState.currentRenderPass->getSubpasses()[deviceState->graphicsPipelineState.currentSubpassIndex];
	}
};

class EndRenderPassCommand final : public Command
{
public:
#if CV_DEBUG_LEVEL > 0
	void DebugOutput(DeviceState* deviceState) override
	{
		*deviceState->debugOutput << "EndRenderPass: Ending render pass" << std::endl;
	}
#endif
	
	void Process(DeviceState* deviceState) override
	{
#if CV_DEBUG_LEVEL >= CV_DEBUG_IMAGE
		for (auto attachmentReference : deviceState->currentRenderPass->getSubpasses()[0].ColourAttachments)
		{
			if (attachmentReference.attachment != VK_ATTACHMENT_UNUSED)
			{
				auto imageView = deviceState->currentFramebuffer->getAttachments()[attachmentReference.attachment];
				DumpImage("LatestRender" + std::to_string(attachmentReference.attachment) + ".dds", imageView->getImage(), imageView);
			}
		}

		const auto attachmentReference = deviceState->currentRenderPass->getSubpasses()[0].DepthStencilAttachment;
		if (attachmentReference.attachment != VK_ATTACHMENT_UNUSED)
		{
			const auto imageView = deviceState->currentFramebuffer->getAttachments()[attachmentReference.attachment];
			DumpImage("LatestRenderDepth.dds", imageView->getImage(), imageView);
		}
#endif

		deviceState->graphicsPipelineState.currentSubpass = nullptr;
		deviceState->graphicsPipelineState.currentSubpassIndex = 0;
		deviceState->graphicsPipelineState.currentRenderPass = nullptr;
		deviceState->graphicsPipelineState.currentFramebuffer = nullptr;
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

	~ExecuteCommandsCommand() override = default;

#if CV_DEBUG_LEVEL > 0
	void DebugOutput(DeviceState* deviceState) override
	{
		*deviceState->debugOutput << "ExecuteCommands: Executing commands" << std::endl;
	}
#endif

	void Process(DeviceState* deviceState) override
	{
		for (const auto commandBuffer : commands)
		{
			RunCommands(deviceState, commandBuffer->commands);
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

	auto next = static_cast<const VkBaseInStructure*>(pBeginInfo->pNext);
	while (next)
	{
		const auto type = next->sType;
		switch (type)
		{
		case VK_STRUCTURE_TYPE_DEVICE_GROUP_COMMAND_BUFFER_BEGIN_INFO:
			TODO_ERROR();

		default:
			break;
		}
		next = next->pNext;
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

		next = static_cast<const VkBaseInStructure*>(pBeginInfo->pInheritanceInfo->pNext);
		while (next)
		{
			const auto type = next->sType;
			switch (type)
			{
			case VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_CONDITIONAL_RENDERING_INFO_EXT:
				TODO_ERROR();

			default:
				break;
			}
			next = next->pNext;
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
	ForceReset();

	return VK_SUCCESS;
}

void CommandBuffer::BlitImage(VkImage srcImage, VkImageLayout, VkImage dstImage, VkImageLayout, uint32_t regionCount, const VkImageBlit* pRegions, VkFilter filter)
{
	assert(state == State::Recording);
	commands.push_back(std::make_unique<BlitImageCommand>(UnwrapVulkan<Image>(srcImage), UnwrapVulkan<Image>(dstImage), ArrayToVector(regionCount, pRegions), filter));
}

void CommandBuffer::UpdateBuffer(VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize dataSize, const void* pData)
{
	assert(state == State::Recording);
	commands.push_back(std::make_unique<UpdateBufferCommand>(UnwrapVulkan<Buffer>(dstBuffer), dstOffset, dataSize, pData));
}

void CommandBuffer::FillBuffer(VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize size, uint32_t data)
{
	assert(state == State::Recording);
	commands.push_back(std::make_unique<FillBufferCommand>(UnwrapVulkan<Buffer>(dstBuffer), dstOffset, size, data));
}

void CommandBuffer::ResolveImage(VkImage srcImage, VkImageLayout, VkImage dstImage, VkImageLayout, uint32_t regionCount, const VkImageResolve* pRegions)
{
	assert(state == State::Recording);
	commands.push_back(std::make_unique<ResolveImageCommand>(UnwrapVulkan<Image>(srcImage), UnwrapVulkan<Image>(dstImage), ArrayToVector(regionCount, pRegions)));
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

	auto renderArea = pRenderPassBegin->renderArea;
	
	auto next = static_cast<const VkBaseInStructure*>(pRenderPassBegin->pNext);
	while (next)
	{
		const auto type = next->sType;
		switch (type)
		{
		case VK_STRUCTURE_TYPE_DEVICE_GROUP_RENDER_PASS_BEGIN_INFO:
			{
				const auto renderPassInfo = reinterpret_cast<const VkDeviceGroupRenderPassBeginInfo*>(next);
				if (renderPassInfo->deviceMask != 1)
				{
					TODO_ERROR();
				}
				if (renderPassInfo->deviceRenderAreaCount)
				{
					renderArea = renderPassInfo->pDeviceRenderAreas[0];
				}
				break;
			}
			
		case VK_STRUCTURE_TYPE_RENDER_PASS_ATTACHMENT_BEGIN_INFO_KHR:
			TODO_ERROR();
			
		case VK_STRUCTURE_TYPE_RENDER_PASS_SAMPLE_LOCATIONS_BEGIN_INFO_EXT:
			TODO_ERROR();

		default:
			break;
		}
		next = next->pNext;
	}

	std::vector<VkClearValue> clearValues(pRenderPassBegin->clearValueCount);
	memcpy(clearValues.data(), pRenderPassBegin->pClearValues, sizeof(VkClearValue) * pRenderPassBegin->clearValueCount);
	commands.push_back(std::make_unique<BeginRenderPassCommand>(UnwrapVulkan<RenderPass>(pRenderPassBegin->renderPass), 
	                                                            UnwrapVulkan<Framebuffer>(pRenderPassBegin->framebuffer),
	                                                            renderArea,
	                                                            clearValues));
}

void CommandBuffer::NextSubpass(VkSubpassContents contents)
{
	assert(state == State::Recording);
	commands.push_back(std::make_unique<NextSubpassCommand>());
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
		TODO_ERROR();
	}

	commands.clear();

	state = State::Initial;
}

VkResult CommandBuffer::Submit()
{
	RunCommands(deviceState, commands);
	return VK_SUCCESS;
}
