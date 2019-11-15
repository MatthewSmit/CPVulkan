#pragma once
#include "Base.h"

class Buffer;
class DescriptorSet;
class Framebuffer;
class Pipeline;
class RenderPass;
class SpirvJit;

struct ImageFunctions
{
	float (*GetPixelDepth)(const void* ptr);
	void (*GetPixelF32)(const void* ptr, void* values);
	void (*GetPixelI32)(const void* ptr, void* values);
	void (*GetPixelU32)(const void* ptr, void* values);
	
	void (*SetPixelDepthStencil)(void* ptr, float depth, uint8_t stencil);
	void (*SetPixelF32)(void* ptr, const float* values);
	void (*SetPixelI32)(void* ptr, const int32_t* values);
	void (*SetPixelU32)(void* ptr, const uint32_t* values);
};

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
	uint8_t pushConstants[MAX_PUSH_CONSTANTS_SIZE];

	RenderPass* currentRenderPass;
	Framebuffer* currentFramebuffer;
	VkRect2D currentRenderArea;

	SpirvJit* jit;
	std::unordered_map<VkFormat, ImageFunctions> imageFunctions{};
	
#if CV_DEBUG_LEVEL > 0
	std::ofstream* debugOutput;
#endif
};