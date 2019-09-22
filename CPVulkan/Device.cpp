#include "Device.h"

#include "DeviceState.h"
#include "Extensions.h"
#include "Instance.h"
#include "Queue.h"
#include "Util.h"

#include <Converter.h>

#include <cassert>

Device::Device() :
	state{std::make_unique<DeviceState>()}
{
	state->jit = new SpirvJit();
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

	WrapVulkan(queues[queueIndex].get(), pQueue);
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
				const auto allocateInfo = reinterpret_cast<const VkMemoryDedicatedAllocateInfo*>(next);
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

void Device::GetDeviceQueue2(const VkDeviceQueueInfo2* pQueueInfo, VkQueue* pQueue)
{
	assert(pQueueInfo->sType == VK_STRUCTURE_TYPE_DEVICE_QUEUE_INFO_2);
	assert(pQueueInfo->pNext == nullptr);
	assert(pQueueInfo->flags == 1);
	
	if (pQueueInfo->queueFamilyIndex != 0)
	{
		FATAL_ERROR();
	}

	const auto queue = queues[pQueueInfo->queueIndex].get();
	if (queue->getFlags() != pQueueInfo->flags)
	{
		*pQueue = VK_NULL_HANDLE;
	}
	else
	{
		WrapVulkan(queue, pQueue);
	}
}

VkResult Device::Create(Instance* instance, const VkDeviceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDevice* pDevice)
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
