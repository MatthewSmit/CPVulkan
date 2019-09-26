#pragma once
#include "Base.h"

template<typename ReturnType>
ReturnType SampleImage(Image* image, float u, float v, float w, float q, float a, VkFilter filter, VkSamplerMipmapMode mipmapMode, VkSamplerAddressMode addressMode);