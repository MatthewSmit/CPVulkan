#include "Device.h"

#include "DeviceState.h"
#include "Extensions.h"
#include "GlslFunctions.h"
#include "Instance.h"
#include "Queue.h"
#include "Util.h"

#include <Converter.h>

#include <cassert>

Device::Device() noexcept :
	state{std::make_unique<DeviceState>()}
{
	state->jit = new SpirvJit();
	AddGlslFunctions(state->jit);
}

Device::~Device()
{
	delete state->jit;
	for (auto& queue : queues)
	{
		const auto queuePtr = queue.release();
		Free(queuePtr, nullptr);
	}
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
	
	auto next = pAllocateInfo->pNext;
	while (next)
	{
		const auto type = *static_cast<const VkStructureType*>(next);
		switch (type)
		{
		case VK_STRUCTURE_TYPE_DEDICATED_ALLOCATION_MEMORY_ALLOCATE_INFO_NV:
			FATAL_ERROR();

		case VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO:
			FATAL_ERROR();

		case VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO_NV:
			FATAL_ERROR();

		case VK_STRUCTURE_TYPE_EXPORT_MEMORY_WIN32_HANDLE_INFO_KHR:
			FATAL_ERROR();

		case VK_STRUCTURE_TYPE_EXPORT_MEMORY_WIN32_HANDLE_INFO_NV:
			FATAL_ERROR();

		case VK_STRUCTURE_TYPE_IMPORT_ANDROID_HARDWARE_BUFFER_INFO_ANDROID:
			FATAL_ERROR();

		case VK_STRUCTURE_TYPE_IMPORT_MEMORY_FD_INFO_KHR:
			FATAL_ERROR();

		case VK_STRUCTURE_TYPE_IMPORT_MEMORY_HOST_POINTER_INFO_EXT:
			FATAL_ERROR();

		case VK_STRUCTURE_TYPE_IMPORT_MEMORY_WIN32_HANDLE_INFO_KHR:
			FATAL_ERROR();

		case VK_STRUCTURE_TYPE_IMPORT_MEMORY_WIN32_HANDLE_INFO_NV:
			FATAL_ERROR();

		case VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO:
			FATAL_ERROR();

		case VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO:
			{
				const auto allocateInfo = static_cast<const VkMemoryDedicatedAllocateInfo*>(next);
				(void)allocateInfo;
				break;
			}
			
		case VK_STRUCTURE_TYPE_MEMORY_PRIORITY_ALLOCATE_INFO_EXT:
			FATAL_ERROR();
		}
		next = static_cast<const VkBaseInStructure*>(next)->pNext;
	}

	if (pAllocateInfo->memoryTypeIndex != 0)
	{
		FATAL_ERROR();
	}

	WrapVulkan(AllocateSized(pAllocator, pAllocateInfo->allocationSize, VK_SYSTEM_ALLOCATION_SCOPE_DEVICE), pMemory);

	return VK_SUCCESS;
}

void Device::FreeMemory(VkDeviceMemory memory, const VkAllocationCallbacks* pAllocator)
{
	FreeSized(UnwrapVulkan<DeviceMemory>(memory), pAllocator);
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

	auto device = Allocate<Device>(pAllocator, VK_SYSTEM_ALLOCATION_SCOPE_DEVICE);
	if (!device)
	{
		return VK_ERROR_OUT_OF_HOST_MEMORY;
	}

	auto next = pCreateInfo->pNext;
	while (next)
	{
		const auto type = static_cast<const VkBaseInStructure*>(next)->sType;
		switch (type)
		{
		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_16BIT_STORAGE_FEATURES:
			{
				const auto features = static_cast<const VkPhysicalDevice16BitStorageFeatures*>(next);
				// TODO
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_8BIT_STORAGE_FEATURES_KHR:
			{
				const auto features = static_cast<const VkPhysicalDevice8BitStorageFeaturesKHR*>(next);
				// TODO
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ASTC_DECODE_FEATURES_EXT:
			FATAL_ERROR();

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BLEND_OPERATION_ADVANCED_FEATURES_EXT:
			FATAL_ERROR();

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES_EXT:
			FATAL_ERROR();

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COHERENT_MEMORY_FEATURES_AMD:
			FATAL_ERROR();

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COMPUTE_SHADER_DERIVATIVES_FEATURES_NV:
			FATAL_ERROR();

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CONDITIONAL_RENDERING_FEATURES_EXT:
			{
				const auto features = static_cast<const VkPhysicalDeviceConditionalRenderingFeaturesEXT*>(next);
				// TODO
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COOPERATIVE_MATRIX_FEATURES_NV:
			FATAL_ERROR();

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CORNER_SAMPLED_IMAGE_FEATURES_NV:
			FATAL_ERROR();

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COVERAGE_REDUCTION_MODE_FEATURES_NV:
			FATAL_ERROR();

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEDICATED_ALLOCATION_IMAGE_ALIASING_FEATURES_NV:
			FATAL_ERROR();

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEPTH_CLIP_ENABLE_FEATURES_EXT:
			FATAL_ERROR();

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT:
			{
				const auto features = static_cast<const VkPhysicalDeviceDescriptorIndexingFeaturesEXT*>(next);
				// TODO
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXCLUSIVE_SCISSOR_FEATURES_NV:
			{
				const auto features = static_cast<const VkPhysicalDeviceExclusiveScissorFeaturesNV*>(next);
				// TODO
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_DENSITY_MAP_FEATURES_EXT:
			{
				const auto features = static_cast<const VkPhysicalDeviceFragmentDensityMapFeaturesEXT*>(next);
				// TODO
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADER_BARYCENTRIC_FEATURES_NV:
			{
				const auto features = static_cast<const VkPhysicalDeviceFragmentShaderBarycentricFeaturesNV*>(next);
				// TODO
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADER_INTERLOCK_FEATURES_EXT:
			{
				const auto features = static_cast<const VkPhysicalDeviceFragmentShaderInterlockFeaturesEXT*>(next);
				// TODO
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_HOST_QUERY_RESET_FEATURES_EXT:
			{
				const auto features = static_cast<const VkPhysicalDeviceHostQueryResetFeaturesEXT*>(next);
				// TODO
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGELESS_FRAMEBUFFER_FEATURES_KHR:
			{
				const auto features = static_cast<const VkPhysicalDeviceImagelessFramebufferFeaturesKHR*>(next);
				// TODO
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_INDEX_TYPE_UINT8_FEATURES_EXT:
			{
				const auto features = static_cast<const VkPhysicalDeviceIndexTypeUint8FeaturesEXT*>(next);
				// TODO
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_INLINE_UNIFORM_BLOCK_FEATURES_EXT:
			{
				const auto features = static_cast<const VkPhysicalDeviceInlineUniformBlockFeaturesEXT*>(next);
				// TODO
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_LINE_RASTERIZATION_FEATURES_EXT:
			FATAL_ERROR();

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PRIORITY_FEATURES_EXT:
			FATAL_ERROR();

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_NV:
			FATAL_ERROR();

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_FEATURES:
			{
				const auto features = static_cast<const VkPhysicalDeviceMultiviewFeatures*>(next);
				// TODO
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PIPELINE_EXECUTABLE_PROPERTIES_FEATURES_KHR:
			{
				const auto features = static_cast<const VkPhysicalDevicePipelineExecutablePropertiesFeaturesKHR*>(next);
				// TODO
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROTECTED_MEMORY_FEATURES:
			{
				const auto features = static_cast<const VkPhysicalDeviceProtectedMemoryFeatures*>(next);
				// TODO
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_REPRESENTATIVE_FRAGMENT_TEST_FEATURES_NV:
			FATAL_ERROR();

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SAMPLER_YCBCR_CONVERSION_FEATURES:
			{
				const auto features = static_cast<const VkPhysicalDeviceSamplerYcbcrConversionFeatures*>(next);
				// TODO
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SCALAR_BLOCK_LAYOUT_FEATURES_EXT:
			{
				const auto features = static_cast<const VkPhysicalDeviceScalarBlockLayoutFeaturesEXT*>(next);
				// TODO
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ATOMIC_INT64_FEATURES_KHR:
			{
				const auto features = static_cast<const VkPhysicalDeviceShaderAtomicInt64FeaturesKHR*>(next);
				// TODO
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DEMOTE_TO_HELPER_INVOCATION_FEATURES_EXT:
			FATAL_ERROR();

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DRAW_PARAMETERS_FEATURES:
			{
				const auto features = static_cast<const VkPhysicalDeviceShaderDrawParametersFeatures*>(next);
				// TODO
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_FLOAT16_INT8_FEATURES_KHR:
			{
				const auto features = static_cast<const VkPhysicalDeviceShaderFloat16Int8FeaturesKHR*>(next);
				// TODO
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_IMAGE_FOOTPRINT_FEATURES_NV:
			FATAL_ERROR();

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_INTEGER_FUNCTIONS_2_FEATURES_INTEL:
			FATAL_ERROR();

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_SM_BUILTINS_FEATURES_NV:
			FATAL_ERROR();

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADING_RATE_IMAGE_FEATURES_NV:
			FATAL_ERROR();

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_SIZE_CONTROL_FEATURES_EXT:
			FATAL_ERROR();

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TEXEL_BUFFER_ALIGNMENT_FEATURES_EXT:
			FATAL_ERROR();

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TEXTURE_COMPRESSION_ASTC_HDR_FEATURES_EXT:
			FATAL_ERROR();

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TRANSFORM_FEEDBACK_FEATURES_EXT:
			FATAL_ERROR();

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_UNIFORM_BUFFER_STANDARD_LAYOUT_FEATURES_KHR:
			{
				const auto features = static_cast<const VkPhysicalDeviceUniformBufferStandardLayoutFeaturesKHR*>(next);
				// TODO
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VARIABLE_POINTERS_FEATURES:
			{
				const auto features = static_cast<const VkPhysicalDeviceVariablePointersFeatures*>(next);
				// TODO
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VERTEX_ATTRIBUTE_DIVISOR_FEATURES_EXT:
			FATAL_ERROR();

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_MEMORY_MODEL_FEATURES_KHR:
			{
				const auto features = static_cast<const VkPhysicalDeviceVulkanMemoryModelFeaturesKHR*>(next);
				// TODO
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_YCBCR_IMAGE_ARRAYS_FEATURES_EXT:
			FATAL_ERROR();
		}
		next = static_cast<const VkPhysicalDeviceFeatures2*>(next)->pNext;
	}

	for (auto i = 0u; i < pCreateInfo->queueCreateInfoCount; i++)
	{
		device->queues.push_back(std::unique_ptr<Queue>{Queue::Create(&pCreateInfo->pQueueCreateInfos[i], nullptr)});
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
		FATAL_ERROR();
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
	FATAL_ERROR();
}

void Device::DestroySamplerYcbcrConversion(VkSamplerYcbcrConversion ycbcrConversion, const VkAllocationCallbacks* pAllocator)
{
	FATAL_ERROR();
}

VkResult Device::CreateDescriptorUpdateTemplate(const VkDescriptorUpdateTemplateCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDescriptorUpdateTemplate* pDescriptorUpdateTemplate)
{
	FATAL_ERROR();
}

void Device::DestroyDescriptorUpdateTemplate(VkDescriptorUpdateTemplate descriptorUpdateTemplate, const VkAllocationCallbacks* pAllocator)
{
	FATAL_ERROR();
}

void Device::UpdateDescriptorSetWithTemplate(VkDescriptorSet descriptorSet, VkDescriptorUpdateTemplate descriptorUpdateTemplate, const void* pData)
{
	FATAL_ERROR();
}

void Device::GetDescriptorSetLayoutSupport(const VkDescriptorSetLayoutCreateInfo* pCreateInfo, VkDescriptorSetLayoutSupport* pSupport)
{
	FATAL_ERROR();
}

void Device::GetDeviceGroupPeerMemoryFeatures(uint32_t heapIndex, uint32_t localDeviceIndex, uint32_t remoteDeviceIndex, VkPeerMemoryFeatureFlags* pPeerMemoryFeatures)
{
	FATAL_ERROR();
}

void Device::GetImageMemoryRequirements2(const VkImageMemoryRequirementsInfo2* pInfo, VkMemoryRequirements2* pMemoryRequirements)
{
	FATAL_ERROR();
}

void Device::GetImageSparseMemoryRequirements2(const VkImageSparseMemoryRequirementsInfo2* pInfo, uint32_t* pSparseMemoryRequirementCount, VkSparseImageMemoryRequirements2* pSparseMemoryRequirements)
{
	FATAL_ERROR();
}

VkResult Device::GetDeviceGroupPresentCapabilities(VkDeviceGroupPresentCapabilitiesKHR* pDeviceGroupPresentCapabilities)
{
	FATAL_ERROR();
}

VkResult Device::GetDeviceGroupSurfacePresentModes(VkSurfaceKHR surface, VkDeviceGroupPresentModeFlagsKHR* pModes)
{
	FATAL_ERROR();
}

VkResult Device::AcquireNextImage2(const VkAcquireNextImageInfoKHR* pAcquireInfo, uint32_t* pImageIndex)
{
	FATAL_ERROR();
}

VkResult Device::CreateSharedSwapchains(uint32_t swapchainCount, const VkSwapchainCreateInfoKHR* pCreateInfos, const VkAllocationCallbacks* pAllocator, VkSwapchainKHR* pSwapchains)
{
	FATAL_ERROR();
}

VkResult Device::GetMemoryFd(const VkMemoryGetFdInfoKHR* pGetFdInfo, int* pFd)
{
	FATAL_ERROR();
}

VkResult Device::GetMemoryFdProperties(VkExternalMemoryHandleTypeFlagBits handleType, int fd, VkMemoryFdPropertiesKHR* pMemoryFdProperties)
{
	FATAL_ERROR();
}

VkResult Device::GetSwapchainStatus(VkSwapchainKHR swapchain)
{
	FATAL_ERROR();
}

VkResult Device::GetPipelineExecutableProperties(const VkPipelineInfoKHR* pPipelineInfo, uint32_t* pExecutableCount, VkPipelineExecutablePropertiesKHR* pProperties)
{
	FATAL_ERROR();
}

VkResult Device::GetPipelineExecutableStatistics(const VkPipelineExecutableInfoKHR* pExecutableInfo, uint32_t* pStatisticCount, VkPipelineExecutableStatisticKHR* pStatistics)
{
	FATAL_ERROR();
}

VkResult Device::GetPipelineExecutableInternalRepresentations(const VkPipelineExecutableInfoKHR* pExecutableInfo, uint32_t* pInternalRepresentationCount, VkPipelineExecutableInternalRepresentationKHR* pInternalRepresentations)
{
	FATAL_ERROR();
}

VkResult Device::DebugMarkerSetObjectTag(const VkDebugMarkerObjectTagInfoEXT* pTagInfo)
{
	FATAL_ERROR();
}

VkResult Device::DebugMarkerSetObjectName(const VkDebugMarkerObjectNameInfoEXT* pNameInfo)
{
	FATAL_ERROR();
}

uint32_t Device::GetImageViewHandleNVX(const VkImageViewHandleInfoNVX* pInfo)
{
	FATAL_ERROR();
}

VkResult Device::GetShaderInfoAMD(VkPipeline pipeline, VkShaderStageFlagBits shaderStage, VkShaderInfoTypeAMD infoType, size_t* pInfoSize, void* pInfo)
{
	FATAL_ERROR();
}

VkResult Device::CreateIndirectCommandsLayoutNVX(const VkIndirectCommandsLayoutCreateInfoNVX* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkIndirectCommandsLayoutNVX* pIndirectCommandsLayout)
{
	FATAL_ERROR();
}

void Device::DestroyIndirectCommandsLayoutNVX(VkIndirectCommandsLayoutNVX indirectCommandsLayout, const VkAllocationCallbacks* pAllocator)
{
	FATAL_ERROR();
}

VkResult Device::CreateObjectTableNVX(const VkObjectTableCreateInfoNVX* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkObjectTableNVX* pObjectTable)
{
	FATAL_ERROR();
}

void Device::DestroyObjectTableNVX(VkObjectTableNVX objectTable, const VkAllocationCallbacks* pAllocator)
{
	FATAL_ERROR();
}

VkResult Device::RegisterObjectsNVX(VkObjectTableNVX objectTable, uint32_t objectCount, const VkObjectTableEntryNVX* const* ppObjectTableEntries, const uint32_t* pObjectIndices)
{
	FATAL_ERROR();
}

VkResult Device::UnregisterObjectsNVX(VkObjectTableNVX objectTable, uint32_t objectCount, const VkObjectEntryTypeNVX* pObjectEntryTypes, const uint32_t* pObjectIndices)
{
	FATAL_ERROR();
}

VkResult Device::DisplayPowerControl(VkDisplayKHR display, const VkDisplayPowerInfoEXT* pDisplayPowerInfo)
{
	FATAL_ERROR();
}

VkResult Device::RegisterDeviceEvent(const VkDeviceEventInfoEXT* pDeviceEventInfo, const VkAllocationCallbacks* pAllocator, VkFence* pFence)
{
	FATAL_ERROR();
}

VkResult Device::RegisterDisplayEvent(VkDisplayKHR display, const VkDisplayEventInfoEXT* pDisplayEventInfo, const VkAllocationCallbacks* pAllocator, VkFence* pFence)
{
	FATAL_ERROR();
}

VkResult Device::GetSwapchainCounter(VkSwapchainKHR swapchain, VkSurfaceCounterFlagBitsEXT counter, uint64_t* pCounterValue)
{
	FATAL_ERROR();
}

VkResult Device::GetRefreshCycleDurationGOOGLE(VkSwapchainKHR swapchain, VkRefreshCycleDurationGOOGLE* pDisplayTimingProperties)
{
	FATAL_ERROR();
}

VkResult Device::GetPastPresentationTimingGOOGLE(VkSwapchainKHR swapchain, uint32_t* pPresentationTimingCount, VkPastPresentationTimingGOOGLE* pPresentationTimings)
{
	FATAL_ERROR();
}

void Device::SetHdrMetadata(uint32_t swapchainCount, const VkSwapchainKHR* pSwapchains, const VkHdrMetadataEXT* pMetadata)
{
	FATAL_ERROR();
}

VkResult Device::SetDebugUtilsObjectName(const VkDebugUtilsObjectNameInfoEXT* pNameInfo)
{
	FATAL_ERROR();
}

VkResult Device::SetDebugUtilsObjectTag(const VkDebugUtilsObjectTagInfoEXT* pTagInfo)
{
	FATAL_ERROR();
}

VkResult Device::GetImageDrmFormatModifierProperties(VkImage image, VkImageDrmFormatModifierPropertiesEXT* pProperties)
{
	FATAL_ERROR();
}

VkResult Device::CreateValidationCache(const VkValidationCacheCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkValidationCacheEXT* pValidationCache)
{
	FATAL_ERROR();
}

void Device::DestroyValidationCache(VkValidationCacheEXT validationCache, const VkAllocationCallbacks* pAllocator)
{
	FATAL_ERROR();
}

VkResult Device::MergeValidationCaches(VkValidationCacheEXT dstCache, uint32_t srcCacheCount, const VkValidationCacheEXT* pSrcCaches)
{
	FATAL_ERROR();
}

VkResult Device::GetValidationCacheData(VkValidationCacheEXT validationCache, size_t* pDataSize, void* pData)
{
	FATAL_ERROR();
}

VkResult Device::CreateAccelerationStructureNV(const VkAccelerationStructureCreateInfoNV* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkAccelerationStructureNV* pAccelerationStructure)
{
	FATAL_ERROR();
}

void Device::DestroyAccelerationStructureNV(VkAccelerationStructureNV accelerationStructure, const VkAllocationCallbacks* pAllocator)
{
	FATAL_ERROR();
}

void Device::GetAccelerationStructureMemoryRequirementsNV(const VkAccelerationStructureMemoryRequirementsInfoNV* pInfo, VkMemoryRequirements2KHR* pMemoryRequirements)
{
	FATAL_ERROR();
}

VkResult Device::BindAccelerationStructureMemoryNV(uint32_t bindInfoCount, const VkBindAccelerationStructureMemoryInfoNV* pBindInfos)
{
	FATAL_ERROR();
}

VkResult Device::CreateRayTracingPipelinesNV(VkPipelineCache pipelineCache, uint32_t createInfoCount, const VkRayTracingPipelineCreateInfoNV* pCreateInfos, const VkAllocationCallbacks* pAllocator, VkPipeline* pPipelines)
{
	FATAL_ERROR();
}

VkResult Device::GetRayTracingShaderGroupHandlesNV(VkPipeline pipeline, uint32_t firstGroup, uint32_t groupCount, size_t dataSize, void* pData)
{
	FATAL_ERROR();
}

VkResult Device::GetAccelerationStructureHandleNV(VkAccelerationStructureNV accelerationStructure, size_t dataSize, void* pData)
{
	FATAL_ERROR();
}

VkResult Device::CompileDeferredNV(VkPipeline pipeline, uint32_t shader)
{
	FATAL_ERROR();
}

VkResult Device::GetMemoryHostPointerProperties(VkExternalMemoryHandleTypeFlagBits handleType, const void* pHostPointer, VkMemoryHostPointerPropertiesEXT* pMemoryHostPointerProperties)
{
	FATAL_ERROR();
}

VkResult Device::GetCalibratedTimestamps(uint32_t timestampCount, const VkCalibratedTimestampInfoEXT* pTimestampInfos, uint64_t* pTimestamps, uint64_t* pMaxDeviation)
{
	FATAL_ERROR();
}

VkResult Device::InitializePerformanceApiINTEL(const VkInitializePerformanceApiInfoINTEL* pInitializeInfo)
{
	FATAL_ERROR();
}

void Device::UninitializePerformanceApiINTEL()
{
	FATAL_ERROR();
}

VkResult Device::AcquirePerformanceConfigurationINTEL(const VkPerformanceConfigurationAcquireInfoINTEL* pAcquireInfo, VkPerformanceConfigurationINTEL* pConfiguration)
{
	FATAL_ERROR();
}

VkResult Device::ReleasePerformanceConfigurationINTEL(VkPerformanceConfigurationINTEL configuration)
{
	FATAL_ERROR();
}

VkResult Device::GetPerformanceParameterINTEL(VkPerformanceParameterTypeINTEL parameter, VkPerformanceValueINTEL* pValue)
{
	FATAL_ERROR();
}

void Device::SetLocalDimmingAMD(VkSwapchainKHR swapChain, VkBool32 localDimmingEnable)
{
	FATAL_ERROR();
}

VkDeviceAddress Device::GetBufferDeviceAddress(const VkBufferDeviceAddressInfoEXT* pInfo)
{
	FATAL_ERROR();
}

void Device::ResetQueryPool(VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount)
{
	FATAL_ERROR();
}

VkResult Device::GetMemoryWin32Handle(const VkMemoryGetWin32HandleInfoKHR* pGetWin32HandleInfo, HANDLE* pHandle)
{
	FATAL_ERROR();
}

VkResult Device::GetMemoryWin32HandleProperties(VkExternalMemoryHandleTypeFlagBits handleType, HANDLE handle, VkMemoryWin32HandlePropertiesKHR* pMemoryWin32HandleProperties)
{
	FATAL_ERROR();
}

VkResult Device::GetMemoryWin32HandleNV(VkDeviceMemory memory, VkExternalMemoryHandleTypeFlagsNV handleType, HANDLE* pHandle)
{
	FATAL_ERROR();
}

VkResult Device::AcquireFullScreenExclusiveMode(VkSwapchainKHR swapchain)
{
	FATAL_ERROR();
}

VkResult Device::ReleaseFullScreenExclusiveMode(VkSwapchainKHR swapchain)
{
	FATAL_ERROR();
}

VkResult Device::GetDeviceGroupSurfacePresentModes2(const VkPhysicalDeviceSurfaceInfo2KHR* pSurfaceInfo, VkDeviceGroupPresentModeFlagsKHR* pModes)
{
	FATAL_ERROR();
}
