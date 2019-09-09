#pragma once
#include "Base.h"

class Buffer;
class DescriptorSet;
class Framebuffer;
class Pipeline;
class RenderPass;

struct DeviceState
{
	Pipeline* pipeline[2];
	VkViewport viewports[16];
	VkRect2D scissors[16];
	DescriptorSet* descriptorSets[4][2];
	Buffer* vertexBinding[16];
	VkDeviceSize vertexBindingOffset[16];
	RenderPass* renderPass;
	Framebuffer* framebuffer;
	VkRect2D renderArea;
};