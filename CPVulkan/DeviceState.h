#pragma once
#include "Base.h"

class Buffer;
class DescriptorSet;
class Framebuffer;
class Pipeline;
class RenderPass;
class SpirvJit;

struct DeviceState
{
	Pipeline* pipeline[MAX_PIPELINES];
	VkViewport viewports[MAX_VIEWPORTS];
	VkRect2D scissors[MAX_VIEWPORTS];
	DescriptorSet* descriptorSets[MAX_BOUND_DESCRIPTOR_SETS][MAX_PIPELINES];
	Buffer* vertexBinding[MAX_VERTEX_INPUT_BINDINGS];
	uint64_t vertexBindingOffset[MAX_VERTEX_INPUT_BINDINGS];
	Buffer* indexBinding;
	uint64_t indexBindingOffset;
	uint32_t indexBindingStride;
	RenderPass* renderPass;
	Framebuffer* framebuffer;
	VkRect2D renderArea;
	uint8_t pushConstants[MAX_PUSH_CONSTANTS_SIZE];

	SpirvJit* jit;
};