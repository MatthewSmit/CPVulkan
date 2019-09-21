#include "Pipeline.h"

#include "Device.h"
#include "ShaderModule.h"
#include "Util.h"

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

	auto next = pVertexInputState->pNext;
	while (next)
	{
		const auto type = static_cast<const VkBaseInStructure*>(next)->sType;
		switch (type)
		{
		case VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_DIVISOR_STATE_CREATE_INFO_EXT:
			FATAL_ERROR();
		}
		next = static_cast<const VkBaseInStructure*>(next)->pNext;
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

	auto next = pInputAssemblyState->pNext;
	while (next)
	{
		const auto type = static_cast<const VkBaseInStructure*>(next)->sType;
		switch (type)
		{
		}
		next = static_cast<const VkBaseInStructure*>(next)->pNext;
	}

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
		return TessellationState
		{
			0
		};
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
	assert(pViewportState->sType == VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO);

	auto next = pViewportState->pNext;
	while (next)
	{
		const auto type = static_cast<const VkBaseInStructure*>(next)->sType;
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
		next = static_cast<const VkBaseInStructure*>(next)->pNext;
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

	auto next = pRasterizationState->pNext;
	while (next)
	{
		const auto type = static_cast<const VkBaseInStructure*>(next)->sType;
		switch (type)
		{
		case VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_CONSERVATIVE_STATE_CREATE_INFO_EXT:
			FATAL_ERROR();

		case VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_DEPTH_CLIP_STATE_CREATE_INFO_EXT:
			FATAL_ERROR();

		case VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_LINE_STATE_CREATE_INFO_EXT:
			FATAL_ERROR();

		case VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_RASTERIZATION_ORDER_AMD:
			FATAL_ERROR();

		case VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_STREAM_CREATE_INFO_EXT:
			FATAL_ERROR();
		}
		next = static_cast<const VkBaseInStructure*>(next)->pNext;
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
	};
}

static MultisampleState Parse(const VkPipelineMultisampleStateCreateInfo* pMultisampleState)
{
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

	if (pMultisampleState->pSampleMask)
	{
		FATAL_ERROR();
	}

	return MultisampleState
	{
		pMultisampleState->rasterizationSamples,
		pMultisampleState->sampleShadingEnable != VK_FALSE,
		pMultisampleState->minSampleShading,
		pMultisampleState->alphaToCoverageEnable != VK_FALSE,
		pMultisampleState->alphaToOneEnable != VK_FALSE,
	};
}

static DepthStencilState Parse(const VkPipelineDepthStencilStateCreateInfo* pDepthStencilState)
{
	assert(pDepthStencilState->sType == VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO);

	auto next = pDepthStencilState->pNext;
	while (next)
	{
		const auto type = static_cast<const VkBaseInStructure*>(next)->sType;
		switch (type)
		{
		}
		next = static_cast<const VkBaseInStructure*>(next)->pNext;
	}

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

	auto next = pDynamicState->pNext;
	while (next)
	{
		const auto type = static_cast<const VkBaseInStructure*>(next)->sType;
		switch (type)
		{
		}
		next = static_cast<const VkBaseInStructure*>(next)->pNext;
	}

	if (pDynamicState->flags)
	{
		FATAL_ERROR();
	}

	return DynamicState
	{
		ArrayToVector(pDynamicState->dynamicStateCount, pDynamicState->pDynamicStates),
	};
}

static std::tuple<int, ShaderFunction*> LoadShaderStage(const struct VkPipelineShaderStageCreateInfo& stage)
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

	if (stage.pSpecializationInfo)
	{
		FATAL_ERROR();
	}

	const auto stageIndex = GetStageIndex(stage.stage);
	return std::make_tuple(stageIndex, new ShaderFunction(UnwrapVulkan<ShaderModule>(stage.module), stageIndex, stage.pName));
}

ShaderFunction::ShaderFunction(ShaderModule* module, uint32_t stageIndex, const char* name)
{
	this->module = module->getModule();
	this->name = name;
}

Pipeline::~Pipeline() = default;

VkResult Pipeline::Create(VkPipelineCache pipelineCache, const VkGraphicsPipelineCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkPipeline* pPipeline)
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

	for (auto i = 0u; i < pCreateInfo->stageCount; i++)
	{
		int stageIndex;
		ShaderFunction* shaderFunction;
		std::tie(stageIndex, shaderFunction) = LoadShaderStage(pCreateInfo->pStages[i]);
		pipeline->shaderStages[stageIndex] = std::unique_ptr<ShaderFunction>(shaderFunction);
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

	if (pCreateInfo->basePipelineHandle)
	{
		FATAL_ERROR();
	}

	WrapVulkan(pipeline, pPipeline);
	return VK_SUCCESS;
}

VkResult Pipeline::Create(VkPipelineCache pipelineCache, const VkComputePipelineCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkPipeline* pPipeline)
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
	std::tie(stageIndex, shaderFunction) = LoadShaderStage(pCreateInfo->stage);
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
		const auto result = Pipeline::Create(pipelineCache, &pCreateInfos[i], pAllocator, &pPipelines[i]);
		if (result != VK_SUCCESS)
		{
			FATAL_ERROR();
		}
	}

	return VK_SUCCESS;
}

VkResult Device::CreateComputePipelines(VkPipelineCache pipelineCache, uint32_t createInfoCount, const VkComputePipelineCreateInfo* pCreateInfos, const VkAllocationCallbacks* pAllocator, VkPipeline* pPipelines)
{
	for (auto i = 0u; i < createInfoCount; i++)
	{
		const auto result = Pipeline::Create(pipelineCache, &pCreateInfos[i], pAllocator, &pPipelines[i]);
		if (result != VK_SUCCESS)
		{
			FATAL_ERROR();
		}
	}

	return VK_SUCCESS;
}

void Device::DestroyPipeline(VkPipeline pipeline, const VkAllocationCallbacks* pAllocator)
{
	Free(UnwrapVulkan<Pipeline>(pipeline), pAllocator);
}