#include "Pipeline.h"

#include "Device.h"
#include "DeviceState.h"
#include "ShaderModule.h"
#include "Util.h"

#include <Jit.h>
#include <SPIRVFunction.h>
#include <SPIRVModule.h>

#include <cassert>

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
			FATAL_ERROR();
		}
		next = next->pNext;
	}

	if (pVertexInputState->flags)
	{
		FATAL_ERROR();
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
		FATAL_ERROR();
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
			FATAL_ERROR();
		}
		next = static_cast<const VkBaseInStructure*>(next)->pNext;
	}

	if (pTessellationState->flags)
	{
		FATAL_ERROR();
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
			FATAL_ERROR();

		case VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_EXCLUSIVE_SCISSOR_STATE_CREATE_INFO_NV:
			FATAL_ERROR();

		case VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_SHADING_RATE_IMAGE_STATE_CREATE_INFO_NV:
			FATAL_ERROR();

		case VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_SWIZZLE_STATE_CREATE_INFO_NV:
			FATAL_ERROR();

		case VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_W_SCALING_STATE_CREATE_INFO_NV:
			FATAL_ERROR();
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
			FATAL_ERROR();

		case VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_DEPTH_CLIP_STATE_CREATE_INFO_EXT:
			FATAL_ERROR();

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
			FATAL_ERROR();

		case VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_STREAM_CREATE_INFO_EXT:
			FATAL_ERROR();
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
			FATAL_ERROR();

		case VK_STRUCTURE_TYPE_PIPELINE_COVERAGE_REDUCTION_STATE_CREATE_INFO_NV:
			FATAL_ERROR();

		case VK_STRUCTURE_TYPE_PIPELINE_COVERAGE_TO_COLOR_STATE_CREATE_INFO_NV:
			FATAL_ERROR();

		case VK_STRUCTURE_TYPE_PIPELINE_SAMPLE_LOCATIONS_STATE_CREATE_INFO_EXT:
			FATAL_ERROR();
		}
		next = static_cast<const VkBaseInStructure*>(next)->pNext;
	}

	if (pMultisampleState->flags)
	{
		FATAL_ERROR();
	}

	auto sampleMask = 0xFFFFFFFFFFFFFFFF;
	if (pMultisampleState->pSampleMask)
	{
		if (pMultisampleState->rasterizationSamples == VK_SAMPLE_COUNT_64_BIT)
		{
			FATAL_ERROR();
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
		FATAL_ERROR();
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
			FATAL_ERROR();
		}
		next = static_cast<const VkBaseInStructure*>(next)->pNext;
	}

	if (pColorBlendState->flags)
	{
		FATAL_ERROR();
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
		FATAL_ERROR();
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
			
		default: FATAL_ERROR();
		}
	}

	return dynamicState;;
}

static std::tuple<int, ShaderFunction*> LoadShaderStage(CPJit* jit, const struct VkPipelineShaderStageCreateInfo& stage)
{
	assert(stage.sType == VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO);

	auto next = stage.pNext;
	while (next)
	{
		const auto type = static_cast<const VkBaseInStructure*>(next)->sType;
		switch (type)
		{
		case VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_REQUIRED_SUBGROUP_SIZE_CREATE_INFO_EXT:
			FATAL_ERROR();
		}
		next = static_cast<const VkBaseInStructure*>(next)->pNext;
	}

	if (stage.flags)
	{
		FATAL_ERROR();
	}

	const auto stageIndex = GetStageIndex(stage.stage);
	return std::make_tuple(stageIndex, new ShaderFunction(jit, UnwrapVulkan<ShaderModule>(stage.module), stageIndex, stage.pName, stage.pSpecializationInfo));
}

static SPIRV::SPIRVFunction* FindEntryPoint(const SPIRV::SPIRVModule* module, SPIRV::SPIRVExecutionModelKind stage, const char* name)
{
	for (auto i = 0u; i < module->getNumEntryPoints(stage); i++)
	{
		const auto entryPoint = module->getEntryPoint(stage, i);
		if (entryPoint->getName() == name)
		{
			return entryPoint;
		}
	}
	return nullptr;
}

ShaderFunction::ShaderFunction(CPJit* jit, ShaderModule* module, uint32_t stageIndex, const char* name, const VkSpecializationInfo* specializationInfo) :
	jit{jit}
{
	this->module = module->getModule();
	this->name = name;
	const auto entryPoint = FindEntryPoint(this->module, static_cast<SPIRV::SPIRVExecutionModelKind>(stageIndex), name);
	assert(entryPoint);

	if (entryPoint->getExecutionMode(SPIRV::SPIRVExecutionModeKind::ExecutionModeXfb))
	{
		FATAL_ERROR();
	}
	
	if (stageIndex == ExecutionModelTessellationControl || stageIndex == ExecutionModelTessellationEvaluation)
	{
		if (entryPoint->getExecutionMode(SPIRV::SPIRVExecutionModeKind::ExecutionModeSpacingEqual))
		{
			FATAL_ERROR();
		}

		if (entryPoint->getExecutionMode(SPIRV::SPIRVExecutionModeKind::ExecutionModeSpacingFractionalEven))
		{
			FATAL_ERROR();
		}

		if (entryPoint->getExecutionMode(SPIRV::SPIRVExecutionModeKind::ExecutionModeSpacingFractionalOdd))
		{
			FATAL_ERROR();
		}

		if (entryPoint->getExecutionMode(SPIRV::SPIRVExecutionModeKind::ExecutionModeVertexOrderCw))
		{
			FATAL_ERROR();
		}

		if (entryPoint->getExecutionMode(SPIRV::SPIRVExecutionModeKind::ExecutionModeVertexOrderCcw))
		{
			FATAL_ERROR();
		}

		if (entryPoint->getExecutionMode(SPIRV::SPIRVExecutionModeKind::ExecutionModePointMode))
		{
			FATAL_ERROR();
		}

		if (entryPoint->getExecutionMode(SPIRV::SPIRVExecutionModeKind::ExecutionModeTriangles))
		{
			FATAL_ERROR();
		}

		if (entryPoint->getExecutionMode(SPIRV::SPIRVExecutionModeKind::ExecutionModeQuads))
		{
			FATAL_ERROR();
		}

		if (entryPoint->getExecutionMode(SPIRV::SPIRVExecutionModeKind::ExecutionModeIsolines))
		{
			FATAL_ERROR();
		}

		const auto outputVertices = entryPoint->getExecutionMode(SPIRV::SPIRVExecutionModeKind::ExecutionModeOutputVertices);
		if (outputVertices)
		{
			FATAL_ERROR();
		}
	}

	if (stageIndex == ExecutionModelGeometry)
	{
		const auto invocations = entryPoint->getExecutionMode(SPIRV::SPIRVExecutionModeKind::ExecutionModeInvocations);
		if (invocations)
		{
			FATAL_ERROR();
		}

		if (entryPoint->getExecutionMode(SPIRV::SPIRVExecutionModeKind::ExecutionModeInputPoints))
		{
			FATAL_ERROR();
		}

		if (entryPoint->getExecutionMode(SPIRV::SPIRVExecutionModeKind::ExecutionModeInputLines))
		{
			FATAL_ERROR();
		}

		if (entryPoint->getExecutionMode(SPIRV::SPIRVExecutionModeKind::ExecutionModeInputLinesAdjacency))
		{
			FATAL_ERROR();
		}

		if (entryPoint->getExecutionMode(SPIRV::SPIRVExecutionModeKind::ExecutionModeTriangles))
		{
			FATAL_ERROR();
		}

		if (entryPoint->getExecutionMode(SPIRV::SPIRVExecutionModeKind::ExecutionModeInputTrianglesAdjacency))
		{
			FATAL_ERROR();
		}

		const auto outputVertices = entryPoint->getExecutionMode(SPIRV::SPIRVExecutionModeKind::ExecutionModeOutputVertices);
		if (outputVertices)
		{
			FATAL_ERROR();
		}

		if (entryPoint->getExecutionMode(SPIRV::SPIRVExecutionModeKind::ExecutionModeOutputPoints))
		{
			FATAL_ERROR();
		}

		if (entryPoint->getExecutionMode(SPIRV::SPIRVExecutionModeKind::ExecutionModeOutputLineStrip))
		{
			FATAL_ERROR();
		}

		if (entryPoint->getExecutionMode(SPIRV::SPIRVExecutionModeKind::ExecutionModeOutputTriangleStrip))
		{
			FATAL_ERROR();
		}
	}

	if (stageIndex == ExecutionModelFragment)
	{
		if (entryPoint->getExecutionMode(SPIRV::SPIRVExecutionModeKind::ExecutionModePixelCenterInteger))
		{
			FATAL_ERROR();
		}

		if (entryPoint->getExecutionMode(SPIRV::SPIRVExecutionModeKind::ExecutionModeOriginUpperLeft))
		{
			this->fragmentOriginUpper = true;
		}

		if (entryPoint->getExecutionMode(SPIRV::SPIRVExecutionModeKind::ExecutionModeOriginLowerLeft))
		{
			this->fragmentOriginUpper = false;
		}

		if (entryPoint->getExecutionMode(SPIRV::SPIRVExecutionModeKind::ExecutionModeEarlyFragmentTests))
		{
			FATAL_ERROR();
		}

		if (entryPoint->getExecutionMode(SPIRV::SPIRVExecutionModeKind::ExecutionModeDepthReplacing))
		{
			FATAL_ERROR();
		}

		if (entryPoint->getExecutionMode(SPIRV::SPIRVExecutionModeKind::ExecutionModeDepthGreater))
		{
			FATAL_ERROR();
		}

		if (entryPoint->getExecutionMode(SPIRV::SPIRVExecutionModeKind::ExecutionModeDepthLess))
		{
			FATAL_ERROR();
		}

		if (entryPoint->getExecutionMode(SPIRV::SPIRVExecutionModeKind::ExecutionModeDepthUnchanged))
		{
			FATAL_ERROR();
		}
	}

	if (stageIndex == ExecutionModelGLCompute || stageIndex == ExecutionModelKernel)
	{
		const auto localSize = entryPoint->getExecutionMode(SPIRV::SPIRVExecutionModeKind::ExecutionModeLocalSize);
		if (localSize)
		{
			const auto& literals = localSize->getLiterals();
			this->computeLocalSize = glm::ivec3{literals[0], literals[1], literals[2]};
		}

		const auto localSizeId = entryPoint->getExecutionMode(SPIRV::SPIRVExecutionModeKind::ExecutionModeLocalSizeId);
		if (localSizeId)
		{
			FATAL_ERROR();
		}
	}

	if (stageIndex == ExecutionModelKernel)
	{
		const auto localSizeHint = entryPoint->getExecutionMode(SPIRV::SPIRVExecutionModeKind::ExecutionModeLocalSizeHint);
		if (localSizeHint)
		{
			FATAL_ERROR();
		}

		const auto vecTypeHint = entryPoint->getExecutionMode(SPIRV::SPIRVExecutionModeKind::ExecutionModeContractionOff);
		if (vecTypeHint)
		{
			FATAL_ERROR();
		}

		if (entryPoint->getExecutionMode(SPIRV::SPIRVExecutionModeKind::ExecutionModeDepthUnchanged))
		{
			FATAL_ERROR();
		}

		if (entryPoint->getExecutionMode(SPIRV::SPIRVExecutionModeKind::ExecutionModeInitializer))
		{
			FATAL_ERROR();
		}

		if (entryPoint->getExecutionMode(SPIRV::SPIRVExecutionModeKind::ExecutionModeFinalizer))
		{
			FATAL_ERROR();
		}

		const auto subgroupSize = entryPoint->getExecutionMode(SPIRV::SPIRVExecutionModeKind::ExecutionModeSubgroupSize);
		if (subgroupSize)
		{
			FATAL_ERROR();
		}

		const auto subgroupsPerWorkgroup = entryPoint->getExecutionMode(SPIRV::SPIRVExecutionModeKind::ExecutionModeSubgroupsPerWorkgroup);
		if (subgroupsPerWorkgroup)
		{
			FATAL_ERROR();
		}

		const auto subgroupsPerWorkgroupId = entryPoint->getExecutionMode(SPIRV::SPIRVExecutionModeKind::ExecutionModeSubgroupsPerWorkgroupId);
		if (subgroupsPerWorkgroupId)
		{
			FATAL_ERROR();
		}

		const auto localSizeId = entryPoint->getExecutionMode(SPIRV::SPIRVExecutionModeKind::ExecutionModeLocalSizeId);
		if (localSizeId)
		{
			FATAL_ERROR();
		}

		const auto localSizeHintId = entryPoint->getExecutionMode(SPIRV::SPIRVExecutionModeKind::ExecutionModeLocalSizeHintId);
		if (localSizeHintId)
		{
			FATAL_ERROR();
		}
	}

	this->llvmModule = jit->CompileModule(this->module, static_cast<spv::ExecutionModel>(stageIndex), specializationInfo);
	this->entryPoint = jit->getFunctionPointer(llvmModule, name);
}

ShaderFunction::~ShaderFunction()
{
	jit->FreeModule(llvmModule);
}

Pipeline::~Pipeline() = default;

VkResult Pipeline::Create(Device* device, VkPipelineCache pipelineCache, const VkGraphicsPipelineCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkPipeline* pPipeline)
{
	assert(pCreateInfo->sType == VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO);

	if (pipelineCache != VK_NULL_HANDLE)
	{
		// TODO
	}

	auto pipeline = Allocate<Pipeline>(pAllocator, VK_SYSTEM_ALLOCATION_SCOPE_DEVICE);
	if (!pipeline)
	{
		return VK_ERROR_OUT_OF_HOST_MEMORY;
	}

	auto feedback = false;
	auto next = static_cast<const VkBaseInStructure*>(pCreateInfo->pNext);
	while (next)
	{
		const auto type = next->sType;
		switch (type)
		{
		case VK_STRUCTURE_TYPE_PIPELINE_COMPILER_CONTROL_CREATE_INFO_AMD:
			FATAL_ERROR();

		case VK_STRUCTURE_TYPE_PIPELINE_CREATION_FEEDBACK_CREATE_INFO_EXT:
			feedback = true;
			break;

		case VK_STRUCTURE_TYPE_PIPELINE_DISCARD_RECTANGLE_STATE_CREATE_INFO_EXT:
			FATAL_ERROR();

		case VK_STRUCTURE_TYPE_PIPELINE_REPRESENTATIVE_FRAGMENT_TEST_STATE_CREATE_INFO_NV:
			FATAL_ERROR();
		}
		next = next->pNext;
	}

	const auto startTime = feedback ? Platform::GetTimestamp() : 0;

	if (pCreateInfo->flags & VK_PIPELINE_CREATE_DISABLE_OPTIMIZATION_BIT)
	{
		// TODO
	}

	if (pCreateInfo->flags & VK_PIPELINE_CREATE_VIEW_INDEX_FROM_DEVICE_INDEX_BIT)
	{
		FATAL_ERROR();
	}

	if (pCreateInfo->flags & VK_PIPELINE_CREATE_DISPATCH_BASE)
	{
		FATAL_ERROR();
	}

	if (pCreateInfo->flags & VK_PIPELINE_CREATE_DEFER_COMPILE_BIT_NV)
	{
		FATAL_ERROR();
	}

	if (pCreateInfo->flags & VK_PIPELINE_CREATE_CAPTURE_STATISTICS_BIT_KHR)
	{
		FATAL_ERROR();
	}

	if (pCreateInfo->flags & VK_PIPELINE_CREATE_CAPTURE_INTERNAL_REPRESENTATIONS_BIT_KHR)
	{
		FATAL_ERROR();
	}

	auto shaderTimes = std::vector<uint64_t>(feedback ? pCreateInfo->stageCount : 0);

	for (auto i = 0u; i < pCreateInfo->stageCount; i++)
	{
		const auto stageStartTime = feedback ? Platform::GetTimestamp() : 0;
		
		int stageIndex;
		ShaderFunction* shaderFunction;
		std::tie(stageIndex, shaderFunction) = LoadShaderStage(device->getState()->jit, pCreateInfo->pStages[i]);
		pipeline->shaderStages[stageIndex] = std::unique_ptr<ShaderFunction>(shaderFunction);

		const auto stageEndTime = feedback ? Platform::GetTimestamp() : 0;
		if (feedback)
		{
			shaderTimes[i] = stageEndTime - stageStartTime;
		}
	}

	pipeline->vertexInputState = Parse(pCreateInfo->pVertexInputState);
	pipeline->inputAssemblyState = Parse(pCreateInfo->pInputAssemblyState);
	pipeline->tessellationState = Parse(pCreateInfo->pTessellationState);
	pipeline->viewportState = Parse(pCreateInfo->pViewportState);
	pipeline->rasterizationState = Parse(pCreateInfo->pRasterizationState);
	pipeline->multisampleState = Parse(pCreateInfo->pMultisampleState);
	pipeline->depthStencilState = Parse(pCreateInfo->pDepthStencilState);
	pipeline->colourBlendState = Parse(pCreateInfo->pColorBlendState);
	pipeline->dynamicState = Parse(pCreateInfo->pDynamicState);

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
					feedbackInfo->pPipelineStageCreationFeedbacks[i].duration = shaderTimes[i] * static_cast<uint64_t>(Platform::GetTimestampPeriod());
				}
				
				break;
			}
			next = next->pNext;
		}
	}

	WrapVulkan(pipeline, pPipeline);
	return VK_SUCCESS;
}

VkResult Pipeline::Create(Device* device, VkPipelineCache pipelineCache, const VkComputePipelineCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkPipeline* pPipeline)
{
	assert(pCreateInfo->sType == VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO);

	if (pipelineCache != VK_NULL_HANDLE)
	{
		// TODO
	}

	auto pipeline = Allocate<Pipeline>(pAllocator, VK_SYSTEM_ALLOCATION_SCOPE_DEVICE);
	if (!pipeline)
	{
		return VK_ERROR_OUT_OF_HOST_MEMORY;
	}

	auto next = pCreateInfo->pNext;
	while (next)
	{
		const auto type = static_cast<const VkBaseInStructure*>(next)->sType;
		switch (type)
		{
		case VK_STRUCTURE_TYPE_PIPELINE_COMPILER_CONTROL_CREATE_INFO_AMD:
			FATAL_ERROR();

		case VK_STRUCTURE_TYPE_PIPELINE_CREATION_FEEDBACK_CREATE_INFO_EXT:
			FATAL_ERROR();

		case VK_STRUCTURE_TYPE_PIPELINE_DISCARD_RECTANGLE_STATE_CREATE_INFO_EXT:
			FATAL_ERROR();

		case VK_STRUCTURE_TYPE_PIPELINE_REPRESENTATIVE_FRAGMENT_TEST_STATE_CREATE_INFO_NV:
			FATAL_ERROR();
		}
		next = static_cast<const VkBaseInStructure*>(next)->pNext;
	}

	if (pCreateInfo->flags)
	{
		FATAL_ERROR();
	}
	
	int stageIndex;
	ShaderFunction* shaderFunction;
	std::tie(stageIndex, shaderFunction) = LoadShaderStage(device->getState()->jit, pCreateInfo->stage);
	pipeline->shaderStages[stageIndex] = std::unique_ptr<ShaderFunction>(shaderFunction);

	if (pCreateInfo->basePipelineHandle)
	{
		FATAL_ERROR();
	}

	WrapVulkan(pipeline, pPipeline);
	return VK_SUCCESS;
}

VkResult Device::CreateGraphicsPipelines(VkPipelineCache pipelineCache, uint32_t createInfoCount, const VkGraphicsPipelineCreateInfo* pCreateInfos, const VkAllocationCallbacks* pAllocator, VkPipeline* pPipelines)
{
	for (auto i = 0u; i < createInfoCount; i++)
	{
		pPipelines[i] = nullptr;
	}
	
	for (auto i = 0u; i < createInfoCount; i++)
	{
		const auto result = Pipeline::Create(this, pipelineCache, &pCreateInfos[i], pAllocator, &pPipelines[i]);
		if (result != VK_SUCCESS)
		{
			return result;
		}
	}

	return VK_SUCCESS;
}

VkResult Device::CreateComputePipelines(VkPipelineCache pipelineCache, uint32_t createInfoCount, const VkComputePipelineCreateInfo* pCreateInfos, const VkAllocationCallbacks* pAllocator, VkPipeline* pPipelines)
{
	for (auto i = 0u; i < createInfoCount; i++)
	{
		pPipelines[i] = nullptr;
	}

	for (auto i = 0u; i < createInfoCount; i++)
	{
		const auto result = Pipeline::Create(this, pipelineCache, &pCreateInfos[i], pAllocator, &pPipelines[i]);
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
	for (auto i = 0; i < 6; i++)
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

	for (auto i = 0u, j = 0u; i < 6 && j < count; i++)
	{
		const auto shaderStage = pipeline->getShaderStage(i);
		if (shaderStage)
		{
			pProperties[j].stages = 1 << i;
			memset(pProperties[j].name, 0, VK_MAX_DESCRIPTION_SIZE);
			memset(pProperties[j].description, 0, VK_MAX_DESCRIPTION_SIZE);

			// TODO: Expand on this
			strcpy(pProperties[j].name, shaderName[i]);
			strcpy(pProperties[j].description, shaderName[i]);
			
			pProperties[j].subgroupSize = 1;
			j++;
		}
	}
	
	return result;
}

VkResult Device::GetPipelineExecutableStatistics(const VkPipelineExecutableInfoKHR* pExecutableInfo, uint32_t* pStatisticCount, VkPipelineExecutableStatisticKHR* pStatistics)
{
	FATAL_ERROR();
}

VkResult Device::GetPipelineExecutableInternalRepresentations(const VkPipelineExecutableInfoKHR* pExecutableInfo, uint32_t* pInternalRepresentationCount, VkPipelineExecutableInternalRepresentationKHR* pInternalRepresentations)
{
	FATAL_ERROR();
}
#endif