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

class CopyBufferCommand final : public Command
{
public:
	CopyBufferCommand(Buffer* srcBuffer, Buffer* dstBuffer, std::vector<VkBufferCopy> regions):
		srcBuffer{srcBuffer},
		dstBuffer{dstBuffer},
		regions{std::move(regions)}
	{
	}

#if CV_DEBUG_LEVEL > 0
	void DebugOutput(DeviceState* deviceState) override
	{
		*deviceState->debugOutput << "CopyBuffer: copying buffers " <<
			" from " << srcBuffer <<
			" to " << dstBuffer <<
			" on regions " << regions <<
			std::endl;
	}
#endif

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

#if CV_DEBUG_LEVEL > 0
	void DebugOutput(DeviceState* deviceState) override
	{
		*deviceState->debugOutput << "CopyImage: copying images " <<
			" from " << srcImage <<
			" to " << dstImage <<
			" on regions " << regions <<
			std::endl;
	}
#endif

	void Process(DeviceState*) override
	{
		const auto& srcFormat = GetFormatInformation(srcImage->getFormat());
		const auto& dstFormat = GetFormatInformation(dstImage->getFormat());
		
		for (const auto& region : regions)
		{
			if (region.srcSubresource.aspectMask != VK_IMAGE_ASPECT_COLOR_BIT &&
				(region.srcSubresource.aspectMask != VK_IMAGE_ASPECT_DEPTH_BIT || srcFormat.Normal.GreenOffset != INVALID_OFFSET) &&
				(region.srcSubresource.aspectMask != VK_IMAGE_ASPECT_STENCIL_BIT || srcFormat.Normal.GreenOffset != INVALID_OFFSET))
			{
				FATAL_ERROR();
			}

			if (region.dstSubresource.aspectMask != VK_IMAGE_ASPECT_COLOR_BIT &&
				(region.dstSubresource.aspectMask != VK_IMAGE_ASPECT_DEPTH_BIT || dstFormat.Normal.GreenOffset != INVALID_OFFSET) &&
				(region.dstSubresource.aspectMask != VK_IMAGE_ASPECT_STENCIL_BIT || dstFormat.Normal.GreenOffset != INVALID_OFFSET))
			{
				FATAL_ERROR();
			}

			assert(region.srcSubresource.layerCount == region.dstSubresource.layerCount);

			if (region.srcSubresource.layerCount != 1)
			{
				FATAL_ERROR();
			}

			if (region.dstSubresource.layerCount != 1)
			{
				FATAL_ERROR();
			}

			const auto& srcLevel = srcImage->getImageSize().Level[region.srcSubresource.mipLevel];
			const auto& dstLevel = dstImage->getImageSize().Level[region.dstSubresource.mipLevel];
			const auto pixelSize = srcImage->getImageSize().PixelSize;

			assert(srcLevel.LevelSize == dstLevel.LevelSize);
			assert(srcLevel.PlaneSize == dstLevel.PlaneSize);
			assert(srcLevel.Stride == dstLevel.Stride);
			assert(srcLevel.Width == dstLevel.Width);
			assert(srcLevel.Height == dstLevel.Height);
			assert(srcLevel.Depth == dstLevel.Depth);
			
			if (region.srcOffset.x == 0 && region.srcOffset.y == 0 && region.srcOffset.z == 0 &&
				region.dstOffset.x == 0 && region.dstOffset.y == 0 && region.dstOffset.z == 0 &&
				region.extent.width == srcLevel.Width && region.extent.height == srcLevel.Height && region.extent.depth == srcLevel.Depth)
			{
				memcpy(dstImage->getDataPtr(dstLevel.Offset + dstImage->getImageSize().LayerSize * region.dstSubresource.baseArrayLayer, dstLevel.LevelSize),
				       srcImage->getDataPtr(srcLevel.Offset + srcImage->getImageSize().LayerSize * region.srcSubresource.baseArrayLayer, srcLevel.LevelSize),
				       srcLevel.LevelSize);
			}
			else
			{
				const auto srcBaseOffset = srcLevel.Offset + srcImage->getImageSize().LayerSize * region.srcSubresource.baseArrayLayer;
				const auto dstBaseOffset = dstLevel.Offset + dstImage->getImageSize().LayerSize * region.dstSubresource.baseArrayLayer;

				if (srcFormat.Type == FormatType::Normal && dstFormat.Type == FormatType::Normal)
				{
					const auto stride = pixelSize * region.extent.width;
					for (auto z = 0u; z < region.extent.depth; z++)
					{
						const auto srcZ = z + region.srcOffset.z;
						const auto dstZ = z + region.dstOffset.z;
						for (auto y = 0u; y < region.extent.height; y++)
						{
							const auto srcY = y + region.srcOffset.y;
							const auto srcX = region.srcOffset.x;
							const auto srcOffset = srcBaseOffset + srcZ * srcLevel.PlaneSize + srcY * srcLevel.Stride + srcX * pixelSize;
							
							const auto dstY = y + region.dstOffset.y;
							const auto dstX = region.dstOffset.x;
							const auto dstOffset = dstBaseOffset + dstZ * dstLevel.PlaneSize + dstY * dstLevel.Stride + dstX * pixelSize;
							
							memcpy(dstImage->getDataPtr(dstOffset, stride), srcImage->getDataPtr(srcOffset, stride), stride);
						}
					}
				}
				else if (srcFormat.Type == FormatType::Compressed && dstFormat.Type == FormatType::Compressed)
				{
					const auto srcOffsetX = (region.srcOffset.x + srcFormat.Compressed.BlockWidth - 1) / srcFormat.Compressed.BlockWidth;
					const auto srcOffsetY = (region.srcOffset.y + srcFormat.Compressed.BlockHeight - 1) / srcFormat.Compressed.BlockHeight;
					const auto dstOffsetX = (region.dstOffset.x + dstFormat.Compressed.BlockWidth - 1) / dstFormat.Compressed.BlockWidth;
					const auto dstOffsetY = (region.dstOffset.y + dstFormat.Compressed.BlockHeight - 1) / dstFormat.Compressed.BlockHeight;
					const auto width = (region.extent.width + srcFormat.Compressed.BlockWidth - 1) / srcFormat.Compressed.BlockWidth;
					const auto height = (region.extent.height + srcFormat.Compressed.BlockHeight - 1) / srcFormat.Compressed.BlockHeight;
					const auto stride = pixelSize * width;

					for (auto z = 0u; z < region.extent.depth; z++)
					{
						const auto srcZ = z + region.srcOffset.z;
						const auto dstZ = z + region.dstOffset.z;
						for (auto y = 0u; y < height; y++)
						{
							const auto srcY = y + srcOffsetY;
							const auto srcX = srcOffsetX;
							const auto srcOffset = srcBaseOffset + srcZ * srcLevel.PlaneSize + srcY * srcLevel.Stride + srcX * pixelSize;
							
							const auto dstY = y + dstOffsetY;
							const auto dstX = dstOffsetX;
							const auto dstOffset = dstBaseOffset + dstZ * dstLevel.PlaneSize + dstY * dstLevel.Stride + dstX * pixelSize;
							
							memcpy(dstImage->getDataPtr(dstOffset, stride), srcImage->getDataPtr(srcOffset, stride), stride);
						}
					}
				}
				else if (srcFormat.Type == FormatType::Compressed)
				{
					const auto offsetX = (region.srcOffset.x + srcFormat.Compressed.BlockWidth - 1) / srcFormat.Compressed.BlockWidth;
					const auto offsetY = (region.srcOffset.y + srcFormat.Compressed.BlockHeight - 1) / srcFormat.Compressed.BlockHeight;
					const auto width = (region.extent.width + srcFormat.Compressed.BlockWidth - 1) / srcFormat.Compressed.BlockWidth;
					const auto height = (region.extent.height + srcFormat.Compressed.BlockHeight - 1) / srcFormat.Compressed.BlockHeight;
					const auto stride = pixelSize * width;

					for (auto z = 0u; z < region.extent.depth; z++)
					{
						const auto srcZ = z + region.srcOffset.z;
						const auto dstZ = z + region.dstOffset.z;
						for (auto y = 0u; y < height; y++)
						{
							const auto srcY = y + offsetY;
							const auto srcX = offsetX;
							const auto srcOffset = srcBaseOffset + srcZ * srcLevel.PlaneSize + srcY * srcLevel.Stride + srcX * pixelSize;

							const auto dstY = y + region.dstOffset.y;
							const auto dstX = region.dstOffset.x;
							const auto dstOffset = dstBaseOffset + dstZ * dstLevel.PlaneSize + dstY * dstLevel.Stride + dstX * pixelSize;

							memcpy(dstImage->getDataPtr(dstOffset, stride), srcImage->getDataPtr(srcOffset, stride), stride);
						}
					}
				}
				else if (dstFormat.Type == FormatType::Compressed)
				{
					const auto stride = pixelSize * region.extent.width;
					const auto offsetX = (region.dstOffset.x + dstFormat.Compressed.BlockWidth - 1) / dstFormat.Compressed.BlockWidth;
					const auto offsetY = (region.dstOffset.y + dstFormat.Compressed.BlockHeight - 1) / dstFormat.Compressed.BlockHeight;
					
					for (auto z = 0u; z < region.extent.depth; z++)
					{
						const auto srcZ = z + region.srcOffset.z;
						const auto dstZ = z + region.dstOffset.z;
						for (auto y = 0u; y < region.extent.height; y++)
						{
							const auto srcY = y + region.srcOffset.y;
							const auto srcX = region.srcOffset.x;
							const auto srcOffset = srcBaseOffset + srcZ * srcLevel.PlaneSize + srcY * srcLevel.Stride + srcX * pixelSize;
							
							const auto dstY = y + offsetY;
							const auto dstX = offsetX;
							const auto dstOffset = dstBaseOffset + dstZ * dstLevel.PlaneSize + dstY * dstLevel.Stride + dstX * pixelSize;
							
							memcpy(dstImage->getDataPtr(dstOffset, stride), srcImage->getDataPtr(srcOffset, stride), stride);
						}
					}
				}
				else
				{
					assert(false);
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

#if CV_DEBUG_LEVEL > 0
	void DebugOutput(DeviceState* deviceState) override
	{
		*deviceState->debugOutput << "ImageCommand: blitting images " <<
			" from " << srcImage <<
			" to " << dstImage <<
			" on regions " << regions <<
			" with filter " << filter <<
			std::endl;
	}
#endif

	void Process(DeviceState* deviceState) override
	{
		std::function<void(float, float, float, float, float)> blit;
		const auto information = GetFormatInformation(dstImage->getFormat());
		switch (information.Base)
		{
		case BaseType::UNorm:
		case BaseType::SNorm:
		case BaseType::UFloat:
		case BaseType::SFloat:
		case BaseType::SRGB:
			if (information.ElementSize > 4)
			{
				FATAL_ERROR();
			}
			else
			{
				blit = [&](float u, float v, float w, float q, float a)
				{
					auto value = SampleImage<glm::fvec4>(deviceState, srcImage->getFormat(), srcImage->getData(),
					                                     glm::uvec3{srcImage->getWidth(), srcImage->getHeight(), srcImage->getDepth()},
					                                     glm::fvec3{u / srcImage->getWidth(), v / srcImage->getHeight(), w / srcImage->getDepth()},
					                                     filter);
					SetPixel(deviceState, dstImage->getFormat(), dstImage, static_cast<int>(u), static_cast<int>(v), static_cast<int>(w), static_cast<int>(q), static_cast<int>(a), &value.x);
				};
			}
			break;
			
		case BaseType::UScaled:
		case BaseType::UInt:
			if (information.ElementSize > 4)
			{
				FATAL_ERROR();
			}
			else
			{
				blit = [&](float u, float v, float w, float q, float a)
				{
					auto value = SampleImage<glm::uvec4>(deviceState, srcImage->getFormat(), srcImage->getData(),
					                                     glm::uvec3{srcImage->getWidth(), srcImage->getHeight(), srcImage->getDepth()},
					                                     glm::fvec3{u / srcImage->getWidth(), v / srcImage->getHeight(), w / srcImage->getDepth()},
					                                     filter);
					SetPixel(deviceState, dstImage->getFormat(), dstImage, static_cast<int>(u), static_cast<int>(v), static_cast<int>(w), static_cast<int>(q), static_cast<int>(a), &value.x);
				};
			}
			break;

		case BaseType::SScaled:
		case BaseType::SInt:
			FATAL_ERROR();
			
		default: FATAL_ERROR();
		}
		
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
		
			// TODO: Handle region.dstOffsets[1] < region.dstOffsets[0]
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
		
			constexpr auto currentArray = 0;
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
						const auto a = currentArray - region.dstSubresource.baseArrayLayer + region.srcSubresource.baseArrayLayer;

						if (q != 0)
						{
							FATAL_ERROR();
						}
						if (a != 0)
						{
							FATAL_ERROR();
						}

						blit(u, v, w, q, a);
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

class CopyBufferToImageCommand final : public Command
{
public:
	CopyBufferToImageCommand(Buffer* srcBuffer, Image* dstImage, std::vector<VkBufferImageCopy> regions):
		srcBuffer{srcBuffer},
		dstImage{dstImage},
		regions{std::move(regions)}
	{
	}

#if CV_DEBUG_LEVEL > 0
	void DebugOutput(DeviceState* deviceState) override
	{
		*deviceState->debugOutput << "CopyBufferToImage: copying buffer to image " <<
			" from " << srcBuffer <<
			" to " << dstImage <<
			" on regions " << regions <<
			std::endl;
	}
#endif

	void Process(DeviceState*) override
	{
		const auto& format = GetFormatInformation(dstImage->getFormat());
		if (format.Type == FormatType::Compressed)
		{
			ProcessCompressed(format);
		}
		else
		{
			ProcessNormal(format);
		}
	}

	void ProcessNormal(const FormatInformation& format)
	{
		for (const auto& region : regions)
		{
			if (region.imageSubresource.aspectMask != VK_IMAGE_ASPECT_COLOR_BIT &&
				(region.imageSubresource.aspectMask != VK_IMAGE_ASPECT_DEPTH_BIT || format.Normal.GreenOffset != INVALID_OFFSET) &&
				(region.imageSubresource.aspectMask != VK_IMAGE_ASPECT_STENCIL_BIT || format.Normal.GreenOffset != INVALID_OFFSET))
			{
				FATAL_ERROR();
			}

			if (region.imageSubresource.layerCount != 1)
			{
				FATAL_ERROR();
			}

			auto rowLength = region.bufferRowLength;
			if (rowLength == 0)
			{
				rowLength = region.imageExtent.width;
			}

			auto imageHeight = region.bufferImageHeight;
			if (rowLength == 0)
			{
				imageHeight = region.imageExtent.height;
			}

			const auto& level = dstImage->getImageSize().Level[region.imageSubresource.mipLevel];
			if (region.imageOffset.x == 0 && region.imageOffset.y == 0 && region.imageOffset.z == 0 &&
				region.imageExtent.width == level.Width && region.imageExtent.height == level.Height && region.imageExtent.depth == level.Depth &&
				rowLength == level.Width && imageHeight == level.Height)
			{
				memcpy(dstImage->getDataPtr(level.Offset + dstImage->getImageSize().LayerSize * region.imageSubresource.baseArrayLayer, level.LevelSize),
				       srcBuffer->getDataPtr(region.bufferOffset, level.LevelSize),
				       level.LevelSize);
			}
			else
			{
				// address of (x,y,z) = region->bufferOffset + (((z * imageHeight) + y) * rowLength + x) * texelBlockSize;
				//
				// where x,y,z range from (0,0,0) to region->imageExtent.{width,height,depth}.
				FATAL_ERROR();
			}
		}
	}

	void ProcessCompressed(const FormatInformation& format)
	{
		for (const auto& region : regions)
		{
			assert(region.imageSubresource.aspectMask == VK_IMAGE_ASPECT_COLOR_BIT);
			
			if (region.imageSubresource.layerCount != 1)
			{
				FATAL_ERROR();
			}

			const auto extentWidth = (region.imageExtent.width + format.Compressed.BlockWidth - 1) / format.Compressed.BlockWidth;
			const auto extentHeight = (region.imageExtent.height + format.Compressed.BlockHeight - 1) / format.Compressed.BlockHeight;
			
			auto rowLength = region.bufferRowLength;
			if (rowLength == 0)
			{
				rowLength = region.imageExtent.width;
			}

			auto imageHeight = region.bufferImageHeight;
			if (rowLength == 0)
			{
				imageHeight = region.imageExtent.height;
			}

			rowLength /= format.Compressed.BlockWidth;
			imageHeight /= format.Compressed.BlockHeight;

			const auto& level = dstImage->getImageSize().Level[region.imageSubresource.mipLevel];
			if (region.imageOffset.x == 0 && region.imageOffset.y == 0 && region.imageOffset.z == 0 &&
				extentWidth == level.Width && extentHeight == level.Height && region.imageExtent.depth == level.Depth &&
				rowLength == level.Width && imageHeight == level.Height)
			{
				memcpy(dstImage->getDataPtr(level.Offset + dstImage->getImageSize().LayerSize * region.imageSubresource.baseArrayLayer, level.LevelSize),
				       srcBuffer->getDataPtr(region.bufferOffset, level.LevelSize),
				       level.LevelSize);
			}
			else
			{
				// address of (x,y,z) = region->bufferOffset + (((z * imageHeight) + y) * rowLength + x) * compressedTexelBlockSizeInBytes;
				//
				// where x,y,z range from (0,0,0) to region->imageExtent.{width/compressedTexelBlockWidth,height/compressedTexelBlockHeight,depth/compressedTexelBlockDepth}.
				FATAL_ERROR();
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

#if CV_DEBUG_LEVEL > 0
	void DebugOutput(DeviceState* deviceState) override
	{
		*deviceState->debugOutput << "CopyImageToBuffer: copying image to buffer " <<
			" from " << srcImage <<
			" to " << dstBuffer <<
			" on regions " << regions <<
			std::endl;
	}
#endif

	void Process(DeviceState*) override
	{
		const auto& format = GetFormatInformation(srcImage->getFormat());
		if (format.Type == FormatType::Compressed)
		{
			ProcessCompressed(format);
		}
		else
		{
			ProcessNormal(format);
		}
	}

	void ProcessNormal(const FormatInformation& format)
	{
		for (const auto& region : regions)
		{
			if (region.imageSubresource.aspectMask != VK_IMAGE_ASPECT_COLOR_BIT &&
				(region.imageSubresource.aspectMask != VK_IMAGE_ASPECT_DEPTH_BIT || format.Normal.GreenOffset != INVALID_OFFSET) &&
				(region.imageSubresource.aspectMask != VK_IMAGE_ASPECT_STENCIL_BIT || format.Normal.GreenOffset != INVALID_OFFSET))
			{
				FATAL_ERROR();
			}

			if (region.imageSubresource.layerCount != 1)
			{
				FATAL_ERROR();
			}

			auto rowLength = region.bufferRowLength;
			if (rowLength == 0)
			{
				rowLength = region.imageExtent.width;
			}

			auto imageHeight = region.bufferImageHeight;
			if (rowLength == 0)
			{
				imageHeight = region.imageExtent.height;
			}

			const auto& level = srcImage->getImageSize().Level[region.imageSubresource.mipLevel];
			if (region.imageOffset.x == 0 && region.imageOffset.y == 0 && region.imageOffset.z == 0 &&
				region.imageExtent.width == level.Width && region.imageExtent.height == level.Height && region.imageExtent.depth == level.Depth &&
				rowLength == level.Width && imageHeight == level.Height)
			{
				memcpy(dstBuffer->getDataPtr(region.bufferOffset, level.LevelSize),
				       srcImage->getDataPtr(level.Offset + srcImage->getImageSize().LayerSize * region.imageSubresource.baseArrayLayer, level.LevelSize),
				       level.LevelSize);
			}
			else
			{
				// address of (x,y,z) = region->bufferOffset + (((z * imageHeight) + y) * rowLength + x) * texelBlockSize;
				//
				// where x,y,z range from (0,0,0) to region->imageExtent.{width,height,depth}.
				FATAL_ERROR();
			}
		}
	}

	void ProcessCompressed(const FormatInformation& format)
	{
		for (const auto& region : regions)
		{
			assert(region.imageSubresource.aspectMask == VK_IMAGE_ASPECT_COLOR_BIT);
			
			if (region.imageSubresource.layerCount != 1)
			{
				FATAL_ERROR();
			}

			const auto extentWidth = (region.imageExtent.width + format.Compressed.BlockWidth - 1) / format.Compressed.BlockWidth;
			const auto extentHeight = (region.imageExtent.height + format.Compressed.BlockHeight - 1) / format.Compressed.BlockHeight;

			auto rowLength = region.bufferRowLength;
			if (rowLength == 0)
			{
				rowLength = region.imageExtent.width;
			}

			auto imageHeight = region.bufferImageHeight;
			if (rowLength == 0)
			{
				imageHeight = region.imageExtent.height;
			}

			rowLength /= format.Compressed.BlockWidth;
			imageHeight /= format.Compressed.BlockHeight;

			const auto& level = srcImage->getImageSize().Level[region.imageSubresource.mipLevel];
			if (region.imageOffset.x == 0 && region.imageOffset.y == 0 && region.imageOffset.z == 0 &&
				extentWidth == level.Width && extentHeight == level.Height && region.imageExtent.depth == level.Depth &&
				rowLength == level.Width && imageHeight == level.Height)
			{
				memcpy(dstBuffer->getDataPtr(region.bufferOffset, level.LevelSize),
				       srcImage->getDataPtr(level.Offset + srcImage->getImageSize().LayerSize * region.imageSubresource.baseArrayLayer, level.LevelSize),
				       level.LevelSize);
			}
			else
			{
				// address of (x,y,z) = region->bufferOffset + (((z * imageHeight) + y) * rowLength + x) * texelBlockSize;
				//
				// where x,y,z range from (0,0,0) to region->imageExtent.{width,height,depth}.
				FATAL_ERROR();
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
		*deviceState->debugOutput << "PushConstants: pushing constants " <<
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
		deviceState->currentRenderPass = renderPass;
		deviceState->currentFramebuffer = framebuffer;
		deviceState->currentRenderArea = renderArea;
		
		if (renderPass->getSubpasses().size() != 1)
		{
			FATAL_ERROR();
		}

		for (auto attachmentReference : renderPass->getSubpasses()[0].ColourAttachments)
		{
			if (attachmentReference.attachment != VK_ATTACHMENT_UNUSED)
			{
				const auto attachment = renderPass->getAttachments()[attachmentReference.attachment];
				auto imageView = framebuffer->getAttachments()[attachmentReference.attachment];
				if (attachment.loadOp == VK_ATTACHMENT_LOAD_OP_CLEAR)
				{
					ClearImage(deviceState, imageView->getImage(), attachment.format, 0, 
					           imageView->getImage()->getMipLevels(), 0, imageView->getImage()->getArrayLayers(),
					           clearValues[attachmentReference.attachment].color);
				}
			}
		}

		const auto attachmentReference = renderPass->getSubpasses()[0].DepthStencilAttachment;
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
			else if (formatInformation.Normal.GreenOffset == INVALID_OFFSET)
			{
				aspects = VK_IMAGE_ASPECT_DEPTH_BIT;
			}
			else
			{
				aspects = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
			}
			if (attachment.loadOp == VK_ATTACHMENT_LOAD_OP_CLEAR)
			{
				ClearImage(deviceState, imageView->getImage(), attachment.format, 0, 1, 0, 1, aspects, clearValues[attachmentReference.attachment].depthStencil);
			}
		}
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
		
		deviceState->currentRenderPass = nullptr;
		deviceState->currentFramebuffer = nullptr;
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
	RunCommands(deviceState, commands);
	return VK_SUCCESS;
}
