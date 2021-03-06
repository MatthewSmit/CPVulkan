#include "Pipeline.h"

#include "DescriptorSetLayout.h"
#include "Device.h"
#include "DeviceState.h"
#include "PipelineCache.h"
#include "PipelineLayout.h"
#include "ShaderModule.h"
#include "Util.h"

#include "sha3.h"

#include <CompiledModule.h>
#include <Compilers.h>
#include <Jit.h>
#include <SPIRVCompiler.h>
#include <SPIRVFunction.h>
#include <SPIRVModule.h>

#include <cassert>

struct StageFeedback
{
	uint64_t duration;
	bool hitCache;
};

static uint32_t GetStageIndex(VkShaderStageFlagBits stage)
{
	switch (stage)
	{
	case VK_SHADER_STAGE_VERTEX_BIT: return 0;
	case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT: return 1;
	case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT: return 2;
	case VK_SHADER_STAGE_GEOMETRY_BIT: return 3;
	case VK_SHADER_STAGE_FRAGMENT_BIT: return 4;
	case VK_SHADER_STAGE_COMPUTE_BIT: return 5;
	default:
		FATAL_ERROR();
	}
}

static VertexInputState Parse(const VkPipelineVertexInputStateCreateInfo* pVertexInputState)
{
	assert(pVertexInputState->sType == VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO);

	auto next = static_cast<const VkBaseInStructure*>(pVertexInputState->pNext);
	while (next)
	{
		const auto type = next->sType;
		switch (type)
		{
		case VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_DIVISOR_STATE_CREATE_INFO_EXT:
			TODO_ERROR();
		}
		next = next->pNext;
	}

	if (pVertexInputState->flags)
	{
		TODO_ERROR();
	}

	return VertexInputState
	{
		ArrayToVector(pVertexInputState->vertexBindingDescriptionCount, pVertexInputState->pVertexBindingDescriptions),
		ArrayToVector(pVertexInputState->vertexAttributeDescriptionCount, pVertexInputState->pVertexAttributeDescriptions),
	};
}

static InputAssemblyState Parse(const VkPipelineInputAssemblyStateCreateInfo* pInputAssemblyState)
{
	assert(pInputAssemblyState->sType == VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO);

	if (pInputAssemblyState->flags)
	{
		TODO_ERROR();
	}

	return InputAssemblyState
	{
		pInputAssemblyState->topology,
		pInputAssemblyState->primitiveRestartEnable != VK_FALSE,
	};
}

static TessellationState Parse(const VkPipelineTessellationStateCreateInfo* pTessellationState)
{
	if (pTessellationState == nullptr)
	{
		return TessellationState{};
	}
	
	assert(pTessellationState->sType == VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO);

	auto next = pTessellationState->pNext;
	while (next)
	{
		const auto type = static_cast<const VkBaseInStructure*>(next)->sType;
		switch (type)
		{
		case VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_DOMAIN_ORIGIN_STATE_CREATE_INFO:
			TODO_ERROR();
		}
		next = static_cast<const VkBaseInStructure*>(next)->pNext;
	}

	if (pTessellationState->flags)
	{
		TODO_ERROR();
	}

	return TessellationState
	{
		pTessellationState->patchControlPoints,
	};
}

static ViewportState Parse(const VkPipelineViewportStateCreateInfo* pViewportState)
{
	if (!pViewportState)
	{
		return ViewportState{};
	}
	
	assert(pViewportState->sType == VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO);

	auto next = static_cast<const VkBaseInStructure*>(pViewportState->pNext);
	while (next)
	{
		const auto type = next->sType;
		switch (type)
		{
		case VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_COARSE_SAMPLE_ORDER_STATE_CREATE_INFO_NV:
			TODO_ERROR();

		case VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_EXCLUSIVE_SCISSOR_STATE_CREATE_INFO_NV:
			TODO_ERROR();

		case VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_SHADING_RATE_IMAGE_STATE_CREATE_INFO_NV:
			TODO_ERROR();

		case VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_SWIZZLE_STATE_CREATE_INFO_NV:
			TODO_ERROR();

		case VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_W_SCALING_STATE_CREATE_INFO_NV:
			TODO_ERROR();
		}
		next = next->pNext;
	}

	if (pViewportState->flags)
	{
		FATAL_ERROR();
	}

	return ViewportState
	{
		ArrayToVector(pViewportState->viewportCount, pViewportState->pViewports),
		ArrayToVector(pViewportState->scissorCount, pViewportState->pScissors),
	};
}

static RasterizationState Parse(const VkPipelineRasterizationStateCreateInfo* pRasterizationState)
{
	assert(pRasterizationState->sType == VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO);

#if defined(VK_EXT_line_rasterization)
	auto lineRasterizationMode = VK_LINE_RASTERIZATION_MODE_DEFAULT_EXT;
	auto stippledLineEnable = false;
	auto lineStippleFactor = 0u;
	auto lineStipplePattern = 0u;
#endif

	auto next = static_cast<const VkBaseInStructure*>(pRasterizationState->pNext);
	while (next)
	{
		const auto type = next->sType;
		switch (type)
		{
		case VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_CONSERVATIVE_STATE_CREATE_INFO_EXT:
			TODO_ERROR();

		case VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_DEPTH_CLIP_STATE_CREATE_INFO_EXT:
			TODO_ERROR();

#if defined(VK_EXT_line_rasterization)
		case VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_LINE_STATE_CREATE_INFO_EXT:
			{
				const auto rasterizationStateExt = reinterpret_cast<const VkPipelineRasterizationLineStateCreateInfoEXT*>(next);
				lineRasterizationMode = rasterizationStateExt->lineRasterizationMode;
				stippledLineEnable = rasterizationStateExt->stippledLineEnable;
				lineStippleFactor = rasterizationStateExt->lineStippleFactor;
				lineStipplePattern = rasterizationStateExt->lineStipplePattern;
				break;
			}
#endif

		case VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_RASTERIZATION_ORDER_AMD:
			TODO_ERROR();

		case VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_STREAM_CREATE_INFO_EXT:
			TODO_ERROR();
		}
		next = next->pNext;
	}

	if (pRasterizationState->flags)
	{
		FATAL_ERROR();
	}

	return RasterizationState
	{
		pRasterizationState->depthClampEnable != VK_FALSE,
		pRasterizationState->rasterizerDiscardEnable != VK_FALSE,
		pRasterizationState->polygonMode,
		pRasterizationState->cullMode,
		pRasterizationState->frontFace,
		pRasterizationState->depthBiasEnable != VK_FALSE,
		pRasterizationState->depthBiasConstantFactor,
		pRasterizationState->depthBiasClamp,
		pRasterizationState->depthBiasSlopeFactor,
		pRasterizationState->lineWidth,

#if defined(VK_EXT_line_rasterization)
		lineRasterizationMode,
		stippledLineEnable,
		lineStippleFactor,
		lineStipplePattern,
#endif
	};
}

static MultisampleState Parse(const VkPipelineMultisampleStateCreateInfo* pMultisampleState)
{
	if (!pMultisampleState)
	{
		return MultisampleState{};
	}
	
	assert(pMultisampleState->sType == VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO);

	auto next = pMultisampleState->pNext;
	while (next)
	{
		const auto type = static_cast<const VkBaseInStructure*>(next)->sType;
		switch (type)
		{
		case VK_STRUCTURE_TYPE_PIPELINE_COVERAGE_MODULATION_STATE_CREATE_INFO_NV:
			TODO_ERROR();

		case VK_STRUCTURE_TYPE_PIPELINE_COVERAGE_REDUCTION_STATE_CREATE_INFO_NV:
			TODO_ERROR();

		case VK_STRUCTURE_TYPE_PIPELINE_COVERAGE_TO_COLOR_STATE_CREATE_INFO_NV:
			TODO_ERROR();

		case VK_STRUCTURE_TYPE_PIPELINE_SAMPLE_LOCATIONS_STATE_CREATE_INFO_EXT:
			TODO_ERROR();
		}
		next = static_cast<const VkBaseInStructure*>(next)->pNext;
	}

	if (pMultisampleState->flags)
	{
		TODO_ERROR();
	}

	auto sampleMask = 0xFFFFFFFFFFFFFFFF;
	if (pMultisampleState->pSampleMask)
	{
		if (pMultisampleState->rasterizationSamples == VK_SAMPLE_COUNT_64_BIT)
		{
			TODO_ERROR();
		}
		else
		{
			sampleMask = *pMultisampleState->pSampleMask;
		}
	}

	return MultisampleState
	{
		pMultisampleState->rasterizationSamples,
		pMultisampleState->sampleShadingEnable != VK_FALSE,
		pMultisampleState->minSampleShading,
		pMultisampleState->alphaToCoverageEnable != VK_FALSE,
		pMultisampleState->alphaToOneEnable != VK_FALSE,
		sampleMask,
	};
}

static DepthStencilState Parse(const VkPipelineDepthStencilStateCreateInfo* pDepthStencilState)
{
	if (!pDepthStencilState)
	{
		return DepthStencilState{};
	}

	assert(pDepthStencilState->sType == VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO);

	if (pDepthStencilState->flags)
	{
		TODO_ERROR();
	}

	return DepthStencilState
	{
		pDepthStencilState->depthTestEnable != VK_FALSE,
		pDepthStencilState->depthWriteEnable != VK_FALSE,
		pDepthStencilState->depthCompareOp,
		pDepthStencilState->depthBoundsTestEnable != VK_FALSE,
		pDepthStencilState->stencilTestEnable != VK_FALSE,
		pDepthStencilState->front,
		pDepthStencilState->back,
		pDepthStencilState->minDepthBounds,
		pDepthStencilState->maxDepthBounds,
	};
}

static ColourBlendState Parse(const VkPipelineColorBlendStateCreateInfo* pColorBlendState)
{
	if (!pColorBlendState)
	{
		return {};
	}
	
	assert(pColorBlendState->sType == VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO);

	auto next = pColorBlendState->pNext;
	while (next)
	{
		const auto type = static_cast<const VkBaseInStructure*>(next)->sType;
		switch (type)
		{
		case VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_ADVANCED_STATE_CREATE_INFO_EXT:
			TODO_ERROR();
		}
		next = static_cast<const VkBaseInStructure*>(next)->pNext;
	}

	if (pColorBlendState->flags)
	{
		TODO_ERROR();
	}

	return ColourBlendState
	{
		pColorBlendState->logicOpEnable != VK_FALSE,
		pColorBlendState->logicOp,
		ArrayToVector(pColorBlendState->attachmentCount, pColorBlendState->pAttachments),
		{
			pColorBlendState->blendConstants[0],
			pColorBlendState->blendConstants[1],
			pColorBlendState->blendConstants[2],
			pColorBlendState->blendConstants[3],
		}
	};
}

static DynamicState Parse(const VkPipelineDynamicStateCreateInfo* pDynamicState)
{
	if (pDynamicState == nullptr)
	{
		return DynamicState{};
	}
	
	assert(pDynamicState->sType == VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO);

	if (pDynamicState->flags)
	{
		TODO_ERROR();
	}

	DynamicState dynamicState{};
	for (auto i = 0u; i < pDynamicState->dynamicStateCount; i++)
	{
		switch (pDynamicState->pDynamicStates[i])
		{
		case VK_DYNAMIC_STATE_VIEWPORT:
			dynamicState.DynamicViewport = true;
			break;
			
		case VK_DYNAMIC_STATE_SCISSOR:
			dynamicState.DynamicScissor = true;
			break;
			
		case VK_DYNAMIC_STATE_LINE_WIDTH:
			dynamicState.DynamicLineWidth = true;
			break;
			
		case VK_DYNAMIC_STATE_DEPTH_BIAS:
			dynamicState.DynamicDepthBias = true;
			break;
			
		case VK_DYNAMIC_STATE_BLEND_CONSTANTS:
			dynamicState.DynamicBlendConstants = true;
			break;
			
		case VK_DYNAMIC_STATE_DEPTH_BOUNDS:
			dynamicState.DynamicDepthBounds = true;
			break;
			
		case VK_DYNAMIC_STATE_STENCIL_COMPARE_MASK:
			dynamicState.DynamicStencilCompareMask = true;
			break;
			
		case VK_DYNAMIC_STATE_STENCIL_WRITE_MASK:
			dynamicState.DynamicStencilWriteMask = true;
			break;
			
		case VK_DYNAMIC_STATE_STENCIL_REFERENCE:
			dynamicState.DynamicStencilReference = true;
			break;
			
		case VK_DYNAMIC_STATE_VIEWPORT_W_SCALING_NV:
			dynamicState.DynamicViewportWScaling = true;
			break;
			
		case VK_DYNAMIC_STATE_DISCARD_RECTANGLE_EXT:
			dynamicState.DynamicDiscardRectangle = true;
			break;
			
		case VK_DYNAMIC_STATE_SAMPLE_LOCATIONS_EXT:
			dynamicState.DynamicSampleLocations = true;
			break;
			
		case VK_DYNAMIC_STATE_VIEWPORT_SHADING_RATE_PALETTE_NV:
			dynamicState.DynamicViewportShadingRatePalette = true;
			break;
			
		case VK_DYNAMIC_STATE_VIEWPORT_COARSE_SAMPLE_ORDER_NV:
			dynamicState.DynamicViewportCoarseSampleOrder = true;
			break;
			
		case VK_DYNAMIC_STATE_EXCLUSIVE_SCISSOR_NV:
			dynamicState.DynamicExclusiveScissor = true;
			break;
			
		case VK_DYNAMIC_STATE_LINE_STIPPLE_EXT:
			dynamicState.DynamicLineStipple = true;
			break;
			
		default:
			FATAL_ERROR();
		}
	}

	return dynamicState;;
}

static SPIRV::SPIRVFunction* FindEntryPoint(const SPIRV::SPIRVModule* module, SPIRV::SPIRVExecutionModelKind stage, const char* name)
{
	for (auto i = 0u; i < module->getNumEntryPoints(stage); i++)
	{
		const auto& entryName = module->getEntryPointName(stage, i);
		if (entryName == name)
		{
			return module->getEntryPoint(stage, i);
		}
	}
	return nullptr;
}

struct MemoryStream final : std::streambuf
{
public:
	const std::vector<char>& getData() const { return data; }
	
protected:
	std::streambuf* setbuf(char* s, std::streamsize n) override
	{
		FATAL_ERROR();
	}

	std::streampos seekoff(std::streamoff off, std::ios_base::seekdir way, std::ios_base::openmode which) override
	{
		FATAL_ERROR();
	}

	std::streampos seekpos(std::streampos sp, std::ios_base::openmode which) override
	{
		FATAL_ERROR();
	}

	int sync() override
	{
		FATAL_ERROR();
	}

	std::streamsize showmanyc() override
	{
		FATAL_ERROR();
	}

	std::streamsize xsgetn(char* s, std::streamsize n) override
	{
		FATAL_ERROR();
	}

	int underflow() override
	{
		FATAL_ERROR();
	}

	int uflow() override
	{
		FATAL_ERROR();
	}

	int pbackfail(int c) override
	{
		FATAL_ERROR();
	}
	
	std::streamsize xsputn(const char* _Ptr, std::streamsize _Count) override
	{
		for (auto i = 0u; i < _Count; i++)
		{
			data.push_back(_Ptr[i]);
		}
		return _Count;
	}

	int overflow(int c) override
	{
		FATAL_ERROR();
	}

private:
	std::vector<char> data{};
};

CompiledShaderModule::~CompiledShaderModule()
{
	delete llvmModule;
}

void Pipeline::CompileBaseShaderModule(ShaderModule* shaderModule, const char* entryName, const VkSpecializationInfo* specializationInfo, spv::ExecutionModel executionModel, 
                                       bool& hitCache, CompiledModule*& llvmModule, SPIRV::SPIRVFunction*& entryPointFunction, 
                                       std::function<CompiledModule*(CPJit*, const SPIRV::SPIRVModule*, spv::ExecutionModel, const SPIRV::SPIRVFunction*, const VkSpecializationInfo*)> compileFunction = CompileSPIRVModule)
{
	entryPointFunction = FindEntryPoint(shaderModule->getModule(), executionModel, entryName);
	assert(entryPointFunction);

	Hash hash{};

	if (cache)
	{
		hash = CalculateHash(shaderModule->getModule(), executionModel, entryPointFunction, specializationInfo);
		llvmModule = cache->FindModule(hash, jit, nullptr);
		if (llvmModule)
		{
			std::cout << "Hit cache" << std::endl;
			hitCache = true;
		}
	}

	if (!llvmModule)
	{
		llvmModule = compileFunction(jit, shaderModule->getModule(), executionModel, entryPointFunction, specializationInfo);
		if (cache)
		{
			cache->AddModule(hash, llvmModule);
		}
	}
}

Hash Pipeline::CalculateHash(SPIRV::SPIRVModule* spirvModule, ExecutionModel stage, const SPIRV::SPIRVFunction* entryPoint, const VkSpecializationInfo* specializationInfo)
{
	sha3_context c;
	sha3_Init256(&c);

	CalculatePipelineHash(&c, stage);

	MemoryStream ms{};
	std::ostream os{&ms};
	os << *spirvModule;
	sha3_Update(&c, ms.getData().data(), ms.getData().size());

	sha3_Update(&c, &stage, sizeof(stage));

	const auto id = entryPoint->getId();
	sha3_Update(&c, &id, sizeof(id));

	if (specializationInfo)
	{
		sha3_Update(&c, specializationInfo->pMapEntries, specializationInfo->mapEntryCount * sizeof(VkSpecializationMapEntry));
		sha3_Update(&c, specializationInfo->pData, specializationInfo->dataSize);
	}
	
	const auto hash = sha3_Finalize(&c);
	Hash result{};
	memcpy(result.bytes, hash, sizeof(result.bytes));
	return result;
}

VkResult GraphicsPipeline::Create(Device* device, VkPipelineCache pipelineCache, const VkGraphicsPipelineCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkPipeline* pPipeline)
{
	assert(pCreateInfo->sType == VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO);

	auto pipeline = Allocate<GraphicsPipeline>(pAllocator, VK_SYSTEM_ALLOCATION_SCOPE_DEVICE);
	if (!pipeline)
	{
		return VK_ERROR_OUT_OF_HOST_MEMORY;
	}

	pipeline->jit = device->getState()->jit;

	auto feedback = false;
	auto next = static_cast<const VkBaseInStructure*>(pCreateInfo->pNext);
	while (next)
	{
		const auto type = next->sType;
		switch (type)
		{
		case VK_STRUCTURE_TYPE_PIPELINE_COMPILER_CONTROL_CREATE_INFO_AMD:
			TODO_ERROR();

		case VK_STRUCTURE_TYPE_PIPELINE_CREATION_FEEDBACK_CREATE_INFO_EXT:
			feedback = true;
			break;

		case VK_STRUCTURE_TYPE_PIPELINE_DISCARD_RECTANGLE_STATE_CREATE_INFO_EXT:
			TODO_ERROR();

		case VK_STRUCTURE_TYPE_PIPELINE_REPRESENTATIVE_FRAGMENT_TEST_STATE_CREATE_INFO_NV:
			TODO_ERROR();
		}
		next = next->pNext;
	}

	if (pCreateInfo->flags & VK_PIPELINE_CREATE_DISABLE_OPTIMIZATION_BIT)
	{
		// TODO
	}

	if (pCreateInfo->flags & VK_PIPELINE_CREATE_VIEW_INDEX_FROM_DEVICE_INDEX_BIT)
	{
		TODO_ERROR();
	}

	if (pCreateInfo->flags & VK_PIPELINE_CREATE_DEFER_COMPILE_BIT_NV)
	{
		TODO_ERROR();
	}

	if (pCreateInfo->flags & VK_PIPELINE_CREATE_CAPTURE_STATISTICS_BIT_KHR)
	{
		TODO_ERROR();
	}

	if (pCreateInfo->flags & VK_PIPELINE_CREATE_CAPTURE_INTERNAL_REPRESENTATIONS_BIT_KHR)
	{
		TODO_ERROR();
	}

	const auto startTime = feedback ? Platform::GetTimestamp() : 0;

	pipeline->vertexInputState = Parse(pCreateInfo->pVertexInputState);
	pipeline->inputAssemblyState = Parse(pCreateInfo->pInputAssemblyState);
	pipeline->tessellationState = Parse(pCreateInfo->pTessellationState);
	pipeline->viewportState = Parse(pCreateInfo->pViewportState);
	pipeline->rasterizationState = Parse(pCreateInfo->pRasterizationState);
	pipeline->multisampleState = Parse(pCreateInfo->pMultisampleState);
	pipeline->depthStencilState = Parse(pCreateInfo->pDepthStencilState);
	pipeline->colourBlendState = Parse(pCreateInfo->pColorBlendState);
	pipeline->dynamicState = Parse(pCreateInfo->pDynamicState);
	pipeline->layout = UnwrapVulkan<PipelineLayout>(pCreateInfo->layout);
	pipeline->renderPass = UnwrapVulkan<RenderPass>(pCreateInfo->renderPass);
	pipeline->subpass = pCreateInfo->subpass;
	pipeline->cache = pipelineCache ? UnwrapVulkan<PipelineCache>(pipelineCache) : nullptr;

	auto shaderFeedback = std::vector<StageFeedback>(pCreateInfo->stageCount);

	for (auto i = 0u; i < pCreateInfo->stageCount; i++)
	{
		pipeline->LoadShaderStage(device, feedback, shaderFeedback[i], pCreateInfo->pStages[i]);
	}

	const auto endTime = feedback ? Platform::GetTimestamp() : 0;

	if (feedback)
	{
		next = static_cast<const VkBaseInStructure*>(pCreateInfo->pNext);
		while (next)
		{
			if (next->sType == VK_STRUCTURE_TYPE_PIPELINE_CREATION_FEEDBACK_CREATE_INFO_EXT)
			{
				const auto feedbackInfo = reinterpret_cast<const VkPipelineCreationFeedbackCreateInfoEXT*>(next);
				feedbackInfo->pPipelineCreationFeedback->flags = VK_PIPELINE_CREATION_FEEDBACK_VALID_BIT_EXT;
				feedbackInfo->pPipelineCreationFeedback->duration = (endTime - startTime) * static_cast<uint64_t>(Platform::GetTimestampPeriod());
				assert(feedbackInfo->pipelineStageCreationFeedbackCount == pCreateInfo->stageCount);

				for (auto i = 0u; i < pCreateInfo->stageCount; i++)
				{
					feedbackInfo->pPipelineStageCreationFeedbacks[i].flags = VK_PIPELINE_CREATION_FEEDBACK_VALID_BIT_EXT;
					feedbackInfo->pPipelineStageCreationFeedbacks[i].duration = shaderFeedback[i].duration * static_cast<uint64_t>(Platform::GetTimestampPeriod());
					if (shaderFeedback[i].hitCache)
					{
						feedbackInfo->pPipelineStageCreationFeedbacks[i].flags |= VK_PIPELINE_CREATION_FEEDBACK_APPLICATION_PIPELINE_CACHE_HIT_BIT_EXT;
					}
				}
				
				break;
			}
			next = next->pNext;
		}
	}

	WrapVulkan(pipeline, pPipeline);
	return VK_SUCCESS;
}

VkResult Device::CreateGraphicsPipelines(VkPipelineCache pipelineCache, uint32_t createInfoCount, const VkGraphicsPipelineCreateInfo* pCreateInfos, const VkAllocationCallbacks* pAllocator, VkPipeline* pPipelines)
{
	for (auto i = 0u; i < createInfoCount; i++)
	{
		pPipelines[i] = VK_NULL_HANDLE;
	}

	for (auto i = 0u; i < createInfoCount; i++)
	{
		const auto result = GraphicsPipeline::Create(this, pipelineCache, &pCreateInfos[i], pAllocator, &pPipelines[i]);
		if (result != VK_SUCCESS)
		{
			return result;
		}
	}

	return VK_SUCCESS;
}

void GraphicsPipeline::CalculatePipelineHash(sha3_context* context, spv::ExecutionModel stage)
{
	// TODO: Ideally only save state relevant to specific module
	sha3_Update(context, vertexInputState.VertexAttributeDescriptions.data(), sizeof(vertexInputState.VertexAttributeDescriptions[0]) * vertexInputState.VertexAttributeDescriptions.size());
	sha3_Update(context, vertexInputState.VertexBindingDescriptions.data(), sizeof(vertexInputState.VertexBindingDescriptions[0]) * vertexInputState.VertexBindingDescriptions.size());
	sha3_Update(context, &inputAssemblyState, sizeof(inputAssemblyState));
	sha3_Update(context, &tessellationState, sizeof(tessellationState));
	sha3_Update(context, viewportState.Viewports.data(), sizeof(viewportState.Viewports[0]) * viewportState.Viewports.size());
	sha3_Update(context, viewportState.Scissors.data(), sizeof(viewportState.Scissors[0]) * viewportState.Scissors.size());
	sha3_Update(context, &rasterizationState, sizeof(rasterizationState));
	sha3_Update(context, &multisampleState, sizeof(multisampleState));
	sha3_Update(context, &depthStencilState, sizeof(depthStencilState));
	sha3_Update(context, colourBlendState.Attachments.data(), sizeof(colourBlendState.Attachments[0]) * colourBlendState.Attachments.size());
	sha3_Update(context, &colourBlendState.BlendConstants, sizeof(colourBlendState.BlendConstants));
	sha3_Update(context, &colourBlendState.LogicOp, sizeof(colourBlendState.LogicOp));
	sha3_Update(context, &colourBlendState.LogicOpEnable, sizeof(colourBlendState.LogicOpEnable));
	sha3_Update(context, &dynamicState, sizeof(dynamicState));
	sha3_Update(context, renderPass->getAttachments().data(), sizeof(renderPass->getAttachments()[0]) * renderPass->getAttachments().size());

	const auto& currentSubpass = renderPass->getSubpasses()[subpass];
	sha3_Update(context, &currentSubpass.flags, sizeof(currentSubpass.flags));
	sha3_Update(context, &currentSubpass.pipelineBindPoint, sizeof(currentSubpass.pipelineBindPoint));
	sha3_Update(context, &currentSubpass.viewMask, sizeof(currentSubpass.viewMask));
	sha3_Update(context, currentSubpass.inputAttachments.data(), sizeof(currentSubpass.inputAttachments[0]) * currentSubpass.inputAttachments.size());
	sha3_Update(context, currentSubpass.colourAttachments.data(), sizeof(currentSubpass.colourAttachments[0]) * currentSubpass.colourAttachments.size());
	sha3_Update(context, currentSubpass.resolveAttachments.data(), sizeof(currentSubpass.resolveAttachments[0]) * currentSubpass.resolveAttachments.size());
	sha3_Update(context, &currentSubpass.depthStencilAttachment, sizeof(currentSubpass.depthStencilAttachment));
	sha3_Update(context, currentSubpass.preserveAttachments.data(), sizeof(currentSubpass.preserveAttachments[0]) * currentSubpass.preserveAttachments.size());
}

void GraphicsPipeline::LoadShaderStage(Device* device, bool fetchFeedback, StageFeedback& stageFeedback, const VkPipelineShaderStageCreateInfo& stage)
{
	const auto stageStartTime = fetchFeedback ? Platform::GetTimestamp() : 0;
	
	assert(stage.sType == VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO);

	auto next = static_cast<const VkBaseInStructure*>(stage.pNext);
	while (next)
	{
		const auto type = next->sType;
		switch (type)
		{
		case VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_REQUIRED_SUBGROUP_SIZE_CREATE_INFO_EXT:
			TODO_ERROR();
		}
		next = next->pNext;
	}

	if (stage.flags)
	{
		TODO_ERROR();
	}

	const auto stageIndex = GetStageIndex(stage.stage);
	auto hitCache = false;
	switch (stageIndex)
	{
	case 0:
		vertexShaderModule = CompileVertexShaderModule(device, UnwrapVulkan<ShaderModule>(stage.module), stage.pName, stage.pSpecializationInfo, hitCache);
		break;

	case 1:
		TODO_ERROR();

		// 	if (stageIndex == ExecutionModelTessellationControl || stageIndex == ExecutionModelTessellationEvaluation)
		// 	{
		// 		if (entryPointFunction->getExecutionMode(SPIRV::SPIRVExecutionModeKind::ExecutionModeSpacingEqual))
		// 		{
		// 			TODO_ERROR();
		// 		}
		//
		// 		if (entryPointFunction->getExecutionMode(SPIRV::SPIRVExecutionModeKind::ExecutionModeSpacingFractionalEven))
		// 		{
		// 			TODO_ERROR();
		// 		}
		//
		// 		if (entryPointFunction->getExecutionMode(SPIRV::SPIRVExecutionModeKind::ExecutionModeSpacingFractionalOdd))
		// 		{
		// 			TODO_ERROR();
		// 		}
		//
		// 		if (entryPointFunction->getExecutionMode(SPIRV::SPIRVExecutionModeKind::ExecutionModeVertexOrderCw))
		// 		{
		// 			TODO_ERROR();
		// 		}
		//
		// 		if (entryPointFunction->getExecutionMode(SPIRV::SPIRVExecutionModeKind::ExecutionModeVertexOrderCcw))
		// 		{
		// 			TODO_ERROR();
		// 		}
		//
		// 		if (entryPointFunction->getExecutionMode(SPIRV::SPIRVExecutionModeKind::ExecutionModePointMode))
		// 		{
		// 			TODO_ERROR();
		// 		}
		//
		// 		if (entryPointFunction->getExecutionMode(SPIRV::SPIRVExecutionModeKind::ExecutionModeTriangles))
		// 		{
		// 			TODO_ERROR();
		// 		}
		//
		// 		if (entryPointFunction->getExecutionMode(SPIRV::SPIRVExecutionModeKind::ExecutionModeQuads))
		// 		{
		// 			TODO_ERROR();
		// 		}
		//
		// 		if (entryPointFunction->getExecutionMode(SPIRV::SPIRVExecutionModeKind::ExecutionModeIsolines))
		// 		{
		// 			TODO_ERROR();
		// 		}
		//
		// 		const auto outputVertices = entryPoint->getExecutionMode(SPIRV::SPIRVExecutionModeKind::ExecutionModeOutputVertices);
		// 		if (outputVertices)
		// 		{
		// 			TODO_ERROR();
		// 		}
		// 	}

	case 2:
		TODO_ERROR();

	case 3:
		// 	if (stageIndex == ExecutionModelGeometry)
		// 	{
		// 		const auto invocations = entryPoint->getExecutionMode(SPIRV::SPIRVExecutionModeKind::ExecutionModeInvocations);
		// 		if (invocations)
		// 		{
		// 			TODO_ERROR();
		// 		}
		//
		// 		if (entryPointFunction->getExecutionMode(SPIRV::SPIRVExecutionModeKind::ExecutionModeInputPoints))
		// 		{
		// 			TODO_ERROR();
		// 		}
		//
		// 		if (entryPointFunction->getExecutionMode(SPIRV::SPIRVExecutionModeKind::ExecutionModeInputLines))
		// 		{
		// 			TODO_ERROR();
		// 		}
		//
		// 		if (entryPointFunction->getExecutionMode(SPIRV::SPIRVExecutionModeKind::ExecutionModeInputLinesAdjacency))
		// 		{
		// 			TODO_ERROR();
		// 		}
		//
		// 		if (entryPointFunction->getExecutionMode(SPIRV::SPIRVExecutionModeKind::ExecutionModeTriangles))
		// 		{
		// 			TODO_ERROR();
		// 		}
		//
		// 		if (entryPointFunction->getExecutionMode(SPIRV::SPIRVExecutionModeKind::ExecutionModeInputTrianglesAdjacency))
		// 		{
		// 			TODO_ERROR();
		// 		}
		//
		// 		const auto outputVertices = entryPoint->getExecutionMode(SPIRV::SPIRVExecutionModeKind::ExecutionModeOutputVertices);
		// 		if (outputVertices)
		// 		{
		// 			TODO_ERROR();
		// 		}
		//
		// 		if (entryPointFunction->getExecutionMode(SPIRV::SPIRVExecutionModeKind::ExecutionModeOutputPoints))
		// 		{
		// 			TODO_ERROR();
		// 		}
		//
		// 		if (entryPointFunction->getExecutionMode(SPIRV::SPIRVExecutionModeKind::ExecutionModeOutputLineStrip))
		// 		{
		// 			TODO_ERROR();
		// 		}
		//
		// 		if (entryPointFunction->getExecutionMode(SPIRV::SPIRVExecutionModeKind::ExecutionModeOutputTriangleStrip))
		// 		{
		// 			TODO_ERROR();
		// 		}
		// 	}
		TODO_ERROR();

	case 4:
		fragmentShaderModule = CompileFragmentShaderModule(device, UnwrapVulkan<ShaderModule>(stage.module), stage.pName, stage.pSpecializationInfo, hitCache);
		break;
		
	default:
		FATAL_ERROR();
	}

	if (fetchFeedback)
	{
		const auto stageEndTime = Platform::GetTimestamp();
		stageFeedback.duration = stageEndTime - stageStartTime;
		stageFeedback.hitCache = hitCache;
	}
}

std::unique_ptr<VertexShaderModule> GraphicsPipeline::CompileVertexShaderModule(Device* device, ShaderModule* shaderModule, const char* entryName, const VkSpecializationInfo* specializationInfo, bool& hitCache)
{
	CompiledModule* llvmModule{};
	SPIRV::SPIRVFunction* entryPointFunction;
	CompileBaseShaderModule(shaderModule, entryName, specializationInfo, ExecutionModelVertex, 
	                        hitCache, llvmModule, entryPointFunction, 
	                        [this, device](CPJit* jit, const SPIRV::SPIRVModule* spirvModule, spv::ExecutionModel, const SPIRV::SPIRVFunction* entryPoint, const VkSpecializationInfo* specializationInfo)
	                        {
		                        const auto llvmModule = CompileVertexPipeline(jit, this, spirvModule, entryPoint, specializationInfo);
		                        *static_cast<GraphicsNativeState**>(llvmModule->getPointer("@pipelineState")) = &device->getState()->graphicsPipelineState.nativeState;
		                        return llvmModule;
	                        });
	
	if (entryPointFunction->getExecutionMode(SPIRV::SPIRVExecutionModeKind::ExecutionModeXfb))
	{
		TODO_ERROR();
	}

	const auto entryPoint = llvmModule->getFunctionPointer("@main");
	return std::make_unique<VertexShaderModule>(shaderModule->getModule(), llvmModule, entryPoint);
}

std::unique_ptr<FragmentShaderModule> GraphicsPipeline::CompileFragmentShaderModule(Device* device, ShaderModule* shaderModule, const char* entryName, const VkSpecializationInfo* specializationInfo, bool& hitCache)
{
	CompiledModule* llvmModule{};
	SPIRV::SPIRVFunction* entryPointFunction;
	CompileBaseShaderModule(shaderModule, entryName, specializationInfo, ExecutionModelFragment, hitCache, llvmModule, entryPointFunction, 
	                        [this, device](CPJit* jit, const SPIRV::SPIRVModule* spirvModule, spv::ExecutionModel, const SPIRV::SPIRVFunction* entryPoint, const VkSpecializationInfo* specializationInfo)
	                        {
		                        const auto llvmModule = CompileFragmentPipeline(jit, this, spirvModule, entryPoint, specializationInfo);
		                        *static_cast<GraphicsNativeState**>(llvmModule->getPointer("@pipelineState")) = &device->getState()->graphicsPipelineState.nativeState;
		                        return llvmModule;
	                        });

	if (entryPointFunction->getExecutionMode(SPIRV::SPIRVExecutionModeKind::ExecutionModeXfb))
	{
		TODO_ERROR();
	}

	const auto entryPoint = llvmModule->getFunctionPointer("@main");
	auto result = std::make_unique<FragmentShaderModule>(shaderModule->getModule(), llvmModule, entryPoint);
	
	if (entryPointFunction->getExecutionMode(SPIRV::SPIRVExecutionModeKind::ExecutionModePixelCenterInteger))
	{
		TODO_ERROR();
	}
	
	if (entryPointFunction->getExecutionMode(SPIRV::SPIRVExecutionModeKind::ExecutionModeOriginUpperLeft))
	{
		result->originUpper = true;
	}
	
	if (entryPointFunction->getExecutionMode(SPIRV::SPIRVExecutionModeKind::ExecutionModeOriginLowerLeft))
	{
		result->originUpper = false;
	}
	
	if (entryPointFunction->getExecutionMode(SPIRV::SPIRVExecutionModeKind::ExecutionModeEarlyFragmentTests))
	{
		TODO_ERROR();
	}
	
	if (entryPointFunction->getExecutionMode(SPIRV::SPIRVExecutionModeKind::ExecutionModeDepthReplacing))
	{
		TODO_ERROR();
	}
	
	if (entryPointFunction->getExecutionMode(SPIRV::SPIRVExecutionModeKind::ExecutionModeDepthGreater))
	{
		TODO_ERROR();
	}
	
	if (entryPointFunction->getExecutionMode(SPIRV::SPIRVExecutionModeKind::ExecutionModeDepthLess))
	{
		TODO_ERROR();
	}
	
	if (entryPointFunction->getExecutionMode(SPIRV::SPIRVExecutionModeKind::ExecutionModeDepthUnchanged))
	{
		TODO_ERROR();
	}
	
	return result;
}

VkResult ComputePipeline::Create(Device* device, VkPipelineCache pipelineCache, const VkComputePipelineCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkPipeline* pPipeline)
{
	assert(pCreateInfo->sType == VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO);

	auto pipeline = Allocate<ComputePipeline>(pAllocator, VK_SYSTEM_ALLOCATION_SCOPE_DEVICE);
	if (!pipeline)
	{
		return VK_ERROR_OUT_OF_HOST_MEMORY;
	}

	pipeline->jit = device->getState()->jit;

	auto feedback = false;
	auto next = static_cast<const VkBaseInStructure*>(pCreateInfo->pNext);
	while (next)
	{
		const auto type = next->sType;
		switch (type)
		{
		case VK_STRUCTURE_TYPE_PIPELINE_CREATION_FEEDBACK_CREATE_INFO_EXT:
			feedback = true;
			break;
		}
		next = next->pNext;
	}

	if (pCreateInfo->flags & VK_PIPELINE_CREATE_DISABLE_OPTIMIZATION_BIT)
	{
		// TODO
	}

	if (pCreateInfo->flags & VK_PIPELINE_CREATE_DISPATCH_BASE)
	{
		TODO_ERROR();
	}

	if (pCreateInfo->flags & VK_PIPELINE_CREATE_DEFER_COMPILE_BIT_NV)
	{
		TODO_ERROR();
	}

	if (pCreateInfo->flags & VK_PIPELINE_CREATE_CAPTURE_STATISTICS_BIT_KHR)
	{
		TODO_ERROR();
	}

	if (pCreateInfo->flags & VK_PIPELINE_CREATE_CAPTURE_INTERNAL_REPRESENTATIONS_BIT_KHR)
	{
		TODO_ERROR();
	}

	const auto startTime = feedback ? Platform::GetTimestamp() : 0;
	
	pipeline->cache = pipelineCache ? UnwrapVulkan<PipelineCache>(pipelineCache) : nullptr;

	StageFeedback shaderFeedback{};
	pipeline->LoadShaderStage(device, feedback, shaderFeedback, pCreateInfo->stage);

	const auto endTime = feedback ? Platform::GetTimestamp() : 0;

	if (feedback)
	{
		next = static_cast<const VkBaseInStructure*>(pCreateInfo->pNext);
		while (next)
		{
			if (next->sType == VK_STRUCTURE_TYPE_PIPELINE_CREATION_FEEDBACK_CREATE_INFO_EXT)
			{
				const auto feedbackInfo = reinterpret_cast<const VkPipelineCreationFeedbackCreateInfoEXT*>(next);
				feedbackInfo->pPipelineCreationFeedback->flags = VK_PIPELINE_CREATION_FEEDBACK_VALID_BIT_EXT;
				feedbackInfo->pPipelineCreationFeedback->duration = (endTime - startTime) * static_cast<uint64_t>(Platform::GetTimestampPeriod());
				assert(feedbackInfo->pipelineStageCreationFeedbackCount == 1);

				feedbackInfo->pPipelineStageCreationFeedbacks[0].flags = VK_PIPELINE_CREATION_FEEDBACK_VALID_BIT_EXT;
				feedbackInfo->pPipelineStageCreationFeedbacks[0].duration = shaderFeedback.duration * static_cast<uint64_t>(Platform::GetTimestampPeriod());
				if (shaderFeedback.hitCache)
				{
					feedbackInfo->pPipelineStageCreationFeedbacks[0].flags |= VK_PIPELINE_CREATION_FEEDBACK_APPLICATION_PIPELINE_CACHE_HIT_BIT_EXT;
				}
				
				break;
			}
			next = next->pNext;
		}
	}

	WrapVulkan(pipeline, pPipeline);
	return VK_SUCCESS;
}

void ComputePipeline::CalculatePipelineHash(sha3_context*, spv::ExecutionModel)
{
	// No state to hash
}

void ComputePipeline::LoadShaderStage(Device* device, bool fetchFeedback, StageFeedback& stageFeedback, const VkPipelineShaderStageCreateInfo& stage)
{
	const auto stageStartTime = fetchFeedback ? Platform::GetTimestamp() : 0;

	assert(stage.sType == VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO);
	
	auto next = static_cast<const VkBaseInStructure*>(stage.pNext);
	while (next)
	{
		const auto type = next->sType;
		switch (type)
		{
		case VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_REQUIRED_SUBGROUP_SIZE_CREATE_INFO_EXT:
			TODO_ERROR();
		}
		next = next->pNext;
	}
	
	if (stage.flags)
	{
		TODO_ERROR();
	}

	if (stage.stage != VK_SHADER_STAGE_COMPUTE_BIT)
	{
		FATAL_ERROR();
	}

	auto hitCache = false;
	computeShaderModule = CompileComputeShaderModule(device, UnwrapVulkan<ShaderModule>(stage.module), stage.pName, stage.pSpecializationInfo, hitCache);

	if (fetchFeedback)
	{
		const auto stageEndTime = Platform::GetTimestamp();
		stageFeedback.duration = stageEndTime - stageStartTime;
		stageFeedback.hitCache = hitCache;
	}
}

std::unique_ptr<ComputeShaderModule> ComputePipeline::CompileComputeShaderModule(Device* device, ShaderModule* shaderModule, const char* entryName, const VkSpecializationInfo* specializationInfo, bool& hitCache)
{
	CompiledModule* llvmModule{};
	SPIRV::SPIRVFunction* entryPointFunction;
	CompileBaseShaderModule(shaderModule, entryName, specializationInfo, ExecutionModelGLCompute, hitCache, llvmModule, entryPointFunction);
	
	if (entryPointFunction->getExecutionMode(SPIRV::SPIRVExecutionModeKind::ExecutionModeXfb))
	{
		TODO_ERROR();
	}

	const auto entryPoint = llvmModule->getFunctionPointer(MangleName(entryPointFunction));
	auto result = std::make_unique<ComputeShaderModule>(shaderModule->getModule(), llvmModule, entryPoint);
	
	const auto localSize = entryPointFunction->getExecutionMode(SPIRV::SPIRVExecutionModeKind::ExecutionModeLocalSize);
	if (localSize)
	{
		const auto& literals = localSize->getLiterals();
		result->localSize = glm::uvec3{literals[0], literals[1], literals[2]};
	}
	
	const auto localSizeId = entryPointFunction->getExecutionMode(SPIRV::SPIRVExecutionModeKind::ExecutionModeLocalSizeId);
	if (localSizeId)
	{
		TODO_ERROR();
	}

	// 	if (stageIndex == ExecutionModelKernel)
	// 	{
	// 		const auto localSizeHint = entryPoint->getExecutionMode(SPIRV::SPIRVExecutionModeKind::ExecutionModeLocalSizeHint);
	// 		if (localSizeHint)
	// 		{
	// 			TODO_ERROR();
	// 		}
	//
	// 		const auto vecTypeHint = entryPoint->getExecutionMode(SPIRV::SPIRVExecutionModeKind::ExecutionModeContractionOff);
	// 		if (vecTypeHint)
	// 		{
	// 			TODO_ERROR();
	// 		}
	//
	// 		if (entryPointFunction->getExecutionMode(SPIRV::SPIRVExecutionModeKind::ExecutionModeDepthUnchanged))
	// 		{
	// 			TODO_ERROR();
	// 		}
	//
	// 		if (entryPointFunction->getExecutionMode(SPIRV::SPIRVExecutionModeKind::ExecutionModeInitializer))
	// 		{
	// 			TODO_ERROR();
	// 		}
	//
	// 		if (entryPointFunction->getExecutionMode(SPIRV::SPIRVExecutionModeKind::ExecutionModeFinalizer))
	// 		{
	// 			TODO_ERROR();
	// 		}
	//
	// 		const auto subgroupSize = entryPoint->getExecutionMode(SPIRV::SPIRVExecutionModeKind::ExecutionModeSubgroupSize);
	// 		if (subgroupSize)
	// 		{
	// 			TODO_ERROR();
	// 		}
	//
	// 		const auto subgroupsPerWorkgroup = entryPoint->getExecutionMode(SPIRV::SPIRVExecutionModeKind::ExecutionModeSubgroupsPerWorkgroup);
	// 		if (subgroupsPerWorkgroup)
	// 		{
	// 			TODO_ERROR();
	// 		}
	//
	// 		const auto subgroupsPerWorkgroupId = entryPoint->getExecutionMode(SPIRV::SPIRVExecutionModeKind::ExecutionModeSubgroupsPerWorkgroupId);
	// 		if (subgroupsPerWorkgroupId)
	// 		{
	// 			TODO_ERROR();
	// 		}
	//
	// 		const auto localSizeId = entryPoint->getExecutionMode(SPIRV::SPIRVExecutionModeKind::ExecutionModeLocalSizeId);
	// 		if (localSizeId)
	// 		{
	// 			TODO_ERROR();
	// 		}
	//
	// 		const auto localSizeHintId = entryPoint->getExecutionMode(SPIRV::SPIRVExecutionModeKind::ExecutionModeLocalSizeHintId);
	// 		if (localSizeHintId)
	// 		{
	// 			TODO_ERROR();
	// 		}
	// 	}

	const auto workgroupSizePtr = llvmModule->getOptionalPointer("@WorkgroupSize");
	if (workgroupSizePtr)
	{
		const auto workgroupSizeValue = static_cast<glm::uvec3*>(workgroupSizePtr);
		result->localSize = *workgroupSizeValue;
	}

	return result;
}

VkResult Device::CreateComputePipelines(VkPipelineCache pipelineCache, uint32_t createInfoCount, const VkComputePipelineCreateInfo* pCreateInfos, const VkAllocationCallbacks* pAllocator, VkPipeline* pPipelines)
{
	for (auto i = 0u; i < createInfoCount; i++)
	{
		pPipelines[i] = VK_NULL_HANDLE;
	}

	for (auto i = 0u; i < createInfoCount; i++)
	{
		const auto result = ComputePipeline::Create(this, pipelineCache, &pCreateInfos[i], pAllocator, &pPipelines[i]);
		if (result != VK_SUCCESS)
		{
			return result;
		}
	}

	return VK_SUCCESS;
}

void Device::DestroyPipeline(VkPipeline pipeline, const VkAllocationCallbacks* pAllocator)
{
	if (pipeline)
	{
		Free(UnwrapVulkan<Pipeline>(pipeline), pAllocator);
	}
}

#if defined(VK_KHR_pipeline_executable_properties)
VkResult Device::GetPipelineExecutableProperties(const VkPipelineInfoKHR* pPipelineInfo, uint32_t* pExecutableCount, VkPipelineExecutablePropertiesKHR* pProperties)
{
	static const char* shaderName[]
	{
		"Vertex Shader",
		"Tessellation Control Shader",
		"Tessellation Evaluation Shader",
		"Geometry Shader",
		"Fragment Shader",
		"Compute Shader",
	};
	
	const auto pipeline = UnwrapVulkan<Pipeline>(pPipelineInfo->pipeline);
	auto count = 0u;
	for (auto i = 0u; i < pipeline->getMaxShaderStages(); i++)
	{
		if (pipeline->getShaderStage(i) != nullptr)
		{
			count++;
		}
	}
	
	if (pProperties == nullptr)
	{
		*pExecutableCount = count;
		return VK_SUCCESS;
	}

	auto result = VK_SUCCESS;
	if (count > *pExecutableCount)
	{
		count = *pExecutableCount;
		result = VK_INCOMPLETE;
	}

	for (auto i = 0u, j = 0u; i < pipeline->getMaxShaderStages() && j < count; i++)
	{
		const auto shaderStage = pipeline->getShaderStage(i);
		if (shaderStage)
		{
			pProperties[j].stages = 1 << i;
			memset(pProperties[j].name, 0, VK_MAX_DESCRIPTION_SIZE);
			memset(pProperties[j].description, 0, VK_MAX_DESCRIPTION_SIZE);

			// TODO: Expand on this
			strcpy_s(pProperties[j].name, shaderName[i]);
			strcpy_s(pProperties[j].description, shaderName[i]);
			
			pProperties[j].subgroupSize = 1;
			j++;
		}
	}
	
	return result;
}

VkResult Device::GetPipelineExecutableStatistics(const VkPipelineExecutableInfoKHR* pExecutableInfo, uint32_t* pStatisticCount, VkPipelineExecutableStatisticKHR* pStatistics)
{
	TODO_ERROR();
}

VkResult Device::GetPipelineExecutableInternalRepresentations(const VkPipelineExecutableInfoKHR* pExecutableInfo, uint32_t* pInternalRepresentationCount, VkPipelineExecutableInternalRepresentationKHR* pInternalRepresentations)
{
	TODO_ERROR();
}
#endif