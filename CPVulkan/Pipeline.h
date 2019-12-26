#pragma once
#include "Base.h"

#include <glm/glm.hpp>

#include <array>
#include <vector>

namespace SPIRV
{
	class SPIRVModule;
}

class CompiledModule;
class CPJit;

class Device;
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

#if defined(VK_EXT_line_rasterization)
	VkLineRasterizationModeEXT LineRasterizationMode;
	bool StippledLineEnable;
	uint32_t LineStippleFactor;
	uint32_t LineStipplePattern;
#endif
};

struct MultisampleState
{
	VkSampleCountFlagBits RasterizationSamples;
	bool SampleShadingEnable;
	float MinSampleShading;
	bool AlphaToCoverageEnable;
	bool AlphaToOneEnable;
	uint64_t SampleMask;
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
	bool DynamicViewport;
	bool DynamicScissor;
	bool DynamicLineWidth;
	bool DynamicDepthBias;
	bool DynamicBlendConstants;
	bool DynamicDepthBounds;
	bool DynamicStencilCompareMask;
	bool DynamicStencilWriteMask;
	bool DynamicStencilReference;
	bool DynamicViewportWScaling;
	bool DynamicDiscardRectangle;
	bool DynamicSampleLocations;
	bool DynamicViewportShadingRatePalette;
	bool DynamicViewportCoarseSampleOrder;
	bool DynamicExclusiveScissor;
	bool DynamicLineStipple;
};

using EntryPoint = void (*)();

class ShaderFunction final
{
public:
	ShaderFunction(CPJit* jit, PipelineCache* cache, bool& hitCache, ShaderModule* module, uint32_t stageIndex, const char* name, const VkSpecializationInfo* specializationInfo);
	~ShaderFunction();

	[[nodiscard]] const SPIRV::SPIRVModule* getSPIRVModule() const { return spirvModule; }
	[[nodiscard]] CompiledModule* getLLVMModule() const { return llvmModule; }
	[[nodiscard]] EntryPoint getEntryPoint() const { return entryPoint; }
	
	[[nodiscard]] bool getFragmentOriginUpper() const { return fragmentOriginUpper; }
	[[nodiscard]] bool getFragmentStencilExport() const { return fragmentStencilExport; }
	
	[[nodiscard]] glm::uvec3 getComputeLocalSize() const { return computeLocalSize; }

private:
	CPJit* jit;
	SPIRV::SPIRVModule* spirvModule;
	CompiledModule* llvmModule;
	EntryPoint entryPoint;
	
	bool fragmentOriginUpper{};
	bool fragmentStencilExport{};
	
	glm::uvec3 computeLocalSize{};
};

class Pipeline
{
public:
	Pipeline() = default;
	Pipeline(const Pipeline&) = delete;
	Pipeline(Pipeline&&) = delete;
	virtual ~Pipeline() = default;

	Pipeline& operator=(const Pipeline&) = delete;
	Pipeline&& operator=(const Pipeline&&) = delete;

	void OnDelete(const VkAllocationCallbacks*)
	{
	}

	[[nodiscard]] virtual uint32_t getMaxShaderStages() const = 0;
	[[nodiscard]] virtual const ShaderFunction* getShaderStage(uint32_t index) const = 0;

protected:
	CPJit* jit{};

	PipelineLayout* layout{};
	PipelineCache* cache{};
};

class GraphicsPipeline final : public Pipeline
{
public:
	~GraphicsPipeline() override;
	
	static VkResult Create(Device* device, VkPipelineCache pipelineCache, const VkGraphicsPipelineCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkPipeline* pPipeline);

	[[nodiscard]] uint32_t getMaxShaderStages() const override
	{
		return 5;
	}

	[[nodiscard]] const ShaderFunction* getShaderStage(uint32_t index) const override
	{
		return shaderStages.at(index).get();
	}

	[[nodiscard]] const VertexInputState& getVertexInputState() const { return vertexInputState; }
	[[nodiscard]] const InputAssemblyState& getInputAssemblyState() const { return inputAssemblyState; }
	[[nodiscard]] const TessellationState& getTessellationState() const { return tessellationState; }
	[[nodiscard]] const ViewportState& getViewportState() const { return viewportState; }
	[[nodiscard]] const RasterizationState& getRasterizationState() const { return rasterizationState; }
	[[nodiscard]] const MultisampleState& getMultisampleState() const { return multisampleState; }
	[[nodiscard]] const DepthStencilState& getDepthStencilState() const { return depthStencilState; }
	[[nodiscard]] const ColourBlendState& getColourBlendState() const { return colourBlendState; }
	[[nodiscard]] const DynamicState& getDynamicState() const { return dynamicState; }

	[[nodiscard]] CompiledModule* getVertexModule() const { return vertexModule; }
	[[nodiscard]] EntryPoint getVertexEntryPoint() const { return vertexEntryPoint; }

private:
	std::array<std::unique_ptr<ShaderFunction>, 5> shaderStages;

	CompiledModule* vertexModule{};
	EntryPoint vertexEntryPoint{};

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

class ComputePipeline final : public Pipeline
{
public:
	~ComputePipeline() override = default;

	static VkResult Create(Device* device, VkPipelineCache pipelineCache, const VkComputePipelineCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkPipeline* pPipeline);

	[[nodiscard]] uint32_t getMaxShaderStages() const override
	{
		return 1;
	}

	[[nodiscard]] const ShaderFunction* getShaderStage(uint32_t index) const override
	{
		assert(index == 0);
		return computeStage.get();
	}

	[[nodiscard]] const ShaderFunction* getComputeStage() const
	{
		return computeStage.get();
	}

private:
	std::unique_ptr<ShaderFunction> computeStage;

	CPJit* jit{};
};