#pragma once
#include "Base.h"

class Buffer;
class DescriptorSet;
class Framebuffer;
class Pipeline;
class RenderPass;
class SpirvJit;

struct Subpass;

struct ImageFunctions
{
	float (*GetPixelDepth)(const void* ptr);
	uint8_t (*GetPixelStencil)(const void* ptr);
	void (*GetPixelF32)(const void* ptr, void* values);
	void (*GetPixelF32C)(const void* ptr, void* values, uint32_t x, uint32_t y);
	void (*GetPixelI32)(const void* ptr, void* values);
	void (*GetPixelU32)(const void* ptr, void* values);
	
	void (*SetPixelDepthStencil)(void* ptr, float depth, uint8_t stencil);
	void (*SetPixelF32)(void* ptr, const float* values);
	void (*SetPixelI32)(void* ptr, const int32_t* values);
	void (*SetPixelU32)(void* ptr, const uint32_t* values);
};

struct DeviceState
{
	struct
	{
		Pipeline* pipeline;
		DescriptorSet* descriptorSets[MAX_BOUND_DESCRIPTOR_SETS];
		uint32_t descriptorSetDynamicOffset[MAX_BOUND_DESCRIPTOR_SETS][MAX_DESCRIPTOR_SET_UNIFORM_BUFFERS_DYNAMIC + MAX_DESCRIPTOR_SET_STORAGE_BUFFERS_DYNAMIC];
	} pipelineState[MAX_PIPELINES];
	
	VkViewport viewports[MAX_VIEWPORTS];
	VkRect2D scissors[MAX_VIEWPORTS];
	Buffer* vertexBinding[MAX_VERTEX_INPUT_BINDINGS];
	uint64_t vertexBindingOffset[MAX_VERTEX_INPUT_BINDINGS];
	Buffer* indexBinding;
	uint64_t indexBindingOffset;
	uint32_t indexBindingStride;
	uint8_t pushConstants[MAX_PUSH_CONSTANTS_SIZE];

	const Subpass* currentSubpass;
	uint32_t currentSubpassIndex;
	RenderPass* currentRenderPass;
	Framebuffer* currentFramebuffer;
	VkRect2D currentRenderArea;

	SpirvJit* jit;
	std::unordered_map<VkFormat, ImageFunctions> imageFunctions{};
	
#if CV_DEBUG_LEVEL > 0
	std::ofstream* debugOutput;
#endif
};