#include "PhysicalDevice.h"

#include "Device.h"
#include "Formats.h"

#define NOMINMAX
#include <Windows.h>
#include <vulkan/vk_icd.h>

#include <algorithm>
#include <cassert>

void PhysicalDevice::GetFeatures(VkPhysicalDeviceFeatures* pFeatures)
{
	pFeatures->robustBufferAccess = true;
	pFeatures->fullDrawIndexUint32 = true;
	pFeatures->imageCubeArray = true;
	pFeatures->independentBlend = true;
	pFeatures->geometryShader = true;
	pFeatures->tessellationShader = true;
	pFeatures->sampleRateShading = true;
	pFeatures->dualSrcBlend = true;
	pFeatures->logicOp = true;
	pFeatures->multiDrawIndirect = true;
	pFeatures->drawIndirectFirstInstance = true;
	pFeatures->depthClamp = true;
	pFeatures->depthBiasClamp = true;
	pFeatures->fillModeNonSolid = true;
	pFeatures->depthBounds = true;
	pFeatures->wideLines = true;
	pFeatures->largePoints = true;
	pFeatures->alphaToOne = true;
	pFeatures->multiViewport = true;
	pFeatures->samplerAnisotropy = true;
	pFeatures->textureCompressionETC2 = true;
	pFeatures->textureCompressionASTC_LDR = true;
	pFeatures->textureCompressionBC = true;
	pFeatures->occlusionQueryPrecise = true;
	pFeatures->pipelineStatisticsQuery = true;
	pFeatures->vertexPipelineStoresAndAtomics = true;
	pFeatures->fragmentStoresAndAtomics = true;
	pFeatures->shaderTessellationAndGeometryPointSize = true;
	pFeatures->shaderImageGatherExtended = true;
	pFeatures->shaderStorageImageExtendedFormats = true;
	pFeatures->shaderStorageImageMultisample = true;
	pFeatures->shaderStorageImageReadWithoutFormat = true;
	pFeatures->shaderStorageImageWriteWithoutFormat = true;
	pFeatures->shaderUniformBufferArrayDynamicIndexing = true;
	pFeatures->shaderSampledImageArrayDynamicIndexing = true;
	pFeatures->shaderStorageBufferArrayDynamicIndexing = true;
	pFeatures->shaderStorageImageArrayDynamicIndexing = true;
	pFeatures->shaderClipDistance = true;
	pFeatures->shaderCullDistance = true;
	pFeatures->shaderFloat64 = true;
	pFeatures->shaderInt64 = true;
	pFeatures->shaderInt16 = true;
	pFeatures->shaderResourceResidency = true;
	pFeatures->shaderResourceMinLod = true;
	pFeatures->sparseBinding = true;
	pFeatures->sparseResidencyBuffer = true;
	pFeatures->sparseResidencyImage2D = true;
	pFeatures->sparseResidencyImage3D = true;
	pFeatures->sparseResidency2Samples = true;
	pFeatures->sparseResidency4Samples = true;
	pFeatures->sparseResidency8Samples = true;
	pFeatures->sparseResidency16Samples = true;
	pFeatures->sparseResidencyAliased = true;
	pFeatures->variableMultisampleRate = true;
	pFeatures->inheritedQueries = true;
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
	switch (type)
	{
	case VK_IMAGE_TYPE_1D:
		pImageFormatProperties->maxExtent.width = 4096;
		pImageFormatProperties->maxExtent.height = 1;
		pImageFormatProperties->maxExtent.depth = 1;
		break;
	case VK_IMAGE_TYPE_2D:
		pImageFormatProperties->maxExtent.width = 4096;
		pImageFormatProperties->maxExtent.height = 4096;
		pImageFormatProperties->maxExtent.depth = 1;
		break;
	case VK_IMAGE_TYPE_3D:
		pImageFormatProperties->maxExtent.width = 256;
		pImageFormatProperties->maxExtent.height = 256;
		pImageFormatProperties->maxExtent.depth = 256;
		break;
	default: FATAL_ERROR();
	}

	pImageFormatProperties->maxMipLevels = 1; // TODO
	pImageFormatProperties->maxArrayLayers = 256; // TODO
	pImageFormatProperties->sampleCounts = VK_SAMPLE_COUNT_1_BIT | VK_SAMPLE_COUNT_2_BIT | VK_SAMPLE_COUNT_4_BIT | VK_SAMPLE_COUNT_8_BIT | VK_SAMPLE_COUNT_16_BIT | VK_SAMPLE_COUNT_32_BIT | VK_SAMPLE_COUNT_64_BIT; // TODO
	pImageFormatProperties->maxResourceSize = 0x100000000;

	return VK_SUCCESS;
}

void PhysicalDevice::GetProperties(VkPhysicalDeviceProperties* pProperties)
{
	const auto ULP = 1.0f / (2 << 4);
	
	pProperties->apiVersion = LATEST_VERSION;
	pProperties->driverVersion = VK_MAKE_VERSION(1, 0, 0);
	pProperties->vendorID = 0x10000;
	pProperties->deviceID = 0;
	pProperties->deviceType = VK_PHYSICAL_DEVICE_TYPE_CPU;
	strcpy_s(pProperties->deviceName, "CPVulkan");
	memset(pProperties->pipelineCacheUUID, 0, VK_UUID_SIZE);
	pProperties->limits.maxImageDimension1D = 4096;
	pProperties->limits.maxImageDimension2D = 4096;
	pProperties->limits.maxImageDimension3D = 256;
	pProperties->limits.maxImageDimensionCube = 4096;
	pProperties->limits.maxImageArrayLayers = 256;
	pProperties->limits.maxTexelBufferElements = 65536;
	pProperties->limits.maxUniformBufferRange = 16384;
	pProperties->limits.maxStorageBufferRange = 134217728;
	pProperties->limits.maxPushConstantsSize = 128;
	pProperties->limits.maxMemoryAllocationCount = 4096;
	pProperties->limits.maxSamplerAllocationCount = 4000;
	pProperties->limits.bufferImageGranularity = 131072;
	pProperties->limits.sparseAddressSpaceSize = 0x100000000;
	pProperties->limits.maxBoundDescriptorSets = 4;
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
	pProperties->limits.maxVertexInputBindings = 16;
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
	pProperties->limits.maxFragmentOutputAttachments = 4;
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
	pProperties->limits.maxViewports = 16;
	pProperties->limits.maxViewportDimensions[0] = 4096;
	pProperties->limits.maxViewportDimensions[1] = 4096;
	pProperties->limits.viewportBoundsRange[0] = -8192;
	pProperties->limits.viewportBoundsRange[1] = 8192;
	pProperties->limits.viewportSubPixelBits = 0;
	pProperties->limits.minMemoryMapAlignment = 64;
	pProperties->limits.minTexelBufferOffsetAlignment = 256;
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
	pProperties->limits.maxColorAttachments = 4;
	pProperties->limits.sampledImageColorSampleCounts = VK_SAMPLE_COUNT_1_BIT | VK_SAMPLE_COUNT_2_BIT | VK_SAMPLE_COUNT_4_BIT | VK_SAMPLE_COUNT_8_BIT | VK_SAMPLE_COUNT_16_BIT | VK_SAMPLE_COUNT_32_BIT | VK_SAMPLE_COUNT_64_BIT;
	pProperties->limits.sampledImageIntegerSampleCounts = VK_SAMPLE_COUNT_1_BIT | VK_SAMPLE_COUNT_2_BIT | VK_SAMPLE_COUNT_4_BIT | VK_SAMPLE_COUNT_8_BIT | VK_SAMPLE_COUNT_16_BIT | VK_SAMPLE_COUNT_32_BIT | VK_SAMPLE_COUNT_64_BIT;
	pProperties->limits.sampledImageDepthSampleCounts = VK_SAMPLE_COUNT_1_BIT | VK_SAMPLE_COUNT_2_BIT | VK_SAMPLE_COUNT_4_BIT | VK_SAMPLE_COUNT_8_BIT | VK_SAMPLE_COUNT_16_BIT | VK_SAMPLE_COUNT_32_BIT | VK_SAMPLE_COUNT_64_BIT;
	pProperties->limits.sampledImageStencilSampleCounts = VK_SAMPLE_COUNT_1_BIT | VK_SAMPLE_COUNT_2_BIT | VK_SAMPLE_COUNT_4_BIT | VK_SAMPLE_COUNT_8_BIT | VK_SAMPLE_COUNT_16_BIT | VK_SAMPLE_COUNT_32_BIT | VK_SAMPLE_COUNT_64_BIT;
	pProperties->limits.storageImageSampleCounts = VK_SAMPLE_COUNT_1_BIT | VK_SAMPLE_COUNT_2_BIT | VK_SAMPLE_COUNT_4_BIT | VK_SAMPLE_COUNT_8_BIT | VK_SAMPLE_COUNT_16_BIT | VK_SAMPLE_COUNT_32_BIT | VK_SAMPLE_COUNT_64_BIT;
	pProperties->limits.maxSampleMaskWords = 1;
	pProperties->limits.timestampComputeAndGraphics = 0;
	pProperties->limits.timestampPeriod = 0;
	pProperties->limits.maxClipDistances = 8;
	pProperties->limits.maxCullDistances = 8;
	pProperties->limits.maxCombinedClipAndCullDistances = 8;
	pProperties->limits.discreteQueuePriorities = 2;
	pProperties->limits.pointSizeRange[0] = 1.0;
	pProperties->limits.pointSizeRange[1] = 64.0f - ULP;
	pProperties->limits.lineWidthRange[0] = 1;
	pProperties->limits.lineWidthRange[1] = 8.0f - ULP;
	pProperties->limits.pointSizeGranularity = 1;
	pProperties->limits.lineWidthGranularity = 1;
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
	VkQueueFamilyProperties queueFamilyProperties{};
	queueFamilyProperties.queueFlags = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT | VK_QUEUE_PROTECTED_BIT | VK_QUEUE_SPARSE_BINDING_BIT;
	queueFamilyProperties.queueCount = 1;
	HandleEnumeration(pQueueFamilyPropertyCount, pQueueFamilyProperties, std::vector<VkQueueFamilyProperties>
	{
		queueFamilyProperties
	});
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

void PhysicalDevice::GetSparseImageFormatProperties(VkFormat format, VkImageType type,
                                                    VkSampleCountFlagBits samples, VkImageUsageFlags usage, VkImageTiling tiling, uint32_t* pPropertyCount,
                                                    VkSparseImageFormatProperties* pProperties)
{
	FATAL_ERROR();
}

void PhysicalDevice::GetFeatures2(VkPhysicalDeviceFeatures2* pFeatures)
{
	assert(pFeatures->sType == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2);

	GetFeatures(&pFeatures->features);
	
	auto next = pFeatures->pNext;
	while (next)
	{
		switch (*static_cast<VkStructureType*>(next))
		{
		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VARIABLE_POINTERS_FEATURES:
			{
				const auto features = static_cast<VkPhysicalDeviceVariablePointersFeatures*>(next);
				features->variablePointersStorageBuffer = true;
				features->variablePointers = true;
				next = features->pNext;
				break;
			}
		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_FEATURES:
			{
				const auto features = static_cast<VkPhysicalDeviceMultiviewFeatures*>(next);
				features->multiview = true;
				features->multiviewGeometryShader = true;
				features->multiviewTessellationShader = true;
				next = features->pNext;
				break;
			}
		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DRAW_PARAMETERS_FEATURES:
			{
				const auto features = static_cast<VkPhysicalDeviceShaderDrawParametersFeatures*>(next);
				features->shaderDrawParameters = true;
				next = features->pNext;
				break;
			}
		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_16BIT_STORAGE_FEATURES:
			{
				const auto features = static_cast<VkPhysicalDevice16BitStorageFeatures*>(next);
				features->storageBuffer16BitAccess = true;
				features->uniformAndStorageBuffer16BitAccess = true;
				features->storagePushConstant16 = true;
				features->storageInputOutput16 = true;
				next = features->pNext;
				break;
			}
		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROTECTED_MEMORY_FEATURES:
			{
				const auto features = static_cast<VkPhysicalDeviceProtectedMemoryFeatures*>(next);
				features->protectedMemory = true;
				next = features->pNext;
				break;
			}
		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SAMPLER_YCBCR_CONVERSION_FEATURES:
			{
				const auto features = static_cast<VkPhysicalDeviceSamplerYcbcrConversionFeatures*>(next);
				features->samplerYcbcrConversion = true;
				next = features->pNext;
				break;
			}
		default:
			FATAL_ERROR();
		}
	}
}

void PhysicalDevice::GetProperties2(VkPhysicalDeviceProperties2* pProperties)
{
	assert(pProperties->sType == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2);

	GetProperties(&pProperties->properties);

	auto next = pProperties->pNext;
	while (next)
	{
		switch (*static_cast<VkStructureType*>(next))
		{
		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRIVER_PROPERTIES_KHR:
			{
				const auto properties = static_cast<VkPhysicalDeviceDriverPropertiesKHR*>(next);
				properties->driverID = static_cast<VkDriverIdKHR>(0x10000);
				strcpy_s(properties->driverName, "CPVulkan");
				strcpy_s(properties->driverInfo, "Some CPU Driver");
				properties->conformanceVersion.major = 1;
				properties->conformanceVersion.minor = 1;
				properties->conformanceVersion.subminor = 5;
				properties->conformanceVersion.patch = 0;
				next = properties->pNext;
				break;
			}
		default:
			FATAL_ERROR();
		}
	}
}

void PhysicalDevice::GetFormatProperties2(VkFormat format, VkFormatProperties2* pFormatProperties)
{
	FATAL_ERROR();
}

VkResult PhysicalDevice::GetImageFormatProperties2(const VkPhysicalDeviceImageFormatInfo2* pImageFormatInfo, VkImageFormatProperties2* pImageFormatProperties)
{
	FATAL_ERROR();
}

void PhysicalDevice::GetQueueFamilyProperties2(uint32_t* pQueueFamilyPropertyCount, VkQueueFamilyProperties2* pQueueFamilyProperties)
{
	FATAL_ERROR();
}

void PhysicalDevice::GetMemoryProperties2(VkPhysicalDeviceMemoryProperties2* pMemoryProperties)
{
	assert(pMemoryProperties->sType == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2);
	
	GetMemoryProperties(&pMemoryProperties->memoryProperties);

	auto next = pMemoryProperties->pNext;
	while (next)
	{
		switch (*static_cast<VkStructureType*>(next))
		{
		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_BUDGET_PROPERTIES_EXT:
			{
				auto memoryProperties = static_cast<VkPhysicalDeviceMemoryBudgetPropertiesEXT*>(next);
				memoryProperties->heapBudget[0] = 0x100000000;
				memoryProperties->heapUsage[0] = 0;
				for (auto i = 1u; i < VK_MAX_MEMORY_HEAPS; i++)
				{
					memoryProperties->heapBudget[i] = 0;
					memoryProperties->heapUsage[i] = 0;
				}
				next = memoryProperties->pNext;
				break;
			}
		default:
			FATAL_ERROR();
		}
	}
}

void PhysicalDevice::GetSparseImageFormatProperties2(
	const VkPhysicalDeviceSparseImageFormatInfo2* pFormatInfo, uint32_t* pPropertyCount,
	VkSparseImageFormatProperties2* pProperties)
{
	FATAL_ERROR();
}

void PhysicalDevice::GetExternalBufferProperties(
	const VkPhysicalDeviceExternalBufferInfo* pExternalBufferInfo,
	VkExternalBufferProperties* pExternalBufferProperties)
{
	FATAL_ERROR();
}

void PhysicalDevice::GetExternalFenceProperties(
	const VkPhysicalDeviceExternalFenceInfo* pExternalFenceInfo, VkExternalFenceProperties* pExternalFenceProperties)
{
	FATAL_ERROR();
}

void PhysicalDevice::GetExternalSemaphoreProperties(
	const VkPhysicalDeviceExternalSemaphoreInfo* pExternalSemaphoreInfo,
	VkExternalSemaphoreProperties* pExternalSemaphoreProperties)
{
	FATAL_ERROR();
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

VkResult PhysicalDevice::GetSurfaceCapabilities(VkSurfaceKHR surface, VkSurfaceCapabilitiesKHR* pSurfaceCapabilities)
{
	const auto win32Surface = reinterpret_cast<VkIcdSurfaceWin32*>(surface);
	RECT rect;
	assert(GetClientRect(win32Surface->hwnd, &rect));
	
	pSurfaceCapabilities->minImageCount = 1;
	pSurfaceCapabilities->maxImageCount = 0;
	pSurfaceCapabilities->currentExtent.width = rect.right - rect.left;
	pSurfaceCapabilities->currentExtent.height = rect.bottom - rect.top;
	pSurfaceCapabilities->minImageExtent.width = 1;
	pSurfaceCapabilities->minImageExtent.height = 1;
	pSurfaceCapabilities->maxImageExtent.width = 8192;
	pSurfaceCapabilities->maxImageExtent.height = 8192;
	pSurfaceCapabilities->maxImageArrayLayers = 1;
	pSurfaceCapabilities->supportedTransforms = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	pSurfaceCapabilities->currentTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	pSurfaceCapabilities->supportedCompositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	pSurfaceCapabilities->supportedUsageFlags = VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
		VK_IMAGE_USAGE_TRANSFER_DST_BIT |
		VK_IMAGE_USAGE_SAMPLED_BIT |
		VK_IMAGE_USAGE_STORAGE_BIT |
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
		VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT |
		VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT |
		VK_IMAGE_USAGE_SHADING_RATE_IMAGE_BIT_NV |
		VK_IMAGE_USAGE_FRAGMENT_DENSITY_MAP_BIT_EXT;
	return VK_SUCCESS;
}

VkResult PhysicalDevice::GetSurfaceFormats(VkSurfaceKHR surface, uint32_t* pSurfaceFormatCount, VkSurfaceFormatKHR* pSurfaceFormats)
{
	if (pSurfaceFormats)
	{
		if (*pSurfaceFormatCount != 1)
		{
			FATAL_ERROR();
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
		if (*pPresentModeCount != 1)
		{
			FATAL_ERROR();
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
