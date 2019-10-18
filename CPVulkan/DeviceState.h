#pragma once
#include "Base.h"

class Buffer;
class DescriptorSet;
class Framebuffer;
class Pipeline;
class RenderPass;
class SpirvJit;

constexpr auto MAX_DESCRIPTOR_SETS = 4;
constexpr auto MAX_VERTEX_BINDINGS = 16;
constexpr auto MAX_VIEWPORTS = 16;
constexpr auto MAX_SCISSORS = 16;
constexpr auto MAX_PIPELINES = 2;

struct DeviceState
{
	Pipeline* pipeline[MAX_PIPELINES];
	VkViewport viewports[MAX_VIEWPORTS];
	VkRect2D scissors[MAX_SCISSORS];
	DescriptorSet* descriptorSets[MAX_DESCRIPTOR_SETS][MAX_PIPELINES];
	Buffer* vertexBinding[MAX_VERTEX_BINDINGS];
	uint64_t vertexBindingOffset[MAX_VERTEX_BINDINGS];
	Buffer* indexBinding;
	uint64_t indexBindingOffset;
	uint32_t indexBindingStride;
	RenderPass* renderPass;
	Framebuffer* framebuffer;
	VkRect2D renderArea;

	SpirvJit* jit;
};