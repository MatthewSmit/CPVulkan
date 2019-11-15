#include "PhysicalDevice.h"

#include "Device.h"
#include "Formats.h"
#include "Platform.h"
#include "Util.h"

#include <algorithm>
#include <cassert>

static VkResult GetImageFormatPropertiesImpl(VkFormat format, VkImageType type, VkImageTiling tiling, VkImageUsageFlags usage, VkImageCreateFlags flags, VkImageFormatProperties* pImageFormatProperties)
{
	const auto& information = GetFormatInformation(format);

	if ((usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) && information.Type != FormatType::DepthStencil)
	{
		return VK_ERROR_FORMAT_NOT_SUPPORTED;
	}

	if ((usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) && information.Type == FormatType::DepthStencil)
	{
		return VK_ERROR_FORMAT_NOT_SUPPORTED;
	}

	if ((usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT) && (information.Base == BaseType::UScaled || information.Base == BaseType::SScaled))
	{
		return VK_ERROR_FORMAT_NOT_SUPPORTED;
	}

	switch (type)
	{
	case VK_IMAGE_TYPE_1D:
		pImageFormatProperties->maxExtent.width = MAX_IMAGE_DIMENSION_1D;
		pImageFormatProperties->maxExtent.height = 1;
		pImageFormatProperties->maxExtent.depth = 1;
		pImageFormatProperties->maxArrayLayers = MAX_IMAGE_ARRAY_LAYERS;
		pImageFormatProperties->maxMipLevels = CountMipLevels(pImageFormatProperties->maxExtent.width, pImageFormatProperties->maxExtent.height, pImageFormatProperties->maxExtent.depth);
		break;
	case VK_IMAGE_TYPE_2D:
		if (flags & VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT)
		{
			pImageFormatProperties->maxExtent.width = MAX_IMAGE_DIMENSION_CUBE;
			pImageFormatProperties->maxExtent.height = MAX_IMAGE_DIMENSION_CUBE;
		}
		else
		{
			pImageFormatProperties->maxExtent.width = MAX_IMAGE_DIMENSION_2D;
			pImageFormatProperties->maxExtent.height = MAX_IMAGE_DIMENSION_2D;
		}
		pImageFormatProperties->maxExtent.depth = 1;
		pImageFormatProperties->maxArrayLayers = MAX_IMAGE_ARRAY_LAYERS;
		pImageFormatProperties->maxMipLevels = CountMipLevels(pImageFormatProperties->maxExtent.width, pImageFormatProperties->maxExtent.height, pImageFormatProperties->maxExtent.depth);
		break;
	case VK_IMAGE_TYPE_3D:
		pImageFormatProperties->maxExtent.width = MAX_IMAGE_DIMENSION_3D;
		pImageFormatProperties->maxExtent.height = MAX_IMAGE_DIMENSION_3D;
		pImageFormatProperties->maxExtent.depth = MAX_IMAGE_DIMENSION_3D;
		pImageFormatProperties->maxArrayLayers = 1;
		pImageFormatProperties->maxMipLevels = CountMipLevels(pImageFormatProperties->maxExtent.width, pImageFormatProperties->maxExtent.height, pImageFormatProperties->maxExtent.depth);
		break;
	default: FATAL_ERROR();
	}

	if (tiling == VK_IMAGE_TILING_LINEAR ||
		type != VK_IMAGE_TYPE_2D ||
		(flags & VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT) ||
		!(information.OptimalTilingFeatures & (VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT)) ||
		NeedsYCBCRConversion(format) ||
		(usage & (VK_IMAGE_USAGE_SHADING_RATE_IMAGE_BIT_NV | VK_IMAGE_USAGE_FRAGMENT_DENSITY_MAP_BIT_EXT)))
	{
		pImageFormatProperties->sampleCounts = VK_SAMPLE_COUNT_1_BIT;
	}
	else
	{
		pImageFormatProperties->sampleCounts = VK_SAMPLE_COUNT_1_BIT | VK_SAMPLE_COUNT_2_BIT | VK_SAMPLE_COUNT_4_BIT | VK_SAMPLE_COUNT_8_BIT | VK_SAMPLE_COUNT_16_BIT | VK_SAMPLE_COUNT_32_BIT | VK_SAMPLE_COUNT_64_BIT;
	}

	pImageFormatProperties->maxResourceSize = 0x100000000;

	return VK_SUCCESS;
}

static void GetSparseImageFormatPropertiesImpl(VkFormat format, VkImageType type, VkSampleCountFlagBits samples, VkFlags usage, VkImageTiling tiling, uint32_t* pPropertyCount, VkSparseImageFormatProperties* pProperties)
{
	if (!Platform::SupportsSparse())
	{
		*pPropertyCount = 0;
		return;
	}
	
	if (pProperties)
	{
		if (*pPropertyCount > 0)
		{
			pProperties->aspectMask = VK_IMAGE_ASPECT_COLOR_BIT |
				VK_IMAGE_ASPECT_DEPTH_BIT |
				VK_IMAGE_ASPECT_STENCIL_BIT |
				VK_IMAGE_ASPECT_METADATA_BIT |
				VK_IMAGE_ASPECT_PLANE_0_BIT |
				VK_IMAGE_ASPECT_PLANE_1_BIT |
				VK_IMAGE_ASPECT_PLANE_2_BIT |
				VK_IMAGE_ASPECT_MEMORY_PLANE_0_BIT_EXT |
				VK_IMAGE_ASPECT_MEMORY_PLANE_1_BIT_EXT |
				VK_IMAGE_ASPECT_MEMORY_PLANE_2_BIT_EXT |
				VK_IMAGE_ASPECT_MEMORY_PLANE_3_BIT_EXT;
			pProperties->imageGranularity = VkExtent3D{4096, 4096, 4096}; // TODO: use config values
			pProperties->flags = 0;
		}
	}
	else
	{
		*pPropertyCount = 1;
	}
}

void PhysicalDevice::GetFeatures(VkPhysicalDeviceFeatures* pFeatures)
{
	pFeatures->robustBufferAccess = ROBUST_BUFFER_ACCESS;
	pFeatures->fullDrawIndexUint32 = FULL_DRAW_INDEX_UINT32;
	pFeatures->imageCubeArray = IMAGE_CUBE_ARRAY;
	pFeatures->independentBlend = INDEPENDENT_BLEND;
	pFeatures->geometryShader = GEOMETRY_SHADER;
	pFeatures->tessellationShader = TESSELLATION_SHADER;
	pFeatures->sampleRateShading = SAMPLE_RATE_SHADING;
	pFeatures->dualSrcBlend = DUAL_SOURCE_BLEND;
	pFeatures->logicOp = LOGIC_OPERATION;
	pFeatures->multiDrawIndirect = MULTI_DRAW_INDIRECT;
	pFeatures->drawIndirectFirstInstance = DRAW_INDIRECT_FIRST_INSTANCE;
	pFeatures->depthClamp = DEPTH_BIAS_CLAMP;
	pFeatures->depthBiasClamp = DEPTH_BIAS_CLAMP;
	pFeatures->fillModeNonSolid = FILL_MODE_NON_SOLID;
	pFeatures->depthBounds = DEPTH_BOUNDS;
	pFeatures->wideLines = WIDE_LINES;
	pFeatures->largePoints = LARGE_POINTS;
	pFeatures->alphaToOne = ALPHA_TO_ONE;
	pFeatures->multiViewport = MULTI_VIEWPORT;
	pFeatures->samplerAnisotropy = SAMPLER_ANISOTROPY;
	pFeatures->textureCompressionETC2 = TEXTURE_COMPRESSION_ETC2;
	pFeatures->textureCompressionASTC_LDR = TEXTURE_COMPRESSION_ASTC_LDR;
	pFeatures->textureCompressionBC = TEXTURE_COMPRESSION_BC;
	pFeatures->occlusionQueryPrecise = OCCLUSION_QUERY_PRECISE;
	pFeatures->pipelineStatisticsQuery = PIPELINE_STATISTICS_QUERY;
	pFeatures->vertexPipelineStoresAndAtomics = VERTEX_PIPELINE_STORES_AND_ATOMICS;
	pFeatures->fragmentStoresAndAtomics = FRAGMENT_STORES_AND_ATOMICS;
	pFeatures->shaderTessellationAndGeometryPointSize = SHADER_TESSELLATION_AND_GEOMETRY_POINT_SIZE;
	pFeatures->shaderImageGatherExtended = SHADER_IMAGE_GATHER_EXTENDED;
	pFeatures->shaderStorageImageExtendedFormats = SHADER_STORAGE_IMAGE_EXTENDED_FORMATS;
	pFeatures->shaderStorageImageMultisample = SHADER_STORAGE_IMAGE_MULTISAMPLE;
	pFeatures->shaderStorageImageReadWithoutFormat = SHADER_STORAGE_IMAGE_READ_WITHOUT_FORMAT;
	pFeatures->shaderStorageImageWriteWithoutFormat = SHADER_STORAGE_IMAGE_WRITE_WITHOUT_FORMAT;
	pFeatures->shaderUniformBufferArrayDynamicIndexing = SHADER_UNIFORM_BUFFER_ARRAY_DYNAMIC_INDEXING;
	pFeatures->shaderSampledImageArrayDynamicIndexing = SHADER_SAMPLED_IMAGE_ARRAY_DYNAMIC_INDEXING;
	pFeatures->shaderStorageBufferArrayDynamicIndexing = SHADER_STORAGE_BUFFER_ARRAY_DYNAMIC_INDEXING;
	pFeatures->shaderStorageImageArrayDynamicIndexing = SHADER_STORAGE_IMAGE_ARRAY_DYNAMIC_INDEXING;
	pFeatures->shaderClipDistance = SHADER_CLIP_DISTANCE;
	pFeatures->shaderCullDistance = SHADER_CULL_DISTANCE;
	pFeatures->shaderFloat64 = SHADER_FLOAT64;
	pFeatures->shaderInt64 = SHADER_INT64;
	pFeatures->shaderInt16 = SHADER_INT16;
	pFeatures->shaderResourceResidency = SHADER_RESOURCE_RESIDENCY;
	pFeatures->shaderResourceMinLod = SHADER_RESOURCE_MIN_LOD;
	pFeatures->sparseBinding = SPARSE_BINDING && Platform::SupportsSparse();
	pFeatures->sparseResidencyBuffer = SPARSE_BINDING && Platform::SupportsSparse();
	pFeatures->sparseResidencyImage2D = SPARSE_BINDING && Platform::SupportsSparse();
	pFeatures->sparseResidencyImage3D = SPARSE_BINDING && Platform::SupportsSparse();
	pFeatures->sparseResidency2Samples = SPARSE_BINDING && Platform::SupportsSparse();
	pFeatures->sparseResidency4Samples = SPARSE_BINDING && Platform::SupportsSparse();
	pFeatures->sparseResidency8Samples = SPARSE_BINDING && Platform::SupportsSparse();
	pFeatures->sparseResidency16Samples = SPARSE_BINDING && Platform::SupportsSparse();
	pFeatures->sparseResidencyAliased = SPARSE_BINDING && Platform::SupportsSparse();
	pFeatures->variableMultisampleRate = VARIABLE_MULTISAMPLE_RATE;
	pFeatures->inheritedQueries = INHERITED_QUERIES;
}

void PhysicalDevice::GetFormatProperties(VkFormat format, VkFormatProperties* pFormatProperties)
{
	const auto& info = GetFormatInformation(format);
	pFormatProperties->linearTilingFeatures = info.LinearTilingFeatures;
	pFormatProperties->optimalTilingFeatures = info.OptimalTilingFeatures;
	pFormatProperties->bufferFeatures = info.BufferFeatures;
}

VkResult PhysicalDevice::GetImageFormatProperties(VkFormat format, VkImageType type, VkImageTiling tiling, VkImageUsageFlags usage, VkImageCreateFlags flags, VkImageFormatProperties* pImageFormatProperties)
{
	return GetImageFormatPropertiesImpl(format, type, tiling, usage, flags, pImageFormatProperties);
}

void PhysicalDevice::GetProperties(VkPhysicalDeviceProperties* pProperties) const
{
	const auto ULP = 1.0f / (2 << 4);
	
	pProperties->apiVersion = LATEST_VERSION;
	pProperties->driverVersion = VK_MAKE_VERSION(1, 0, 0);
	pProperties->vendorID = VENDOR_ID;
	pProperties->deviceID = DEVICE_ID;
	pProperties->deviceType = DEVICE_TYPE;
	strcpy_s(pProperties->deviceName, DEVICE_NAME);
	memset(pProperties->pipelineCacheUUID, 0, VK_UUID_SIZE);
	pProperties->limits.maxImageDimension1D = MAX_IMAGE_DIMENSION_1D;
	pProperties->limits.maxImageDimension2D = MAX_IMAGE_DIMENSION_2D;
	pProperties->limits.maxImageDimension3D = MAX_IMAGE_DIMENSION_3D;
	pProperties->limits.maxImageDimensionCube = MAX_IMAGE_DIMENSION_CUBE;
	pProperties->limits.maxImageArrayLayers = MAX_IMAGE_ARRAY_LAYERS;
	pProperties->limits.maxTexelBufferElements = 65536;
	pProperties->limits.maxUniformBufferRange = 16384;
	pProperties->limits.maxStorageBufferRange = 134217728;
	pProperties->limits.maxPushConstantsSize = MAX_PUSH_CONSTANTS_SIZE;
	pProperties->limits.maxMemoryAllocationCount = 4096;
	pProperties->limits.maxSamplerAllocationCount = 4000;
	pProperties->limits.bufferImageGranularity = 1;
	pProperties->limits.sparseAddressSpaceSize = std::numeric_limits<size_t>::max();
	pProperties->limits.maxBoundDescriptorSets = MAX_BOUND_DESCRIPTOR_SETS;
	pProperties->limits.maxPerStageDescriptorSamplers = 32;
	pProperties->limits.maxPerStageDescriptorUniformBuffers = 32;
	pProperties->limits.maxPerStageDescriptorStorageBuffers = 32;
	pProperties->limits.maxPerStageDescriptorSampledImages = 32;
	pProperties->limits.maxPerStageDescriptorStorageImages = 32;
	pProperties->limits.maxPerStageDescriptorInputAttachments = 32;
	pProperties->limits.maxPerStageResources = 128;
	pProperties->limits.maxDescriptorSetSamplers = pProperties->limits.maxPerStageDescriptorSamplers * 8;
	pProperties->limits.maxDescriptorSetUniformBuffers = pProperties->limits.maxPerStageDescriptorUniformBuffers * 8;
	pProperties->limits.maxDescriptorSetUniformBuffersDynamic = 8;
	pProperties->limits.maxDescriptorSetStorageBuffers = pProperties->limits.maxPerStageDescriptorStorageBuffers * 8;
	pProperties->limits.maxDescriptorSetStorageBuffersDynamic = 4;
	pProperties->limits.maxDescriptorSetSampledImages = pProperties->limits.maxPerStageDescriptorSampledImages * 8;
	pProperties->limits.maxDescriptorSetStorageImages = pProperties->limits.maxPerStageDescriptorStorageImages * 8;
	pProperties->limits.maxDescriptorSetInputAttachments = pProperties->limits.maxPerStageDescriptorInputAttachments;
	pProperties->limits.maxVertexInputAttributes = 16;
	pProperties->limits.maxVertexInputBindings = MAX_VERTEX_INPUT_BINDINGS;
	pProperties->limits.maxVertexInputAttributeOffset = 2047;
	pProperties->limits.maxVertexInputBindingStride = 2048;
	pProperties->limits.maxVertexOutputComponents = 64;
	pProperties->limits.maxTessellationGenerationLevel = 64;
	pProperties->limits.maxTessellationPatchSize = 32;
	pProperties->limits.maxTessellationControlPerVertexInputComponents = 64;
	pProperties->limits.maxTessellationControlPerVertexOutputComponents = 64;
	pProperties->limits.maxTessellationControlPerPatchOutputComponents = 120;
	pProperties->limits.maxTessellationControlTotalOutputComponents = 2048;
	pProperties->limits.maxTessellationEvaluationInputComponents = 64;
	pProperties->limits.maxTessellationEvaluationOutputComponents = 64;
	pProperties->limits.maxGeometryShaderInvocations = 32;
	pProperties->limits.maxGeometryInputComponents = 64;
	pProperties->limits.maxGeometryOutputComponents = 64;
	pProperties->limits.maxGeometryOutputVertices = 256;
	pProperties->limits.maxGeometryTotalOutputComponents = 1024;
	pProperties->limits.maxFragmentInputComponents = 64;
	pProperties->limits.maxFragmentOutputAttachments = MAX_FRAGMENT_OUTPUT_ATTACHMENTS;
	pProperties->limits.maxFragmentDualSrcAttachments = 1;
	pProperties->limits.maxFragmentCombinedOutputResources = 4;
	pProperties->limits.maxComputeSharedMemorySize = 16384;
	pProperties->limits.maxComputeWorkGroupCount[0] = 65535;
	pProperties->limits.maxComputeWorkGroupCount[1] = 65535;
	pProperties->limits.maxComputeWorkGroupCount[2] = 65535;
	pProperties->limits.maxComputeWorkGroupInvocations = 128;
	pProperties->limits.maxComputeWorkGroupSize[0] = 128;
	pProperties->limits.maxComputeWorkGroupSize[1] = 128;
	pProperties->limits.maxComputeWorkGroupSize[2] = 64;
	pProperties->limits.subPixelPrecisionBits = 4;
	pProperties->limits.subTexelPrecisionBits = 4;
	pProperties->limits.mipmapPrecisionBits = 4;
	pProperties->limits.maxDrawIndexedIndexValue = 0xFFFFFFFF;
	pProperties->limits.maxDrawIndirectCount = 0xFFFF;
	pProperties->limits.maxSamplerLodBias = 2;
	pProperties->limits.maxSamplerAnisotropy = 16;
	pProperties->limits.maxViewports = MAX_VIEWPORTS;
	pProperties->limits.maxViewportDimensions[0] = 4096;
	pProperties->limits.maxViewportDimensions[1] = 4096;
	pProperties->limits.viewportBoundsRange[0] = -8192;
	pProperties->limits.viewportBoundsRange[1] = 8192;
	pProperties->limits.viewportSubPixelBits = 0;
	pProperties->limits.minMemoryMapAlignment = 64;
	pProperties->limits.minTexelBufferOffsetAlignment = 256; // TODO: Lower to 16?
	pProperties->limits.minUniformBufferOffsetAlignment = 256;
	pProperties->limits.minStorageBufferOffsetAlignment = 256;
	pProperties->limits.minTexelOffset = -8;
	pProperties->limits.maxTexelOffset = 7;
	pProperties->limits.minTexelGatherOffset = -8;
	pProperties->limits.maxTexelGatherOffset = 7;
	pProperties->limits.minInterpolationOffset = -0.5;
	pProperties->limits.maxInterpolationOffset = 0.5f - ULP;
	pProperties->limits.subPixelInterpolationOffsetBits = 4;
	pProperties->limits.maxFramebufferWidth = 4096;
	pProperties->limits.maxFramebufferHeight = 4096;
	pProperties->limits.maxFramebufferLayers = 256;
	pProperties->limits.framebufferColorSampleCounts = VK_SAMPLE_COUNT_1_BIT | VK_SAMPLE_COUNT_2_BIT | VK_SAMPLE_COUNT_4_BIT | VK_SAMPLE_COUNT_8_BIT | VK_SAMPLE_COUNT_16_BIT | VK_SAMPLE_COUNT_32_BIT | VK_SAMPLE_COUNT_64_BIT;
	pProperties->limits.framebufferDepthSampleCounts = VK_SAMPLE_COUNT_1_BIT | VK_SAMPLE_COUNT_2_BIT | VK_SAMPLE_COUNT_4_BIT | VK_SAMPLE_COUNT_8_BIT | VK_SAMPLE_COUNT_16_BIT | VK_SAMPLE_COUNT_32_BIT | VK_SAMPLE_COUNT_64_BIT;
	pProperties->limits.framebufferStencilSampleCounts = VK_SAMPLE_COUNT_1_BIT | VK_SAMPLE_COUNT_2_BIT | VK_SAMPLE_COUNT_4_BIT | VK_SAMPLE_COUNT_8_BIT | VK_SAMPLE_COUNT_16_BIT | VK_SAMPLE_COUNT_32_BIT | VK_SAMPLE_COUNT_64_BIT;
	pProperties->limits.framebufferNoAttachmentsSampleCounts = VK_SAMPLE_COUNT_1_BIT | VK_SAMPLE_COUNT_2_BIT | VK_SAMPLE_COUNT_4_BIT | VK_SAMPLE_COUNT_8_BIT | VK_SAMPLE_COUNT_16_BIT | VK_SAMPLE_COUNT_32_BIT | VK_SAMPLE_COUNT_64_BIT;
	pProperties->limits.maxColorAttachments = MAX_COLOUR_ATTACHMENTS;
	pProperties->limits.sampledImageColorSampleCounts = VK_SAMPLE_COUNT_1_BIT | VK_SAMPLE_COUNT_2_BIT | VK_SAMPLE_COUNT_4_BIT | VK_SAMPLE_COUNT_8_BIT | VK_SAMPLE_COUNT_16_BIT | VK_SAMPLE_COUNT_32_BIT | VK_SAMPLE_COUNT_64_BIT;
	pProperties->limits.sampledImageIntegerSampleCounts = VK_SAMPLE_COUNT_1_BIT | VK_SAMPLE_COUNT_2_BIT | VK_SAMPLE_COUNT_4_BIT | VK_SAMPLE_COUNT_8_BIT | VK_SAMPLE_COUNT_16_BIT | VK_SAMPLE_COUNT_32_BIT | VK_SAMPLE_COUNT_64_BIT;
	pProperties->limits.sampledImageDepthSampleCounts = VK_SAMPLE_COUNT_1_BIT | VK_SAMPLE_COUNT_2_BIT | VK_SAMPLE_COUNT_4_BIT | VK_SAMPLE_COUNT_8_BIT | VK_SAMPLE_COUNT_16_BIT | VK_SAMPLE_COUNT_32_BIT | VK_SAMPLE_COUNT_64_BIT;
	pProperties->limits.sampledImageStencilSampleCounts = VK_SAMPLE_COUNT_1_BIT | VK_SAMPLE_COUNT_2_BIT | VK_SAMPLE_COUNT_4_BIT | VK_SAMPLE_COUNT_8_BIT | VK_SAMPLE_COUNT_16_BIT | VK_SAMPLE_COUNT_32_BIT | VK_SAMPLE_COUNT_64_BIT;
	pProperties->limits.storageImageSampleCounts = VK_SAMPLE_COUNT_1_BIT | VK_SAMPLE_COUNT_2_BIT | VK_SAMPLE_COUNT_4_BIT | VK_SAMPLE_COUNT_8_BIT | VK_SAMPLE_COUNT_16_BIT | VK_SAMPLE_COUNT_32_BIT | VK_SAMPLE_COUNT_64_BIT;
	pProperties->limits.maxSampleMaskWords = 1;
	pProperties->limits.timestampComputeAndGraphics = true;
	pProperties->limits.timestampPeriod = 1;
	pProperties->limits.maxClipDistances = 8;
	pProperties->limits.maxCullDistances = 8;
	pProperties->limits.maxCombinedClipAndCullDistances = 8;
	pProperties->limits.discreteQueuePriorities = 2;
	pProperties->limits.pointSizeRange[0] = 1.0;
	pProperties->limits.pointSizeRange[1] = 64.0f - ULP;
	pProperties->limits.lineWidthRange[0] = 1;
	pProperties->limits.lineWidthRange[1] = 8.0f - ULP;
	pProperties->limits.pointSizeGranularity = POINT_SIZE_GRANULARITY;
	pProperties->limits.lineWidthGranularity = LINE_WIDTH_GRANULARITY;
	pProperties->limits.strictLines = true;
	pProperties->limits.standardSampleLocations = true;
	pProperties->limits.optimalBufferCopyOffsetAlignment = 16;
	pProperties->limits.optimalBufferCopyRowPitchAlignment = 16;
	pProperties->limits.nonCoherentAtomSize = 256;
	pProperties->sparseProperties.residencyStandard2DBlockShape = false;
	pProperties->sparseProperties.residencyStandard2DMultisampleBlockShape = false;
	pProperties->sparseProperties.residencyStandard3DBlockShape = false;
	pProperties->sparseProperties.residencyAlignedMipSize = false;
	pProperties->sparseProperties.residencyNonResidentStrict = false;
}

void PhysicalDevice::GetQueueFamilyProperties(uint32_t* pQueueFamilyPropertyCount, VkQueueFamilyProperties* pQueueFamilyProperties)
{
	if (pQueueFamilyProperties)
	{
		if (*pQueueFamilyPropertyCount > 0)
		{
			pQueueFamilyProperties->queueFlags = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT | VK_QUEUE_PROTECTED_BIT;
			if (SPARSE_BINDING && Platform::SupportsSparse())
			{
				pQueueFamilyProperties->queueFlags |= VK_QUEUE_SPARSE_BINDING_BIT;
			}
			pQueueFamilyProperties->queueCount = 1;
			pQueueFamilyProperties->timestampValidBits = 64;
			pQueueFamilyProperties->minImageTransferGranularity = VkExtent3D{0, 0, 0};
			*pQueueFamilyPropertyCount = 1;
		}
	}
	else
	{
		*pQueueFamilyPropertyCount = 1;
	}
}

void PhysicalDevice::GetMemoryProperties(VkPhysicalDeviceMemoryProperties* pMemoryProperties)
{
	pMemoryProperties->memoryTypeCount = 1;
	pMemoryProperties->memoryTypes[0].propertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	pMemoryProperties->memoryTypes[0].heapIndex = 0;
	pMemoryProperties->memoryHeapCount = 1;
	pMemoryProperties->memoryHeaps[0].size = 0xFFFFFFFFFFFFFFFF;
	pMemoryProperties->memoryHeaps[0].flags = VK_MEMORY_HEAP_DEVICE_LOCAL_BIT;
}

void PhysicalDevice::GetSparseImageFormatProperties(VkFormat format, VkImageType type, VkSampleCountFlagBits samples, VkImageUsageFlags usage, VkImageTiling tiling, uint32_t* pPropertyCount, VkSparseImageFormatProperties* pProperties)
{
	GetSparseImageFormatPropertiesImpl(format, type, samples, usage, tiling, pPropertyCount, pProperties);
}

void PhysicalDevice::GetFeatures2(VkPhysicalDeviceFeatures2* pFeatures)
{
	assert(pFeatures->sType == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2);

	GetFeatures(&pFeatures->features);

	auto next = static_cast<VkBaseOutStructure*>(pFeatures->pNext);
	while (next)
	{
		const auto type = next->sType;
		switch (type)
		{
		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_16BIT_STORAGE_FEATURES:
			{
				const auto features = reinterpret_cast<VkPhysicalDevice16BitStorageFeatures*>(next);
				features->storageBuffer16BitAccess = true;
				features->uniformAndStorageBuffer16BitAccess = true;
				features->storagePushConstant16 = true;
				features->storageInputOutput16 = true;
				break;
			}
			
		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_8BIT_STORAGE_FEATURES_KHR:
			{
				const auto features = reinterpret_cast<VkPhysicalDevice8BitStorageFeaturesKHR*>(next);
				features->storageBuffer8BitAccess = true;
				features->uniformAndStorageBuffer8BitAccess = true;
				features->storagePushConstant8 = true;
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ASTC_DECODE_FEATURES_EXT:
			{
				const auto features = reinterpret_cast<VkPhysicalDeviceASTCDecodeFeaturesEXT*>(next);
				features->decodeModeSharedExponent = true;
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BLEND_OPERATION_ADVANCED_FEATURES_EXT:
			{
				const auto features = reinterpret_cast<VkPhysicalDeviceBlendOperationAdvancedFeaturesEXT*>(next);
				features->advancedBlendCoherentOperations = true;
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES_EXT:
			{
				const auto features = reinterpret_cast<VkPhysicalDeviceBufferDeviceAddressFeaturesEXT*>(next);
				features->bufferDeviceAddress = true;
				features->bufferDeviceAddressCaptureReplay = false;
				features->bufferDeviceAddressMultiDevice = true;
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COHERENT_MEMORY_FEATURES_AMD:
			{
				const auto features = reinterpret_cast<VkPhysicalDeviceCoherentMemoryFeaturesAMD*>(next);
				features->deviceCoherentMemory = true;
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COMPUTE_SHADER_DERIVATIVES_FEATURES_NV:
			{
				const auto features = reinterpret_cast<VkPhysicalDeviceComputeShaderDerivativesFeaturesNV*>(next);
				features->computeDerivativeGroupQuads = true;
				features->computeDerivativeGroupLinear = true;
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CONDITIONAL_RENDERING_FEATURES_EXT:
			{
				const auto features = reinterpret_cast<VkPhysicalDeviceConditionalRenderingFeaturesEXT*>(next);
				features->conditionalRendering = true;
				features->inheritedConditionalRendering = true;
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COOPERATIVE_MATRIX_FEATURES_NV:
			{
				const auto features = reinterpret_cast<VkPhysicalDeviceCooperativeMatrixFeaturesNV*>(next);
				features->cooperativeMatrix = true;
				features->cooperativeMatrixRobustBufferAccess = true;
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CORNER_SAMPLED_IMAGE_FEATURES_NV:
			{
				const auto features = reinterpret_cast<VkPhysicalDeviceCornerSampledImageFeaturesNV*>(next);
				features->cornerSampledImage = true;
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COVERAGE_REDUCTION_MODE_FEATURES_NV:
			{
				const auto features = reinterpret_cast<VkPhysicalDeviceCoverageReductionModeFeaturesNV*>(next);
				features->coverageReductionMode = true;
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEDICATED_ALLOCATION_IMAGE_ALIASING_FEATURES_NV:
			{
				const auto features = reinterpret_cast<VkPhysicalDeviceDedicatedAllocationImageAliasingFeaturesNV*>(next);
				features->dedicatedAllocationImageAliasing = true;
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEPTH_CLIP_ENABLE_FEATURES_EXT:
			{
				const auto features = reinterpret_cast<VkPhysicalDeviceDepthClipEnableFeaturesEXT*>(next);
				features->depthClipEnable = true;
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT:
			{
				const auto features = reinterpret_cast<VkPhysicalDeviceDescriptorIndexingFeaturesEXT*>(next);
				features->shaderInputAttachmentArrayDynamicIndexing = true;
				features->shaderUniformTexelBufferArrayDynamicIndexing = true;
				features->shaderStorageTexelBufferArrayDynamicIndexing = true;
				features->shaderUniformBufferArrayNonUniformIndexing = true;
				features->shaderSampledImageArrayNonUniformIndexing = true;
				features->shaderStorageBufferArrayNonUniformIndexing = true;
				features->shaderStorageImageArrayNonUniformIndexing = true;
				features->shaderInputAttachmentArrayNonUniformIndexing = true;
				features->shaderUniformTexelBufferArrayNonUniformIndexing = true;
				features->shaderStorageTexelBufferArrayNonUniformIndexing = true;
				features->descriptorBindingUniformBufferUpdateAfterBind = true;
				features->descriptorBindingSampledImageUpdateAfterBind = true;
				features->descriptorBindingStorageImageUpdateAfterBind = true;
				features->descriptorBindingStorageBufferUpdateAfterBind = true;
				features->descriptorBindingUniformTexelBufferUpdateAfterBind = true;
				features->descriptorBindingStorageTexelBufferUpdateAfterBind = true;
				features->descriptorBindingUpdateUnusedWhilePending = true;
				features->descriptorBindingPartiallyBound = true;
				features->descriptorBindingVariableDescriptorCount = true;
				features->runtimeDescriptorArray = true;
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXCLUSIVE_SCISSOR_FEATURES_NV:
			{
				const auto features = reinterpret_cast<VkPhysicalDeviceExclusiveScissorFeaturesNV*>(next);
				features->exclusiveScissor = true;
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_DENSITY_MAP_FEATURES_EXT:
			{
				const auto features = reinterpret_cast<VkPhysicalDeviceFragmentDensityMapFeaturesEXT*>(next);
				features->fragmentDensityMap = true;
				features->fragmentDensityMapDynamic = true;
				features->fragmentDensityMapNonSubsampledImages = true;
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADER_BARYCENTRIC_FEATURES_NV:
			{
				const auto features = reinterpret_cast<VkPhysicalDeviceFragmentShaderBarycentricFeaturesNV*>(next);
				features->fragmentShaderBarycentric = true;
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADER_INTERLOCK_FEATURES_EXT:
			{
				const auto features = reinterpret_cast<VkPhysicalDeviceFragmentShaderInterlockFeaturesEXT*>(next);
				features->fragmentShaderPixelInterlock = true;
				features->fragmentShaderSampleInterlock = true;
				features->fragmentShaderShadingRateInterlock = true;
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_HOST_QUERY_RESET_FEATURES_EXT:
			{
				const auto features = reinterpret_cast<VkPhysicalDeviceHostQueryResetFeaturesEXT*>(next);
				features->hostQueryReset = true;
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGELESS_FRAMEBUFFER_FEATURES_KHR:
			{
				const auto features = reinterpret_cast<VkPhysicalDeviceImagelessFramebufferFeaturesKHR*>(next);
				features->imagelessFramebuffer = true;
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_INDEX_TYPE_UINT8_FEATURES_EXT:
			{
				const auto features = reinterpret_cast<VkPhysicalDeviceIndexTypeUint8FeaturesEXT*>(next);
				features->indexTypeUint8 = true;
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_INLINE_UNIFORM_BLOCK_FEATURES_EXT:
			{
				const auto features = reinterpret_cast<VkPhysicalDeviceInlineUniformBlockFeaturesEXT*>(next);
				features->inlineUniformBlock = true;
				features->descriptorBindingInlineUniformBlockUpdateAfterBind = true;
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_LINE_RASTERIZATION_FEATURES_EXT:
			{
				const auto features = reinterpret_cast<VkPhysicalDeviceLineRasterizationFeaturesEXT*>(next);
				features->rectangularLines = true;
				features->bresenhamLines = true;
				features->smoothLines = true;
				features->stippledRectangularLines = true;
				features->stippledBresenhamLines = true;
				features->stippledSmoothLines = true;
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PRIORITY_FEATURES_EXT:
			{
				const auto features = reinterpret_cast<VkPhysicalDeviceMemoryPriorityFeaturesEXT*>(next);
				features->memoryPriority = false;
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_NV:
			{
				const auto features = reinterpret_cast<VkPhysicalDeviceMeshShaderFeaturesNV*>(next);
				features->meshShader = true;
				features->taskShader = true;
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_FEATURES:
			{
				const auto features = reinterpret_cast<VkPhysicalDeviceMultiviewFeatures*>(next);
				features->multiview = true;
				features->multiviewGeometryShader = true;
				features->multiviewTessellationShader = true;
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PIPELINE_EXECUTABLE_PROPERTIES_FEATURES_KHR:
			{
				const auto features = reinterpret_cast<VkPhysicalDevicePipelineExecutablePropertiesFeaturesKHR*>(next);
				features->pipelineExecutableInfo = true;
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROTECTED_MEMORY_FEATURES:
			{
				const auto features = reinterpret_cast<VkPhysicalDeviceProtectedMemoryFeatures*>(next);
				features->protectedMemory = true;
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_REPRESENTATIVE_FRAGMENT_TEST_FEATURES_NV:
			{
				const auto features = reinterpret_cast<VkPhysicalDeviceRepresentativeFragmentTestFeaturesNV*>(next);
				features->representativeFragmentTest = true;
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SAMPLER_YCBCR_CONVERSION_FEATURES:
			{
				const auto features = reinterpret_cast<VkPhysicalDeviceSamplerYcbcrConversionFeatures*>(next);
				features->samplerYcbcrConversion = true;
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SCALAR_BLOCK_LAYOUT_FEATURES_EXT:
			{
				const auto features = reinterpret_cast<VkPhysicalDeviceScalarBlockLayoutFeaturesEXT*>(next);
				features->scalarBlockLayout = true;
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ATOMIC_INT64_FEATURES_KHR:
			{
				const auto features = reinterpret_cast<VkPhysicalDeviceShaderAtomicInt64FeaturesKHR*>(next);
				features->shaderBufferInt64Atomics = true;
				features->shaderSharedInt64Atomics = true;
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DEMOTE_TO_HELPER_INVOCATION_FEATURES_EXT:
			{
				const auto features = reinterpret_cast<VkPhysicalDeviceShaderDemoteToHelperInvocationFeaturesEXT*>(next);
				features->shaderDemoteToHelperInvocation = true;
				break;
			}
			
		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DRAW_PARAMETERS_FEATURES:
			{
				const auto features = reinterpret_cast<VkPhysicalDeviceShaderDrawParametersFeatures*>(next);
				features->shaderDrawParameters = true;
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_FLOAT16_INT8_FEATURES_KHR:
			{
				const auto features = reinterpret_cast<VkPhysicalDeviceShaderFloat16Int8FeaturesKHR*>(next);
				features->shaderFloat16 = true;
				features->shaderInt8 = true;
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_IMAGE_FOOTPRINT_FEATURES_NV:
			{
				const auto features = reinterpret_cast<VkPhysicalDeviceShaderImageFootprintFeaturesNV*>(next);
				features->imageFootprint = true;
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_INTEGER_FUNCTIONS_2_FEATURES_INTEL:
			{
				const auto features = reinterpret_cast<VkPhysicalDeviceShaderIntegerFunctions2FeaturesINTEL*>(next);
				features->shaderIntegerFunctions2 = true;
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_SM_BUILTINS_FEATURES_NV:
			{
				const auto features = reinterpret_cast<VkPhysicalDeviceShaderSMBuiltinsFeaturesNV*>(next);
				features->shaderSMBuiltins = true;
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADING_RATE_IMAGE_FEATURES_NV:
			{
				const auto features = reinterpret_cast<VkPhysicalDeviceShadingRateImageFeaturesNV*>(next);
				features->shadingRateImage = true;
				features->shadingRateCoarseSampleOrder = true;
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_SIZE_CONTROL_FEATURES_EXT:
			{
				const auto features = reinterpret_cast<VkPhysicalDeviceSubgroupSizeControlFeaturesEXT*>(next);
				features->computeFullSubgroups = true;
				features->subgroupSizeControl = true;
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TEXEL_BUFFER_ALIGNMENT_FEATURES_EXT:
			{
				const auto features = reinterpret_cast<VkPhysicalDeviceTexelBufferAlignmentFeaturesEXT*>(next);
				features->texelBufferAlignment = true;
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TEXTURE_COMPRESSION_ASTC_HDR_FEATURES_EXT:
			{
				const auto features = reinterpret_cast<VkPhysicalDeviceTextureCompressionASTCHDRFeaturesEXT*>(next);
				features->textureCompressionASTC_HDR = true;
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TRANSFORM_FEEDBACK_FEATURES_EXT:
			{
				const auto features = reinterpret_cast<VkPhysicalDeviceTransformFeedbackFeaturesEXT*>(next);
				features->transformFeedback = true;
				features->geometryStreams = true;
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_UNIFORM_BUFFER_STANDARD_LAYOUT_FEATURES_KHR:
			{
				const auto features = reinterpret_cast<VkPhysicalDeviceUniformBufferStandardLayoutFeaturesKHR*>(next);
				features->uniformBufferStandardLayout = true;
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VARIABLE_POINTERS_FEATURES:
			{
				const auto features = reinterpret_cast<VkPhysicalDeviceVariablePointersFeatures*>(next);
				features->variablePointersStorageBuffer = true;
				features->variablePointers = true;
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VERTEX_ATTRIBUTE_DIVISOR_FEATURES_EXT:
			{
				const auto features = reinterpret_cast<VkPhysicalDeviceVertexAttributeDivisorFeaturesEXT*>(next);
				features->vertexAttributeInstanceRateDivisor = true;
				features->vertexAttributeInstanceRateZeroDivisor = true;
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_MEMORY_MODEL_FEATURES_KHR:
			{
				const auto features = reinterpret_cast<VkPhysicalDeviceVulkanMemoryModelFeaturesKHR*>(next);
				features->vulkanMemoryModel = true;
				features->vulkanMemoryModelDeviceScope = true;
				features->vulkanMemoryModelAvailabilityVisibilityChains = true;
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_YCBCR_IMAGE_ARRAYS_FEATURES_EXT:
			{
				const auto features = reinterpret_cast<VkPhysicalDeviceSamplerYcbcrConversionFeatures*>(next);
				features->samplerYcbcrConversion = true;
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_CLOCK_FEATURES_KHR:
			{
				const auto features = reinterpret_cast<VkPhysicalDeviceShaderClockFeaturesKHR*>(next);
				features->shaderSubgroupClock = true;
				features->shaderDeviceClock = true;
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_SUBGROUP_EXTENDED_TYPES_FEATURES_KHR:
			{
				const auto features = reinterpret_cast<VkPhysicalDeviceShaderSubgroupExtendedTypesFeaturesKHR*>(next);
				features->shaderSubgroupExtendedTypes = true;
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES_KHR:
			{
				const auto features = reinterpret_cast<VkPhysicalDeviceTimelineSemaphoreFeaturesKHR*>(next);
				features->timelineSemaphore = true;
				break;
			}

		default:
			break;
		}
		next = next->pNext;
	}
}

void PhysicalDevice::GetProperties2(VkPhysicalDeviceProperties2* pProperties)
{
	assert(pProperties->sType == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2);

	GetProperties(&pProperties->properties);

	auto next = pProperties->pNext;
	while (next)
	{
		const auto type = static_cast<const VkBaseInStructure*>(next)->sType;
		switch (type)
		{
		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BLEND_OPERATION_ADVANCED_PROPERTIES_EXT:
			{
				const auto properties = static_cast<VkPhysicalDeviceBlendOperationAdvancedPropertiesEXT*>(next);
				properties->advancedBlendMaxColorAttachments = MAX_COLOUR_ATTACHMENTS;
				properties->advancedBlendIndependentBlend = true;
				properties->advancedBlendNonPremultipliedSrcColor = true;
				properties->advancedBlendNonPremultipliedDstColor = true;
				properties->advancedBlendCorrelatedOverlap = true;
				properties->advancedBlendAllOperations = true;
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CONSERVATIVE_RASTERIZATION_PROPERTIES_EXT:
			{
				const auto properties = static_cast<VkPhysicalDeviceConservativeRasterizationPropertiesEXT*>(next);
				FATAL_ERROR();
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COOPERATIVE_MATRIX_PROPERTIES_NV:
			{
				const auto properties = static_cast<VkPhysicalDeviceCooperativeMatrixPropertiesNV*>(next);
				properties->cooperativeMatrixSupportedStages = VK_SHADER_STAGE_ALL;
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEPTH_STENCIL_RESOLVE_PROPERTIES_KHR:
			{
				const auto properties = static_cast<VkPhysicalDeviceDepthStencilResolvePropertiesKHR*>(next);
				properties->supportedDepthResolveModes = VK_RESOLVE_MODE_NONE_KHR | VK_RESOLVE_MODE_SAMPLE_ZERO_BIT_KHR | VK_RESOLVE_MODE_AVERAGE_BIT_KHR | VK_RESOLVE_MODE_MIN_BIT_KHR | VK_RESOLVE_MODE_MAX_BIT_KHR;
				properties->supportedStencilResolveModes = VK_RESOLVE_MODE_NONE_KHR | VK_RESOLVE_MODE_SAMPLE_ZERO_BIT_KHR | VK_RESOLVE_MODE_AVERAGE_BIT_KHR | VK_RESOLVE_MODE_MIN_BIT_KHR | VK_RESOLVE_MODE_MAX_BIT_KHR;
				properties->independentResolveNone = true;
				properties->independentResolve = true;
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_PROPERTIES_EXT:
			{
				const auto properties = static_cast<VkPhysicalDeviceDescriptorIndexingPropertiesEXT*>(next);
				FATAL_ERROR();
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DISCARD_RECTANGLE_PROPERTIES_EXT:
			{
				const auto properties = static_cast<VkPhysicalDeviceDiscardRectanglePropertiesEXT*>(next);
				FATAL_ERROR();
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRIVER_PROPERTIES_KHR:
			{
				const auto properties = static_cast<VkPhysicalDeviceDriverPropertiesKHR*>(next);
				properties->driverID = VK_DRIVER_ID_GOOGLE_SWIFTSHADER_KHR; // TODO
				strcpy_s(properties->driverName, "CPVulkan");
				strcpy_s(properties->driverInfo, "Some CPU Driver");
				properties->conformanceVersion.major = 1;
				properties->conformanceVersion.minor = 1;
				properties->conformanceVersion.subminor = 5;
				properties->conformanceVersion.patch = 0;
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_MEMORY_HOST_PROPERTIES_EXT:
			{
				const auto properties = static_cast<VkPhysicalDeviceExternalMemoryHostPropertiesEXT*>(next);
				FATAL_ERROR();
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FLOAT_CONTROLS_PROPERTIES_KHR:
			{
				const auto properties = static_cast<VkPhysicalDeviceFloatControlsPropertiesKHR*>(next);
				properties->denormBehaviorIndependence = VK_SHADER_FLOAT_CONTROLS_INDEPENDENCE_ALL_KHR;
				properties->roundingModeIndependence = VK_SHADER_FLOAT_CONTROLS_INDEPENDENCE_ALL_KHR;
				properties->shaderSignedZeroInfNanPreserveFloat16 = true;
				properties->shaderSignedZeroInfNanPreserveFloat32 = true;
				properties->shaderSignedZeroInfNanPreserveFloat64 = true;
				properties->shaderDenormPreserveFloat16 = true;
				properties->shaderDenormPreserveFloat32 = true;
				properties->shaderDenormPreserveFloat64 = true;
				properties->shaderDenormFlushToZeroFloat16 = true;
				properties->shaderDenormFlushToZeroFloat32 = true;
				properties->shaderDenormFlushToZeroFloat64 = true;
				properties->shaderRoundingModeRTEFloat16 = true;
				properties->shaderRoundingModeRTEFloat32 = true;
				properties->shaderRoundingModeRTEFloat64 = true;
				properties->shaderRoundingModeRTZFloat16 = true;
				properties->shaderRoundingModeRTZFloat32 = true;
				properties->shaderRoundingModeRTZFloat64 = true;
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_DENSITY_MAP_PROPERTIES_EXT:
			{
				const auto properties = static_cast<VkPhysicalDeviceFragmentDensityMapPropertiesEXT*>(next);
				FATAL_ERROR();
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES:
			{
				const auto properties = static_cast<VkPhysicalDeviceIDProperties*>(next);
				memset(properties->deviceUUID, 0, VK_UUID_SIZE);
				memset(properties->driverUUID, 0, VK_UUID_SIZE);
				memset(properties->deviceLUID, 0, VK_LUID_SIZE);
				properties->deviceNodeMask = 0;
				properties->deviceLUIDValid = false;
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_INLINE_UNIFORM_BLOCK_PROPERTIES_EXT:
			{
				const auto properties = static_cast<VkPhysicalDeviceInlineUniformBlockPropertiesEXT*>(next);
				FATAL_ERROR();
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_LINE_RASTERIZATION_PROPERTIES_EXT:
			{
				const auto properties = static_cast<VkPhysicalDeviceLineRasterizationPropertiesEXT*>(next);
				FATAL_ERROR();
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_3_PROPERTIES:
			{
				const auto properties = static_cast<VkPhysicalDeviceMaintenance3Properties*>(next);
				properties->maxPerSetDescriptors = 128;
				properties->maxMemoryAllocationSize = 0x100000000;
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_PROPERTIES_NV:
			{
				const auto properties = static_cast<VkPhysicalDeviceMeshShaderPropertiesNV*>(next);
				FATAL_ERROR();
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_PER_VIEW_ATTRIBUTES_PROPERTIES_NVX:
			{
				const auto properties = static_cast<VkPhysicalDeviceMultiviewPerViewAttributesPropertiesNVX*>(next);
				FATAL_ERROR();
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_PROPERTIES:
			{
				const auto properties = static_cast<VkPhysicalDeviceMultiviewProperties*>(next);
				properties->maxMultiviewViewCount = 1;
				properties->maxMultiviewInstanceIndex = 1;
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PCI_BUS_INFO_PROPERTIES_EXT:
			{
				const auto properties = static_cast<VkPhysicalDevicePCIBusInfoPropertiesEXT*>(next);
				FATAL_ERROR();
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_POINT_CLIPPING_PROPERTIES:
			{
				const auto properties = static_cast<VkPhysicalDevicePointClippingProperties*>(next);
				properties->pointClippingBehavior = VK_POINT_CLIPPING_BEHAVIOR_ALL_CLIP_PLANES;
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROTECTED_MEMORY_PROPERTIES:
			{
				const auto properties = static_cast<VkPhysicalDeviceProtectedMemoryProperties*>(next);
				properties->protectedNoFault = true;
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PUSH_DESCRIPTOR_PROPERTIES_KHR:
			{
				const auto properties = static_cast<VkPhysicalDevicePushDescriptorPropertiesKHR*>(next);
				properties->maxPushDescriptors = 128;
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PROPERTIES_NV:
			{
				const auto properties = static_cast<VkPhysicalDeviceRayTracingPropertiesNV*>(next);
				FATAL_ERROR();
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SAMPLE_LOCATIONS_PROPERTIES_EXT:
			{
				const auto properties = static_cast<VkPhysicalDeviceSampleLocationsPropertiesEXT*>(next);
				FATAL_ERROR();
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SAMPLER_FILTER_MINMAX_PROPERTIES_EXT:
			{
				const auto properties = static_cast<VkPhysicalDeviceSamplerFilterMinmaxPropertiesEXT*>(next);
				properties->filterMinmaxSingleComponentFormats = true;
				properties->filterMinmaxImageComponentMapping = true;
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_CORE_PROPERTIES_2_AMD:
			{
				const auto properties = static_cast<VkPhysicalDeviceShaderCoreProperties2AMD*>(next);
				FATAL_ERROR();
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_CORE_PROPERTIES_AMD:
			{
				const auto properties = static_cast<VkPhysicalDeviceShaderCorePropertiesAMD*>(next);
				FATAL_ERROR();
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_SM_BUILTINS_PROPERTIES_NV:
			{
				const auto properties = static_cast<VkPhysicalDeviceShaderSMBuiltinsPropertiesNV*>(next);
				FATAL_ERROR();
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADING_RATE_IMAGE_PROPERTIES_NV:
			{
				const auto properties = static_cast<VkPhysicalDeviceShadingRateImagePropertiesNV*>(next);
				FATAL_ERROR();
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_PROPERTIES:
			{
				const auto properties = static_cast<VkPhysicalDeviceSubgroupProperties*>(next);
				properties->subgroupSize = 1;
				properties->supportedStages = VK_SHADER_STAGE_ALL;
				properties->supportedOperations = VK_SUBGROUP_FEATURE_BASIC_BIT | VK_SUBGROUP_FEATURE_VOTE_BIT | VK_SUBGROUP_FEATURE_ARITHMETIC_BIT | VK_SUBGROUP_FEATURE_BALLOT_BIT | VK_SUBGROUP_FEATURE_SHUFFLE_BIT |
					VK_SUBGROUP_FEATURE_SHUFFLE_RELATIVE_BIT | VK_SUBGROUP_FEATURE_CLUSTERED_BIT | VK_SUBGROUP_FEATURE_QUAD_BIT | VK_SUBGROUP_FEATURE_PARTITIONED_BIT_NV;
				properties->quadOperationsInAllStages = true;
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_SIZE_CONTROL_PROPERTIES_EXT:
			{
				const auto properties = static_cast<VkPhysicalDeviceSubgroupSizeControlPropertiesEXT*>(next);
				FATAL_ERROR();
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TEXEL_BUFFER_ALIGNMENT_PROPERTIES_EXT:
			{
				const auto properties = static_cast<VkPhysicalDeviceTexelBufferAlignmentPropertiesEXT*>(next);
				FATAL_ERROR();
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TRANSFORM_FEEDBACK_PROPERTIES_EXT:
			{
				const auto properties = static_cast<VkPhysicalDeviceTransformFeedbackPropertiesEXT*>(next);
				FATAL_ERROR();
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VERTEX_ATTRIBUTE_DIVISOR_PROPERTIES_EXT:
			{
				const auto properties = static_cast<VkPhysicalDeviceVertexAttributeDivisorPropertiesEXT*>(next);
				FATAL_ERROR();
				break;
			}

		default:
			break;
		}
		next = static_cast<VkBaseOutStructure*>(next)->pNext;
	}
}

void PhysicalDevice::GetFormatProperties2(VkFormat format, VkFormatProperties2* pFormatProperties)
{
	assert(pFormatProperties->sType == VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2);

	GetFormatProperties(format, &pFormatProperties->formatProperties);

	auto next = pFormatProperties->pNext;
	while (next)
	{
		const auto type = static_cast<const VkBaseInStructure*>(next)->sType;
		switch (type)
		{
		case VK_STRUCTURE_TYPE_DRM_FORMAT_MODIFIER_PROPERTIES_LIST_EXT:
			FATAL_ERROR();

		default:
			break;
		}
		next = static_cast<VkBaseOutStructure*>(next)->pNext;
	}
}

VkResult PhysicalDevice::GetImageFormatProperties2(const VkPhysicalDeviceImageFormatInfo2* pImageFormatInfo, VkImageFormatProperties2* pImageFormatProperties)
{
	assert(pImageFormatInfo->sType == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_FORMAT_INFO_2);
	assert(pImageFormatProperties->sType == VK_STRUCTURE_TYPE_IMAGE_FORMAT_PROPERTIES_2);

	const auto format = pImageFormatInfo->format;
	const auto formatType = pImageFormatInfo->type;
	const auto tiling = pImageFormatInfo->tiling;
	const auto usage = pImageFormatInfo->usage;
	const auto flags = pImageFormatInfo->flags;

	{
		auto next = pImageFormatInfo->pNext;
		while (next)
		{
			const auto type = static_cast<const VkBaseInStructure*>(next)->sType;
			switch (type)
			{
			case VK_STRUCTURE_TYPE_IMAGE_FORMAT_LIST_CREATE_INFO_KHR:
				FATAL_ERROR();

			case VK_STRUCTURE_TYPE_IMAGE_STENCIL_USAGE_CREATE_INFO_EXT:
				FATAL_ERROR();

			case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_IMAGE_FORMAT_INFO:
				{
					const auto info = reinterpret_cast<const VkPhysicalDeviceExternalImageFormatInfo*>(next);
					const auto property = GetStructure<VkExternalImageFormatProperties>(reinterpret_cast<VkBaseOutStructure*>(pImageFormatProperties), VK_STRUCTURE_TYPE_EXTERNAL_IMAGE_FORMAT_PROPERTIES);
					assert(property);

#if defined(VK_KHR_external_memory_win32)
					if (info->handleType == VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT)
					{
						property->externalMemoryProperties.exportFromImportedHandleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;
						property->externalMemoryProperties.compatibleHandleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;
						property->externalMemoryProperties.externalMemoryFeatures = VK_EXTERNAL_MEMORY_FEATURE_EXPORTABLE_BIT | VK_EXTERNAL_MEMORY_FEATURE_IMPORTABLE_BIT;
						break;
					}
#endif

#if defined(VK_KHR_external_memory_fd)
					if (info->handleType == VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT)
					{
						FATAL_ERROR();
					}
#endif
					
					property->externalMemoryProperties.exportFromImportedHandleTypes = 0;
					property->externalMemoryProperties.compatibleHandleTypes = 0;
					property->externalMemoryProperties.externalMemoryFeatures = 0;
					break;
				}

			case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_DRM_FORMAT_MODIFIER_INFO_EXT:
				FATAL_ERROR();

			case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_VIEW_IMAGE_FORMAT_INFO_EXT:
				FATAL_ERROR();

			default:
				break;
			}
			next = static_cast<const VkBaseInStructure*>(next)->pNext;
		}
	}

	return GetImageFormatPropertiesImpl(format, formatType, tiling, usage, flags, &pImageFormatProperties->imageFormatProperties);
}

void PhysicalDevice::GetQueueFamilyProperties2(uint32_t* pQueueFamilyPropertyCount, VkQueueFamilyProperties2* pQueueFamilyProperties)
{
	if (pQueueFamilyProperties)
	{
		if (*pQueueFamilyPropertyCount > 0)
		{
			assert(pQueueFamilyProperties->sType == VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2);
			
			GetQueueFamilyProperties(pQueueFamilyPropertyCount, &pQueueFamilyProperties->queueFamilyProperties);
			
			auto next = pQueueFamilyProperties->pNext;
			while (next)
			{
				const auto type = static_cast<const VkBaseInStructure*>(next)->sType;
				switch (type)
				{
				case VK_STRUCTURE_TYPE_QUEUE_FAMILY_CHECKPOINT_PROPERTIES_NV:
					FATAL_ERROR();

				default:
					break;
				}
				next = static_cast<VkBaseOutStructure*>(next)->pNext;
			}
		}
	}
	else
	{
		*pQueueFamilyPropertyCount = 1;
	}
}

void PhysicalDevice::GetMemoryProperties2(VkPhysicalDeviceMemoryProperties2* pMemoryProperties)
{
	assert(pMemoryProperties->sType == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2);
	
	GetMemoryProperties(&pMemoryProperties->memoryProperties);

	auto next = pMemoryProperties->pNext;
	while (next)
	{
		const auto type = static_cast<const VkBaseInStructure*>(next)->sType;
		switch (type)
		{
		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_BUDGET_PROPERTIES_EXT:
			{
				auto memoryProperties = static_cast<VkPhysicalDeviceMemoryBudgetPropertiesEXT*>(next);
				memoryProperties->heapBudget[0] = 0x100000000; // TODO: Real memory size?
				memoryProperties->heapUsage[0] = 0;
				for (auto i = 1u; i < VK_MAX_MEMORY_HEAPS; i++)
				{
					memoryProperties->heapBudget[i] = 0;
					memoryProperties->heapUsage[i] = 0;
				}
				break;
			}

		default:
			break;
		}
		next = static_cast<VkBaseOutStructure*>(next)->pNext;
	}
}

void PhysicalDevice::GetSparseImageFormatProperties2(const VkPhysicalDeviceSparseImageFormatInfo2* pFormatInfo, uint32_t* pPropertyCount, VkSparseImageFormatProperties2* pProperties)
{
	if (!Platform::SupportsSparse())
	{
		*pPropertyCount = 0;
		return;
	}
	
	if (pProperties)
	{
		if (*pPropertyCount > 0)
		{
			assert(pFormatInfo->sType == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SPARSE_IMAGE_FORMAT_INFO_2);
			assert(pProperties->sType == VK_STRUCTURE_TYPE_SPARSE_IMAGE_FORMAT_PROPERTIES_2);

			const auto format = pFormatInfo->format;
			const auto formatType = pFormatInfo->type;
			const auto samples = pFormatInfo->samples;
			const auto tiling = pFormatInfo->tiling;
			const auto usage = pFormatInfo->usage;

			GetSparseImageFormatPropertiesImpl(format, formatType, samples, usage, tiling, pPropertyCount, &pProperties->properties);
		}
	}
	else
	{
		*pPropertyCount = 1;
	}
}

void PhysicalDevice::GetExternalBufferProperties(const VkPhysicalDeviceExternalBufferInfo* pExternalBufferInfo, VkExternalBufferProperties* pExternalBufferProperties)
{
	assert(pExternalBufferInfo->sType == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_BUFFER_INFO);
	assert(pExternalBufferProperties->sType == VK_STRUCTURE_TYPE_EXTERNAL_BUFFER_PROPERTIES);

#if defined(VK_KHR_external_memory_win32)
	if (pExternalBufferInfo->handleType == VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT)
	{
		pExternalBufferProperties->externalMemoryProperties.exportFromImportedHandleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;
		pExternalBufferProperties->externalMemoryProperties.compatibleHandleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;
		pExternalBufferProperties->externalMemoryProperties.externalMemoryFeatures = VK_EXTERNAL_MEMORY_FEATURE_EXPORTABLE_BIT | VK_EXTERNAL_MEMORY_FEATURE_IMPORTABLE_BIT;
		return;
	}
#endif

#if defined(VK_KHR_external_memory_fd)
	if (pExternalBufferInfo->handleType == VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT)
	{
		FATAL_ERROR();
	}
#endif

	pExternalBufferProperties->externalMemoryProperties.exportFromImportedHandleTypes = 0;
	pExternalBufferProperties->externalMemoryProperties.compatibleHandleTypes = 0;
	pExternalBufferProperties->externalMemoryProperties.externalMemoryFeatures = 0;
}

void PhysicalDevice::GetExternalFenceProperties(const VkPhysicalDeviceExternalFenceInfo* pExternalFenceInfo, VkExternalFenceProperties* pExternalFenceProperties)
{
	assert(pExternalFenceInfo->sType == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_FENCE_INFO);
	assert(pExternalFenceProperties->sType == VK_STRUCTURE_TYPE_EXTERNAL_FENCE_PROPERTIES);

#if defined(VK_KHR_external_fence_win32)
	if (pExternalFenceInfo->handleType == VK_EXTERNAL_FENCE_HANDLE_TYPE_OPAQUE_WIN32_BIT)
	{
		pExternalFenceProperties->exportFromImportedHandleTypes = VK_EXTERNAL_FENCE_HANDLE_TYPE_OPAQUE_WIN32_BIT;
		pExternalFenceProperties->compatibleHandleTypes = VK_EXTERNAL_FENCE_HANDLE_TYPE_OPAQUE_WIN32_BIT;
		pExternalFenceProperties->externalFenceFeatures = VK_EXTERNAL_FENCE_FEATURE_EXPORTABLE_BIT | VK_EXTERNAL_FENCE_FEATURE_IMPORTABLE_BIT;
		return;
	}
#endif

#if defined(VK_KHR_external_fence_fd)
	if (pExternalFenceInfo->handleType == VK_EXTERNAL_FENCE_HANDLE_TYPE_OPAQUE_FD_BIT)
	{
		FATAL_ERROR();
	}
#endif

	pExternalFenceProperties->exportFromImportedHandleTypes = 0;
	pExternalFenceProperties->compatibleHandleTypes = 0;
	pExternalFenceProperties->externalFenceFeatures = 0;
}

void PhysicalDevice::GetExternalSemaphoreProperties(const VkPhysicalDeviceExternalSemaphoreInfo* pExternalSemaphoreInfo, VkExternalSemaphoreProperties* pExternalSemaphoreProperties)
{
	assert(pExternalSemaphoreInfo->sType == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_SEMAPHORE_INFO);
	assert(pExternalSemaphoreProperties->sType == VK_STRUCTURE_TYPE_EXTERNAL_SEMAPHORE_PROPERTIES);
	
#if defined(VK_KHR_external_semaphore_win32)
	if (pExternalSemaphoreInfo->handleType == VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT)
	{
		pExternalSemaphoreProperties->exportFromImportedHandleTypes = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT;
		pExternalSemaphoreProperties->compatibleHandleTypes = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT;
		pExternalSemaphoreProperties->externalSemaphoreFeatures = VK_EXTERNAL_SEMAPHORE_FEATURE_EXPORTABLE_BIT | VK_EXTERNAL_SEMAPHORE_FEATURE_IMPORTABLE_BIT;
		return;
	}
#endif
	
#if defined(VK_KHR_external_semaphore_fd)
	if (pExternalSemaphoreInfo->handleType == VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT || pExternalSemaphoreInfo->handleType == VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_SYNC_FD_BIT)
	{
		FATAL_ERROR();
	}
#endif

	pExternalSemaphoreProperties->exportFromImportedHandleTypes = 0;
	pExternalSemaphoreProperties->compatibleHandleTypes = 0;
	pExternalSemaphoreProperties->externalSemaphoreFeatures = 0;
}

VkResult PhysicalDevice::CreateDevice(const VkDeviceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDevice* pDevice)
{
	return Device::Create(instance, pCreateInfo, pAllocator, pDevice);
}

VkResult PhysicalDevice::EnumerateDeviceExtensionProperties(const char* pLayerName, uint32_t* pPropertyCount, VkExtensionProperties* pProperties)
{
	assert(pLayerName == nullptr);

	if (pProperties)
	{
		const auto toCopy = std::min(*pPropertyCount, GetInitialExtensions().getNumberDeviceExtensions());
		GetInitialExtensions().FillExtensionProperties(pProperties, toCopy, true);

		if (toCopy < GetInitialExtensions().getNumberDeviceExtensions())
		{
			return VK_INCOMPLETE;
		}

		*pPropertyCount = GetInitialExtensions().getNumberDeviceExtensions();
	}
	else
	{
		*pPropertyCount = GetInitialExtensions().getNumberDeviceExtensions();
	}

	return VK_SUCCESS;
}

VkResult PhysicalDevice::GetSurfaceSupport(uint32_t queueFamilyIndex, VkSurfaceKHR surface, VkBool32* pSupported)
{
	*pSupported = true;
	return VK_SUCCESS;
}

VkResult PhysicalDevice::GetSurfaceFormats(VkSurfaceKHR surface, uint32_t* pSurfaceFormatCount, VkSurfaceFormatKHR* pSurfaceFormats)
{
	if (pSurfaceFormats)
	{
		if (*pSurfaceFormatCount == 0)
		{
			return VK_INCOMPLETE;
		}
		
		pSurfaceFormats->format = VK_FORMAT_B8G8R8A8_UNORM;
		pSurfaceFormats->colorSpace = VK_COLOR_SPACE_PASS_THROUGH_EXT;
		*pSurfaceFormatCount = 1;
	}
	else
	{
		*pSurfaceFormatCount = 1;
	}

	return VK_SUCCESS;
}

VkResult PhysicalDevice::GetSurfacePresentModes(VkSurfaceKHR surface, uint32_t* pPresentModeCount, VkPresentModeKHR* pPresentModes)
{
	if (pPresentModes)
	{
		if (*pPresentModeCount == 0)
		{
			return VK_INCOMPLETE;
		}

		*pPresentModes = VK_PRESENT_MODE_FIFO_KHR;
		*pPresentModeCount = 1;
	}
	else
	{
		*pPresentModeCount = 1;
	}

	return VK_SUCCESS;
}

#if defined(VK_KHR_display)
VkResult PhysicalDevice::GetDisplayProperties(uint32_t* pPropertyCount, VkDisplayPropertiesKHR* pProperties)
{
	FATAL_ERROR();
}
#endif

VkResult PhysicalDevice::GetSurfaceCapabilities2KHR(const VkPhysicalDeviceSurfaceInfo2KHR* pSurfaceInfo, VkSurfaceCapabilities2KHR* pSurfaceCapabilities)
{
	assert(pSurfaceInfo->sType == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SURFACE_INFO_2_KHR);

	const auto result = GetSurfaceCapabilities(pSurfaceInfo->surface, &pSurfaceCapabilities->surfaceCapabilities);
	if (result != VK_SUCCESS)
	{
		return result;
	}

	auto next = static_cast<const VkBaseInStructure*>(pSurfaceInfo->pNext);
	while (next)
	{
		switch (next->sType)
		{
		case VK_STRUCTURE_TYPE_SURFACE_FULL_SCREEN_EXCLUSIVE_INFO_EXT:
			FATAL_ERROR();

		case VK_STRUCTURE_TYPE_SURFACE_FULL_SCREEN_EXCLUSIVE_WIN32_INFO_EXT:
			FATAL_ERROR();

		default:
			break;
		}
		next = next->pNext;
	}

	return VK_SUCCESS;
}

VkResult PhysicalDevice::GetSurfaceFormats2(const VkPhysicalDeviceSurfaceInfo2KHR* pSurfaceInfo, uint32_t* pSurfaceFormatCount, VkSurfaceFormat2KHR* pSurfaceFormats)
{
	assert(pSurfaceInfo->sType == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SURFACE_INFO_2_KHR);

	if (pSurfaceFormats)
	{
		if (*pSurfaceFormatCount == 0)
		{
			return VK_INCOMPLETE;
		}

		pSurfaceFormats->surfaceFormat.format = VK_FORMAT_B8G8R8A8_UNORM;
		pSurfaceFormats->surfaceFormat.colorSpace = VK_COLOR_SPACE_PASS_THROUGH_EXT;
		*pSurfaceFormatCount = 1;
	}
	else
	{
		*pSurfaceFormatCount = 1;
	}

	return VK_SUCCESS;
}
