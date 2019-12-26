#include "Device.h"

#include "DescriptorSet.h"
#include "DeviceState.h"
#include "Extensions.h"
#include "GlslFunctions.h"
#include "Instance.h"
#include "Queue.h"
#include "Util.h"

#include <Jit.h>

#include <cassert>
#include <fstream>
#include <iostream>

Device::Device() :
	state{std::make_unique<DeviceState>()}
{
#if CV_DEBUG_LEVEL > 0
	static auto output = new std::ofstream("commandStream.txt");
	state->debugOutput = output;
#endif
	
	state->jit = new CPJit();
	AddGlslFunctions(state.get());
}

Device::~Device()
{
#if CV_DEBUG_LEVEL > 0
	try
	{
		state->debugOutput->flush();
	}
	catch (std::exception e)
	{
		// Ignore
	}
#endif

	state->imageFunctions.clear();
	delete state->jit;
}

void Device::OnDelete(const VkAllocationCallbacks* pAllocator)
{
	for (auto& pushDescriptorSet : state->graphicsPipelineState.pushDescriptorSets)
	{
		delete pushDescriptorSet;
	}

	for (auto& pushDescriptorSet : state->computePipelineState.pushDescriptorSets)
	{
		delete pushDescriptorSet;
	}

	for (auto& pushDescriptorSet : state->rayTracingPipelineState.pushDescriptorSets)
	{
		delete pushDescriptorSet;
	}
	
	for (auto& queue : queues)
	{
		const auto queuePtr = queue.release();
		Free(queuePtr, pAllocator);
	}
	queues.clear();
}

PFN_vkVoidFunction Device::GetProcAddress(const char* pName) const
{
	return enabledExtensions.getFunction(pName, true);
}

void Device::DestroyDevice(const VkAllocationCallbacks* pAllocator)
{
	Free(this, pAllocator);
}

void Device::GetDeviceQueue(uint32_t queueFamilyIndex, uint32_t queueIndex, VkQueue* pQueue)
{
	if (queueFamilyIndex != 0)
	{
		FATAL_ERROR();
	}

	WrapVulkan(queues.at(queueIndex).get(), pQueue);
}

VkResult Device::WaitIdle()
{
	// TODO:
	return VK_SUCCESS;
}

VkResult Device::AllocateMemory(const VkMemoryAllocateInfo* pAllocateInfo, const VkAllocationCallbacks* pAllocator, VkDeviceMemory* pMemory)
{
	assert(pAllocateInfo->sType == VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO);
	
	auto next = reinterpret_cast<const VkBaseInStructure*>(pAllocateInfo->pNext);
	while (next)
	{
		const auto type = next->sType;
		switch (type)
		{
		case VK_STRUCTURE_TYPE_DEDICATED_ALLOCATION_MEMORY_ALLOCATE_INFO_NV:
			TODO_ERROR();

		case VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO:
			break;

		case VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO_NV:
			TODO_ERROR();

		case VK_STRUCTURE_TYPE_EXPORT_MEMORY_WIN32_HANDLE_INFO_KHR:
			TODO_ERROR();

		case VK_STRUCTURE_TYPE_EXPORT_MEMORY_WIN32_HANDLE_INFO_NV:
			TODO_ERROR();

		case VK_STRUCTURE_TYPE_IMPORT_ANDROID_HARDWARE_BUFFER_INFO_ANDROID:
			TODO_ERROR();

		case VK_STRUCTURE_TYPE_IMPORT_MEMORY_FD_INFO_KHR:
			TODO_ERROR();

		case VK_STRUCTURE_TYPE_IMPORT_MEMORY_HOST_POINTER_INFO_EXT:
			TODO_ERROR();

		case VK_STRUCTURE_TYPE_IMPORT_MEMORY_WIN32_HANDLE_INFO_KHR:
			TODO_ERROR();

		case VK_STRUCTURE_TYPE_IMPORT_MEMORY_WIN32_HANDLE_INFO_NV:
			TODO_ERROR();

		case VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO:
			TODO_ERROR();

		case VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO:
			{
				const auto allocateInfo = reinterpret_cast<const VkMemoryDedicatedAllocateInfo*>(next);
				(void)allocateInfo;
				break;
			}
			
		case VK_STRUCTURE_TYPE_MEMORY_PRIORITY_ALLOCATE_INFO_EXT:
			TODO_ERROR();
		}
		next = next->pNext;
	}

	if (pAllocateInfo->memoryTypeIndex != 0)
	{
		TODO_ERROR();
	}

	const auto memory = AllocateSized(pAllocator, pAllocateInfo->allocationSize, VK_SYSTEM_ALLOCATION_SCOPE_DEVICE);
	if (!memory)
	{
		return VK_ERROR_OUT_OF_HOST_MEMORY;
	}
	
	WrapVulkan(memory, pMemory);
	return VK_SUCCESS;
}

void Device::FreeMemory(VkDeviceMemory memory, const VkAllocationCallbacks* pAllocator)
{
	if (memory)
	{
		FreeSized(UnwrapVulkan<DeviceMemory>(memory), pAllocator);
	}
}

VkResult Device::MapMemory(VkDeviceMemory memory, VkDeviceSize offset, VkDeviceSize size, VkMemoryMapFlags flags, void** ppData)
{
	assert(flags == 0);
	// TODO: Bounds checks
	*ppData = UnwrapVulkan<DeviceMemory>(memory)->Data + offset;
	return VK_SUCCESS;
}

void Device::UnmapMemory(VkDeviceMemory)
{
}

VkResult Device::FlushMappedMemoryRanges(uint32_t, const VkMappedMemoryRange*)
{
	return VK_SUCCESS;
}

VkResult Device::InvalidateMappedMemoryRanges(uint32_t, const VkMappedMemoryRange*)
{
	return VK_SUCCESS;
}

void Device::GetDeviceMemoryCommitment(VkDeviceMemory memory, VkDeviceSize* pCommittedMemoryInBytes) const
{
	*pCommittedMemoryInBytes = UnwrapVulkan<DeviceMemory>(memory)->Size;
}

VkResult Device::Create(const Instance* instance, const VkDeviceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDevice* pDevice)
{
	assert(pCreateInfo->sType == VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO);
	assert(pCreateInfo->flags == 0);

	if (pCreateInfo->pEnabledFeatures != nullptr)
	{
		if ((pCreateInfo->pEnabledFeatures->sparseBinding ||
			pCreateInfo->pEnabledFeatures->sparseResidencyBuffer ||
			pCreateInfo->pEnabledFeatures->sparseResidencyImage2D ||
			pCreateInfo->pEnabledFeatures->sparseResidencyImage3D ||
			pCreateInfo->pEnabledFeatures->sparseResidency2Samples ||
			pCreateInfo->pEnabledFeatures->sparseResidency4Samples ||
			pCreateInfo->pEnabledFeatures->sparseResidency8Samples ||
			pCreateInfo->pEnabledFeatures->sparseResidency16Samples ||
			pCreateInfo->pEnabledFeatures->sparseResidencyAliased) && (!Platform::SupportsSparse() || !SPARSE_BINDING))
		{
			return VK_ERROR_FEATURE_NOT_PRESENT;
		}

		if (pCreateInfo->pEnabledFeatures->robustBufferAccess && !ROBUST_BUFFER_ACCESS) { return VK_ERROR_FEATURE_NOT_PRESENT; }
		if (pCreateInfo->pEnabledFeatures->fullDrawIndexUint32 && !FULL_DRAW_INDEX_UINT32) { return VK_ERROR_FEATURE_NOT_PRESENT; }
		if (pCreateInfo->pEnabledFeatures->imageCubeArray && !IMAGE_CUBE_ARRAY) { return VK_ERROR_FEATURE_NOT_PRESENT; }
		if (pCreateInfo->pEnabledFeatures->independentBlend && !INDEPENDENT_BLEND) { return VK_ERROR_FEATURE_NOT_PRESENT; }
		if (pCreateInfo->pEnabledFeatures->geometryShader && !GEOMETRY_SHADER) { return VK_ERROR_FEATURE_NOT_PRESENT; }
		if (pCreateInfo->pEnabledFeatures->tessellationShader && !TESSELLATION_SHADER) { return VK_ERROR_FEATURE_NOT_PRESENT; }
		if (pCreateInfo->pEnabledFeatures->sampleRateShading && !SAMPLE_RATE_SHADING) { return VK_ERROR_FEATURE_NOT_PRESENT; }
		if (pCreateInfo->pEnabledFeatures->dualSrcBlend && !DUAL_SOURCE_BLEND) { return VK_ERROR_FEATURE_NOT_PRESENT; }
		if (pCreateInfo->pEnabledFeatures->logicOp && !LOGIC_OPERATION) { return VK_ERROR_FEATURE_NOT_PRESENT; }
		if (pCreateInfo->pEnabledFeatures->multiDrawIndirect && !MULTI_DRAW_INDIRECT) { return VK_ERROR_FEATURE_NOT_PRESENT; }
		if (pCreateInfo->pEnabledFeatures->drawIndirectFirstInstance && !DRAW_INDIRECT_FIRST_INSTANCE) { return VK_ERROR_FEATURE_NOT_PRESENT; }
		if (pCreateInfo->pEnabledFeatures->depthClamp && !DEPTH_CLAMP) { return VK_ERROR_FEATURE_NOT_PRESENT; }
		if (pCreateInfo->pEnabledFeatures->depthBiasClamp && !DEPTH_BIAS_CLAMP) { return VK_ERROR_FEATURE_NOT_PRESENT; }
		if (pCreateInfo->pEnabledFeatures->fillModeNonSolid && !FILL_MODE_NON_SOLID) { return VK_ERROR_FEATURE_NOT_PRESENT; }
		if (pCreateInfo->pEnabledFeatures->depthBounds && !DEPTH_BOUNDS) { return VK_ERROR_FEATURE_NOT_PRESENT; }
		if (pCreateInfo->pEnabledFeatures->wideLines && !WIDE_LINES) { return VK_ERROR_FEATURE_NOT_PRESENT; }
		if (pCreateInfo->pEnabledFeatures->largePoints && !LARGE_POINTS) { return VK_ERROR_FEATURE_NOT_PRESENT; }
		if (pCreateInfo->pEnabledFeatures->alphaToOne && !ALPHA_TO_ONE) { return VK_ERROR_FEATURE_NOT_PRESENT; }
		if (pCreateInfo->pEnabledFeatures->multiViewport && !MULTI_VIEWPORT) { return VK_ERROR_FEATURE_NOT_PRESENT; }
		if (pCreateInfo->pEnabledFeatures->samplerAnisotropy && !SAMPLER_ANISOTROPY) { return VK_ERROR_FEATURE_NOT_PRESENT; }
		if (pCreateInfo->pEnabledFeatures->textureCompressionETC2 && !TEXTURE_COMPRESSION_ETC2) { return VK_ERROR_FEATURE_NOT_PRESENT; }
		if (pCreateInfo->pEnabledFeatures->textureCompressionASTC_LDR && !TEXTURE_COMPRESSION_ASTC_LDR) { return VK_ERROR_FEATURE_NOT_PRESENT; }
		if (pCreateInfo->pEnabledFeatures->textureCompressionBC && !TEXTURE_COMPRESSION_BC) { return VK_ERROR_FEATURE_NOT_PRESENT; }
		if (pCreateInfo->pEnabledFeatures->occlusionQueryPrecise && !OCCLUSION_QUERY_PRECISE) { return VK_ERROR_FEATURE_NOT_PRESENT; }
		if (pCreateInfo->pEnabledFeatures->pipelineStatisticsQuery && !PIPELINE_STATISTICS_QUERY) { return VK_ERROR_FEATURE_NOT_PRESENT; }
		if (pCreateInfo->pEnabledFeatures->vertexPipelineStoresAndAtomics && !VERTEX_PIPELINE_STORES_AND_ATOMICS) { return VK_ERROR_FEATURE_NOT_PRESENT; }
		if (pCreateInfo->pEnabledFeatures->fragmentStoresAndAtomics && !FRAGMENT_STORES_AND_ATOMICS) { return VK_ERROR_FEATURE_NOT_PRESENT; }
		if (pCreateInfo->pEnabledFeatures->shaderTessellationAndGeometryPointSize && !SHADER_TESSELLATION_AND_GEOMETRY_POINT_SIZE) { return VK_ERROR_FEATURE_NOT_PRESENT; }
		if (pCreateInfo->pEnabledFeatures->shaderImageGatherExtended && !SHADER_IMAGE_GATHER_EXTENDED) { return VK_ERROR_FEATURE_NOT_PRESENT; }
		if (pCreateInfo->pEnabledFeatures->shaderStorageImageExtendedFormats && !SHADER_STORAGE_IMAGE_EXTENDED_FORMATS) { return VK_ERROR_FEATURE_NOT_PRESENT; }
		if (pCreateInfo->pEnabledFeatures->shaderStorageImageMultisample && !SHADER_STORAGE_IMAGE_MULTISAMPLE) { return VK_ERROR_FEATURE_NOT_PRESENT; }
		if (pCreateInfo->pEnabledFeatures->shaderStorageImageReadWithoutFormat && !SHADER_STORAGE_IMAGE_READ_WITHOUT_FORMAT) { return VK_ERROR_FEATURE_NOT_PRESENT; }
		if (pCreateInfo->pEnabledFeatures->shaderStorageImageWriteWithoutFormat && !SHADER_STORAGE_IMAGE_WRITE_WITHOUT_FORMAT) { return VK_ERROR_FEATURE_NOT_PRESENT; }
		if (pCreateInfo->pEnabledFeatures->shaderUniformBufferArrayDynamicIndexing && !SHADER_UNIFORM_BUFFER_ARRAY_DYNAMIC_INDEXING) { return VK_ERROR_FEATURE_NOT_PRESENT; }
		if (pCreateInfo->pEnabledFeatures->shaderSampledImageArrayDynamicIndexing && !SHADER_SAMPLED_IMAGE_ARRAY_DYNAMIC_INDEXING) { return VK_ERROR_FEATURE_NOT_PRESENT; }
		if (pCreateInfo->pEnabledFeatures->shaderStorageBufferArrayDynamicIndexing && !SHADER_STORAGE_BUFFER_ARRAY_DYNAMIC_INDEXING) { return VK_ERROR_FEATURE_NOT_PRESENT; }
		if (pCreateInfo->pEnabledFeatures->shaderStorageImageArrayDynamicIndexing && !SHADER_STORAGE_IMAGE_ARRAY_DYNAMIC_INDEXING) { return VK_ERROR_FEATURE_NOT_PRESENT; }
		if (pCreateInfo->pEnabledFeatures->shaderClipDistance && !SHADER_CLIP_DISTANCE) { return VK_ERROR_FEATURE_NOT_PRESENT; }
		if (pCreateInfo->pEnabledFeatures->shaderCullDistance && !SHADER_CULL_DISTANCE) { return VK_ERROR_FEATURE_NOT_PRESENT; }
		if (pCreateInfo->pEnabledFeatures->shaderFloat64 && !SHADER_FLOAT64) { return VK_ERROR_FEATURE_NOT_PRESENT; }
		if (pCreateInfo->pEnabledFeatures->shaderInt64 && !SHADER_INT64) { return VK_ERROR_FEATURE_NOT_PRESENT; }
		if (pCreateInfo->pEnabledFeatures->shaderInt16 && !SHADER_INT16) { return VK_ERROR_FEATURE_NOT_PRESENT; }
		if (pCreateInfo->pEnabledFeatures->shaderResourceResidency && !SHADER_RESOURCE_RESIDENCY) { return VK_ERROR_FEATURE_NOT_PRESENT; }
		if (pCreateInfo->pEnabledFeatures->shaderResourceMinLod && !SHADER_RESOURCE_MIN_LOD) { return VK_ERROR_FEATURE_NOT_PRESENT; }
		if (pCreateInfo->pEnabledFeatures->variableMultisampleRate && !VARIABLE_MULTISAMPLE_RATE) { return VK_ERROR_FEATURE_NOT_PRESENT; }
		if (pCreateInfo->pEnabledFeatures->inheritedQueries && !INHERITED_QUERIES) { return VK_ERROR_FEATURE_NOT_PRESENT; }
	}

	auto next = static_cast<const VkBaseInStructure*>(pCreateInfo->pNext);
	while (next)
	{
		const auto type = next->sType;
		switch (type)
		{
		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_16BIT_STORAGE_FEATURES:
			{
				const auto features = reinterpret_cast<const VkPhysicalDevice16BitStorageFeatures*>(next);
				// TODO
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_8BIT_STORAGE_FEATURES_KHR:
			{
				const auto features = reinterpret_cast<const VkPhysicalDevice8BitStorageFeaturesKHR*>(next);
				// TODO
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ASTC_DECODE_FEATURES_EXT:
			{
				const auto features = reinterpret_cast<const VkPhysicalDeviceASTCDecodeFeaturesEXT*>(next);
				// TODO
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BLEND_OPERATION_ADVANCED_FEATURES_EXT:
			{
				const auto features = reinterpret_cast<const VkPhysicalDeviceBlendOperationAdvancedFeaturesEXT*>(next);
				// TODO
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES_EXT:
			{
				const auto features = reinterpret_cast<const VkPhysicalDeviceBufferDeviceAddressFeaturesEXT*>(next);
				if (features->bufferDeviceAddressCaptureReplay)
				{
					return VK_ERROR_FEATURE_NOT_PRESENT;
				}
				// TODO
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COHERENT_MEMORY_FEATURES_AMD:
			{
				const auto features = reinterpret_cast<const VkPhysicalDeviceCoherentMemoryFeaturesAMD*>(next);
				// TODO
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COMPUTE_SHADER_DERIVATIVES_FEATURES_NV:
			{
				const auto features = reinterpret_cast<const VkPhysicalDeviceComputeShaderDerivativesFeaturesNV*>(next);
				// TODO
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CONDITIONAL_RENDERING_FEATURES_EXT:
			{
				const auto features = reinterpret_cast<const VkPhysicalDeviceConditionalRenderingFeaturesEXT*>(next);
				// TODO
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COOPERATIVE_MATRIX_FEATURES_NV:
			{
				const auto features = reinterpret_cast<const VkPhysicalDeviceCooperativeMatrixFeaturesNV*>(next);
				// TODO
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CORNER_SAMPLED_IMAGE_FEATURES_NV:
			{
				const auto features = reinterpret_cast<const VkPhysicalDeviceCornerSampledImageFeaturesNV*>(next);
				// TODO
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COVERAGE_REDUCTION_MODE_FEATURES_NV:
			{
				const auto features = reinterpret_cast<const VkPhysicalDeviceCoverageReductionModeFeaturesNV*>(next);
				// TODO
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEDICATED_ALLOCATION_IMAGE_ALIASING_FEATURES_NV:
			{
				const auto features = reinterpret_cast<const VkPhysicalDeviceDedicatedAllocationImageAliasingFeaturesNV*>(next);
				// TODO
				break;
			}
			
		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEPTH_CLIP_ENABLE_FEATURES_EXT:
			{
				const auto features = reinterpret_cast<const VkPhysicalDeviceDepthClipEnableFeaturesEXT*>(next);
				// TODO
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT:
			{
				const auto features = reinterpret_cast<const VkPhysicalDeviceDescriptorIndexingFeaturesEXT*>(next);
				// TODO
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXCLUSIVE_SCISSOR_FEATURES_NV:
			{
				const auto features = reinterpret_cast<const VkPhysicalDeviceExclusiveScissorFeaturesNV*>(next);
				// TODO
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_DENSITY_MAP_FEATURES_EXT:
			{
				const auto features = reinterpret_cast<const VkPhysicalDeviceFragmentDensityMapFeaturesEXT*>(next);
				// TODO
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADER_BARYCENTRIC_FEATURES_NV:
			{
				const auto features = reinterpret_cast<const VkPhysicalDeviceFragmentShaderBarycentricFeaturesNV*>(next);
				// TODO
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADER_INTERLOCK_FEATURES_EXT:
			{
				const auto features = reinterpret_cast<const VkPhysicalDeviceFragmentShaderInterlockFeaturesEXT*>(next);
				// TODO
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_HOST_QUERY_RESET_FEATURES_EXT:
			{
				const auto features = reinterpret_cast<const VkPhysicalDeviceHostQueryResetFeaturesEXT*>(next);
				// TODO
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGELESS_FRAMEBUFFER_FEATURES_KHR:
			{
				const auto features = reinterpret_cast<const VkPhysicalDeviceImagelessFramebufferFeaturesKHR*>(next);
				// TODO
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_INDEX_TYPE_UINT8_FEATURES_EXT:
			{
				const auto features = reinterpret_cast<const VkPhysicalDeviceIndexTypeUint8FeaturesEXT*>(next);
				// TODO
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_INLINE_UNIFORM_BLOCK_FEATURES_EXT:
			{
				const auto features = reinterpret_cast<const VkPhysicalDeviceInlineUniformBlockFeaturesEXT*>(next);
				// TODO
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_LINE_RASTERIZATION_FEATURES_EXT:
			{
				const auto features = reinterpret_cast<const VkPhysicalDeviceLineRasterizationFeaturesEXT*>(next);
				// TODO
				break;
			}
			
		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PRIORITY_FEATURES_EXT:
			{
				const auto features = reinterpret_cast<const VkPhysicalDeviceMemoryPriorityFeaturesEXT*>(next);
				if (features->memoryPriority)
				{
					return VK_ERROR_FEATURE_NOT_PRESENT;
				}
				// TODO
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_NV:
			{
				const auto features = reinterpret_cast<const VkPhysicalDeviceMeshShaderFeaturesNV*>(next);
				// TODO
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_FEATURES:
			{
				const auto features = reinterpret_cast<const VkPhysicalDeviceMultiviewFeatures*>(next);
				// TODO
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PIPELINE_EXECUTABLE_PROPERTIES_FEATURES_KHR:
			{
				const auto features = reinterpret_cast<const VkPhysicalDevicePipelineExecutablePropertiesFeaturesKHR*>(next);
				// TODO
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROTECTED_MEMORY_FEATURES:
			{
				const auto features = reinterpret_cast<const VkPhysicalDeviceProtectedMemoryFeatures*>(next);
				// TODO
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_REPRESENTATIVE_FRAGMENT_TEST_FEATURES_NV:
			{
				const auto features = reinterpret_cast<const VkPhysicalDeviceRepresentativeFragmentTestFeaturesNV*>(next);
				// TODO
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SAMPLER_YCBCR_CONVERSION_FEATURES:
			{
				const auto features = reinterpret_cast<const VkPhysicalDeviceSamplerYcbcrConversionFeatures*>(next);
				// TODO
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SCALAR_BLOCK_LAYOUT_FEATURES_EXT:
			{
				const auto features = reinterpret_cast<const VkPhysicalDeviceScalarBlockLayoutFeaturesEXT*>(next);
				// TODO
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ATOMIC_INT64_FEATURES_KHR:
			{
				const auto features = reinterpret_cast<const VkPhysicalDeviceShaderAtomicInt64FeaturesKHR*>(next);
				// TODO
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DEMOTE_TO_HELPER_INVOCATION_FEATURES_EXT:
			{
				const auto features = reinterpret_cast<const VkPhysicalDeviceShaderDemoteToHelperInvocationFeaturesEXT*>(next);
				// TODO
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DRAW_PARAMETERS_FEATURES:
			{
				const auto features = reinterpret_cast<const VkPhysicalDeviceShaderDrawParametersFeatures*>(next);
				// TODO
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_FLOAT16_INT8_FEATURES_KHR:
			{
				const auto features = reinterpret_cast<const VkPhysicalDeviceShaderFloat16Int8FeaturesKHR*>(next);
				// TODO
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_IMAGE_FOOTPRINT_FEATURES_NV:
			{
				const auto features = reinterpret_cast<const VkPhysicalDeviceShaderImageFootprintFeaturesNV*>(next);
				// TODO
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_INTEGER_FUNCTIONS_2_FEATURES_INTEL:
			{
				const auto features = reinterpret_cast<const VkPhysicalDeviceShaderIntegerFunctions2FeaturesINTEL*>(next);
				// TODO
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_SM_BUILTINS_FEATURES_NV:
			{
				const auto features = reinterpret_cast<const VkPhysicalDeviceShaderSMBuiltinsFeaturesNV*>(next);
				// TODO
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADING_RATE_IMAGE_FEATURES_NV:
			{
				const auto features = reinterpret_cast<const VkPhysicalDeviceShadingRateImageFeaturesNV*>(next);
				// TODO
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_SIZE_CONTROL_FEATURES_EXT:
			{
				const auto features = reinterpret_cast<const VkPhysicalDeviceSubgroupSizeControlFeaturesEXT*>(next);
				// TODO
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TEXEL_BUFFER_ALIGNMENT_FEATURES_EXT:
			{
				const auto features = reinterpret_cast<const VkPhysicalDeviceTexelBufferAlignmentFeaturesEXT*>(next);
				// TODO
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TEXTURE_COMPRESSION_ASTC_HDR_FEATURES_EXT:
			{
				const auto features = reinterpret_cast<const VkPhysicalDeviceTextureCompressionASTCHDRFeaturesEXT*>(next);
				// TODO
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TRANSFORM_FEEDBACK_FEATURES_EXT:
			{
				const auto features = reinterpret_cast<const VkPhysicalDeviceTransformFeedbackFeaturesEXT*>(next);
				// TODO
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_UNIFORM_BUFFER_STANDARD_LAYOUT_FEATURES_KHR:
			{
				const auto features = reinterpret_cast<const VkPhysicalDeviceUniformBufferStandardLayoutFeaturesKHR*>(next);
				// TODO
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VARIABLE_POINTERS_FEATURES:
			{
				const auto features = reinterpret_cast<const VkPhysicalDeviceVariablePointersFeatures*>(next);
				// TODO
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VERTEX_ATTRIBUTE_DIVISOR_FEATURES_EXT:
			{
				const auto features = reinterpret_cast<const VkPhysicalDeviceVertexAttributeDivisorFeaturesEXT*>(next);
				// TODO
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_MEMORY_MODEL_FEATURES_KHR:
			{
				const auto features = reinterpret_cast<const VkPhysicalDeviceVulkanMemoryModelFeaturesKHR*>(next);
				// TODO
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_YCBCR_IMAGE_ARRAYS_FEATURES_EXT:
			{
				const auto features = reinterpret_cast<const VkPhysicalDeviceSamplerYcbcrConversionFeatures*>(next);
				// TODO
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_CLOCK_FEATURES_KHR:
			{
				const auto features = reinterpret_cast<const VkPhysicalDeviceShaderClockFeaturesKHR*>(next);
				// TODO
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_SUBGROUP_EXTENDED_TYPES_FEATURES_KHR:
			{
				const auto features = reinterpret_cast<const VkPhysicalDeviceShaderSubgroupExtendedTypesFeaturesKHR*>(next);
				// TODO
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES_KHR:
			{
				const auto features = reinterpret_cast<const VkPhysicalDeviceTimelineSemaphoreFeaturesKHR*>(next);
				// TODO
				break;
			}

		default:
			break;
		}
		next = next->pNext;
	}

	auto device = Allocate<Device>(pAllocator, VK_SYSTEM_ALLOCATION_SCOPE_DEVICE);
	if (!device)
	{
		return VK_ERROR_OUT_OF_HOST_MEMORY;
	}

	for (auto i = 0u; i < pCreateInfo->queueCreateInfoCount; i++)
	{
		const auto queue = Queue::Create(&pCreateInfo->pQueueCreateInfos[i], pAllocator);
		if (!queue)
		{
			Free(device, pAllocator);
			return VK_ERROR_OUT_OF_HOST_MEMORY;
		}
		device->queues.push_back(std::unique_ptr<Queue>{queue});
	}

	std::vector<const char*> enabledExtensions{};
	if (pCreateInfo->enabledExtensionCount)
	{
		for (auto i = 0u; i < pCreateInfo->enabledExtensionCount; i++)
		{
			enabledExtensions.push_back(pCreateInfo->ppEnabledExtensionNames[i]);
		}
	}

	const auto result = GetInitialExtensions().MakeDeviceCopy(instance->getEnabledExtensions(), enabledExtensions, &device->enabledExtensions);

	if (result != VK_SUCCESS)
	{
		Free(device, pAllocator);
		return result;
	}

	if (pCreateInfo->pEnabledFeatures)
	{
		// TODO
	}

	WrapVulkan(device, pDevice);
	return VK_SUCCESS;
}

void Device::GetDeviceQueue2(const VkDeviceQueueInfo2* pQueueInfo, VkQueue* pQueue)
{
	assert(pQueueInfo->sType == VK_STRUCTURE_TYPE_DEVICE_QUEUE_INFO_2);
	assert(pQueueInfo->flags == 1);
	
	if (pQueueInfo->queueFamilyIndex != 0)
	{
		TODO_ERROR();
	}

	const auto queue = queues.at(pQueueInfo->queueIndex).get();
	if (queue->getFlags() != pQueueInfo->flags)
	{
		*pQueue = VK_NULL_HANDLE;
	}
	else
	{
		WrapVulkan(queue, pQueue);
	}
}

VkResult Device::CreateSamplerYcbcrConversion(const VkSamplerYcbcrConversionCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSamplerYcbcrConversion* pYcbcrConversion)
{
	TODO_ERROR();
}

void Device::DestroySamplerYcbcrConversion(VkSamplerYcbcrConversion ycbcrConversion, const VkAllocationCallbacks* pAllocator)
{
	TODO_ERROR();
}

VkResult Device::CreateDescriptorUpdateTemplate(const VkDescriptorUpdateTemplateCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDescriptorUpdateTemplate* pDescriptorUpdateTemplate)
{
	TODO_ERROR();
}

void Device::DestroyDescriptorUpdateTemplate(VkDescriptorUpdateTemplate descriptorUpdateTemplate, const VkAllocationCallbacks* pAllocator)
{
	TODO_ERROR();
}

void Device::UpdateDescriptorSetWithTemplate(VkDescriptorSet descriptorSet, VkDescriptorUpdateTemplate descriptorUpdateTemplate, const void* pData)
{
	TODO_ERROR();
}

void Device::GetDeviceGroupPeerMemoryFeatures(uint32_t heapIndex, uint32_t localDeviceIndex, uint32_t remoteDeviceIndex, VkPeerMemoryFeatureFlags* pPeerMemoryFeatures)
{
	TODO_ERROR();
}

VkResult Device::GetDeviceGroupPresentCapabilities(VkDeviceGroupPresentCapabilitiesKHR* pDeviceGroupPresentCapabilities)
{
	assert(pDeviceGroupPresentCapabilities->sType == VK_STRUCTURE_TYPE_DEVICE_GROUP_PRESENT_CAPABILITIES_KHR);

	for (auto i = 0; i < VK_MAX_DEVICE_GROUP_SIZE; i++)
	{
		pDeviceGroupPresentCapabilities->presentMask[i] = 0;
	}
	pDeviceGroupPresentCapabilities->modes = VK_DEVICE_GROUP_PRESENT_MODE_LOCAL_BIT_KHR;

	return VK_SUCCESS;
}

VkResult Device::GetDeviceGroupSurfacePresentModes(VkSurfaceKHR surface, VkDeviceGroupPresentModeFlagsKHR* pModes)
{
	TODO_ERROR();
}

VkResult Device::CreateSharedSwapchains(uint32_t swapchainCount, const VkSwapchainCreateInfoKHR* pCreateInfos, const VkAllocationCallbacks* pAllocator, VkSwapchainKHR* pSwapchains)
{
	TODO_ERROR();
}

VkResult Device::GetMemoryFd(const VkMemoryGetFdInfoKHR* pGetFdInfo, int* pFd)
{
	TODO_ERROR();
}

VkResult Device::GetMemoryFdProperties(VkExternalMemoryHandleTypeFlagBits handleType, int fd, VkMemoryFdPropertiesKHR* pMemoryFdProperties)
{
	TODO_ERROR();
}

VkResult Device::GetSwapchainStatus(VkSwapchainKHR swapchain)
{
	TODO_ERROR();
}

VkResult Device::DebugMarkerSetObjectTag(const VkDebugMarkerObjectTagInfoEXT* pTagInfo)
{
	TODO_ERROR();
}

VkResult Device::DebugMarkerSetObjectName(const VkDebugMarkerObjectNameInfoEXT* pNameInfo)
{
	TODO_ERROR();
}

uint32_t Device::GetImageViewHandleNVX(const VkImageViewHandleInfoNVX* pInfo)
{
	TODO_ERROR();
}

VkResult Device::GetShaderInfoAMD(VkPipeline pipeline, VkShaderStageFlagBits shaderStage, VkShaderInfoTypeAMD infoType, size_t* pInfoSize, void* pInfo)
{
	TODO_ERROR();
}

VkResult Device::CreateIndirectCommandsLayoutNVX(const VkIndirectCommandsLayoutCreateInfoNVX* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkIndirectCommandsLayoutNVX* pIndirectCommandsLayout)
{
	TODO_ERROR();
}

void Device::DestroyIndirectCommandsLayoutNVX(VkIndirectCommandsLayoutNVX indirectCommandsLayout, const VkAllocationCallbacks* pAllocator)
{
	TODO_ERROR();
}

VkResult Device::CreateObjectTableNVX(const VkObjectTableCreateInfoNVX* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkObjectTableNVX* pObjectTable)
{
	TODO_ERROR();
}

void Device::DestroyObjectTableNVX(VkObjectTableNVX objectTable, const VkAllocationCallbacks* pAllocator)
{
	TODO_ERROR();
}

VkResult Device::RegisterObjectsNVX(VkObjectTableNVX objectTable, uint32_t objectCount, const VkObjectTableEntryNVX* const* ppObjectTableEntries, const uint32_t* pObjectIndices)
{
	TODO_ERROR();
}

VkResult Device::UnregisterObjectsNVX(VkObjectTableNVX objectTable, uint32_t objectCount, const VkObjectEntryTypeNVX* pObjectEntryTypes, const uint32_t* pObjectIndices)
{
	TODO_ERROR();
}

VkResult Device::DisplayPowerControl(VkDisplayKHR display, const VkDisplayPowerInfoEXT* pDisplayPowerInfo)
{
	TODO_ERROR();
}

VkResult Device::RegisterDeviceEvent(const VkDeviceEventInfoEXT* pDeviceEventInfo, const VkAllocationCallbacks* pAllocator, VkFence* pFence)
{
	TODO_ERROR();
}

VkResult Device::RegisterDisplayEvent(VkDisplayKHR display, const VkDisplayEventInfoEXT* pDisplayEventInfo, const VkAllocationCallbacks* pAllocator, VkFence* pFence)
{
	TODO_ERROR();
}

VkResult Device::GetSwapchainCounter(VkSwapchainKHR swapchain, VkSurfaceCounterFlagBitsEXT counter, uint64_t* pCounterValue)
{
	TODO_ERROR();
}

VkResult Device::GetRefreshCycleDurationGOOGLE(VkSwapchainKHR swapchain, VkRefreshCycleDurationGOOGLE* pDisplayTimingProperties)
{
	TODO_ERROR();
}

VkResult Device::GetPastPresentationTimingGOOGLE(VkSwapchainKHR swapchain, uint32_t* pPresentationTimingCount, VkPastPresentationTimingGOOGLE* pPresentationTimings)
{
	TODO_ERROR();
}

#if defined(VK_EXT_hdr_metadata)
void Device::SetHdrMetadata(uint32_t swapchainCount, const VkSwapchainKHR* pSwapchains, const VkHdrMetadataEXT* pMetadata)
{
	TODO_ERROR();
}
#endif

VkResult Device::SetDebugUtilsObjectName(const VkDebugUtilsObjectNameInfoEXT* pNameInfo)
{
	TODO_ERROR();
}

VkResult Device::SetDebugUtilsObjectTag(const VkDebugUtilsObjectTagInfoEXT* pTagInfo)
{
	TODO_ERROR();
}

VkResult Device::GetImageDrmFormatModifierProperties(VkImage image, VkImageDrmFormatModifierPropertiesEXT* pProperties)
{
	TODO_ERROR();
}

VkResult Device::CreateValidationCache(const VkValidationCacheCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkValidationCacheEXT* pValidationCache)
{
	TODO_ERROR();
}

void Device::DestroyValidationCache(VkValidationCacheEXT validationCache, const VkAllocationCallbacks* pAllocator)
{
	TODO_ERROR();
}

VkResult Device::MergeValidationCaches(VkValidationCacheEXT dstCache, uint32_t srcCacheCount, const VkValidationCacheEXT* pSrcCaches)
{
	TODO_ERROR();
}

VkResult Device::GetValidationCacheData(VkValidationCacheEXT validationCache, size_t* pDataSize, void* pData)
{
	TODO_ERROR();
}

VkResult Device::CreateAccelerationStructureNV(const VkAccelerationStructureCreateInfoNV* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkAccelerationStructureNV* pAccelerationStructure)
{
	TODO_ERROR();
}

void Device::DestroyAccelerationStructureNV(VkAccelerationStructureNV accelerationStructure, const VkAllocationCallbacks* pAllocator)
{
	TODO_ERROR();
}

void Device::GetAccelerationStructureMemoryRequirementsNV(const VkAccelerationStructureMemoryRequirementsInfoNV* pInfo, VkMemoryRequirements2KHR* pMemoryRequirements)
{
	TODO_ERROR();
}

VkResult Device::BindAccelerationStructureMemoryNV(uint32_t bindInfoCount, const VkBindAccelerationStructureMemoryInfoNV* pBindInfos)
{
	TODO_ERROR();
}

VkResult Device::CreateRayTracingPipelinesNV(VkPipelineCache pipelineCache, uint32_t createInfoCount, const VkRayTracingPipelineCreateInfoNV* pCreateInfos, const VkAllocationCallbacks* pAllocator, VkPipeline* pPipelines)
{
	TODO_ERROR();
}

VkResult Device::GetRayTracingShaderGroupHandlesNV(VkPipeline pipeline, uint32_t firstGroup, uint32_t groupCount, size_t dataSize, void* pData)
{
	TODO_ERROR();
}

VkResult Device::GetAccelerationStructureHandleNV(VkAccelerationStructureNV accelerationStructure, size_t dataSize, void* pData)
{
	TODO_ERROR();
}

VkResult Device::CompileDeferredNV(VkPipeline pipeline, uint32_t shader)
{
	TODO_ERROR();
}

VkResult Device::GetMemoryHostPointerProperties(VkExternalMemoryHandleTypeFlagBits handleType, const void* pHostPointer, VkMemoryHostPointerPropertiesEXT* pMemoryHostPointerProperties)
{
	TODO_ERROR();
}

VkResult Device::GetCalibratedTimestamps(uint32_t timestampCount, const VkCalibratedTimestampInfoEXT* pTimestampInfos, uint64_t* pTimestamps, uint64_t* pMaxDeviation)
{
	for (auto i = 0u; i < timestampCount; i++)
	{
		assert(pTimestampInfos[i].timeDomain == Platform::GetTimeDomain());
		pTimestamps[i] = Platform::GetTimestamp();
	}
	*pMaxDeviation = 0;
	return VK_SUCCESS;
}

VkResult Device::InitializePerformanceApiINTEL(const VkInitializePerformanceApiInfoINTEL* pInitializeInfo)
{
	TODO_ERROR();
}

void Device::UninitializePerformanceApiINTEL()
{
	TODO_ERROR();
}

VkResult Device::AcquirePerformanceConfigurationINTEL(const VkPerformanceConfigurationAcquireInfoINTEL* pAcquireInfo, VkPerformanceConfigurationINTEL* pConfiguration)
{
	TODO_ERROR();
}

VkResult Device::ReleasePerformanceConfigurationINTEL(VkPerformanceConfigurationINTEL configuration)
{
	TODO_ERROR();
}

VkResult Device::GetPerformanceParameterINTEL(VkPerformanceParameterTypeINTEL parameter, VkPerformanceValueINTEL* pValue)
{
	TODO_ERROR();
}

void Device::SetLocalDimmingAMD(VkSwapchainKHR swapChain, VkBool32 localDimmingEnable)
{
	TODO_ERROR();
}

#if defined(VK_KHR_timeline_semaphore)
VkResult Device::GetSemaphoreCounterValue(VkSemaphore semaphore, uint64_t* pValue)
{
	TODO_ERROR();
}

VkResult Device::WaitSemaphores(const VkSemaphoreWaitInfoKHR* pWaitInfo, uint64_t timeout)
{
	TODO_ERROR();
}

VkResult Device::SignalSemaphore(const VkSemaphoreSignalInfoKHR* pSignalInfo)
{
	TODO_ERROR();
}
#endif

#if defined(VK_KHR_external_memory_win32)
VkResult Device::GetMemoryWin32Handle(const VkMemoryGetWin32HandleInfoKHR* pGetWin32HandleInfo, HANDLE* pHandle)
{
	TODO_ERROR();
}

VkResult Device::GetMemoryWin32HandleProperties(VkExternalMemoryHandleTypeFlagBits handleType, HANDLE handle, VkMemoryWin32HandlePropertiesKHR* pMemoryWin32HandleProperties)
{
	TODO_ERROR();
}
#endif

#if defined(VK_NV_external_memory_win32)
VkResult Device::GetMemoryWin32HandleNV(VkDeviceMemory memory, VkExternalMemoryHandleTypeFlagsNV handleType, HANDLE* pHandle)
{
	TODO_ERROR();
}
#endif

VkResult Device::AcquireFullScreenExclusiveMode(VkSwapchainKHR swapchain)
{
	TODO_ERROR();
}

VkResult Device::ReleaseFullScreenExclusiveMode(VkSwapchainKHR swapchain)
{
	TODO_ERROR();
}

VkResult Device::GetDeviceGroupSurfacePresentModes2(const VkPhysicalDeviceSurfaceInfo2KHR* pSurfaceInfo, VkDeviceGroupPresentModeFlagsKHR* pModes)
{
	TODO_ERROR();
}

ImageFunctions::ImageFunctions(CPJit* jit)
{
	this->jit = jit;
}

ImageFunctions::~ImageFunctions() = default;