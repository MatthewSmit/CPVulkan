#pragma once
#include "Base.h"

class CompiledModule;
class CPJit;

struct SubpassDescription;

class ImageFunctions
{
public:
	ImageFunctions(CPJit* jit);
	~ImageFunctions();

	float (*GetPixelDepth)(const void* ptr){};
	uint8_t (*GetPixelStencil)(const void* ptr){};
	void (*GetPixelF32)(const void* ptr, void* values){};
	void (*GetPixelF32C)(const void* ptr, void* values, uint32_t x, uint32_t y){};
	void (*GetPixelI32)(const void* ptr, void* values){};
	void (*GetPixelU32)(const void* ptr, void* values){};
	
	void (*SetPixelDepthStencil)(void* ptr, float depth, uint8_t stencil){};
	void (*SetPixelF32)(void* ptr, const float* values){};
	void (*SetPixelI32)(void* ptr, const int32_t* values){};
	void (*SetPixelU32)(void* ptr, const uint32_t* values){};

private:
	std::vector<CompiledModule*> modules{};
	CPJit* jit;
};

struct DynamicPipelineState
{
	VkViewport viewports[MAX_VIEWPORTS];
	VkRect2D scissors[MAX_VIEWPORTS];
	float minDepthBounds;
	float maxDepthBounds;
};

class CommonPipelineState
{
public:
	std::array<DescriptorSet*, MAX_BOUND_DESCRIPTOR_SETS> descriptorSets{};
	std::array<DescriptorSet*, MAX_BOUND_DESCRIPTOR_SETS> pushDescriptorSets{};
	// TODO: turn this into a lookup table
	std::array<std::array<std::array<uint32_t, MAX_PER_STAGE_RESOURCES * 5>, MAX_PER_STAGE_RESOURCES * 5>, MAX_BOUND_DESCRIPTOR_SETS> descriptorSetDynamicOffset{};
};

struct GraphicsNativeState
{
	uint8_t* vertexBindingPtr[MAX_VERTEX_INPUT_BINDINGS];
	ImageView* imageAttachment[MAX_FRAGMENT_OUTPUT_ATTACHMENTS];
	ImageView* depthStencilAttachment;
};

class GraphicsPipelineState final : public CommonPipelineState
{
public:
	GraphicsPipeline* pipeline{};
	std::array<Buffer*, MAX_VERTEX_INPUT_BINDINGS> vertexBinding;
	GraphicsNativeState nativeState;

	Buffer* indexBinding;
	uint64_t indexBindingOffset;
	uint32_t indexBindingStride;

	const SubpassDescription* currentSubpass;
	uint32_t currentSubpassIndex;
	RenderPass* currentRenderPass;
	Framebuffer* currentFramebuffer;
	VkRect2D currentRenderArea;

	std::vector<uint8_t> vertexOutputStorage{};

	DynamicPipelineState dynamicState;
};

class ComputePipelineState final : public CommonPipelineState
{
public:
	ComputePipeline* pipeline{};
};

#if defined(VK_NV_ray_tracing)
class RayTracingPipelineState final : public CommonPipelineState
{
public:
	RayTracingPipeline* pipeline{};
};
#endif

struct DeviceState
{
public:
	GraphicsPipelineState graphicsPipelineState{};
	ComputePipelineState computePipelineState{};
#if defined(VK_NV_ray_tracing)
	RayTracingPipelineState rayTracingPipelineState{};
#endif

	uint8_t pushConstants[MAX_PUSH_CONSTANTS_SIZE];

	std::unordered_map<VkFormat, std::unique_ptr<ImageFunctions>> imageFunctions{};
	CPJit* jit;
	
#if CV_DEBUG_LEVEL > 0
	std::ofstream* debugOutput;
#endif

	ImageFunctions* getImageFunctions(VkFormat format)
	{
		auto ptr = imageFunctions.find(format);
		if (ptr != imageFunctions.end())
		{
			return ptr->second.get();
		}

		imageFunctions[format] = std::make_unique<ImageFunctions>(jit);
		return imageFunctions.at(format).get();
	}
};