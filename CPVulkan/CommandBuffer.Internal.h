#pragma once
#include "Base.h"

class Image;

void ClearImage(Image* image, VkFormat format, VkClearColorValue colour);
void ClearImage(Image* image, VkFormat format, VkClearDepthStencilValue colour);