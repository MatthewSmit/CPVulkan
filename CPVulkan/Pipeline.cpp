#include "Pipeline.h"

#include "ShaderModule.h"

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
		const auto type = *static_cast<const VkStructureType*>(next);
		switch (type)
		{
		default:
			FATAL_ERROR();
		}
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
		const auto type = *static_cast<const VkStructureType*>(next);
		switch (type)
		{
		default:
			FATAL_ERROR();
		}
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
		const auto type = *static_cast<const VkStructureType*>(next);
		switch (type)
		{
		default:
			FATAL_ERROR();
		}
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
		const auto type = *static_cast<const VkStructureType*>(next);
		switch (type)
		{
		default:
			FATAL_ERROR();
		}
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
		const auto type = *static_cast<const VkStructureType*>(next);
		switch (type)
		{
		default:
			FATAL_ERROR();
		}
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
		const auto type = *static_cast<const VkStructureType*>(next);
		switch (type)
		{
		default:
			FATAL_ERROR();
		}
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
		const auto type = *static_cast<const VkStructureType*>(next);
		switch (type)
		{
		default:
			FATAL_ERROR();
		}
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
		const auto type = *static_cast<const VkStructureType*>(next);
		switch (type)
		{
		default:
			FATAL_ERROR();
		}
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
	assert(pDynamicState->sType == VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO);

	auto next = pDynamicState->pNext;
	while (next)
	{
		const auto type = *static_cast<const VkStructureType*>(next);
		switch (type)
		{
		default:
			FATAL_ERROR();
		}
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

	auto pipeline = Allocate<Pipeline>(pAllocator);

	auto next = pCreateInfo->pNext;
	while (next)
	{
		const auto type = *static_cast<const VkStructureType*>(next);
		switch (type)
		{
		default:
			FATAL_ERROR();
		}
	}

	if (pCreateInfo->flags)
	{
		FATAL_ERROR();
	}

	for (auto i = 0u; i < pCreateInfo->stageCount; i++)
	{
		const auto& stage = pCreateInfo->pStages[i];
		assert(stage.sType == VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO);

		next = stage.pNext;
		while (next)
		{
			const auto type = *static_cast<const VkStructureType*>(next);
			switch (type)
			{
			default:
				FATAL_ERROR();
			}
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
		pipeline->shaderStages[stageIndex] = std::make_unique<ShaderFunction>(reinterpret_cast<ShaderModule*>(stage.module), stageIndex, stage.pName);
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

	*pPipeline = reinterpret_cast<VkPipeline>(pipeline);
	return VK_SUCCESS;
}
