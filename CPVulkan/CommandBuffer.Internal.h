#pragma once
#include "Base.h"

class Image;

void ClearImage(Image* image, VkFormat format, uint32_t baseMipLevel, uint32_t levelCount, uint32_t baseArrayLayer, uint32_t layerCount, VkClearColorValue colour);
void ClearImage(Image* image, VkFormat format, uint32_t baseMipLevel, uint32_t levelCount, uint32_t baseArrayLayer, uint32_t layerCount, VkImageAspectFlags aspects, VkClearDepthStencilValue colour);
