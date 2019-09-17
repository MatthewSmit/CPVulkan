#pragma once
#include "Base.h"

#include <array>

namespace SPIRV
{
	class SPIRVModule;
}

class ShaderFunction;
class ShaderModule;

struct VertexInputState
{
	std::vector<VkVertexInputBindingDescription> VertexBindingDescriptions;
	std::vector<VkVertexInputAttributeDescription> VertexAttributeDescriptions;
};

struct InputAssemblyState
{
	VkPrimitiveTopology Topology;
	bool PrimitiveRestartEnable;
};

struct TessellationState
{
	uint32_t PatchControlPoints;
};

struct ViewportState
{
	std::vector<VkViewport> Viewports;
	std::vector<VkRect2D> Scissors;
};

struct RasterizationState
{
	bool DepthClampEnable;
	bool RasterizerDiscardEnable;
	VkPolygonMode PolygonMode;
	VkCullModeFlags CullMode;
	VkFrontFace FrontFace;
	bool DepthBiasEnable;
	float DepthBiasConstantFactor;
	float DepthBiasClamp;
	float DepthBiasSlopeFactor;
	float LineWidth;
};

struct MultisampleState
{
	VkSampleCountFlagBits RasterizationSamples;
	bool SampleShadingEnable;
	float MinSampleShading;
	bool AlphaToCoverageEnable;
	bool AlphaToOneEnable;
};

struct DepthStencilState
{
	bool DepthTestEnable;
	bool DepthWriteEnable;
	VkCompareOp DepthCompareOp;
	bool DepthBoundsTestEnable;
	bool StencilTestEnable;
	VkStencilOpState Front;
	VkStencilOpState Back;
	float MinDepthBounds;
	float MaxDepthBounds;
};

struct ColourBlendState
{
	bool LogicOpEnable;
	VkLogicOp LogicOp;
	std::vector<VkPipelineColorBlendAttachmentState> Attachments;
	float BlendConstants[4];
};

struct DynamicState
{
	std::vector<VkDynamicState> DynamicStates;
};

class ShaderFunction final
{
public:
	ShaderFunction(ShaderModule* module, uint32_t stageIndex, const char* name);

	const SPIRV::SPIRVModule* getModule() const { return module; }
	const std::string& getName() const { return name; }

private:
	const SPIRV::SPIRVModule* module;
	std::string name;
};

class Pipeline final
{
public:
	Pipeline() = default;
	Pipeline(const Pipeline&) = delete;
	Pipeline(Pipeline&&) = delete;
	~Pipeline();

	Pipeline& operator=(const Pipeline&) = delete;
	Pipeline&& operator=(const Pipeline&&) = delete;

	static VkResult Create(VkPipelineCache pipelineCache, const VkGraphicsPipelineCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkPipeline* pPipeline);

	[[nodiscard]] const ShaderFunction* getShaderStage(uint32_t index) const { return shaderStages[index].get(); }
	
	[[nodiscard]] const VertexInputState& getVertexInputState() const { return vertexInputState; }
	[[nodiscard]] const InputAssemblyState& getInputAssemblyState() const { return inputAssemblyState; }
	[[nodiscard]] const TessellationState& getTessellationState() const { return tessellationState; }
	[[nodiscard]] const ViewportState& getViewportState() const { return viewportState; }
	[[nodiscard]] const RasterizationState& getRasterizationState() const { return rasterizationState; }
	[[nodiscard]] const MultisampleState& getMultisampleState() const { return multisampleState; }
	[[nodiscard]] const DepthStencilState& getDepthStencilState() const { return depthStencilState; }
	[[nodiscard]] const ColourBlendState& getColourBlendState() const { return colourBlendState; }
	[[nodiscard]] const DynamicState& getDynamicState() const { return dynamicState; }

private:
	std::array<std::unique_ptr<ShaderFunction>, 6> shaderStages;
	
	VertexInputState vertexInputState{};
	InputAssemblyState inputAssemblyState{};
	TessellationState tessellationState{};
	ViewportState viewportState{};
	RasterizationState rasterizationState{};
	MultisampleState multisampleState{};
	DepthStencilState depthStencilState{};
	ColourBlendState colourBlendState{};
	DynamicState dynamicState{};
};
