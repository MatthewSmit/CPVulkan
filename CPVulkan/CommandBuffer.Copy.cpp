#include "CommandBuffer.h"
#include "CommandBuffer.Internal.h"

#include "Buffer.h"
#include "DebugHelper.h"
#include "DeviceState.h"
#include "Formats.h"
#include "Image.h"
#include "Util.h"

#include <glm/glm.hpp>

#include <fstream>

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

		if (srcFormat.Type == FormatType::DepthStencil)
		{
			assert(dstFormat.Type == FormatType::DepthStencil);
			ProcessDepthStencil(srcFormat, dstFormat);
		}
		else
		{
			for (const auto& region : regions)
			{
				assert(region.srcSubresource.aspectMask == VK_IMAGE_ASPECT_COLOR_BIT);
				assert(region.dstSubresource.aspectMask == VK_IMAGE_ASPECT_COLOR_BIT);

				auto layerCount = region.srcSubresource.layerCount;
				auto depth = region.extent.depth;
				auto src3DTexture = false;
				auto dst3DTexture = false;
				if (region.srcSubresource.layerCount != region.dstSubresource.layerCount)
				{
					depth = 1;
					if (region.srcSubresource.layerCount == 1)
					{
						src3DTexture = true;
						layerCount = region.dstSubresource.layerCount;
						assert(layerCount == region.extent.depth);
					}
					else if (region.dstSubresource.layerCount == 1)
					{
						dst3DTexture = true;
						layerCount = region.srcSubresource.layerCount;
						assert(layerCount == region.extent.depth);
					}
					else
					{
						assert(false);
					}
				}

				const auto& srcLevel = gsl::at(srcImage->getImageSize().Level, region.srcSubresource.mipLevel);
				const auto& dstLevel = gsl::at(dstImage->getImageSize().Level, region.dstSubresource.mipLevel);
				const auto pixelSize = srcImage->getImageSize().PixelSize;
				for (auto layer = 0u; layer < layerCount; layer++)
				{
					const auto srcBaseOffset = src3DTexture
						                           ? srcLevel.Offset
						                           : srcLevel.Offset + srcImage->getImageSize().LayerSize * (layer + region.srcSubresource.baseArrayLayer);
					const auto dstBaseOffset = dst3DTexture
						                           ? dstLevel.Offset
						                           : dstLevel.Offset + dstImage->getImageSize().LayerSize * (layer + region.dstSubresource.baseArrayLayer);

					if (region.srcOffset.x == 0 && region.srcOffset.y == 0 && region.srcOffset.z == 0 &&
						region.dstOffset.x == 0 && region.dstOffset.y == 0 && region.dstOffset.z == 0 &&
						region.extent.width == srcLevel.Width && region.extent.height == srcLevel.Height && region.extent.depth == srcLevel.Depth &&
						srcLevel.Width == dstLevel.Width && srcLevel.Height == dstLevel.Height && srcLevel.Depth == dstLevel.Depth)
					{
						assert(srcLevel.LevelSize == dstLevel.LevelSize);
						assert(srcLevel.PlaneSize == dstLevel.PlaneSize);
						assert(srcLevel.Stride == dstLevel.Stride);

						memcpy(dstImage->getDataPtr(dstBaseOffset, dstLevel.LevelSize),
						       srcImage->getDataPtr(srcBaseOffset, srcLevel.LevelSize),
						       srcLevel.LevelSize);
					}
					else
					{
						if (srcFormat.Type == FormatType::Normal && dstFormat.Type == FormatType::Normal)
						{
							const auto stride = pixelSize * region.extent.width;
							for (auto z = 0u; z < depth; z++)
							{
								const auto srcZ = z + region.srcOffset.z + (src3DTexture ? layer : 0);
								const auto dstZ = z + region.dstOffset.z + (dst3DTexture ? layer : 0);
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

							for (auto z = 0u; z < depth; z++)
							{
								const auto srcZ = z + region.srcOffset.z + (src3DTexture ? layer : 0);
								const auto dstZ = z + region.dstOffset.z + (dst3DTexture ? layer : 0);
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

							for (auto z = 0u; z < depth; z++)
							{
								const auto srcZ = z + region.srcOffset.z + (src3DTexture ? layer : 0);
								const auto dstZ = z + region.dstOffset.z + (dst3DTexture ? layer : 0);
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

							for (auto z = 0u; z < depth; z++)
							{
								const auto srcZ = z + region.srcOffset.z + (src3DTexture ? layer : 0);
								const auto dstZ = z + region.dstOffset.z + (dst3DTexture ? layer : 0);
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
		}
	}

	void ProcessDepthStencil(const FormatInformation& srcFormat, const FormatInformation& dstFormat)
	{
		assert(srcFormat.Format == dstFormat.Format);
		for (const auto& region : regions)
		{
			assert(region.srcSubresource.aspectMask == region.dstSubresource.aspectMask);

			auto layerCount = region.srcSubresource.layerCount;
			auto depth = region.extent.depth;
			auto src3DTexture = false;
			auto dst3DTexture = false;
			if (region.srcSubresource.layerCount != region.dstSubresource.layerCount)
			{
				depth = 1;
				if (region.srcSubresource.layerCount == 1)
				{
					src3DTexture = true;
					layerCount = region.dstSubresource.layerCount;
					assert(layerCount == region.extent.depth);
				}
				else if (region.dstSubresource.layerCount == 1)
				{
					dst3DTexture = true;
					layerCount = region.srcSubresource.layerCount;
					assert(layerCount == region.extent.depth);
				}
				else
				{
					assert(false);
				}
			}

			const auto& srcLevel = gsl::at(srcImage->getImageSize().Level, region.srcSubresource.mipLevel);
			const auto& dstLevel = gsl::at(dstImage->getImageSize().Level, region.dstSubresource.mipLevel);
			const auto pixelSize = dstImage->getImageSize().PixelSize;
			for (auto layer = 0u; layer < layerCount; layer++)
			{
				const auto srcBaseOffset = src3DTexture
					                           ? srcLevel.Offset
					                           : srcLevel.Offset + srcImage->getImageSize().LayerSize * (layer + region.srcSubresource.baseArrayLayer);
				const auto dstBaseOffset = dst3DTexture
					                           ? dstLevel.Offset
					                           : dstLevel.Offset + dstImage->getImageSize().LayerSize * (layer + region.dstSubresource.baseArrayLayer);

				if (region.srcSubresource.aspectMask == VK_IMAGE_ASPECT_DEPTH_BIT)
				{
					assert(srcFormat.DepthStencil.DepthOffset != INVALID_OFFSET);

					if (srcFormat.DepthStencil.StencilOffset == INVALID_OFFSET &&
						region.srcOffset.x == 0 && region.srcOffset.y == 0 && region.srcOffset.z == 0 &&
						region.dstOffset.x == 0 && region.dstOffset.y == 0 && region.dstOffset.z == 0 &&
						region.extent.width == srcLevel.Width && region.extent.height == srcLevel.Height && region.extent.depth == srcLevel.Depth &&
						srcLevel.Width == dstLevel.Width && srcLevel.Height == dstLevel.Height && srcLevel.Depth == dstLevel.Depth)
					{
						assert(srcLevel.LevelSize == dstLevel.LevelSize);
						assert(srcLevel.PlaneSize == dstLevel.PlaneSize);
						assert(srcLevel.Stride == dstLevel.Stride);

						memcpy(dstImage->getDataPtr(dstBaseOffset, dstLevel.LevelSize),
						       srcImage->getDataPtr(srcBaseOffset, srcLevel.LevelSize),
						       srcLevel.LevelSize);
					}
					else if (srcFormat.DepthStencil.StencilOffset == INVALID_OFFSET)
					{
						const auto stride = pixelSize * region.extent.width;
						for (auto z = 0u; z < region.extent.depth; z++)
						{
							const auto srcZ = z + region.srcOffset.z + (src3DTexture ? layer : 0);
							const auto dstZ = z + region.dstOffset.z + (dst3DTexture ? layer : 0);
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
					else
					{
						auto depthSize = 0U;
						switch (srcFormat.Format)
						{
						case VK_FORMAT_D16_UNORM:
						case VK_FORMAT_D16_UNORM_S8_UINT:
							depthSize = 2;
							break;

						case VK_FORMAT_D24_UNORM_S8_UINT:
						case VK_FORMAT_X8_D24_UNORM_PACK32:
						case VK_FORMAT_D32_SFLOAT:
						case VK_FORMAT_D32_SFLOAT_S8_UINT:
							depthSize = 4;
							break;

						default:
							assert(false);
						}

						for (auto z = 0u; z < region.extent.depth; z++)
						{
							const auto srcZ = z + region.srcOffset.z + (src3DTexture ? layer : 0);
							const auto dstZ = z + region.dstOffset.z + (dst3DTexture ? layer : 0);
							for (auto y = 0u; y < region.extent.height; y++)
							{
								const auto srcY = y + region.srcOffset.y;
								const auto dstY = y + region.dstOffset.y;
								for (auto x = 0u; x < region.extent.width; x++)
								{
									const auto srcX = x + region.srcOffset.x;
									const auto dstX = x + region.dstOffset.x;

									const auto srcOffset = srcBaseOffset + srcZ * srcLevel.PlaneSize + srcY * srcLevel.Stride + srcX * pixelSize;
									const auto dstOffset = dstBaseOffset + dstZ * dstLevel.PlaneSize + dstY * dstLevel.Stride + dstX * pixelSize;

									memcpy(dstImage->getDataPtr(dstOffset, depthSize), srcImage->getDataPtr(srcOffset, depthSize), depthSize);
								}
							}
						}
					}
				}
				else if (region.srcSubresource.aspectMask == VK_IMAGE_ASPECT_STENCIL_BIT)
				{
					assert(srcFormat.DepthStencil.StencilOffset != INVALID_OFFSET);

					if (srcFormat.DepthStencil.DepthOffset == INVALID_OFFSET &&
						region.srcOffset.x == 0 && region.srcOffset.y == 0 && region.srcOffset.z == 0 &&
						region.dstOffset.x == 0 && region.dstOffset.y == 0 && region.dstOffset.z == 0 &&
						region.extent.width == srcLevel.Width && region.extent.height == srcLevel.Height && region.extent.depth == srcLevel.Depth)
					{
						assert(srcLevel.LevelSize == dstLevel.LevelSize);
						assert(srcLevel.PlaneSize == dstLevel.PlaneSize);
						assert(srcLevel.Stride == dstLevel.Stride);
						assert(srcLevel.Width == dstLevel.Width);
						assert(srcLevel.Height == dstLevel.Height);
						assert(srcLevel.Depth == dstLevel.Depth);

						memcpy(dstImage->getDataPtr(dstBaseOffset, dstLevel.LevelSize),
						       srcImage->getDataPtr(srcBaseOffset, srcLevel.LevelSize),
						       srcLevel.LevelSize);
					}
					else if (srcFormat.DepthStencil.DepthOffset == INVALID_OFFSET)
					{
						const auto stride = pixelSize * region.extent.width;
						for (auto z = 0u; z < region.extent.depth; z++)
						{
							const auto srcZ = z + region.srcOffset.z + (src3DTexture ? layer : 0);
							const auto dstZ = z + region.dstOffset.z + (dst3DTexture ? layer : 0);
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
					else
					{
						for (auto z = 0u; z < region.extent.depth; z++)
						{
							const auto srcZ = z + region.srcOffset.z + (src3DTexture ? layer : 0);
							const auto dstZ = z + region.dstOffset.z + (dst3DTexture ? layer : 0);
							for (auto y = 0u; y < region.extent.height; y++)
							{
								const auto srcY = y + region.srcOffset.y;
								const auto dstY = y + region.dstOffset.y;
								for (auto x = 0u; x < region.extent.width; x++)
								{
									const auto srcX = x + region.srcOffset.x;
									const auto dstX = x + region.dstOffset.x;

									const auto srcOffset = srcBaseOffset + srcZ * srcLevel.PlaneSize + srcY * srcLevel.Stride + srcX * pixelSize + srcFormat.DepthStencil.StencilOffset;
									const auto dstOffset = dstBaseOffset + dstZ * dstLevel.PlaneSize + dstY * dstLevel.Stride + dstX * pixelSize + srcFormat.DepthStencil.StencilOffset;

									memcpy(dstImage->getDataPtr(dstOffset, 1), srcImage->getDataPtr(srcOffset, 1), 1);
								}
							}
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
		else if (format.Type == FormatType::DepthStencil)
		{
			ProcessDepthStencil(format);
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
			assert(region.imageSubresource.aspectMask == VK_IMAGE_ASPECT_COLOR_BIT);

			auto rowLength = region.bufferRowLength;
			if (rowLength == 0)
			{
				rowLength = region.imageExtent.width;
			}

			auto imageHeight = region.bufferImageHeight;
			if (imageHeight == 0)
			{
				imageHeight = region.imageExtent.height;
			}

			const auto& level = gsl::at(dstImage->getImageSize().Level, region.imageSubresource.mipLevel);
			auto bufferOffset = region.bufferOffset;
			for (auto layer = region.imageSubresource.baseArrayLayer; layer < region.imageSubresource.layerCount + region.imageSubresource.baseArrayLayer; layer++)
			{
				const auto baseOffset = level.Offset + dstImage->getImageSize().LayerSize * layer;
				if (region.imageOffset.x == 0 && region.imageOffset.y == 0 && region.imageOffset.z == 0 &&
					region.imageExtent.width == level.Width && region.imageExtent.height == level.Height && region.imageExtent.depth == level.Depth &&
					rowLength == level.Width && imageHeight == level.Height)
				{
					memcpy(dstImage->getDataPtr(baseOffset, level.LevelSize),
					       srcBuffer->getDataPtr(bufferOffset, level.LevelSize),
					       level.LevelSize);
					bufferOffset += level.LevelSize;
				}
				else
				{
					const auto stride = format.TotalSize * region.imageExtent.width;
					for (auto z = 0u; z < region.imageExtent.depth; z++)
					{
						const auto imageZ = z + region.imageOffset.z;
						for (auto y = 0u; y < region.imageExtent.height; y++)
						{
							const auto imageY = y + region.imageOffset.y;
							const auto imageX = region.imageOffset.x;
							const auto bufferAddress = bufferOffset + ((z * imageHeight) + y) * rowLength * format.TotalSize;
							const auto imageOffset = baseOffset + imageZ * level.PlaneSize + imageY * level.Stride + imageX * format.TotalSize;
							memcpy(dstImage->getDataPtr(imageOffset, stride), srcBuffer->getDataPtr(bufferAddress, stride), stride);
						}
					}
					bufferOffset += stride * region.imageExtent.height * region.imageExtent.depth;
				}
			}
		}
	}

	void ProcessDepthStencil(const FormatInformation& format)
	{
		for (const auto& region : regions)
		{
			auto rowLength = region.bufferRowLength;
			if (rowLength == 0)
			{
				rowLength = region.imageExtent.width;
			}
			
			auto imageHeight = region.bufferImageHeight;
			if (imageHeight == 0)
			{
				imageHeight = region.imageExtent.height;
			}

			const auto& level = gsl::at(dstImage->getImageSize().Level, region.imageSubresource.mipLevel);
			auto bufferOffset = region.bufferOffset;
			for (auto layer = region.imageSubresource.baseArrayLayer; layer < region.imageSubresource.layerCount + region.imageSubresource.baseArrayLayer; layer++)
			{
				const auto baseOffset = level.Offset + dstImage->getImageSize().LayerSize * layer;
				if (region.imageSubresource.aspectMask == VK_IMAGE_ASPECT_DEPTH_BIT)
				{
					assert(format.DepthStencil.DepthOffset != INVALID_OFFSET);

					if (format.DepthStencil.StencilOffset == INVALID_OFFSET &&
						region.imageOffset.x == 0 && region.imageOffset.y == 0 && region.imageOffset.z == 0 &&
						region.imageExtent.width == level.Width && region.imageExtent.height == level.Height && region.imageExtent.depth == level.Depth &&
						rowLength == level.Width && imageHeight == level.Height)
					{
						memcpy(dstImage->getDataPtr(baseOffset, level.LevelSize),
						       srcBuffer->getDataPtr(bufferOffset, level.LevelSize),
						       level.LevelSize);
						bufferOffset += level.LevelSize;
					}
					else if (format.DepthStencil.StencilOffset == INVALID_OFFSET)
					{
						const auto stride = format.TotalSize * region.imageExtent.width;
						for (auto z = 0u; z < region.imageExtent.depth; z++)
						{
							const auto imageZ = z + region.imageOffset.z;
							for (auto y = 0u; y < region.imageExtent.height; y++)
							{
								const auto imageY = y + region.imageOffset.y;
								const auto imageX = region.imageOffset.x;
								const auto bufferAddress = bufferOffset + ((z * imageHeight) + y) * rowLength * format.TotalSize;
								const auto imageOffset = baseOffset + imageZ * level.PlaneSize + imageY * level.Stride + imageX * format.TotalSize;
								memcpy(dstImage->getDataPtr(imageOffset, stride), srcBuffer->getDataPtr(bufferAddress, stride), stride);
							}
						}
						bufferOffset += stride * region.imageExtent.height * region.imageExtent.depth;
					}
					else
					{
						auto depthSize = 0U;
						switch (format.Format)
						{
						case VK_FORMAT_D16_UNORM:
						case VK_FORMAT_D16_UNORM_S8_UINT:
							depthSize = 2;
							break;

						case VK_FORMAT_D24_UNORM_S8_UINT:
						case VK_FORMAT_X8_D24_UNORM_PACK32:
						case VK_FORMAT_D32_SFLOAT:
						case VK_FORMAT_D32_SFLOAT_S8_UINT:
							depthSize = 4;
							break;

						default:
							assert(false);
						}

						for (auto z = 0u; z < region.imageExtent.depth; z++)
						{
							const auto imageZ = z + region.imageOffset.z;
							for (auto y = 0u; y < region.imageExtent.height; y++)
							{
								const auto imageY = y + region.imageOffset.y;
								for (auto x = 0u; x < region.imageExtent.width; x++)
								{
									const auto bufferAddress = bufferOffset + (((z * imageHeight) + y) * rowLength + x) * depthSize;
									const auto bufferData = srcBuffer->getDataPtr(bufferAddress, depthSize);
									const auto imageX = x + region.imageOffset.x;
									const auto imageOffset = baseOffset + imageZ * level.PlaneSize + imageY * level.Stride + imageX * format.TotalSize;
									memcpy(dstImage->getDataPtr(imageOffset, depthSize), bufferData, depthSize);
								}
							}
						}
						bufferOffset += depthSize * region.imageExtent.width * region.imageExtent.height * region.imageExtent.depth;
					}
				}
				else if (region.imageSubresource.aspectMask == VK_IMAGE_ASPECT_STENCIL_BIT)
				{
					assert(format.DepthStencil.StencilOffset != INVALID_OFFSET);

					if (format.DepthStencil.DepthOffset == INVALID_OFFSET &&
						region.imageOffset.x == 0 && region.imageOffset.y == 0 && region.imageOffset.z == 0 &&
						region.imageExtent.width == level.Width && region.imageExtent.height == level.Height && region.imageExtent.depth == level.Depth &&
						rowLength == level.Width && imageHeight == level.Height)
					{
						memcpy(dstImage->getDataPtr(baseOffset, level.LevelSize),
						       srcBuffer->getDataPtr(bufferOffset, level.LevelSize),
						       level.LevelSize);
						bufferOffset += level.LevelSize;
					}
					else if (format.DepthStencil.DepthOffset == INVALID_OFFSET)
					{
						const auto stride = format.TotalSize * region.imageExtent.width;
						for (auto z = 0u; z < region.imageExtent.depth; z++)
						{
							const auto imageZ = z + region.imageOffset.z;
							for (auto y = 0u; y < region.imageExtent.height; y++)
							{
								const auto imageY = y + region.imageOffset.y;
								const auto imageX = region.imageOffset.x;
								const auto bufferAddress = bufferOffset + ((z * imageHeight) + y) * rowLength * format.TotalSize;
								const auto imageOffset = baseOffset + imageZ * level.PlaneSize + imageY * level.Stride + imageX * format.TotalSize;
								memcpy(dstImage->getDataPtr(imageOffset, stride), srcBuffer->getDataPtr(bufferAddress, stride), stride);
							}
						}
						bufferOffset += stride * region.imageExtent.height * region.imageExtent.depth;
					}
					else
					{
						for (auto z = 0u; z < region.imageExtent.depth; z++)
						{
							const auto imageZ = z + region.imageOffset.z;
							for (auto y = 0u; y < region.imageExtent.height; y++)
							{
								const auto imageY = y + region.imageOffset.y;
								for (auto x = 0u; x < region.imageExtent.width; x++)
								{
									const auto bufferAddress = bufferOffset + ((z * imageHeight) + y) * rowLength + x;
									const auto bufferData = srcBuffer->getDataPtr(bufferAddress, 1);
									const auto imageX = x + region.imageOffset.x;
									const auto imageOffset = baseOffset + imageZ * level.PlaneSize + imageY * level.Stride + imageX * format.TotalSize + format.DepthStencil.StencilOffset;
									memcpy(dstImage->getDataPtr(imageOffset, 1), bufferData, 1);
								}
							}
						}
						bufferOffset += region.imageExtent.width * region.imageExtent.height * region.imageExtent.depth;
					}
				}
				else
				{
					assert(false);
				}
			}
		}
	}

	void ProcessCompressed(const FormatInformation& format)
	{
		for (const auto& region : regions)
		{
			assert(region.imageSubresource.aspectMask == VK_IMAGE_ASPECT_COLOR_BIT);

			const auto extentWidth = (region.imageExtent.width + format.Compressed.BlockWidth - 1) / format.Compressed.BlockWidth;
			const auto extentHeight = (region.imageExtent.height + format.Compressed.BlockHeight - 1) / format.Compressed.BlockHeight;
			
			auto rowLength = region.bufferRowLength;
			if (rowLength == 0)
			{
				rowLength = region.imageExtent.width;
			}

			auto imageHeight = region.bufferImageHeight;
			if (imageHeight == 0)
			{
				imageHeight = region.imageExtent.height;
			}

			rowLength /= format.Compressed.BlockWidth;
			imageHeight /= format.Compressed.BlockHeight;

			const auto& level = gsl::at(dstImage->getImageSize().Level, region.imageSubresource.mipLevel);
			auto bufferOffset = region.bufferOffset;
			for (auto layer = region.imageSubresource.baseArrayLayer; layer < region.imageSubresource.layerCount + region.imageSubresource.baseArrayLayer; layer++)
			{
				const auto baseOffset = level.Offset + dstImage->getImageSize().LayerSize * layer;
				if (region.imageOffset.x == 0 && region.imageOffset.y == 0 && region.imageOffset.z == 0 &&
					extentWidth == level.Width && extentHeight == level.Height && region.imageExtent.depth == level.Depth &&
					rowLength == level.Width && imageHeight == level.Height)
				{
					memcpy(dstImage->getDataPtr(baseOffset, level.LevelSize),
					       srcBuffer->getDataPtr(bufferOffset, level.LevelSize),
					       level.LevelSize);
					bufferOffset += level.LevelSize;
				}
				else
				{
					const auto stride = format.TotalSize * region.imageExtent.width;
					for (auto z = 0u; z < region.imageExtent.depth; z++)
					{
						const auto imageZ = z + region.imageOffset.z;
						for (auto y = 0u; y < region.imageExtent.height; y++)
						{
							const auto imageY = y + region.imageOffset.y;
							const auto imageX = region.imageOffset.x;
							const auto bufferAddress = bufferOffset + ((z * imageHeight) + y) * rowLength * format.TotalSize;
							const auto imageOffset = baseOffset + imageZ * level.PlaneSize + imageY * level.Stride + imageX * format.TotalSize;
							memcpy(dstImage->getDataPtr(imageOffset, stride), srcBuffer->getDataPtr(bufferAddress, stride), stride);
						}
					}
					bufferOffset += stride * region.imageExtent.height * region.imageExtent.depth;
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
		else if (format.Type == FormatType::DepthStencil)
		{
			ProcessDepthStencil(format);
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
			assert(region.imageSubresource.aspectMask == VK_IMAGE_ASPECT_COLOR_BIT);

			auto rowLength = region.bufferRowLength;
			if (rowLength == 0)
			{
				rowLength = region.imageExtent.width;
			}

			auto imageHeight = region.bufferImageHeight;
			if (imageHeight == 0)
			{
				imageHeight = region.imageExtent.height;
			}

			const auto& level = gsl::at(srcImage->getImageSize().Level, region.imageSubresource.mipLevel);
			auto bufferOffset = region.bufferOffset;
			for (auto layer = region.imageSubresource.baseArrayLayer; layer < region.imageSubresource.layerCount + region.imageSubresource.baseArrayLayer; layer++)
			{
				const auto baseOffset = level.Offset + srcImage->getImageSize().LayerSize * layer;
				if (region.imageOffset.x == 0 && region.imageOffset.y == 0 && region.imageOffset.z == 0 &&
					region.imageExtent.width == level.Width && region.imageExtent.height == level.Height && region.imageExtent.depth == level.Depth &&
					rowLength == level.Width && imageHeight == level.Height)
				{
					memcpy(dstBuffer->getDataPtr(bufferOffset, level.LevelSize),
					       srcImage->getDataPtr(baseOffset, level.LevelSize),
					       level.LevelSize);
					bufferOffset += level.LevelSize;
				}
				else
				{
					const auto stride = format.TotalSize * region.imageExtent.width;
					for (auto z = 0u; z < region.imageExtent.depth; z++)
					{
						const auto imageZ = z + region.imageOffset.z;
						for (auto y = 0u; y < region.imageExtent.height; y++)
						{
							const auto imageY = y + region.imageOffset.y;
							const auto imageX = region.imageOffset.x;
							const auto bufferAddress = bufferOffset + ((z * imageHeight) + y) * rowLength * format.TotalSize;
							const auto bufferData = dstBuffer->getDataPtr(bufferAddress, stride);
							const auto imageOffset = baseOffset + imageZ * level.PlaneSize + imageY * level.Stride + imageX * format.TotalSize;
							memcpy(bufferData, srcImage->getDataPtr(imageOffset, stride), stride);
						}
					}
					bufferOffset += stride * region.imageExtent.height * region.imageExtent.depth;
				}
			}
		}
	}

	void ProcessDepthStencil(const FormatInformation& format)
	{
		for (const auto& region : regions)
		{
			auto rowLength = region.bufferRowLength;
			if (rowLength == 0)
			{
				rowLength = region.imageExtent.width;
			}
			
			auto imageHeight = region.bufferImageHeight;
			if (imageHeight == 0)
			{
				imageHeight = region.imageExtent.height;
			}
			
			const auto& level = gsl::at(srcImage->getImageSize().Level, region.imageSubresource.mipLevel);
			auto bufferOffset = region.bufferOffset;
			for (auto layer = region.imageSubresource.baseArrayLayer; layer < region.imageSubresource.layerCount + region.imageSubresource.baseArrayLayer; layer++)
			{
				const auto baseOffset = level.Offset + srcImage->getImageSize().LayerSize * layer;
				if (region.imageSubresource.aspectMask == VK_IMAGE_ASPECT_DEPTH_BIT)
				{
					assert(format.DepthStencil.DepthOffset != INVALID_OFFSET);

					if (format.DepthStencil.StencilOffset == INVALID_OFFSET &&
						region.imageOffset.x == 0 && region.imageOffset.y == 0 && region.imageOffset.z == 0 &&
						region.imageExtent.width == level.Width && region.imageExtent.height == level.Height && region.imageExtent.depth == level.Depth &&
						rowLength == level.Width && imageHeight == level.Height)
					{
						memcpy(dstBuffer->getDataPtr(bufferOffset, level.LevelSize),
						       srcImage->getDataPtr(baseOffset, level.LevelSize),
						       level.LevelSize);
						bufferOffset += level.LevelSize;
					}
					else if (format.DepthStencil.StencilOffset == INVALID_OFFSET)
					{
						const auto stride = format.TotalSize * region.imageExtent.width;
						for (auto z = 0u; z < region.imageExtent.depth; z++)
						{
							const auto imageZ = z + region.imageOffset.z;
							for (auto y = 0u; y < region.imageExtent.height; y++)
							{
								const auto imageY = y + region.imageOffset.y;
								const auto imageX = region.imageOffset.x;
								const auto bufferAddress = bufferOffset + ((z * imageHeight) + y) * rowLength * format.TotalSize;
								const auto bufferData = dstBuffer->getDataPtr(bufferAddress, stride);
								const auto imageOffset = baseOffset + imageZ * level.PlaneSize + imageY * level.Stride + imageX * format.TotalSize;
								memcpy(bufferData, srcImage->getDataPtr(imageOffset, stride), stride);
							}
						}
						bufferOffset += stride * region.imageExtent.height * region.imageExtent.depth;
					}
					else
					{
						auto depthSize = 0U;
						switch (format.Format)
						{
						case VK_FORMAT_D16_UNORM:
						case VK_FORMAT_D16_UNORM_S8_UINT:
							depthSize = 2;
							break;

						case VK_FORMAT_D24_UNORM_S8_UINT:
						case VK_FORMAT_X8_D24_UNORM_PACK32:
						case VK_FORMAT_D32_SFLOAT:
						case VK_FORMAT_D32_SFLOAT_S8_UINT:
							depthSize = 4;
							break;

						default:
							assert(false);
						}

						for (auto z = 0u; z < region.imageExtent.depth; z++)
						{
							const auto imageZ = z + region.imageOffset.z;
							for (auto y = 0u; y < region.imageExtent.height; y++)
							{
								const auto imageY = y + region.imageOffset.y;
								for (auto x = 0u; x < region.imageExtent.width; x++)
								{
									const auto bufferAddress = bufferOffset + (((z * imageHeight) + y) * rowLength + x) * depthSize;
									const auto bufferData = dstBuffer->getDataPtr(bufferAddress, depthSize);
									const auto imageX = x + region.imageOffset.x;
									const auto imageOffset = baseOffset + imageZ * level.PlaneSize + imageY * level.Stride + imageX * format.TotalSize;
									memcpy(bufferData, srcImage->getDataPtr(imageOffset, depthSize), depthSize);
								}
							}
						}
						bufferOffset += depthSize * region.imageExtent.width * region.imageExtent.height * region.imageExtent.depth;
					}
				}
				else if (region.imageSubresource.aspectMask == VK_IMAGE_ASPECT_STENCIL_BIT)
				{
					assert(format.DepthStencil.StencilOffset != INVALID_OFFSET);

					if (format.DepthStencil.DepthOffset == INVALID_OFFSET &&
						region.imageOffset.x == 0 && region.imageOffset.y == 0 && region.imageOffset.z == 0 &&
						region.imageExtent.width == level.Width && region.imageExtent.height == level.Height && region.imageExtent.depth == level.Depth &&
						rowLength == level.Width && imageHeight == level.Height)
					{
						memcpy(dstBuffer->getDataPtr(bufferOffset, level.LevelSize),
						       srcImage->getDataPtr(baseOffset, level.LevelSize),
						       level.LevelSize);
						bufferOffset += level.LevelSize;
					}
					else if (format.DepthStencil.DepthOffset == INVALID_OFFSET)
					{
						const auto stride = format.TotalSize * region.imageExtent.width;
						for (auto z = 0u; z < region.imageExtent.depth; z++)
						{
							const auto imageZ = z + region.imageOffset.z;
							for (auto y = 0u; y < region.imageExtent.height; y++)
							{
								const auto imageY = y + region.imageOffset.y;
								const auto imageX = region.imageOffset.x;
								const auto bufferAddress = bufferOffset + ((z * imageHeight) + y) * rowLength * format.TotalSize;
								const auto bufferData = dstBuffer->getDataPtr(bufferAddress, stride);
								const auto imageOffset = baseOffset + imageZ * level.PlaneSize + imageY * level.Stride + imageX * format.TotalSize;
								memcpy(bufferData, srcImage->getDataPtr(imageOffset, stride), stride);
							}
						}
						bufferOffset += stride * region.imageExtent.height * region.imageExtent.depth;
					}
					else
					{
						for (auto z = 0u; z < region.imageExtent.depth; z++)
						{
							const auto imageZ = z + region.imageOffset.z;
							for (auto y = 0u; y < region.imageExtent.height; y++)
							{
								const auto imageY = y + region.imageOffset.y;
								for (auto x = 0u; x < region.imageExtent.width; x++)
								{
									const auto bufferAddress = bufferOffset + ((z * imageHeight) + y) * rowLength + x;
									const auto bufferData = dstBuffer->getDataPtr(bufferAddress, 1);
									const auto imageX = x + region.imageOffset.x;
									const auto imageOffset = baseOffset + imageZ * level.PlaneSize + imageY * level.Stride + imageX * format.TotalSize + format.DepthStencil.StencilOffset;
									memcpy(bufferData, srcImage->getDataPtr(imageOffset, 1), 1);
								}
							}
						}
						bufferOffset += region.imageExtent.width * region.imageExtent.height * region.imageExtent.depth;
					}
				}
				else
				{
					assert(false);
				}
			}
		}
	}

	void ProcessCompressed(const FormatInformation& format)
	{
		for (const auto& region : regions)
		{
			assert(region.imageSubresource.aspectMask == VK_IMAGE_ASPECT_COLOR_BIT);
			
			const auto extentWidth = (region.imageExtent.width + format.Compressed.BlockWidth - 1) / format.Compressed.BlockWidth;
			const auto extentHeight = (region.imageExtent.height + format.Compressed.BlockHeight - 1) / format.Compressed.BlockHeight;

			auto rowLength = region.bufferRowLength;
			if (rowLength == 0)
			{
				rowLength = region.imageExtent.width;
			}

			auto imageHeight = region.bufferImageHeight;
			if (imageHeight == 0)
			{
				imageHeight = region.imageExtent.height;
			}

			rowLength /= format.Compressed.BlockWidth;
			imageHeight /= format.Compressed.BlockHeight;

			const auto& level = gsl::at(srcImage->getImageSize().Level, region.imageSubresource.mipLevel);
			auto bufferOffset = region.bufferOffset;
			for (auto layer = region.imageSubresource.baseArrayLayer; layer < region.imageSubresource.layerCount + region.imageSubresource.baseArrayLayer; layer++)
			{
				const auto baseOffset = level.Offset + srcImage->getImageSize().LayerSize * layer;
				if (region.imageOffset.x == 0 && region.imageOffset.y == 0 && region.imageOffset.z == 0 &&
					extentWidth == level.Width && extentHeight == level.Height && region.imageExtent.depth == level.Depth &&
					rowLength == level.Width && imageHeight == level.Height)
				{
					memcpy(dstBuffer->getDataPtr(bufferOffset, level.LevelSize),
					       srcImage->getDataPtr(baseOffset, level.LevelSize),
					       level.LevelSize);
					bufferOffset += level.LevelSize;
				}
				else
				{
					const auto stride = format.TotalSize * region.imageExtent.width;
					for (auto z = 0u; z < region.imageExtent.depth; z++)
					{
						const auto imageZ = z + region.imageOffset.z;
						for (auto y = 0u; y < region.imageExtent.height; y++)
						{
							const auto imageY = y + region.imageOffset.y;
							const auto imageX = region.imageOffset.x;
							const auto bufferAddress = bufferOffset + ((z * imageHeight) + y) * rowLength * format.TotalSize;
							const auto bufferData = dstBuffer->getDataPtr(bufferAddress, stride);
							const auto imageOffset = baseOffset + imageZ * level.PlaneSize + imageY * level.Stride + imageX * format.TotalSize;
							memcpy(bufferData, srcImage->getDataPtr(imageOffset, stride), stride);
						}
					}
					bufferOffset += stride * region.imageExtent.height * region.imageExtent.depth;
				}
			}
		}
	}

private:
	Image* srcImage;
	Buffer* dstBuffer;
	std::vector<VkBufferImageCopy> regions;
};

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
