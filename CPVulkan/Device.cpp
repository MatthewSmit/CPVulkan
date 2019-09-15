#include "Device.h"

#include "Buffer.h"
#include "CommandPool.h"
#include "DescriptorPool.h"
#include "DescriptorSet.h"
#include "DescriptorSetLayout.h"
#include "DeviceState.h"
#include "Extensions.h"
#include "Fence.h"
#include "Framebuffer.h"
#include "Image.h"
#include "ImageView.h"
#include "Instance.h"
#include "Pipeline.h"
#include "PipelineCache.h"
#include "PipelineLayout.h"
#include "Queue.h"
#include "RenderPass.h"
#include "Sampler.h"
#include "Semaphore.h"
#include "ShaderModule.h"
#include "Swapchain.h"

#include <Windows.h>
#include <vulkan/vk_icd.h>
#include <vulkan/vk_layer.h>

#include <cassert>

Device::Device() :
	state{std::make_unique<DeviceState>()}
{
}

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
		default:
			FATAL_ERROR();
		}
	}

	if (pAllocateInfo->memoryTypeIndex != 0)
	{
		FATAL_ERROR();
	}

	*pMemory = reinterpret_cast<VkDeviceMemory>(AllocateSized(pAllocator, pAllocateInfo->allocationSize));

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

VkResult Device::InvalidateMappedMemoryRanges(uint32_t memoryRangeCount, const VkMappedMemoryRange* pMemoryRanges)
{
	return VK_SUCCESS;
}

VkResult Device::BindBufferMemory(VkBuffer buffer, VkDeviceMemory memory, VkDeviceSize memoryOffset)
{
	return reinterpret_cast<Buffer*>(buffer)->BindMemory(memory, memoryOffset);
}

VkResult Device::BindImageMemory(VkImage image, VkDeviceMemory memory, VkDeviceSize memoryOffset)
{
	return reinterpret_cast<Image*>(image)->BindMemory(memory, memoryOffset);
}

void Device::GetBufferMemoryRequirements(VkBuffer buffer, VkMemoryRequirements* pMemoryRequirements)
{
	reinterpret_cast<Buffer*>(buffer)->GetMemoryRequirements(pMemoryRequirements);
}

void Device::GetImageMemoryRequirements(VkImage image, VkMemoryRequirements* pMemoryRequirements)
{
	reinterpret_cast<Image*>(image)->GetMemoryRequirements(pMemoryRequirements);
}

VkResult Device::CreateFence(const VkFenceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkFence* pFence)
{
	return Fence::Create(pCreateInfo, pAllocator, pFence);
}

void Device::DestroyFence(VkFence fence, const VkAllocationCallbacks* pAllocator)
{
	Free(reinterpret_cast<Fence*>(fence), pAllocator);
}

VkResult Device::WaitForFences(uint32_t fenceCount, const VkFence* pFences, VkBool32 waitAll, uint64_t timeout)
{
	auto handles = std::vector<HANDLE>(fenceCount);
	for (auto i = 0u; i < fenceCount; i++)
	{
		handles[i] = reinterpret_cast<Fence*>(pFences[i])->getHandle();
	}

	const auto result = WaitForMultipleObjects(fenceCount, handles.data(), waitAll, timeout == UINT64_MAX ? INFINITE : timeout / 1000000);
	if (result == WAIT_FAILED)
	{
		FATAL_ERROR();
	}

	if (result == WAIT_TIMEOUT)
	{
		return VK_TIMEOUT;
	}
	
	return VK_SUCCESS;
}

#undef CreateSemaphore

VkResult Device::CreateSemaphore(const VkSemaphoreCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSemaphore* pSemaphore)
{
	return Semaphore::Create(pCreateInfo, pAllocator, pSemaphore);
}

void Device::DestroySemaphore(VkSemaphore semaphore, const VkAllocationCallbacks* pAllocator)
{
	Free(reinterpret_cast<Semaphore*>(semaphore), pAllocator);
}

VkResult Device::CreateBuffer(const VkBufferCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkBuffer* pBuffer)
{
	return Buffer::Create(pCreateInfo, pAllocator, pBuffer);
}

void Device::DestroyBuffer(VkBuffer buffer, const VkAllocationCallbacks* pAllocator)
{
	Free(reinterpret_cast<Buffer*>(buffer), pAllocator);
}

VkResult Device::CreateImage(const VkImageCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkImage* pImage)
{
	return Image::Create(pCreateInfo, pAllocator, pImage);
}

void Device::DestroyImage(VkImage image, const VkAllocationCallbacks* pAllocator)
{
	Free(reinterpret_cast<Image*>(image), pAllocator);
}

VkResult Device::CreateImageView(const VkImageViewCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkImageView* pView)
{
	return ImageView::Create(pCreateInfo, pAllocator, pView);
}

void Device::DestroyImageView(VkImageView imageView, const VkAllocationCallbacks* pAllocator)
{
	Free(reinterpret_cast<ImageView*>(imageView), pAllocator);
}

VkResult Device::CreateShaderModule(const VkShaderModuleCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkShaderModule* pShaderModule)
{
	return ShaderModule::Create(pCreateInfo, pAllocator, pShaderModule);
}

void Device::DestroyShaderModule(VkShaderModule shaderModule, const VkAllocationCallbacks* pAllocator)
{
	Free(reinterpret_cast<ShaderModule*>(shaderModule), pAllocator);
}

VkResult Device::CreatePipelineCache(const VkPipelineCacheCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkPipelineCache* pPipelineCache)
{
	return PipelineCache::Create(pCreateInfo, pAllocator, pPipelineCache);
}

void Device::DestroyPipelineCache(VkPipelineCache pipelineCache, const VkAllocationCallbacks* pAllocator)
{
	Free(reinterpret_cast<PipelineCache*>(pipelineCache), pAllocator);
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

VkResult Device::CreateSampler(const VkSamplerCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSampler* pSampler)
{
	return Sampler::Create(pCreateInfo, pAllocator, pSampler);
}

void Device::DestroySampler(VkSampler sampler, const VkAllocationCallbacks* pAllocator)
{
	Free(reinterpret_cast<Sampler*>(sampler), pAllocator);
}

VkResult Device::CreateDescriptorSetLayout(const VkDescriptorSetLayoutCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDescriptorSetLayout* pSetLayout)
{
	return DescriptorSetLayout::Create(pCreateInfo, pAllocator, pSetLayout);
}

void Device::DestroyDescriptorSetLayout(VkDescriptorSetLayout descriptorSetLayout, const VkAllocationCallbacks* pAllocator)
{
	Free(reinterpret_cast<DescriptorSetLayout*>(descriptorSetLayout), pAllocator);
}

VkResult Device::CreateDescriptorPool(const VkDescriptorPoolCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDescriptorPool* pDescriptorPool)
{
	return DescriptorPool::Create(pCreateInfo, pAllocator, pDescriptorPool);
}

void Device::DestroyDescriptorPool(VkDescriptorPool descriptorPool, const VkAllocationCallbacks* pAllocator)
{
	Free(reinterpret_cast<DescriptorPool*>(descriptorPool), pAllocator);
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

VkResult Device::CreateFramebuffer(const VkFramebufferCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkFramebuffer* pFramebuffer)
{
	return Framebuffer::Create(pCreateInfo, pAllocator, pFramebuffer);
}

void Device::DestroyFramebuffer(VkFramebuffer framebuffer, const VkAllocationCallbacks* pAllocator)
{
	Free(reinterpret_cast<Framebuffer*>(framebuffer), pAllocator);
}

VkResult Device::CreateRenderPass(const VkRenderPassCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkRenderPass* pRenderPass)
{
	return RenderPass::Create(pCreateInfo, pAllocator, pRenderPass);
}

void Device::DestroyRenderPass(VkRenderPass renderPass, const VkAllocationCallbacks* pAllocator)
{
	Free(reinterpret_cast<RenderPass*>(renderPass), pAllocator);
}

VkResult Device::CreateCommandPool(const VkCommandPoolCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkCommandPool* pCommandPool)
{
	return CommandPool::Create(state.get(), pCreateInfo, pAllocator, pCommandPool);
}

void Device::DestroyCommandPool(VkCommandPool commandPool, const VkAllocationCallbacks* pAllocator)
{
	Free(reinterpret_cast<CommandPool*>(commandPool), pAllocator);
}

VkResult Device::AllocateCommandBuffers(const VkCommandBufferAllocateInfo* pAllocateInfo, VkCommandBuffer* pCommandBuffers)
{
	return reinterpret_cast<CommandPool*>(pAllocateInfo->commandPool)->AllocateCommandBuffers(pAllocateInfo, pCommandBuffers);
}

void Device::FreeCommandBuffers(VkCommandPool commandPool, uint32_t commandBufferCount, const VkCommandBuffer* pCommandBuffers)
{
	reinterpret_cast<CommandPool*>(commandPool)->FreeCommandBuffers(commandBufferCount, pCommandBuffers);
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

VkResult Device::CreateSwapchain(const VkSwapchainCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSwapchainKHR* pSwapchain)
{
	return Swapchain::Create(pCreateInfo, pAllocator, pSwapchain);
}

void Device::DestroySwapchain(VkSwapchainKHR swapchain, const VkAllocationCallbacks* pAllocator)
{
	reinterpret_cast<Swapchain*>(swapchain)->DeleteImages(pAllocator);
	Free(reinterpret_cast<Swapchain*>(swapchain), pAllocator);
}

VkResult Device::GetSwapchainImages(VkSwapchainKHR swapchain, uint32_t* pSwapchainImageCount, VkImage* pSwapchainImages)
{
	return reinterpret_cast<Swapchain*>(swapchain)->GetImages(pSwapchainImageCount, pSwapchainImages);
}

VkResult Device::AcquireNextImage(VkSwapchainKHR swapchain, uint64_t timeout, VkSemaphore semaphore, VkFence fence, uint32_t* pImageIndex)
{
	return reinterpret_cast<Swapchain*>(swapchain)->AcquireNextImage(timeout, semaphore, fence, pImageIndex);
}

VkResult Device::Create(Instance* instance, const VkDeviceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDevice* pDevice)
{
	assert(pCreateInfo->sType == VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO);
	assert(pCreateInfo->flags == 0);

	auto device = Allocate<Device>(pAllocator);

	auto next = pCreateInfo->pNext;
	while (next)
	{
		auto type = *(VkStructureType*)next;
		switch (type)
		{
		case VK_STRUCTURE_TYPE_LOADER_DEVICE_CREATE_INFO:
			next = ((VkLayerDeviceCreateInfo*)next)->pNext;
			break;
		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2:
			{
				// TODO
				auto features = (VkPhysicalDeviceFeatures2*)next;
				next = features->pNext;
				break;
			}
		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VARIABLE_POINTERS_FEATURES:
			{
				// TODO
				auto features = (VkPhysicalDeviceVariablePointersFeatures*)next;
				next = features->pNext;
				break;
			}
		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_FEATURES:
			{
				// TODO
				auto features = (VkPhysicalDeviceMultiviewFeatures*)next;
				next = features->pNext;
				break;
			}
		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DRAW_PARAMETERS_FEATURES:
			{
				// TODO
				auto features = (VkPhysicalDeviceShaderDrawParametersFeatures*)next;
				next = features->pNext;
				break;
			}
		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_16BIT_STORAGE_FEATURES:
			{
				// TODO
				auto features = (VkPhysicalDevice16BitStorageFeatures*)next;
				next = features->pNext;
				break;
			}
		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROTECTED_MEMORY_FEATURES:
			{
				// TODO
				auto features = (VkPhysicalDeviceProtectedMemoryFeatures*)next;
				next = features->pNext;
				break;
			}
		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SAMPLER_YCBCR_CONVERSION_FEATURES:
			{
				// TODO
				auto features = (VkPhysicalDeviceSamplerYcbcrConversionFeatures*)next;
				next = features->pNext;
				break;
			}
		default:
			FATAL_ERROR();
		}
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
