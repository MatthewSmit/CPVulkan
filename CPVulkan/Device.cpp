#include "Device.h"

#include "DescriptorSet.h"
#include "DescriptorSetLayout.h"
#include "DeviceState.h"
#include "Extensions.h"
#include "Instance.h"
#include "Pipeline.h"
#include "PipelineLayout.h"
#include "Queue.h"

#include <cassert>

Device::Device() :
	state{std::make_unique<DeviceState>()}
{
}

Device::~Device() = default;

PFN_vkVoidFunction Device::GetProcAddress(const char* pName)
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

	*pQueue = reinterpret_cast<VkQueue>(queues[queueIndex].get());
}

VkResult Device::DeviceWaitIdle()
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

	*pMemory = reinterpret_cast<VkDeviceMemory>(AllocateSized(pAllocator, pAllocateInfo->allocationSize, VK_SYSTEM_ALLOCATION_SCOPE_DEVICE));

	return VK_SUCCESS;
}

void Device::FreeMemory(VkDeviceMemory memory, const VkAllocationCallbacks* pAllocator)
{
	FreeSized(reinterpret_cast<DeviceMemory*>(memory), pAllocator);
}

VkResult Device::MapMemory(VkDeviceMemory memory, VkDeviceSize offset, VkDeviceSize size, VkMemoryMapFlags flags, void** ppData)
{
	assert(flags == 0); 
	// TODO: Bounds checks
	*ppData = reinterpret_cast<DeviceMemory*>(memory)->Data + offset;
	return VK_SUCCESS;
}

void Device::UnmapMemory(VkDeviceMemory memory)
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

VkResult Device::CreateGraphicsPipelines(VkPipelineCache pipelineCache, uint32_t createInfoCount, const VkGraphicsPipelineCreateInfo* pCreateInfos, const VkAllocationCallbacks* pAllocator, VkPipeline* pPipelines)
{
	for (auto i = 0u; i < createInfoCount; i++)
	{
		auto result = Pipeline::Create(pipelineCache, &pCreateInfos[i], pAllocator, &pPipelines[i]);
		if (result != VK_SUCCESS)
		{
			FATAL_ERROR();
		}
	}

	return VK_SUCCESS;
}

void Device::DestroyPipeline(VkPipeline pipeline, const VkAllocationCallbacks* pAllocator)
{
	Free(reinterpret_cast<Pipeline*>(pipeline), pAllocator);
}

VkResult Device::CreatePipelineLayout(const VkPipelineLayoutCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkPipelineLayout* pPipelineLayout)
{
	return PipelineLayout::Create(pCreateInfo, pAllocator, pPipelineLayout);
}

void Device::DestroyPipelineLayout(VkPipelineLayout pipelineLayout, const VkAllocationCallbacks* pAllocator)
{
	Free(reinterpret_cast<PipelineLayout*>(pipelineLayout), pAllocator);
}

VkResult Device::CreateDescriptorSetLayout(const VkDescriptorSetLayoutCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDescriptorSetLayout* pSetLayout)
{
	return DescriptorSetLayout::Create(pCreateInfo, pAllocator, pSetLayout);
}

void Device::DestroyDescriptorSetLayout(VkDescriptorSetLayout descriptorSetLayout, const VkAllocationCallbacks* pAllocator)
{
	Free(reinterpret_cast<DescriptorSetLayout*>(descriptorSetLayout), pAllocator);
}

VkResult Device::AllocateDescriptorSets(const VkDescriptorSetAllocateInfo* pAllocateInfo, VkDescriptorSet* pDescriptorSets)
{
	assert(pAllocateInfo->sType == VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO);

	auto next = pAllocateInfo->pNext;
	while (next)
	{
		const auto type = *static_cast<const VkStructureType*>(next);
		switch (type)
		{
		default:
			FATAL_ERROR();
		}
	}

	for (auto i = 0u; i < pAllocateInfo->descriptorSetCount; i++)
	{
		const auto result = DescriptorSet::Create(pAllocateInfo->descriptorPool, pAllocateInfo->pSetLayouts[i], &pDescriptorSets[i]);
		if (result != VK_SUCCESS)
		{
			FATAL_ERROR();
		}
	}

	return VK_SUCCESS;
}

VkResult Device::FreeDescriptorSets(VkDescriptorPool descriptorPool, uint32_t descriptorSetCount, const VkDescriptorSet* pDescriptorSets)
{
	for (auto i = 0u; i < descriptorSetCount; i++)
	{
		Free(reinterpret_cast<DescriptorSet*>(pDescriptorSets[i]), nullptr);
	}
	
	return VK_SUCCESS;
}

void Device::UpdateDescriptorSets(uint32_t descriptorWriteCount, const VkWriteDescriptorSet* pDescriptorWrites, uint32_t descriptorCopyCount, const VkCopyDescriptorSet* pDescriptorCopies)
{
	for (auto i = 0u; i < descriptorWriteCount; i++)
	{
		const auto& descriptorWrite = pDescriptorWrites[i];
		assert(descriptorWrite.sType == VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET);

		auto next = descriptorWrite.pNext;
		while (next)
		{
			const auto type = *static_cast<const VkStructureType*>(next);
			switch (type)
			{
			default:
				FATAL_ERROR();
			}
		}

		reinterpret_cast<DescriptorSet*>(descriptorWrite.dstSet)->Update(descriptorWrite);
	}

	for (auto i = 0u; i < descriptorCopyCount; i++)
	{
		FATAL_ERROR();
	}
}

void Device::GetDeviceQueue2(const VkDeviceQueueInfo2* pQueueInfo, VkQueue* pQueue)
{
	assert(pQueueInfo->sType == VK_STRUCTURE_TYPE_DEVICE_QUEUE_INFO_2);

	auto next = pQueueInfo->pNext;
	while (next)
	{
		FATAL_ERROR();
	}
	
	if (pQueueInfo->queueFamilyIndex != 0)
	{
		FATAL_ERROR();
	}

	auto queue = queues[pQueueInfo->queueIndex].get();
	if (queue->getFlags() != pQueueInfo->flags)
	{
		*pQueue = VK_NULL_HANDLE;
	}
	else
	{
		*pQueue = reinterpret_cast<VkQueue>(queue);
	}
}

VkResult Device::Create(Instance* instance, const VkDeviceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDevice* pDevice)
{
	assert(pCreateInfo->sType == VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO);
	assert(pCreateInfo->flags == 0);

	auto device = Allocate<Device>(pAllocator, VK_SYSTEM_ALLOCATION_SCOPE_DEVICE);

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
			FATAL_ERROR();

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXCLUSIVE_SCISSOR_FEATURES_NV:
			FATAL_ERROR();

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_DENSITY_MAP_FEATURES_EXT:
			FATAL_ERROR();

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADER_BARYCENTRIC_FEATURES_NV:
			FATAL_ERROR();

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADER_INTERLOCK_FEATURES_EXT:
			FATAL_ERROR();

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_HOST_QUERY_RESET_FEATURES_EXT:
			FATAL_ERROR();

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGELESS_FRAMEBUFFER_FEATURES_KHR:
			FATAL_ERROR();

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_INDEX_TYPE_UINT8_FEATURES_EXT:
			FATAL_ERROR();

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_INLINE_UNIFORM_BLOCK_FEATURES_EXT:
			FATAL_ERROR();

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
			FATAL_ERROR();

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
			FATAL_ERROR();

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DEMOTE_TO_HELPER_INVOCATION_FEATURES_EXT:
			FATAL_ERROR();
			
		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DRAW_PARAMETERS_FEATURES:
			{
				const auto features = static_cast<const VkPhysicalDeviceShaderDrawParametersFeatures*>(next);
				// TODO
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_FLOAT16_INT8_FEATURES_KHR:
			FATAL_ERROR();

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
			FATAL_ERROR();

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VARIABLE_POINTERS_FEATURES:
			{
				const auto features = static_cast<const VkPhysicalDeviceVariablePointersFeatures*>(next);
				// TODO
				break;
			}

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VERTEX_ATTRIBUTE_DIVISOR_FEATURES_EXT:
			FATAL_ERROR();

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_MEMORY_MODEL_FEATURES_KHR:
			FATAL_ERROR();

		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_YCBCR_IMAGE_ARRAYS_FEATURES_EXT:
			FATAL_ERROR();
		}
		
		next = static_cast<const VkPhysicalDeviceFeatures2*>(next)->pNext;
	}

	for (auto i = 0u; i < pCreateInfo->queueCreateInfoCount; i++)
	{
		device->queues.push_back(std::unique_ptr<Queue>{Queue::Create(pCreateInfo->pQueueCreateInfos[i], pAllocator)});
	}

	std::vector<const char*> enabledExtensions{};
	if (pCreateInfo->enabledExtensionCount)
	{
		for (auto i = 0u; i < pCreateInfo->enabledExtensionCount; i++)
		{
			enabledExtensions.push_back(pCreateInfo->ppEnabledExtensionNames[i]);
		}
	}

	auto result = GetInitialExtensions().MakeDeviceCopy(instance->getEnabledExtensions(), enabledExtensions, &device->enabledExtensions);

	if (result != VK_SUCCESS)
	{
		Free(device, pAllocator);
		return result;
	}

	if (pCreateInfo->pEnabledFeatures)
	{
		// TODO
	}

	*pDevice = reinterpret_cast<VkDevice>(device);
	return VK_SUCCESS;
}
