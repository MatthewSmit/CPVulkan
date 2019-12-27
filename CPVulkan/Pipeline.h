#pragma once
#include "Base.h"

#include <glm/glm.hpp>

#include <spirv.hpp>

#include <vector>

namespace SPIRV
{
	class SPIRVFunction;
	class SPIRVModule;
}

class CompiledModule;
class CPJit;

struct StageFeedback;

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

class CompiledShaderModule
{
public:
	CompiledShaderModule(const SPIRV::SPIRVModule* spirvModule, CompiledModule* llvmModule, EntryPoint entryPoint) :
		spirvModule{spirvModule},
		llvmModule{llvmModule},
		entryPoint{entryPoint}
	{
	}

	virtual ~CompiledShaderModule();

	[[nodiscard]] const SPIRV::SPIRVModule* getSPIRVModule() const { return spirvModule; }
	[[nodiscard]] CompiledModule* getLLVMModule() const { return llvmModule; }
	[[nodiscard]] EntryPoint getEntryPoint() const { return entryPoint; }

private:
	const SPIRV::SPIRVModule* spirvModule;
	CompiledModule* llvmModule;
	EntryPoint entryPoint;
};

class VertexShaderModule final : public CompiledShaderModule
{
public:
	VertexShaderModule(const SPIRV::SPIRVModule* spirvModule, CompiledModule* llvmModule, EntryPoint entryPoint) :
		CompiledShaderModule{spirvModule, llvmModule, entryPoint}
	{
	}

	~VertexShaderModule() override = default;

	friend class GraphicsPipeline;
};

class TessellationControlShaderModule final : public CompiledShaderModule
{
public:
	TessellationControlShaderModule(const SPIRV::SPIRVModule* spirvModule, CompiledModule* llvmModule, EntryPoint entryPoint) :
		CompiledShaderModule{spirvModule, llvmModule, entryPoint}
	{
	}

	~TessellationControlShaderModule() override = default;

	friend class GraphicsPipeline;
};

class TessellationEvaluationShaderModule final : public CompiledShaderModule
{
public:
	TessellationEvaluationShaderModule(const SPIRV::SPIRVModule* spirvModule, CompiledModule* llvmModule, EntryPoint entryPoint) :
		CompiledShaderModule{spirvModule, llvmModule, entryPoint}
	{
	}

	~TessellationEvaluationShaderModule() override = default;

	friend class GraphicsPipeline;
};

class GeometryShaderModule final : public CompiledShaderModule
{
public:
	GeometryShaderModule(const SPIRV::SPIRVModule* spirvModule, CompiledModule* llvmModule, EntryPoint entryPoint) :
		CompiledShaderModule{spirvModule, llvmModule, entryPoint}
	{
	}

	~GeometryShaderModule() override = default;

	friend class GraphicsPipeline;
};

class FragmentShaderModule final : public CompiledShaderModule
{
public:
	FragmentShaderModule(const SPIRV::SPIRVModule* spirvModule, CompiledModule* llvmModule, EntryPoint entryPoint) :
		CompiledShaderModule{spirvModule, llvmModule, entryPoint}
	{
	}

	~FragmentShaderModule() override = default;
	
	[[nodiscard]] bool getOriginUpper() const { return originUpper; }
	[[nodiscard]] bool getStencilExport() const { return stencilExport; }

	friend class GraphicsPipeline;

private:
	bool originUpper{};
	bool stencilExport{};
};

class ComputeShaderModule final : public CompiledShaderModule
{
public:
	ComputeShaderModule(const SPIRV::SPIRVModule* spirvModule, CompiledModule* llvmModule, EntryPoint entryPoint) :
		CompiledShaderModule{spirvModule, llvmModule, entryPoint}
	{
	}

	~ComputeShaderModule() override = default;
	
	[[nodiscard]] glm::uvec3 getLocalSize() const { return localSize; }

	friend class ComputePipeline;

private:
	glm::uvec3 localSize{};
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
	[[nodiscard]] virtual const CompiledShaderModule* getShaderStage(uint32_t index) const = 0;

protected:
	CPJit* jit{};

	PipelineLayout* layout{};
	PipelineCache* cache{};
	
	void CompileBaseShaderModule(ShaderModule* shaderModule, const char* entryName, const VkSpecializationInfo* specializationInfo, spv::ExecutionModel executionModel,
	                             bool& hitCache, CompiledModule*& llvmModule, SPIRV::SPIRVFunction*& entryPointFunction,
	                             std::function<CompiledModule*(CPJit*, const SPIRV::SPIRVModule*, spv::ExecutionModel, const SPIRV::SPIRVFunction*, const VkSpecializationInfo*)> compileFunction);
};

class GraphicsPipeline final : public Pipeline
{
public:
	~GraphicsPipeline() override = default;
	
	static VkResult Create(Device* device, VkPipelineCache pipelineCache, const VkGraphicsPipelineCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkPipeline* pPipeline);

	[[nodiscard]] uint32_t getMaxShaderStages() const override
	{
		return 5;
	}

	[[nodiscard]] const CompiledShaderModule* getShaderStage(uint32_t index) const override
	{
		switch (index)
		{
		case 0:
			return vertexShaderModule.get();

		case 1:
			return tessellationControlShaderModule.get();

		case 2:
			return tessellationEvaluationShaderModule.get();

		case 3:
			return geometryShaderModule.get();

		case 4:
			return fragmentShaderModule.get();

		default:
			assert(false);
			return nullptr;
		}
	}

	[[nodiscard]] const VertexShaderModule* getVertexShaderModule() const { return vertexShaderModule.get(); }
	[[nodiscard]] const TessellationControlShaderModule* getTessellationControlShaderModule() const { return tessellationControlShaderModule.get(); }
	[[nodiscard]] const TessellationEvaluationShaderModule* getTessellationEvaluationShaderModule() const { return tessellationEvaluationShaderModule.get(); }
	[[nodiscard]] const GeometryShaderModule* getGeometryShaderModule() const { return geometryShaderModule.get(); }
	[[nodiscard]] const FragmentShaderModule* getFragmentShaderModule() const { return fragmentShaderModule.get(); }

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
	std::unique_ptr<VertexShaderModule> vertexShaderModule;
	std::unique_ptr<TessellationControlShaderModule> tessellationControlShaderModule;
	std::unique_ptr<TessellationEvaluationShaderModule> tessellationEvaluationShaderModule;
	std::unique_ptr<GeometryShaderModule> geometryShaderModule;
	std::unique_ptr<FragmentShaderModule> fragmentShaderModule;

	VertexInputState vertexInputState{};
	InputAssemblyState inputAssemblyState{};
	TessellationState tessellationState{};
	ViewportState viewportState{};
	RasterizationState rasterizationState{};
	MultisampleState multisampleState{};
	DepthStencilState depthStencilState{};
	ColourBlendState colourBlendState{};
	DynamicState dynamicState{};

	void LoadShaderStage(Device* device, bool fetchFeedback, StageFeedback& stageFeedback, const VkPipelineShaderStageCreateInfo& stage);

	std::unique_ptr<VertexShaderModule> CompileVertexShaderModule(Device* device, ShaderModule* shaderModule, const char* entryName, const VkSpecializationInfo* specializationInfo, bool& hitCache);
	std::unique_ptr<FragmentShaderModule> CompileFragmentShaderModule(Device* device, ShaderModule* shaderModule, const char* entryName, const VkSpecializationInfo* specializationInfo, bool& hitCache);
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

	[[nodiscard]] const CompiledShaderModule* getShaderStage(uint32_t index) const override
	{
		assert(index == 0);
		return computeShaderModule.get();
	}

	[[nodiscard]] const ComputeShaderModule* getComputeShaderModule() const
	{
		return computeShaderModule.get();
	}

private:
	std::unique_ptr<ComputeShaderModule> computeShaderModule;

	void LoadShaderStage(Device* device, bool fetchFeedback, StageFeedback& stageFeedback, const VkPipelineShaderStageCreateInfo& stage);
	
	std::unique_ptr<ComputeShaderModule> CompileComputeShaderModule(Device* device, ShaderModule* shaderModule, const char* entryName, const VkSpecializationInfo* specializationInfo, bool& hitCache);
};