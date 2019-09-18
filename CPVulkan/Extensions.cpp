#include "Extensions.h"

#include "CommandBuffer.h"
#include "Instance.h"
#include "Trampoline.h"

ExtensionGroup::ExtensionGroup() = default;

ExtensionGroup::ExtensionGroup(std::vector<Extension>&& extensions):
	extensions{extensions}
{
}

VkResult ExtensionGroup::MakeInstanceCopy(int vulkanVersion, const std::vector<const char*>& extensionNames, ExtensionGroup* extensionGroup) const
{
	auto newExtensions = std::vector<Extension>{};
	if (vulkanVersion >= VK_API_VERSION_1_0)
	{
		newExtensions.push_back(extensions[0]);
	}
#if defined(VK_VERSION_1_1)
	if (vulkanVersion >= VK_API_VERSION_1_1)
	{
		newExtensions.push_back(extensions[1]);
	}
#endif

	for (const auto& extensionName : extensionNames)
	{
		auto extension = FindExtension(extensionName);
		if (extension.Name == nullptr)
		{
			return VK_ERROR_EXTENSION_NOT_PRESENT;
		}
		newExtensions.push_back(extension);
	}
	
	*extensionGroup = ExtensionGroup(std::move(newExtensions));
	return VK_SUCCESS;
}

VkResult ExtensionGroup::MakeDeviceCopy(const ExtensionGroup& extraExtensions, const std::vector<const char*>& extensionNames, ExtensionGroup* extensionGroup) const
{
	auto newExtensions = extraExtensions.extensions;

	for (const auto& extensionName : extensionNames)
	{
		auto extension = FindExtension(extensionName);
		if (extension.Name == nullptr)
		{
			return VK_ERROR_EXTENSION_NOT_PRESENT;
		}
		newExtensions.push_back(extension);
	}

	*extensionGroup = ExtensionGroup(std::move(newExtensions));
	return VK_SUCCESS;
}

void ExtensionGroup::FillExtensionProperties(VkExtensionProperties* pProperties, uint32_t toCopy, bool device) const
{
	auto i = 0u;
	for (const auto& extension : extensions)
	{
		if (i >= toCopy)
		{
			break;
		}
		
		if (extension.Device == device && extension.Name[0] != '_')
		{
			strcpy_s(pProperties[i].extensionName, extension.Name);
			pProperties[i].specVersion = extension.Version;
			i += 1;
		}
	}
}

Extension ExtensionGroup::FindExtension(const char* extensionName) const
{
	for (const auto& extension : extensions)
	{
		if (strcmp(extension.Name, extensionName) == 0)
		{
			return extension;
		}
	}

	return Extension{};
}

PFN_vkVoidFunction ExtensionGroup::getFunction(const char* name, bool device) const
{
	for (const auto& extension : extensions)
	{
		for (const auto& entryPoint : extension.EntryPoints)
		{
			if (entryPoint.Device == device || (device && extension.Device))
			{
				if (strcmp(entryPoint.Name, name) == 0)
				{
					return entryPoint.Function;
				}
			}
		}
	}
	
	return nullptr;
}

uint32_t ExtensionGroup::getNumberInstanceExtensions() const
{
	if (numberInstanceExtensions == 0xFFFFFFFFu)
	{
		numberInstanceExtensions = 0;
		for (const auto& extension : extensions)
		{
			if (!extension.Device && extension.Name[0] != '_')
			{
				numberInstanceExtensions++;
			}
		}
	}
	return numberInstanceExtensions;
}

uint32_t ExtensionGroup::getNumberDeviceExtensions() const
{
	if (numberDeviceExtensions == 0xFFFFFFFFu)
	{
		numberDeviceExtensions = 0;
		for (const auto& extension : extensions)
		{
			if (extension.Device && extension.Name[0] != '_')
			{
				numberDeviceExtensions++;
			}
		}
	}
	return numberDeviceExtensions;
}

ExtensionGroup& GetInitialExtensions()
{
	static ExtensionGroup result
	{
		{
			{
				"_VULKAN_1_0",
				VK_MAKE_VERSION(1, 0, 0),
				false,
				{
					{"vkCreateInstance", reinterpret_cast<PFN_vkVoidFunction>(CreateInstance), false},
					{"vkDestroyInstance", GET_TRAMPOLINE(Instance, DestroyInstance), false},
					{"vkEnumeratePhysicalDevices", GET_TRAMPOLINE(Instance, EnumeratePhysicalDevices), false},
					{"vkGetPhysicalDeviceFeatures", GET_TRAMPOLINE(PhysicalDevice, GetFeatures), false},
					{"vkGetPhysicalDeviceFormatProperties", GET_TRAMPOLINE(PhysicalDevice, GetFormatProperties), false},
					{"vkGetPhysicalDeviceImageFormatProperties", GET_TRAMPOLINE(PhysicalDevice, GetImageFormatProperties), false},
					{"vkGetPhysicalDeviceProperties", GET_TRAMPOLINE(PhysicalDevice, GetProperties), false},
					{"vkGetPhysicalDeviceQueueFamilyProperties", GET_TRAMPOLINE(PhysicalDevice, GetQueueFamilyProperties), false},
					{"vkGetPhysicalDeviceMemoryProperties", GET_TRAMPOLINE(PhysicalDevice, GetMemoryProperties), false},
					{"vkGetInstanceProcAddr", GET_TRAMPOLINE(Instance, GetProcAddress), false},
					{"vkGetDeviceProcAddr", GET_TRAMPOLINE(Device, GetProcAddress), false},
					{"vkCreateDevice", GET_TRAMPOLINE(PhysicalDevice, CreateDevice), false},
					{"vkDestroyDevice", GET_TRAMPOLINE(Device, DestroyDevice), true},
					{"vkEnumerateInstanceExtensionProperties", reinterpret_cast<PFN_vkVoidFunction>(EnumerateInstanceExtensionProperties), false},
					{"vkEnumerateDeviceExtensionProperties", GET_TRAMPOLINE(PhysicalDevice, EnumerateDeviceExtensionProperties), false},
					{"vkEnumerateInstanceLayerProperties", reinterpret_cast<PFN_vkVoidFunction>(EnumerateInstanceLayerProperties), false},
					{"vkEnumerateDeviceLayerProperties", GET_TRAMPOLINE(PhysicalDevice, EnumerateDeviceLayerProperties), false},
					{"vkGetDeviceQueue", GET_TRAMPOLINE(Device, GetDeviceQueue), true},
					{"vkQueueSubmit", GET_TRAMPOLINE(Queue, Submit), true},
					{"vkQueueWaitIdle", GET_TRAMPOLINE(Queue, WaitIdle), true},
					{"vkDeviceWaitIdle", GET_TRAMPOLINE(Device, WaitIdle), true},
					{"vkAllocateMemory", GET_TRAMPOLINE(Device, AllocateMemory), true},
					{"vkFreeMemory", GET_TRAMPOLINE(Device, FreeMemory), true},
					{"vkMapMemory", GET_TRAMPOLINE(Device, MapMemory), true},
					{"vkUnmapMemory", GET_TRAMPOLINE(Device, UnmapMemory), true},
					{"vkFlushMappedMemoryRanges", GET_TRAMPOLINE(Device, FlushMappedMemoryRanges), true},
					{"vkInvalidateMappedMemoryRanges", GET_TRAMPOLINE(Device, InvalidateMappedMemoryRanges), true},
					{"vkGetDeviceMemoryCommitment", GET_TRAMPOLINE(Device, GetDeviceMemoryCommitment), true},
					{"vkBindBufferMemory", GET_TRAMPOLINE(Device, BindBufferMemory), true},
					{"vkBindImageMemory", GET_TRAMPOLINE(Device, BindImageMemory), true},
					{"vkGetBufferMemoryRequirements", GET_TRAMPOLINE(Device, GetBufferMemoryRequirements), true},
					{"vkGetImageMemoryRequirements", GET_TRAMPOLINE(Device, GetImageMemoryRequirements), true},
					{"vkGetImageSparseMemoryRequirements", GET_TRAMPOLINE(Device, GetImageSparseMemoryRequirements), true},
					{"vkGetPhysicalDeviceSparseImageFormatProperties", GET_TRAMPOLINE(PhysicalDevice, GetSparseImageFormatProperties), false},
					{"vkQueueBindSparse", GET_TRAMPOLINE(Queue, BindSparse), true},
					{"vkCreateFence", GET_TRAMPOLINE(Device, CreateFence), true},
					{"vkDestroyFence", GET_TRAMPOLINE(Device, DestroyFence), true},
					{"vkResetFences", GET_TRAMPOLINE(Device, ResetFences), true},
					{"vkGetFenceStatus", GET_TRAMPOLINE(Device, GetFenceStatus), true},
					{"vkWaitForFences", GET_TRAMPOLINE(Device, WaitForFences), true},
					{"vkCreateSemaphore", GET_TRAMPOLINE(Device, CreateSemaphore), true},
					{"vkDestroySemaphore", GET_TRAMPOLINE(Device, DestroySemaphore), true},
					{"vkCreateEvent", GET_TRAMPOLINE(Device, CreateEvent), true},
					{"vkDestroyEvent", GET_TRAMPOLINE(Device, DestroyEvent), true},
					{"vkGetEventStatus", GET_TRAMPOLINE(Device, GetEventStatus), true},
					{"vkSetEvent", GET_TRAMPOLINE(Device, SetEvent), true},
					{"vkResetEvent", GET_TRAMPOLINE(Device, ResetEvent), true},
					{"vkCreateQueryPool", GET_TRAMPOLINE(Device, CreateQueryPool), true},
					{"vkDestroyQueryPool", GET_TRAMPOLINE(Device, DestroyQueryPool), true},
					{"vkGetQueryPoolResults", GET_TRAMPOLINE(Device, GetQueryPoolResults), true},
					{"vkCreateBuffer", GET_TRAMPOLINE(Device, CreateBuffer), true},
					{"vkDestroyBuffer", GET_TRAMPOLINE(Device, DestroyBuffer), true},
					{"vkCreateBufferView", GET_TRAMPOLINE(Device, CreateBufferView), true},
					{"vkDestroyBufferView", GET_TRAMPOLINE(Device, DestroyBufferView), true},
					{"vkCreateImage", GET_TRAMPOLINE(Device, CreateImage), true},
					{"vkDestroyImage", GET_TRAMPOLINE(Device, DestroyImage), true},
					{"vkGetImageSubresourceLayout", GET_TRAMPOLINE(Device, GetImageSubresourceLayout), true},
					{"vkCreateImageView", GET_TRAMPOLINE(Device, CreateImageView), true},
					{"vkDestroyImageView", GET_TRAMPOLINE(Device, DestroyImageView), true},
					{"vkCreateShaderModule", GET_TRAMPOLINE(Device, CreateShaderModule), true},
					{"vkDestroyShaderModule", GET_TRAMPOLINE(Device, DestroyShaderModule), true},
					{"vkCreatePipelineCache", GET_TRAMPOLINE(Device, CreatePipelineCache), true},
					{"vkDestroyPipelineCache", GET_TRAMPOLINE(Device, DestroyPipelineCache), true},
					{"vkGetPipelineCacheData", GET_TRAMPOLINE(Device, GetPipelineCacheData), true},
					{"vkMergePipelineCaches", GET_TRAMPOLINE(Device, MergePipelineCaches), true},
					{"vkCreateGraphicsPipelines", GET_TRAMPOLINE(Device, CreateGraphicsPipelines), true},
					{"vkCreateComputePipelines", GET_TRAMPOLINE(Device, CreateComputePipelines), true},
					{"vkDestroyPipeline", GET_TRAMPOLINE(Device, DestroyPipeline), true},
					{"vkCreatePipelineLayout", GET_TRAMPOLINE(Device, CreatePipelineLayout), true},
					{"vkDestroyPipelineLayout", GET_TRAMPOLINE(Device, DestroyPipelineLayout), true},
					{"vkCreateSampler", GET_TRAMPOLINE(Device, CreateSampler), true},
					{"vkDestroySampler", GET_TRAMPOLINE(Device, DestroySampler), true},
					{"vkCreateDescriptorSetLayout", GET_TRAMPOLINE(Device, CreateDescriptorSetLayout), true},
					{"vkDestroyDescriptorSetLayout", GET_TRAMPOLINE(Device, DestroyDescriptorSetLayout), true},
					{"vkCreateDescriptorPool", GET_TRAMPOLINE(Device, CreateDescriptorPool), true},
					{"vkDestroyDescriptorPool", GET_TRAMPOLINE(Device, DestroyDescriptorPool), true},
					{"vkResetDescriptorPool", GET_TRAMPOLINE(Device, ResetDescriptorPool), true},
					{"vkAllocateDescriptorSets", GET_TRAMPOLINE(Device, AllocateDescriptorSets), true},
					{"vkFreeDescriptorSets", GET_TRAMPOLINE(Device, FreeDescriptorSets), true},
					{"vkUpdateDescriptorSets", GET_TRAMPOLINE(Device, UpdateDescriptorSets), true},
					{"vkCreateFramebuffer", GET_TRAMPOLINE(Device, CreateFramebuffer), true},
					{"vkDestroyFramebuffer", GET_TRAMPOLINE(Device, DestroyFramebuffer), true},
					{"vkCreateRenderPass", GET_TRAMPOLINE(Device, CreateRenderPass), true},
					{"vkDestroyRenderPass", GET_TRAMPOLINE(Device, DestroyRenderPass), true},
					{"vkGetRenderAreaGranularity", GET_TRAMPOLINE(Device, GetRenderAreaGranularity), true},
					{"vkCreateCommandPool", GET_TRAMPOLINE(Device, CreateCommandPool), true},
					{"vkDestroyCommandPool", GET_TRAMPOLINE(Device, DestroyCommandPool), true},
					{"vkResetCommandPool", GET_TRAMPOLINE(Device, ResetCommandPool), true},
					{"vkAllocateCommandBuffers", GET_TRAMPOLINE(Device, AllocateCommandBuffers), true},
					{"vkFreeCommandBuffers", GET_TRAMPOLINE(Device, FreeCommandBuffers), true},
					{"vkBeginCommandBuffer", GET_TRAMPOLINE(CommandBuffer, Begin), true},
					{"vkEndCommandBuffer", GET_TRAMPOLINE(CommandBuffer, End), true},
					{"vkResetCommandBuffer", GET_TRAMPOLINE(CommandBuffer, Reset), true},
					{"vkCmdBindPipeline", GET_TRAMPOLINE(CommandBuffer, BindPipeline), true},
					{"vkCmdSetViewport", GET_TRAMPOLINE(CommandBuffer, SetViewport), true},
					{"vkCmdSetScissor", GET_TRAMPOLINE(CommandBuffer, SetScissor), true},
					{"vkCmdSetLineWidth", GET_TRAMPOLINE(CommandBuffer, SetLineWidth), true},
					{"vkCmdSetDepthBias", GET_TRAMPOLINE(CommandBuffer, SetDepthBias), true},
					{"vkCmdSetBlendConstants", GET_TRAMPOLINE(CommandBuffer, SetBlendConstants), true},
					{"vkCmdSetDepthBounds", GET_TRAMPOLINE(CommandBuffer, SetDepthBounds), true},
					{"vkCmdSetStencilCompareMask", GET_TRAMPOLINE(CommandBuffer, SetStencilCompareMask), true},
					{"vkCmdSetStencilWriteMask", GET_TRAMPOLINE(CommandBuffer, SetStencilWriteMask), true},
					{"vkCmdSetStencilReference", GET_TRAMPOLINE(CommandBuffer, SetStencilReference), true},
					{"vkCmdBindDescriptorSets", GET_TRAMPOLINE(CommandBuffer, BindDescriptorSets), true},
					{"vkCmdBindIndexBuffer", GET_TRAMPOLINE(CommandBuffer, BindIndexBuffer), true},
					{"vkCmdBindVertexBuffers", GET_TRAMPOLINE(CommandBuffer, BindVertexBuffers), true},
					{"vkCmdDraw", GET_TRAMPOLINE(CommandBuffer, Draw), true},
					{"vkCmdDrawIndexed", GET_TRAMPOLINE(CommandBuffer, DrawIndexed), true},
					{"vkCmdDrawIndirect", GET_TRAMPOLINE(CommandBuffer, DrawIndirect), true},
					{"vkCmdDrawIndexedIndirect", GET_TRAMPOLINE(CommandBuffer, DrawIndexedIndirect), true},
					{"vkCmdDispatch", GET_TRAMPOLINE(CommandBuffer, Dispatch), true},
					{"vkCmdDispatchIndirect", GET_TRAMPOLINE(CommandBuffer, DispatchIndirect), true},
					{"vkCmdCopyBuffer", GET_TRAMPOLINE(CommandBuffer, CopyBuffer), true},
					{"vkCmdCopyImage", GET_TRAMPOLINE(CommandBuffer, CopyImage), true},
					{"vkCmdBlitImage", GET_TRAMPOLINE(CommandBuffer, BlitImage), true},
					{"vkCmdCopyBufferToImage", GET_TRAMPOLINE(CommandBuffer, CopyBufferToImage), true},
					{"vkCmdCopyImageToBuffer", GET_TRAMPOLINE(CommandBuffer, CopyImageToBuffer), true},
					{"vkCmdUpdateBuffer", GET_TRAMPOLINE(CommandBuffer, UpdateBuffer), true},
					{"vkCmdFillBuffer", GET_TRAMPOLINE(CommandBuffer, FillBuffer), true},
					{"vkCmdClearColorImage", GET_TRAMPOLINE(CommandBuffer, ClearColorImage), true},
					{"vkCmdClearDepthStencilImage", GET_TRAMPOLINE(CommandBuffer, ClearDepthStencilImage), true},
					{"vkCmdClearAttachments", GET_TRAMPOLINE(CommandBuffer, ClearAttachments), true},
					{"vkCmdResolveImage", GET_TRAMPOLINE(CommandBuffer, ResolveImage), true},
					{"vkCmdSetEvent", GET_TRAMPOLINE(CommandBuffer, SetEvent), true},
					{"vkCmdResetEvent", GET_TRAMPOLINE(CommandBuffer, ResetEvent), true},
					{"vkCmdWaitEvents", GET_TRAMPOLINE(CommandBuffer, WaitEvents), true},
					{"vkCmdPipelineBarrier", GET_TRAMPOLINE(CommandBuffer, PipelineBarrier), true},
					{"vkCmdBeginQuery", GET_TRAMPOLINE(CommandBuffer, BeginQuery), true},
					{"vkCmdEndQuery", GET_TRAMPOLINE(CommandBuffer, EndQuery), true},
					{"vkCmdResetQueryPool", GET_TRAMPOLINE(CommandBuffer, ResetQueryPool), true},
					{"vkCmdWriteTimestamp", GET_TRAMPOLINE(CommandBuffer, WriteTimestamp), true},
					{"vkCmdCopyQueryPoolResults", GET_TRAMPOLINE(CommandBuffer, CopyQueryPoolResults), true},
					{"vkCmdPushConstants", GET_TRAMPOLINE(CommandBuffer, PushConstants), true},
					{"vkCmdBeginRenderPass", GET_TRAMPOLINE(CommandBuffer, BeginRenderPass), true},
					{"vkCmdNextSubpass", GET_TRAMPOLINE(CommandBuffer, NextSubpass), true},
					{"vkCmdEndRenderPass", GET_TRAMPOLINE(CommandBuffer, EndRenderPass), true},
					{"vkCmdExecuteCommands", GET_TRAMPOLINE(CommandBuffer, ExecuteCommands), true},
				}
			},
#if defined(VK_VERSION_1_1)
			{
				"_VULKAN_1_1",
				VK_MAKE_VERSION(1, 1, 0),
				false,
				{
					{"vkEnumerateInstanceVersion", reinterpret_cast<PFN_vkVoidFunction>(EnumerateInstanceVersion), false},
					{"vkBindBufferMemory2", GET_TRAMPOLINE(Device, BindBufferMemory2), true},
					{"vkBindImageMemory2", GET_TRAMPOLINE(Device, BindImageMemory2), true},
					{"vkGetDeviceGroupPeerMemoryFeatures", GET_TRAMPOLINE(Device, GetDeviceGroupPeerMemoryFeatures), true},
					{"vkCmdSetDeviceMask", GET_TRAMPOLINE(CommandBuffer, SetDeviceMask), true},
					{"vkCmdDispatchBase", GET_TRAMPOLINE(CommandBuffer, DispatchBase), true},
					{"vkEnumeratePhysicalDeviceGroups", GET_TRAMPOLINE(Instance, EnumeratePhysicalDeviceGroups), false},
					{"vkGetImageMemoryRequirements2", GET_TRAMPOLINE(Device, GetImageMemoryRequirements2), true},
					{"vkGetBufferMemoryRequirements2", GET_TRAMPOLINE(Device, GetBufferMemoryRequirements2), true},
					{"vkGetImageSparseMemoryRequirements2", GET_TRAMPOLINE(Device, GetImageSparseMemoryRequirements2), true},
					{"vkGetPhysicalDeviceFeatures2", GET_TRAMPOLINE(PhysicalDevice, GetFeatures2), false},
					{"vkGetPhysicalDeviceProperties2", GET_TRAMPOLINE(PhysicalDevice, GetProperties2), false},
					{"vkGetPhysicalDeviceFormatProperties2", GET_TRAMPOLINE(PhysicalDevice, GetFormatProperties2), false},
					{"vkGetPhysicalDeviceImageFormatProperties2", GET_TRAMPOLINE(PhysicalDevice, GetImageFormatProperties2), false},
					{"vkGetPhysicalDeviceQueueFamilyProperties2", GET_TRAMPOLINE(PhysicalDevice, GetQueueFamilyProperties2), false},
					{"vkGetPhysicalDeviceMemoryProperties2", GET_TRAMPOLINE(PhysicalDevice, GetMemoryProperties2), false},
					{"vkGetPhysicalDeviceSparseImageFormatProperties2", GET_TRAMPOLINE(PhysicalDevice, GetSparseImageFormatProperties2), false},
					{"vkTrimCommandPool", GET_TRAMPOLINE(Device, TrimCommandPool), true},
					{"vkGetDeviceQueue2", GET_TRAMPOLINE(Device, GetDeviceQueue2), true},
					{"vkCreateSamplerYcbcrConversion", GET_TRAMPOLINE(Device, CreateSamplerYcbcrConversion), true},
					{"vkDestroySamplerYcbcrConversion", GET_TRAMPOLINE(Device, DestroySamplerYcbcrConversion), true},
					{"vkCreateDescriptorUpdateTemplate", GET_TRAMPOLINE(Device, CreateDescriptorUpdateTemplate), true},
					{"vkDestroyDescriptorUpdateTemplate", GET_TRAMPOLINE(Device, DestroyDescriptorUpdateTemplate), true},
					{"vkUpdateDescriptorSetWithTemplate", GET_TRAMPOLINE(Device, UpdateDescriptorSetWithTemplate), true},
					{"vkGetPhysicalDeviceExternalBufferProperties", GET_TRAMPOLINE(PhysicalDevice, GetExternalBufferProperties), false},
					{"vkGetPhysicalDeviceExternalFenceProperties", GET_TRAMPOLINE(PhysicalDevice, GetExternalFenceProperties), false},
					{"vkGetPhysicalDeviceExternalSemaphoreProperties", GET_TRAMPOLINE(PhysicalDevice, GetExternalSemaphoreProperties), false},
					{"vkGetDescriptorSetLayoutSupport", GET_TRAMPOLINE(Device, GetDescriptorSetLayoutSupport), true},
				}
			},
#endif
#if defined(VK_KHR_surface)
			{
				VK_KHR_SURFACE_EXTENSION_NAME,
				VK_KHR_SURFACE_SPEC_VERSION,
				false,
				{
					{"vkDestroySurfaceKHR", GET_TRAMPOLINE(Instance, DestroySurface), false},
					{"vkGetPhysicalDeviceSurfaceSupportKHR", GET_TRAMPOLINE(PhysicalDevice, GetSurfaceSupport), false},
					{"vkGetPhysicalDeviceSurfaceCapabilitiesKHR", GET_TRAMPOLINE(PhysicalDevice, GetSurfaceCapabilities), false},
					{"vkGetPhysicalDeviceSurfaceFormatsKHR", GET_TRAMPOLINE(PhysicalDevice, GetSurfaceFormats), false},
					{"vkGetPhysicalDeviceSurfacePresentModesKHR", GET_TRAMPOLINE(PhysicalDevice, GetSurfacePresentModes), false},
				}
			},
#endif
#if defined(VK_KHR_swapchain)
			{
				VK_KHR_SWAPCHAIN_EXTENSION_NAME,
				VK_KHR_SWAPCHAIN_SPEC_VERSION,
				true,
				{
					{"vkCreateSwapchainKHR", GET_TRAMPOLINE(Device, CreateSwapchain), true},
					{"vkDestroySwapchainKHR", GET_TRAMPOLINE(Device, DestroySwapchain), true},
					{"vkGetSwapchainImagesKHR", GET_TRAMPOLINE(Device, GetSwapchainImages), true},
					{"vkAcquireNextImageKHR", GET_TRAMPOLINE(Device, AcquireNextImage), true},
					{"vkQueuePresentKHR", GET_TRAMPOLINE(Queue, Present), true},
					{"vkGetDeviceGroupPresentCapabilitiesKHR", GET_TRAMPOLINE(Device, GetDeviceGroupPresentCapabilities), true},
					{"vkGetDeviceGroupSurfacePresentModesKHR", GET_TRAMPOLINE(Device, GetDeviceGroupSurfacePresentModes), true},
					{"vkGetPhysicalDevicePresentRectanglesKHR", GET_TRAMPOLINE(PhysicalDevice, GetPresentRectangles), false},
					{"vkAcquireNextImage2KHR", GET_TRAMPOLINE(Device, AcquireNextImage2), true},
				}
			},
#endif
#if defined(VK_KHR_display)
			{
				VK_KHR_DISPLAY_EXTENSION_NAME,
				VK_KHR_DISPLAY_SPEC_VERSION,
				false,
				{
					{"vkGetPhysicalDeviceDisplayPropertiesKHR", GET_TRAMPOLINE(PhysicalDevice, GetDisplayProperties), false},
					{"vkGetPhysicalDeviceDisplayPlanePropertiesKHR", GET_TRAMPOLINE(PhysicalDevice, GetDisplayPlaneProperties), false},
					{"vkGetDisplayPlaneSupportedDisplaysKHR", GET_TRAMPOLINE(PhysicalDevice, GetDisplayPlaneSupportedDisplays), false},
					{"vkGetDisplayModePropertiesKHR", GET_TRAMPOLINE(PhysicalDevice, GetDisplayModeProperties), false},
					{"vkCreateDisplayModeKHR", GET_TRAMPOLINE(PhysicalDevice, CreateDisplayMode), false},
					{"vkGetDisplayPlaneCapabilitiesKHR", GET_TRAMPOLINE(PhysicalDevice, GetDisplayPlaneCapabilities), false},
					{"vkCreateDisplayPlaneSurfaceKHR", GET_TRAMPOLINE(Instance, CreateDisplayPlaneSurface), false},
				}
			},
#endif
#if defined(VK_KHR_display_swapchain)
			{
				VK_KHR_DISPLAY_SWAPCHAIN_EXTENSION_NAME,
				VK_KHR_DISPLAY_SWAPCHAIN_SPEC_VERSION,
				true,
				{
					{"vkCreateSharedSwapchainsKHR", GET_TRAMPOLINE(Device, CreateSharedSwapchains), false},
				}
			},
#endif
#if defined(VK_KHR_sampler_mirror_clamp_to_edge)
			{
				VK_KHR_SAMPLER_MIRROR_CLAMP_TO_EDGE_EXTENSION_NAME,
				VK_KHR_SAMPLER_MIRROR_CLAMP_TO_EDGE_SPEC_VERSION,
				true,
				{
				}
			},
#endif
#if defined(VK_KHR_multiview)
			{
				VK_KHR_MULTIVIEW_EXTENSION_NAME,
				VK_KHR_MULTIVIEW_SPEC_VERSION,
				true,
				{
				}
			},
#endif
#if defined(VK_KHR_get_physical_device_properties2)
			{
				VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
				VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_SPEC_VERSION,
				false,
				{
					{"vkGetPhysicalDeviceFeatures2KHR", GET_TRAMPOLINE(PhysicalDevice, GetFeatures2), false},
					{"vkGetPhysicalDeviceProperties2KHR", GET_TRAMPOLINE(PhysicalDevice, GetProperties2), false},
					{"vkGetPhysicalDeviceFormatProperties2KHR", GET_TRAMPOLINE(PhysicalDevice, GetFormatProperties2), false},
					{"vkGetPhysicalDeviceImageFormatProperties2KHR", GET_TRAMPOLINE(PhysicalDevice, GetImageFormatProperties2), false},
					{"vkGetPhysicalDeviceQueueFamilyProperties2KHR", GET_TRAMPOLINE(PhysicalDevice, GetQueueFamilyProperties2), false},
					{"vkGetPhysicalDeviceMemoryProperties2KHR", GET_TRAMPOLINE(PhysicalDevice, GetMemoryProperties2), false},
					{"vkGetPhysicalDeviceSparseImageFormatProperties2KHR", GET_TRAMPOLINE(PhysicalDevice, GetSparseImageFormatProperties2), false},
				}
			},
#endif
#if defined(VK_KHR_device_group)
			{
				VK_KHR_DEVICE_GROUP_EXTENSION_NAME,
				VK_KHR_DEVICE_GROUP_SPEC_VERSION,
				true,
				{
					{"vkGetDeviceGroupPeerMemoryFeaturesKHR", GET_TRAMPOLINE(Device, GetDeviceGroupPeerMemoryFeatures), false},
					{"vkCmdSetDeviceMaskKHR", GET_TRAMPOLINE(CommandBuffer, SetDeviceMask), false},
					{"vkCmdDispatchBaseKHR", GET_TRAMPOLINE(CommandBuffer, DispatchBase), false},
				}
			},
#endif
#if defined(VK_KHR_shader_draw_parameters)
			{
				VK_KHR_SHADER_DRAW_PARAMETERS_EXTENSION_NAME,
				VK_KHR_SHADER_DRAW_PARAMETERS_SPEC_VERSION,
				true,
				{
				}
			},
#endif
#if defined(VK_KHR_maintenance1)
			{
				VK_KHR_MAINTENANCE1_EXTENSION_NAME,
				VK_KHR_MAINTENANCE1_SPEC_VERSION,
				true,
				{
					{"vkTrimCommandPoolKHR", GET_TRAMPOLINE(Device, TrimCommandPool), false},
				}
			},
#endif
#if defined(VK_KHR_device_group_creation)
			{
				VK_KHR_DEVICE_GROUP_CREATION_EXTENSION_NAME,
				VK_KHR_DEVICE_GROUP_CREATION_SPEC_VERSION,
				false,
				{
					{"vkEnumeratePhysicalDeviceGroupsKHR", GET_TRAMPOLINE(Instance, EnumeratePhysicalDeviceGroups), false},
				}
			},
#endif
#if defined(VK_KHR_external_memory_capabilities)
			{
				VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME,
				VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_SPEC_VERSION,
				false,
				{
					{"vkGetPhysicalDeviceExternalBufferPropertiesKHR", GET_TRAMPOLINE(PhysicalDevice, GetExternalBufferProperties), false},
				}
			},
#endif
#if defined(VK_KHR_external_memory)
			{
				VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME,
				VK_KHR_EXTERNAL_MEMORY_SPEC_VERSION,
				true,
				{
				}
			},
#endif
#if defined(VK_KHR_external_memory_fd)
			{
				VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME,
				VK_KHR_EXTERNAL_MEMORY_FD_SPEC_VERSION,
				true,
				{
					{"vkGetMemoryFdKHR", GET_TRAMPOLINE(Device, GetMemoryFd), false},
					{"vkGetMemoryFdPropertiesKHR", GET_TRAMPOLINE(Device, GetMemoryFdProperties), false},
				}
			},
#endif
#if defined(VK_KHR_external_semaphore_capabilities)
			{
				VK_KHR_EXTERNAL_SEMAPHORE_CAPABILITIES_EXTENSION_NAME,
				VK_KHR_EXTERNAL_SEMAPHORE_CAPABILITIES_SPEC_VERSION,
				false,
				{
					{"vkGetPhysicalDeviceExternalSemaphorePropertiesKHR", GET_TRAMPOLINE(PhysicalDevice, GetExternalSemaphoreProperties), false},
				}
			},
#endif
#if defined(VK_KHR_external_semaphore)
			{
				VK_KHR_EXTERNAL_SEMAPHORE_EXTENSION_NAME,
				VK_KHR_EXTERNAL_SEMAPHORE_SPEC_VERSION,
				true,
				{
				}
			},
#endif
#if defined(VK_KHR_external_semaphore_fd)
			{
				VK_KHR_EXTERNAL_SEMAPHORE_FD_EXTENSION_NAME,
				VK_KHR_EXTERNAL_SEMAPHORE_FD_SPEC_VERSION,
				true,
				{
					{"vkImportSemaphoreFdKHR", GET_TRAMPOLINE(Device, ImportSemaphoreFd), false},
					{"vkGetSemaphoreFdKHR", GET_TRAMPOLINE(Device, GetSemaphoreFd), false},
				}
			},
#endif
#if defined(VK_KHR_push_descriptor)
			{
				VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME,
				VK_KHR_PUSH_DESCRIPTOR_SPEC_VERSION,
				true,
				{
					{"vkCmdPushDescriptorSetKHR", GET_TRAMPOLINE(CommandBuffer, PushDescriptorSet), false},
					{"vkCmdPushDescriptorSetWithTemplateKHR", GET_TRAMPOLINE(CommandBuffer, PushDescriptorSetWithTemplate), false},
				}
			},
#endif
#if defined(VK_KHR_shader_float16_int8)
			{
				VK_KHR_SHADER_FLOAT16_INT8_EXTENSION_NAME,
				VK_KHR_SHADER_FLOAT16_INT8_SPEC_VERSION,
				true,
				{
				}
			},
#endif
#if defined(VK_KHR_16bit_storage)
			{
				VK_KHR_16BIT_STORAGE_EXTENSION_NAME,
				VK_KHR_16BIT_STORAGE_SPEC_VERSION,
				true,
				{
				}
			},
#endif
#if defined(VK_KHR_incremental_present)
			{
				VK_KHR_INCREMENTAL_PRESENT_EXTENSION_NAME,
				VK_KHR_INCREMENTAL_PRESENT_SPEC_VERSION,
				true,
				{
				}
			},
#endif
#if defined(VK_KHR_descriptor_update_template)
			{
				VK_KHR_DESCRIPTOR_UPDATE_TEMPLATE_EXTENSION_NAME,
				VK_KHR_DESCRIPTOR_UPDATE_TEMPLATE_SPEC_VERSION,
				true,
				{
					{"vkCreateDescriptorUpdateTemplateKHR", GET_TRAMPOLINE(Device, CreateDescriptorUpdateTemplate), false},
					{"vkDestroyDescriptorUpdateTemplateKHR", GET_TRAMPOLINE(Device, DestroyDescriptorUpdateTemplate), false},
					{"vkUpdateDescriptorSetWithTemplateKHR", GET_TRAMPOLINE(Device, UpdateDescriptorSetWithTemplate), false},
				}
			},
#endif
#if defined(VK_KHR_imageless_framebuffer)
			{
				VK_KHR_IMAGELESS_FRAMEBUFFER_EXTENSION_NAME,
				VK_KHR_IMAGELESS_FRAMEBUFFER_SPEC_VERSION,
				true,
				{
				}
			},
#endif
#if defined(VK_KHR_create_renderpass2)
			{
				VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME,
				VK_KHR_CREATE_RENDERPASS_2_SPEC_VERSION,
				true,
				{
					{"vkCreateRenderPass2KHR", GET_TRAMPOLINE(Device, CreateRenderPass2), false},
					{"vkCmdBeginRenderPass2KHR", GET_TRAMPOLINE(CommandBuffer, BeginRenderPass2), false},
					{"vkCmdNextSubpass2KHR", GET_TRAMPOLINE(CommandBuffer, NextSubpass2), false},
					{"vkCmdEndRenderPass2KHR", GET_TRAMPOLINE(CommandBuffer, EndRenderPass2), false},
				}
			},
#endif
#if defined(VK_KHR_shared_presentable_image)
			{
				VK_KHR_SHARED_PRESENTABLE_IMAGE_EXTENSION_NAME,
				VK_KHR_SHARED_PRESENTABLE_IMAGE_SPEC_VERSION,
				true,
				{
					{"vkGetSwapchainStatusKHR", GET_TRAMPOLINE(Device, GetSwapchainStatus), false},
				}
			},
#endif
#if defined(VK_KHR_external_fence_capabilities)
			{
				VK_KHR_EXTERNAL_FENCE_CAPABILITIES_EXTENSION_NAME,
				VK_KHR_EXTERNAL_FENCE_CAPABILITIES_SPEC_VERSION,
				false,
				{
					{"vkGetPhysicalDeviceExternalFencePropertiesKHR", GET_TRAMPOLINE(PhysicalDevice, GetExternalFenceProperties), false},
				}
			},
#endif
#if defined(VK_KHR_external_fence)
			{
				VK_KHR_EXTERNAL_FENCE_EXTENSION_NAME,
				VK_KHR_EXTERNAL_FENCE_SPEC_VERSION,
				true,
				{
				}
			},
#endif
#if defined(VK_KHR_external_fence_fd)
			{
				VK_KHR_EXTERNAL_FENCE_FD_EXTENSION_NAME,
				VK_KHR_EXTERNAL_FENCE_FD_SPEC_VERSION,
				true,
				{
					{"vkImportFenceFdKHR", GET_TRAMPOLINE(Device, ImportFenceFd), false},
					{"vkGetFenceFdKHR", GET_TRAMPOLINE(Device, GetFenceFd), false},
				}
			},
#endif
#if defined(VK_KHR_maintenance2)
			{
				VK_KHR_MAINTENANCE2_EXTENSION_NAME,
				VK_KHR_MAINTENANCE2_SPEC_VERSION,
				true,
				{
				}
			},
#endif
#if defined(VK_KHR_get_surface_capabilities2)
			{
				VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME,
				VK_KHR_GET_SURFACE_CAPABILITIES_2_SPEC_VERSION,
				false,
				{
					{"vkGetPhysicalDeviceSurfaceCapabilities2KHR", GET_TRAMPOLINE(PhysicalDevice, GetSurfaceCapabilities2KHR), false},
					{"vkGetPhysicalDeviceSurfaceFormats2KHR", GET_TRAMPOLINE(PhysicalDevice, GetSurfaceFormats2), false},
				}
			},
#endif
#if defined(VK_KHR_variable_pointers)
			{
				VK_KHR_VARIABLE_POINTERS_EXTENSION_NAME,
				VK_KHR_VARIABLE_POINTERS_SPEC_VERSION,
				true,
				{
				}
			},
#endif
#if defined(VK_KHR_get_display_properties2)
			{
				VK_KHR_GET_DISPLAY_PROPERTIES_2_EXTENSION_NAME,
				VK_KHR_GET_DISPLAY_PROPERTIES_2_SPEC_VERSION,
				false,
				{
					{"vkGetPhysicalDeviceDisplayProperties2KHR", GET_TRAMPOLINE(PhysicalDevice, GetDisplayProperties2), false},
					{"vkGetPhysicalDeviceDisplayPlaneProperties2KHR", GET_TRAMPOLINE(PhysicalDevice, GetDisplayPlaneProperties2), false},
					{"vkGetDisplayModeProperties2KHR", GET_TRAMPOLINE(PhysicalDevice, GetDisplayModeProperties2), false},
					{"vkGetDisplayPlaneCapabilities2KHR", GET_TRAMPOLINE(PhysicalDevice, GetDisplayPlaneCapabilities2), false},
				}
			},
#endif
#if defined(VK_KHR_dedicated_allocation)
			{
				VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME,
				VK_KHR_DEDICATED_ALLOCATION_SPEC_VERSION,
				true,
				{
				}
			},
#endif
#if defined(VK_KHR_storage_buffer_storage_class)
			{
				VK_KHR_STORAGE_BUFFER_STORAGE_CLASS_EXTENSION_NAME,
				VK_KHR_STORAGE_BUFFER_STORAGE_CLASS_SPEC_VERSION,
				true,
				{
				}
			},
#endif
#if defined(VK_KHR_relaxed_block_layout)
			{
				VK_KHR_RELAXED_BLOCK_LAYOUT_EXTENSION_NAME,
				VK_KHR_RELAXED_BLOCK_LAYOUT_SPEC_VERSION,
				true,
				{
				}
			},
#endif
#if defined(VK_KHR_get_memory_requirements2)
			{
				VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME,
				VK_KHR_GET_MEMORY_REQUIREMENTS_2_SPEC_VERSION,
				true,
				{
					{"vkGetImageMemoryRequirements2KHR", GET_TRAMPOLINE(Device, GetImageMemoryRequirements2), false},
					{"vkGetBufferMemoryRequirements2KHR", GET_TRAMPOLINE(Device, GetBufferMemoryRequirements2), false},
					{"vkGetImageSparseMemoryRequirements2KHR", GET_TRAMPOLINE(Device, GetImageSparseMemoryRequirements2), false},
				}
			},
#endif
#if defined(VK_KHR_image_format_list)
			{
				VK_KHR_IMAGE_FORMAT_LIST_EXTENSION_NAME,
				VK_KHR_IMAGE_FORMAT_LIST_SPEC_VERSION,
				true,
				{
				}
			},
#endif
#if defined(VK_KHR_sampler_ycbcr_conversion)
			{
				VK_KHR_SAMPLER_YCBCR_CONVERSION_EXTENSION_NAME,
				VK_KHR_SAMPLER_YCBCR_CONVERSION_SPEC_VERSION,
				true,
				{
					{"vkCreateSamplerYcbcrConversionKHR", GET_TRAMPOLINE(Device, CreateSamplerYcbcrConversion), false},
					{"vkDestroySamplerYcbcrConversionKHR", GET_TRAMPOLINE(Device, DestroySamplerYcbcrConversion), false},
				}
			},
#endif
#if defined(VK_KHR_bind_memory2)
			{
				VK_KHR_BIND_MEMORY_2_EXTENSION_NAME,
				VK_KHR_BIND_MEMORY_2_SPEC_VERSION,
				true,
				{
					{"vkBindBufferMemory2KHR", GET_TRAMPOLINE(Device, BindBufferMemory2), false},
					{"vkBindImageMemory2KHR", GET_TRAMPOLINE(Device, BindImageMemory2), false},
				}
			},
#endif
#if defined(VK_KHR_maintenance3)
			{
				VK_KHR_MAINTENANCE3_EXTENSION_NAME,
				VK_KHR_MAINTENANCE3_SPEC_VERSION,
				true,
				{
					{"vkGetDescriptorSetLayoutSupportKHR", GET_TRAMPOLINE(Device, GetDescriptorSetLayoutSupport), false},
				}
			},
#endif
#if defined(VK_KHR_draw_indirect_count)
			{
				VK_KHR_DRAW_INDIRECT_COUNT_EXTENSION_NAME,
				VK_KHR_DRAW_INDIRECT_COUNT_SPEC_VERSION,
				true,
				{
					{"vkCmdDrawIndirectCountKHR", GET_TRAMPOLINE(CommandBuffer, DrawIndirectCount), false},
					{"vkCmdDrawIndexedIndirectCountKHR", GET_TRAMPOLINE(CommandBuffer, DrawIndexedIndirectCount), false},
				}
			},
#endif
#if defined(VK_KHR_8bit_storage)
			{
				VK_KHR_8BIT_STORAGE_EXTENSION_NAME,
				VK_KHR_8BIT_STORAGE_SPEC_VERSION,
				true,
				{
				}
			},
#endif
#if defined(VK_KHR_shader_atomic_int64)
			{
				VK_KHR_SHADER_ATOMIC_INT64_EXTENSION_NAME,
				VK_KHR_SHADER_ATOMIC_INT64_SPEC_VERSION,
				true,
				{
				}
			},
#endif
#if defined(VK_KHR_driver_properties)
			{
				VK_KHR_DRIVER_PROPERTIES_EXTENSION_NAME,
				VK_KHR_DRIVER_PROPERTIES_SPEC_VERSION,
				true,
				{
				}
			},
#endif
#if defined(VK_KHR_shader_float_controls)
			{
				VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME,
				VK_KHR_SHADER_FLOAT_CONTROLS_SPEC_VERSION,
				true,
				{
				}
			},
#endif
#if defined(VK_KHR_depth_stencil_resolve)
			{
				VK_KHR_DEPTH_STENCIL_RESOLVE_EXTENSION_NAME,
				VK_KHR_DEPTH_STENCIL_RESOLVE_SPEC_VERSION,
				true,
				{
				}
			},
#endif
#if defined(VK_KHR_swapchain_mutable_format)
			{
				VK_KHR_SWAPCHAIN_MUTABLE_FORMAT_EXTENSION_NAME,
				VK_KHR_SWAPCHAIN_MUTABLE_FORMAT_SPEC_VERSION,
				true,
				{
				}
			},
#endif
#if defined(VK_KHR_vulkan_memory_model)
			{
				VK_KHR_VULKAN_MEMORY_MODEL_EXTENSION_NAME,
				VK_KHR_VULKAN_MEMORY_MODEL_SPEC_VERSION,
				true,
				{
				}
			},
#endif
#if defined(VK_KHR_surface_protected_capabilities)
			{
				VK_KHR_SURFACE_PROTECTED_CAPABILITIES_EXTENSION_NAME,
				VK_KHR_SURFACE_PROTECTED_CAPABILITIES_SPEC_VERSION,
				false,
				{
				}
			},
#endif
#if defined(VK_KHR_uniform_buffer_standard_layout)
			{
				VK_KHR_UNIFORM_BUFFER_STANDARD_LAYOUT_EXTENSION_NAME,
				VK_KHR_UNIFORM_BUFFER_STANDARD_LAYOUT_SPEC_VERSION,
				true,
				{
				}
			},
#endif
#if defined(VK_KHR_pipeline_executable_properties)
			{
				VK_KHR_PIPELINE_EXECUTABLE_PROPERTIES_EXTENSION_NAME,
				VK_KHR_PIPELINE_EXECUTABLE_PROPERTIES_SPEC_VERSION,
				true,
				{
					{"vkGetPipelineExecutablePropertiesKHR", GET_TRAMPOLINE(Device, GetPipelineExecutableProperties), false},
					{"vkGetPipelineExecutableStatisticsKHR", GET_TRAMPOLINE(Device, GetPipelineExecutableStatistics), false},
					{"vkGetPipelineExecutableInternalRepresentationsKHR", GET_TRAMPOLINE(Device, GetPipelineExecutableInternalRepresentations), false},
				}
			},
#endif
#if defined(VK_EXT_debug_report)
			{
				VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
				VK_EXT_DEBUG_REPORT_SPEC_VERSION,
				false,
				{
					{"vkCreateDebugReportCallbackEXT", GET_TRAMPOLINE(Instance, CreateDebugReportCallback), false},
					{"vkDestroyDebugReportCallbackEXT", GET_TRAMPOLINE(Instance, DestroyDebugReportCallback), false},
					{"vkDebugReportMessageEXT", GET_TRAMPOLINE(Instance, DebugReportMessage), false},
				}
			},
#endif
#if defined(VK_NV_glsl_shader)
			{
				VK_NV_GLSL_SHADER_EXTENSION_NAME,
				VK_NV_GLSL_SHADER_SPEC_VERSION,
				true,
				{
				}
			},
#endif
#if defined(VK_EXT_depth_range_unrestricted)
			{
				VK_EXT_DEPTH_RANGE_UNRESTRICTED_EXTENSION_NAME,
				VK_EXT_DEPTH_RANGE_UNRESTRICTED_SPEC_VERSION,
				true,
				{
				}
			},
#endif
#if defined(VK_IMG_filter_cubic)
			{
				VK_IMG_FILTER_CUBIC_EXTENSION_NAME,
				VK_IMG_FILTER_CUBIC_SPEC_VERSION,
				true,
				{
				}
			},
#endif
#if defined(VK_AMD_rasterization_order)
			{
				VK_AMD_RASTERIZATION_ORDER_EXTENSION_NAME,
				VK_AMD_RASTERIZATION_ORDER_SPEC_VERSION,
				true,
				{
				}
			},
#endif
#if defined(VK_AMD_shader_trinary_minmax)
			{
				VK_AMD_SHADER_TRINARY_MINMAX_EXTENSION_NAME,
				VK_AMD_SHADER_TRINARY_MINMAX_SPEC_VERSION,
				true,
				{
				}
			},
#endif
#if defined(VK_AMD_shader_explicit_vertex_parameter)
			{
				VK_AMD_SHADER_EXPLICIT_VERTEX_PARAMETER_EXTENSION_NAME,
				VK_AMD_SHADER_EXPLICIT_VERTEX_PARAMETER_SPEC_VERSION,
				true,
				{
				}
			},
#endif
#if defined(VK_EXT_debug_marker)
			{
				VK_EXT_DEBUG_MARKER_EXTENSION_NAME,
				VK_EXT_DEBUG_MARKER_SPEC_VERSION,
				true,
				{
					{"vkDebugMarkerSetObjectTagEXT", GET_TRAMPOLINE(Device, DebugMarkerSetObjectTag), false},
					{"vkDebugMarkerSetObjectNameEXT", GET_TRAMPOLINE(Device, DebugMarkerSetObjectName), false},
					{"vkCmdDebugMarkerBeginEXT", GET_TRAMPOLINE(CommandBuffer, DebugMarkerBegin), false},
					{"vkCmdDebugMarkerEndEXT", GET_TRAMPOLINE(CommandBuffer, DebugMarkerEnd), false},
					{"vkCmdDebugMarkerInsertEXT", GET_TRAMPOLINE(CommandBuffer, DebugMarkerInsert), false},
				}
			},
#endif
#if XXX
#define VK_AMD_gcn_shader 1
#define VK_AMD_GCN_SHADER_SPEC_VERSION    1
#define VK_AMD_GCN_SHADER_EXTENSION_NAME  "VK_AMD_gcn_shader"


#define VK_NV_dedicated_allocation 1
#define VK_NV_DEDICATED_ALLOCATION_SPEC_VERSION 1
#define VK_NV_DEDICATED_ALLOCATION_EXTENSION_NAME "VK_NV_dedicated_allocation"
			typedef struct VkDedicatedAllocationImageCreateInfoNV {
				VkStructureType    sType;
				const void* pNext;
				VkBool32           dedicatedAllocation;
			} VkDedicatedAllocationImageCreateInfoNV;

			typedef struct VkDedicatedAllocationBufferCreateInfoNV {
				VkStructureType    sType;
				const void* pNext;
				VkBool32           dedicatedAllocation;
			} VkDedicatedAllocationBufferCreateInfoNV;

			typedef struct VkDedicatedAllocationMemoryAllocateInfoNV {
				VkStructureType    sType;
				const void* pNext;
				VkImage            image;
				VkBuffer           buffer;
			} VkDedicatedAllocationMemoryAllocateInfoNV;



#define VK_EXT_transform_feedback 1
#define VK_EXT_TRANSFORM_FEEDBACK_SPEC_VERSION 1
#define VK_EXT_TRANSFORM_FEEDBACK_EXTENSION_NAME "VK_EXT_transform_feedback"
			typedef VkFlags VkPipelineRasterizationStateStreamCreateFlagsEXT;
			typedef struct VkPhysicalDeviceTransformFeedbackFeaturesEXT {
				VkStructureType    sType;
				void* pNext;
				VkBool32           transformFeedback;
				VkBool32           geometryStreams;
			} VkPhysicalDeviceTransformFeedbackFeaturesEXT;

			typedef struct VkPhysicalDeviceTransformFeedbackPropertiesEXT {
				VkStructureType    sType;
				void* pNext;
				uint32_t           maxTransformFeedbackStreams;
				uint32_t           maxTransformFeedbackBuffers;
				VkDeviceSize       maxTransformFeedbackBufferSize;
				uint32_t           maxTransformFeedbackStreamDataSize;
				uint32_t           maxTransformFeedbackBufferDataSize;
				uint32_t           maxTransformFeedbackBufferDataStride;
				VkBool32           transformFeedbackQueries;
				VkBool32           transformFeedbackStreamsLinesTriangles;
				VkBool32           transformFeedbackRasterizationStreamSelect;
				VkBool32           transformFeedbackDraw;
			} VkPhysicalDeviceTransformFeedbackPropertiesEXT;

			typedef struct VkPipelineRasterizationStateStreamCreateInfoEXT {
				VkStructureType                                     sType;
				const void* pNext;
				VkPipelineRasterizationStateStreamCreateFlagsEXT    flags;
				uint32_t                                            rasterizationStream;
			} VkPipelineRasterizationStateStreamCreateInfoEXT;

			typedef void (VKAPI_PTR* PFN_vkCmdBindTransformFeedbackBuffersEXT)(VkCommandBuffer commandBuffer, uint32_t firstBinding, uint32_t bindingCount, const VkBuffer* pBuffers, const VkDeviceSize* pOffsets, const VkDeviceSize* pSizes);
			typedef void (VKAPI_PTR* PFN_vkCmdBeginTransformFeedbackEXT)(VkCommandBuffer commandBuffer, uint32_t firstCounterBuffer, uint32_t counterBufferCount, const VkBuffer* pCounterBuffers, const VkDeviceSize* pCounterBufferOffsets);
			typedef void (VKAPI_PTR* PFN_vkCmdEndTransformFeedbackEXT)(VkCommandBuffer commandBuffer, uint32_t firstCounterBuffer, uint32_t counterBufferCount, const VkBuffer* pCounterBuffers, const VkDeviceSize* pCounterBufferOffsets);
			typedef void (VKAPI_PTR* PFN_vkCmdBeginQueryIndexedEXT)(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t query, VkQueryControlFlags flags, uint32_t index);
			typedef void (VKAPI_PTR* PFN_vkCmdEndQueryIndexedEXT)(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t query, uint32_t index);
			typedef void (VKAPI_PTR* PFN_vkCmdDrawIndirectByteCountEXT)(VkCommandBuffer commandBuffer, uint32_t instanceCount, uint32_t firstInstance, VkBuffer counterBuffer, VkDeviceSize counterBufferOffset, uint32_t counterOffset, uint32_t vertexStride);

#ifndef VK_NO_PROTOTYPES
			VKAPI_ATTR void VKAPI_CALL vkCmdBindTransformFeedbackBuffersEXT(
				VkCommandBuffer                             commandBuffer,
				uint32_t                                    firstBinding,
				uint32_t                                    bindingCount,
				const VkBuffer* pBuffers,
				const VkDeviceSize* pOffsets,
				const VkDeviceSize* pSizes);

			VKAPI_ATTR void VKAPI_CALL vkCmdBeginTransformFeedbackEXT(
				VkCommandBuffer                             commandBuffer,
				uint32_t                                    firstCounterBuffer,
				uint32_t                                    counterBufferCount,
				const VkBuffer* pCounterBuffers,
				const VkDeviceSize* pCounterBufferOffsets);

			VKAPI_ATTR void VKAPI_CALL vkCmdEndTransformFeedbackEXT(
				VkCommandBuffer                             commandBuffer,
				uint32_t                                    firstCounterBuffer,
				uint32_t                                    counterBufferCount,
				const VkBuffer* pCounterBuffers,
				const VkDeviceSize* pCounterBufferOffsets);

			VKAPI_ATTR void VKAPI_CALL vkCmdBeginQueryIndexedEXT(
				VkCommandBuffer                             commandBuffer,
				VkQueryPool                                 queryPool,
				uint32_t                                    query,
				VkQueryControlFlags                         flags,
				uint32_t                                    index);

			VKAPI_ATTR void VKAPI_CALL vkCmdEndQueryIndexedEXT(
				VkCommandBuffer                             commandBuffer,
				VkQueryPool                                 queryPool,
				uint32_t                                    query,
				uint32_t                                    index);

			VKAPI_ATTR void VKAPI_CALL vkCmdDrawIndirectByteCountEXT(
				VkCommandBuffer                             commandBuffer,
				uint32_t                                    instanceCount,
				uint32_t                                    firstInstance,
				VkBuffer                                    counterBuffer,
				VkDeviceSize                                counterBufferOffset,
				uint32_t                                    counterOffset,
				uint32_t                                    vertexStride);
#endif


#define VK_NVX_image_view_handle 1
#define VK_NVX_IMAGE_VIEW_HANDLE_SPEC_VERSION 1
#define VK_NVX_IMAGE_VIEW_HANDLE_EXTENSION_NAME "VK_NVX_image_view_handle"
			typedef struct VkImageViewHandleInfoNVX {
				VkStructureType     sType;
				const void* pNext;
				VkImageView         imageView;
				VkDescriptorType    descriptorType;
				VkSampler           sampler;
			} VkImageViewHandleInfoNVX;

			typedef uint32_t(VKAPI_PTR* PFN_vkGetImageViewHandleNVX)(VkDevice device, const VkImageViewHandleInfoNVX* pInfo);

#ifndef VK_NO_PROTOTYPES
			VKAPI_ATTR uint32_t VKAPI_CALL vkGetImageViewHandleNVX(
				VkDevice                                    device,
				const VkImageViewHandleInfoNVX* pInfo);
#endif


#define VK_AMD_draw_indirect_count 1
#define VK_AMD_DRAW_INDIRECT_COUNT_SPEC_VERSION 1
#define VK_AMD_DRAW_INDIRECT_COUNT_EXTENSION_NAME "VK_AMD_draw_indirect_count"
			typedef void (VKAPI_PTR * PFN_vkCmdDrawIndirectCountAMD)(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, VkBuffer countBuffer, VkDeviceSize countBufferOffset, uint32_t maxDrawCount, uint32_t stride);
			typedef void (VKAPI_PTR* PFN_vkCmdDrawIndexedIndirectCountAMD)(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, VkBuffer countBuffer, VkDeviceSize countBufferOffset, uint32_t maxDrawCount, uint32_t stride);

#ifndef VK_NO_PROTOTYPES
			VKAPI_ATTR void VKAPI_CALL vkCmdDrawIndirectCountAMD(
				VkCommandBuffer                             commandBuffer,
				VkBuffer                                    buffer,
				VkDeviceSize                                offset,
				VkBuffer                                    countBuffer,
				VkDeviceSize                                countBufferOffset,
				uint32_t                                    maxDrawCount,
				uint32_t                                    stride);

			VKAPI_ATTR void VKAPI_CALL vkCmdDrawIndexedIndirectCountAMD(
				VkCommandBuffer                             commandBuffer,
				VkBuffer                                    buffer,
				VkDeviceSize                                offset,
				VkBuffer                                    countBuffer,
				VkDeviceSize                                countBufferOffset,
				uint32_t                                    maxDrawCount,
				uint32_t                                    stride);
#endif


#define VK_AMD_negative_viewport_height 1
#define VK_AMD_NEGATIVE_VIEWPORT_HEIGHT_SPEC_VERSION 1
#define VK_AMD_NEGATIVE_VIEWPORT_HEIGHT_EXTENSION_NAME "VK_AMD_negative_viewport_height"


#define VK_AMD_gpu_shader_half_float 1
#define VK_AMD_GPU_SHADER_HALF_FLOAT_SPEC_VERSION 2
#define VK_AMD_GPU_SHADER_HALF_FLOAT_EXTENSION_NAME "VK_AMD_gpu_shader_half_float"


#define VK_AMD_shader_ballot 1
#define VK_AMD_SHADER_BALLOT_SPEC_VERSION 1
#define VK_AMD_SHADER_BALLOT_EXTENSION_NAME "VK_AMD_shader_ballot"


#define VK_AMD_texture_gather_bias_lod 1
#define VK_AMD_TEXTURE_GATHER_BIAS_LOD_SPEC_VERSION 1
#define VK_AMD_TEXTURE_GATHER_BIAS_LOD_EXTENSION_NAME "VK_AMD_texture_gather_bias_lod"
			typedef struct VkTextureLODGatherFormatPropertiesAMD {
				VkStructureType    sType;
				void* pNext;
				VkBool32           supportsTextureGatherLODBiasAMD;
			} VkTextureLODGatherFormatPropertiesAMD;



#define VK_AMD_shader_info 1
#define VK_AMD_SHADER_INFO_SPEC_VERSION   1
#define VK_AMD_SHADER_INFO_EXTENSION_NAME "VK_AMD_shader_info"

			typedef enum VkShaderInfoTypeAMD {
				VK_SHADER_INFO_TYPE_STATISTICS_AMD = 0,
				VK_SHADER_INFO_TYPE_BINARY_AMD = 1,
				VK_SHADER_INFO_TYPE_DISASSEMBLY_AMD = 2,
				VK_SHADER_INFO_TYPE_BEGIN_RANGE_AMD = VK_SHADER_INFO_TYPE_STATISTICS_AMD,
				VK_SHADER_INFO_TYPE_END_RANGE_AMD = VK_SHADER_INFO_TYPE_DISASSEMBLY_AMD,
				VK_SHADER_INFO_TYPE_RANGE_SIZE_AMD = (VK_SHADER_INFO_TYPE_DISASSEMBLY_AMD - VK_SHADER_INFO_TYPE_STATISTICS_AMD + 1),
				VK_SHADER_INFO_TYPE_MAX_ENUM_AMD = 0x7FFFFFFF
			} VkShaderInfoTypeAMD;
			typedef struct VkShaderResourceUsageAMD {
				uint32_t    numUsedVgprs;
				uint32_t    numUsedSgprs;
				uint32_t    ldsSizePerLocalWorkGroup;
				size_t      ldsUsageSizeInBytes;
				size_t      scratchMemUsageInBytes;
			} VkShaderResourceUsageAMD;

			typedef struct VkShaderStatisticsInfoAMD {
				VkShaderStageFlags          shaderStageMask;
				VkShaderResourceUsageAMD    resourceUsage;
				uint32_t                    numPhysicalVgprs;
				uint32_t                    numPhysicalSgprs;
				uint32_t                    numAvailableVgprs;
				uint32_t                    numAvailableSgprs;
				uint32_t                    computeWorkGroupSize[3];
			} VkShaderStatisticsInfoAMD;

			typedef VkResult(VKAPI_PTR* PFN_vkGetShaderInfoAMD)(VkDevice device, VkPipeline pipeline, VkShaderStageFlagBits shaderStage, VkShaderInfoTypeAMD infoType, size_t* pInfoSize, void* pInfo);

#ifndef VK_NO_PROTOTYPES
			VKAPI_ATTR VkResult VKAPI_CALL vkGetShaderInfoAMD(
				VkDevice                                    device,
				VkPipeline                                  pipeline,
				VkShaderStageFlagBits                       shaderStage,
				VkShaderInfoTypeAMD                         infoType,
				size_t* pInfoSize,
				void* pInfo);
#endif


#define VK_AMD_shader_image_load_store_lod 1
#define VK_AMD_SHADER_IMAGE_LOAD_STORE_LOD_SPEC_VERSION 1
#define VK_AMD_SHADER_IMAGE_LOAD_STORE_LOD_EXTENSION_NAME "VK_AMD_shader_image_load_store_lod"


#define VK_NV_corner_sampled_image 1
#define VK_NV_CORNER_SAMPLED_IMAGE_SPEC_VERSION 2
#define VK_NV_CORNER_SAMPLED_IMAGE_EXTENSION_NAME "VK_NV_corner_sampled_image"
			typedef struct VkPhysicalDeviceCornerSampledImageFeaturesNV {
				VkStructureType    sType;
				void* pNext;
				VkBool32           cornerSampledImage;
			} VkPhysicalDeviceCornerSampledImageFeaturesNV;



#define VK_IMG_format_pvrtc 1
#define VK_IMG_FORMAT_PVRTC_SPEC_VERSION  1
#define VK_IMG_FORMAT_PVRTC_EXTENSION_NAME "VK_IMG_format_pvrtc"


#define VK_NV_external_memory_capabilities 1
#define VK_NV_EXTERNAL_MEMORY_CAPABILITIES_SPEC_VERSION 1
#define VK_NV_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME "VK_NV_external_memory_capabilities"

			typedef enum VkExternalMemoryHandleTypeFlagBitsNV {
				VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT_NV = 0x00000001,
				VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_KMT_BIT_NV = 0x00000002,
				VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D11_IMAGE_BIT_NV = 0x00000004,
				VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D11_IMAGE_KMT_BIT_NV = 0x00000008,
				VK_EXTERNAL_MEMORY_HANDLE_TYPE_FLAG_BITS_MAX_ENUM_NV = 0x7FFFFFFF
			} VkExternalMemoryHandleTypeFlagBitsNV;
			typedef VkFlags VkExternalMemoryHandleTypeFlagsNV;

			typedef enum VkExternalMemoryFeatureFlagBitsNV {
				VK_EXTERNAL_MEMORY_FEATURE_DEDICATED_ONLY_BIT_NV = 0x00000001,
				VK_EXTERNAL_MEMORY_FEATURE_EXPORTABLE_BIT_NV = 0x00000002,
				VK_EXTERNAL_MEMORY_FEATURE_IMPORTABLE_BIT_NV = 0x00000004,
				VK_EXTERNAL_MEMORY_FEATURE_FLAG_BITS_MAX_ENUM_NV = 0x7FFFFFFF
			} VkExternalMemoryFeatureFlagBitsNV;
			typedef VkFlags VkExternalMemoryFeatureFlagsNV;
			typedef struct VkExternalImageFormatPropertiesNV {
				VkImageFormatProperties              imageFormatProperties;
				VkExternalMemoryFeatureFlagsNV       externalMemoryFeatures;
				VkExternalMemoryHandleTypeFlagsNV    exportFromImportedHandleTypes;
				VkExternalMemoryHandleTypeFlagsNV    compatibleHandleTypes;
			} VkExternalImageFormatPropertiesNV;

			typedef VkResult(VKAPI_PTR* PFN_vkGetPhysicalDeviceExternalImageFormatPropertiesNV)(VkPhysicalDevice physicalDevice, VkFormat format, VkImageType type, VkImageTiling tiling, VkImageUsageFlags usage, VkImageCreateFlags flags, VkExternalMemoryHandleTypeFlagsNV externalHandleType, VkExternalImageFormatPropertiesNV* pExternalImageFormatProperties);

#ifndef VK_NO_PROTOTYPES
			VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceExternalImageFormatPropertiesNV(
				VkPhysicalDevice                            physicalDevice,
				VkFormat                                    format,
				VkImageType                                 type,
				VkImageTiling                               tiling,
				VkImageUsageFlags                           usage,
				VkImageCreateFlags                          flags,
				VkExternalMemoryHandleTypeFlagsNV           externalHandleType,
				VkExternalImageFormatPropertiesNV* pExternalImageFormatProperties);
#endif


#define VK_NV_external_memory 1
#define VK_NV_EXTERNAL_MEMORY_SPEC_VERSION 1
#define VK_NV_EXTERNAL_MEMORY_EXTENSION_NAME "VK_NV_external_memory"
			typedef struct VkExternalMemoryImageCreateInfoNV {
				VkStructureType                      sType;
				const void* pNext;
				VkExternalMemoryHandleTypeFlagsNV    handleTypes;
			} VkExternalMemoryImageCreateInfoNV;

			typedef struct VkExportMemoryAllocateInfoNV {
				VkStructureType                      sType;
				const void* pNext;
				VkExternalMemoryHandleTypeFlagsNV    handleTypes;
			} VkExportMemoryAllocateInfoNV;



#define VK_EXT_validation_flags 1
#define VK_EXT_VALIDATION_FLAGS_SPEC_VERSION 1
#define VK_EXT_VALIDATION_FLAGS_EXTENSION_NAME "VK_EXT_validation_flags"

			typedef enum VkValidationCheckEXT {
				VK_VALIDATION_CHECK_ALL_EXT = 0,
				VK_VALIDATION_CHECK_SHADERS_EXT = 1,
				VK_VALIDATION_CHECK_BEGIN_RANGE_EXT = VK_VALIDATION_CHECK_ALL_EXT,
				VK_VALIDATION_CHECK_END_RANGE_EXT = VK_VALIDATION_CHECK_SHADERS_EXT,
				VK_VALIDATION_CHECK_RANGE_SIZE_EXT = (VK_VALIDATION_CHECK_SHADERS_EXT - VK_VALIDATION_CHECK_ALL_EXT + 1),
				VK_VALIDATION_CHECK_MAX_ENUM_EXT = 0x7FFFFFFF
			} VkValidationCheckEXT;
			typedef struct VkValidationFlagsEXT {
				VkStructureType                sType;
				const void* pNext;
				uint32_t                       disabledValidationCheckCount;
				const VkValidationCheckEXT* pDisabledValidationChecks;
			} VkValidationFlagsEXT;



#define VK_EXT_shader_subgroup_ballot 1
#define VK_EXT_SHADER_SUBGROUP_BALLOT_SPEC_VERSION 1
#define VK_EXT_SHADER_SUBGROUP_BALLOT_EXTENSION_NAME "VK_EXT_shader_subgroup_ballot"


#define VK_EXT_shader_subgroup_vote 1
#define VK_EXT_SHADER_SUBGROUP_VOTE_SPEC_VERSION 1
#define VK_EXT_SHADER_SUBGROUP_VOTE_EXTENSION_NAME "VK_EXT_shader_subgroup_vote"


#define VK_EXT_texture_compression_astc_hdr 1
#define VK_EXT_TEXTURE_COMPRESSION_ASTC_HDR_SPEC_VERSION 1
#define VK_EXT_TEXTURE_COMPRESSION_ASTC_HDR_EXTENSION_NAME "VK_EXT_texture_compression_astc_hdr"
typedef struct VkPhysicalDeviceTextureCompressionASTCHDRFeaturesEXT {
    VkStructureType    sType;
    const void*        pNext;
    VkBool32           textureCompressionASTC_HDR;
} VkPhysicalDeviceTextureCompressionASTCHDRFeaturesEXT;


#define VK_EXT_astc_decode_mode 1
#define VK_EXT_ASTC_DECODE_MODE_SPEC_VERSION 1
#define VK_EXT_ASTC_DECODE_MODE_EXTENSION_NAME "VK_EXT_astc_decode_mode"
			typedef struct VkImageViewASTCDecodeModeEXT {
				VkStructureType    sType;
				const void* pNext;
				VkFormat           decodeMode;
			} VkImageViewASTCDecodeModeEXT;

			typedef struct VkPhysicalDeviceASTCDecodeFeaturesEXT {
				VkStructureType    sType;
				void* pNext;
				VkBool32           decodeModeSharedExponent;
			} VkPhysicalDeviceASTCDecodeFeaturesEXT;
#endif
#if defined(VK_EXT_conditional_rendering)
			{
				VK_EXT_CONDITIONAL_RENDERING_EXTENSION_NAME,
				VK_EXT_CONDITIONAL_RENDERING_SPEC_VERSION,
				true,
				{
					{"vkCmdBeginConditionalRenderingEXT", GET_TRAMPOLINE(CommandBuffer, EndConditionalRendering), false},
					{"vkCmdEndConditionalRenderingEXT", GET_TRAMPOLINE(CommandBuffer, EndConditionalRendering), false},
				}
			},
#endif
#ifdef XXX
#define VK_NVX_device_generated_commands 1
			VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkObjectTableNVX)
				VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkIndirectCommandsLayoutNVX)
#define VK_NVX_DEVICE_GENERATED_COMMANDS_SPEC_VERSION 3
#define VK_NVX_DEVICE_GENERATED_COMMANDS_EXTENSION_NAME "VK_NVX_device_generated_commands"

				typedef enum VkIndirectCommandsTokenTypeNVX {
				VK_INDIRECT_COMMANDS_TOKEN_TYPE_PIPELINE_NVX = 0,
				VK_INDIRECT_COMMANDS_TOKEN_TYPE_DESCRIPTOR_SET_NVX = 1,
				VK_INDIRECT_COMMANDS_TOKEN_TYPE_INDEX_BUFFER_NVX = 2,
				VK_INDIRECT_COMMANDS_TOKEN_TYPE_VERTEX_BUFFER_NVX = 3,
				VK_INDIRECT_COMMANDS_TOKEN_TYPE_PUSH_CONSTANT_NVX = 4,
				VK_INDIRECT_COMMANDS_TOKEN_TYPE_DRAW_INDEXED_NVX = 5,
				VK_INDIRECT_COMMANDS_TOKEN_TYPE_DRAW_NVX = 6,
				VK_INDIRECT_COMMANDS_TOKEN_TYPE_DISPATCH_NVX = 7,
				VK_INDIRECT_COMMANDS_TOKEN_TYPE_BEGIN_RANGE_NVX = VK_INDIRECT_COMMANDS_TOKEN_TYPE_PIPELINE_NVX,
				VK_INDIRECT_COMMANDS_TOKEN_TYPE_END_RANGE_NVX = VK_INDIRECT_COMMANDS_TOKEN_TYPE_DISPATCH_NVX,
				VK_INDIRECT_COMMANDS_TOKEN_TYPE_RANGE_SIZE_NVX = (VK_INDIRECT_COMMANDS_TOKEN_TYPE_DISPATCH_NVX - VK_INDIRECT_COMMANDS_TOKEN_TYPE_PIPELINE_NVX + 1),
				VK_INDIRECT_COMMANDS_TOKEN_TYPE_MAX_ENUM_NVX = 0x7FFFFFFF
			} VkIndirectCommandsTokenTypeNVX;

			typedef enum VkObjectEntryTypeNVX {
				VK_OBJECT_ENTRY_TYPE_DESCRIPTOR_SET_NVX = 0,
				VK_OBJECT_ENTRY_TYPE_PIPELINE_NVX = 1,
				VK_OBJECT_ENTRY_TYPE_INDEX_BUFFER_NVX = 2,
				VK_OBJECT_ENTRY_TYPE_VERTEX_BUFFER_NVX = 3,
				VK_OBJECT_ENTRY_TYPE_PUSH_CONSTANT_NVX = 4,
				VK_OBJECT_ENTRY_TYPE_BEGIN_RANGE_NVX = VK_OBJECT_ENTRY_TYPE_DESCRIPTOR_SET_NVX,
				VK_OBJECT_ENTRY_TYPE_END_RANGE_NVX = VK_OBJECT_ENTRY_TYPE_PUSH_CONSTANT_NVX,
				VK_OBJECT_ENTRY_TYPE_RANGE_SIZE_NVX = (VK_OBJECT_ENTRY_TYPE_PUSH_CONSTANT_NVX - VK_OBJECT_ENTRY_TYPE_DESCRIPTOR_SET_NVX + 1),
				VK_OBJECT_ENTRY_TYPE_MAX_ENUM_NVX = 0x7FFFFFFF
			} VkObjectEntryTypeNVX;

			typedef enum VkIndirectCommandsLayoutUsageFlagBitsNVX {
				VK_INDIRECT_COMMANDS_LAYOUT_USAGE_UNORDERED_SEQUENCES_BIT_NVX = 0x00000001,
				VK_INDIRECT_COMMANDS_LAYOUT_USAGE_SPARSE_SEQUENCES_BIT_NVX = 0x00000002,
				VK_INDIRECT_COMMANDS_LAYOUT_USAGE_EMPTY_EXECUTIONS_BIT_NVX = 0x00000004,
				VK_INDIRECT_COMMANDS_LAYOUT_USAGE_INDEXED_SEQUENCES_BIT_NVX = 0x00000008,
				VK_INDIRECT_COMMANDS_LAYOUT_USAGE_FLAG_BITS_MAX_ENUM_NVX = 0x7FFFFFFF
			} VkIndirectCommandsLayoutUsageFlagBitsNVX;
			typedef VkFlags VkIndirectCommandsLayoutUsageFlagsNVX;

			typedef enum VkObjectEntryUsageFlagBitsNVX {
				VK_OBJECT_ENTRY_USAGE_GRAPHICS_BIT_NVX = 0x00000001,
				VK_OBJECT_ENTRY_USAGE_COMPUTE_BIT_NVX = 0x00000002,
				VK_OBJECT_ENTRY_USAGE_FLAG_BITS_MAX_ENUM_NVX = 0x7FFFFFFF
			} VkObjectEntryUsageFlagBitsNVX;
			typedef VkFlags VkObjectEntryUsageFlagsNVX;
			typedef struct VkDeviceGeneratedCommandsFeaturesNVX {
				VkStructureType    sType;
				const void* pNext;
				VkBool32           computeBindingPointSupport;
			} VkDeviceGeneratedCommandsFeaturesNVX;

			typedef struct VkDeviceGeneratedCommandsLimitsNVX {
				VkStructureType    sType;
				const void* pNext;
				uint32_t           maxIndirectCommandsLayoutTokenCount;
				uint32_t           maxObjectEntryCounts;
				uint32_t           minSequenceCountBufferOffsetAlignment;
				uint32_t           minSequenceIndexBufferOffsetAlignment;
				uint32_t           minCommandsTokenBufferOffsetAlignment;
			} VkDeviceGeneratedCommandsLimitsNVX;

			typedef struct VkIndirectCommandsTokenNVX {
				VkIndirectCommandsTokenTypeNVX    tokenType;
				VkBuffer                          buffer;
				VkDeviceSize                      offset;
			} VkIndirectCommandsTokenNVX;

			typedef struct VkIndirectCommandsLayoutTokenNVX {
				VkIndirectCommandsTokenTypeNVX    tokenType;
				uint32_t                          bindingUnit;
				uint32_t                          dynamicCount;
				uint32_t                          divisor;
			} VkIndirectCommandsLayoutTokenNVX;

			typedef struct VkIndirectCommandsLayoutCreateInfoNVX {
				VkStructureType                            sType;
				const void* pNext;
				VkPipelineBindPoint                        pipelineBindPoint;
				VkIndirectCommandsLayoutUsageFlagsNVX      flags;
				uint32_t                                   tokenCount;
				const VkIndirectCommandsLayoutTokenNVX* pTokens;
			} VkIndirectCommandsLayoutCreateInfoNVX;

			typedef struct VkCmdProcessCommandsInfoNVX {
				VkStructureType                      sType;
				const void* pNext;
				VkObjectTableNVX                     objectTable;
				VkIndirectCommandsLayoutNVX          indirectCommandsLayout;
				uint32_t                             indirectCommandsTokenCount;
				const VkIndirectCommandsTokenNVX* pIndirectCommandsTokens;
				uint32_t                             maxSequencesCount;
				VkCommandBuffer                      targetCommandBuffer;
				VkBuffer                             sequencesCountBuffer;
				VkDeviceSize                         sequencesCountOffset;
				VkBuffer                             sequencesIndexBuffer;
				VkDeviceSize                         sequencesIndexOffset;
			} VkCmdProcessCommandsInfoNVX;

			typedef struct VkCmdReserveSpaceForCommandsInfoNVX {
				VkStructureType                sType;
				const void* pNext;
				VkObjectTableNVX               objectTable;
				VkIndirectCommandsLayoutNVX    indirectCommandsLayout;
				uint32_t                       maxSequencesCount;
			} VkCmdReserveSpaceForCommandsInfoNVX;

			typedef struct VkObjectTableCreateInfoNVX {
				VkStructureType                      sType;
				const void* pNext;
				uint32_t                             objectCount;
				const VkObjectEntryTypeNVX* pObjectEntryTypes;
				const uint32_t* pObjectEntryCounts;
				const VkObjectEntryUsageFlagsNVX* pObjectEntryUsageFlags;
				uint32_t                             maxUniformBuffersPerDescriptor;
				uint32_t                             maxStorageBuffersPerDescriptor;
				uint32_t                             maxStorageImagesPerDescriptor;
				uint32_t                             maxSampledImagesPerDescriptor;
				uint32_t                             maxPipelineLayouts;
			} VkObjectTableCreateInfoNVX;

			typedef struct VkObjectTableEntryNVX {
				VkObjectEntryTypeNVX          type;
				VkObjectEntryUsageFlagsNVX    flags;
			} VkObjectTableEntryNVX;

			typedef struct VkObjectTablePipelineEntryNVX {
				VkObjectEntryTypeNVX          type;
				VkObjectEntryUsageFlagsNVX    flags;
				VkPipeline                    pipeline;
			} VkObjectTablePipelineEntryNVX;

			typedef struct VkObjectTableDescriptorSetEntryNVX {
				VkObjectEntryTypeNVX          type;
				VkObjectEntryUsageFlagsNVX    flags;
				VkPipelineLayout              pipelineLayout;
				VkDescriptorSet               descriptorSet;
			} VkObjectTableDescriptorSetEntryNVX;

			typedef struct VkObjectTableVertexBufferEntryNVX {
				VkObjectEntryTypeNVX          type;
				VkObjectEntryUsageFlagsNVX    flags;
				VkBuffer                      buffer;
			} VkObjectTableVertexBufferEntryNVX;

			typedef struct VkObjectTableIndexBufferEntryNVX {
				VkObjectEntryTypeNVX          type;
				VkObjectEntryUsageFlagsNVX    flags;
				VkBuffer                      buffer;
				VkIndexType                   indexType;
			} VkObjectTableIndexBufferEntryNVX;

			typedef struct VkObjectTablePushConstantEntryNVX {
				VkObjectEntryTypeNVX          type;
				VkObjectEntryUsageFlagsNVX    flags;
				VkPipelineLayout              pipelineLayout;
				VkShaderStageFlags            stageFlags;
			} VkObjectTablePushConstantEntryNVX;

			typedef void (VKAPI_PTR* PFN_vkCmdProcessCommandsNVX)(VkCommandBuffer commandBuffer, const VkCmdProcessCommandsInfoNVX* pProcessCommandsInfo);
			typedef void (VKAPI_PTR* PFN_vkCmdReserveSpaceForCommandsNVX)(VkCommandBuffer commandBuffer, const VkCmdReserveSpaceForCommandsInfoNVX* pReserveSpaceInfo);
			typedef VkResult(VKAPI_PTR* PFN_vkCreateIndirectCommandsLayoutNVX)(VkDevice device, const VkIndirectCommandsLayoutCreateInfoNVX* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkIndirectCommandsLayoutNVX* pIndirectCommandsLayout);
			typedef void (VKAPI_PTR* PFN_vkDestroyIndirectCommandsLayoutNVX)(VkDevice device, VkIndirectCommandsLayoutNVX indirectCommandsLayout, const VkAllocationCallbacks* pAllocator);
			typedef VkResult(VKAPI_PTR* PFN_vkCreateObjectTableNVX)(VkDevice device, const VkObjectTableCreateInfoNVX* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkObjectTableNVX* pObjectTable);
			typedef void (VKAPI_PTR* PFN_vkDestroyObjectTableNVX)(VkDevice device, VkObjectTableNVX objectTable, const VkAllocationCallbacks* pAllocator);
			typedef VkResult(VKAPI_PTR* PFN_vkRegisterObjectsNVX)(VkDevice device, VkObjectTableNVX objectTable, uint32_t objectCount, const VkObjectTableEntryNVX* const* ppObjectTableEntries, const uint32_t* pObjectIndices);
			typedef VkResult(VKAPI_PTR* PFN_vkUnregisterObjectsNVX)(VkDevice device, VkObjectTableNVX objectTable, uint32_t objectCount, const VkObjectEntryTypeNVX* pObjectEntryTypes, const uint32_t* pObjectIndices);
			typedef void (VKAPI_PTR* PFN_vkGetPhysicalDeviceGeneratedCommandsPropertiesNVX)(VkPhysicalDevice physicalDevice, VkDeviceGeneratedCommandsFeaturesNVX* pFeatures, VkDeviceGeneratedCommandsLimitsNVX* pLimits);

#ifndef VK_NO_PROTOTYPES
			VKAPI_ATTR void VKAPI_CALL vkCmdProcessCommandsNVX(
				VkCommandBuffer                             commandBuffer,
				const VkCmdProcessCommandsInfoNVX* pProcessCommandsInfo);

			VKAPI_ATTR void VKAPI_CALL vkCmdReserveSpaceForCommandsNVX(
				VkCommandBuffer                             commandBuffer,
				const VkCmdReserveSpaceForCommandsInfoNVX* pReserveSpaceInfo);

			VKAPI_ATTR VkResult VKAPI_CALL vkCreateIndirectCommandsLayoutNVX(
				VkDevice                                    device,
				const VkIndirectCommandsLayoutCreateInfoNVX* pCreateInfo,
				const VkAllocationCallbacks* pAllocator,
				VkIndirectCommandsLayoutNVX* pIndirectCommandsLayout);

			VKAPI_ATTR void VKAPI_CALL vkDestroyIndirectCommandsLayoutNVX(
				VkDevice                                    device,
				VkIndirectCommandsLayoutNVX                 indirectCommandsLayout,
				const VkAllocationCallbacks* pAllocator);

			VKAPI_ATTR VkResult VKAPI_CALL vkCreateObjectTableNVX(
				VkDevice                                    device,
				const VkObjectTableCreateInfoNVX* pCreateInfo,
				const VkAllocationCallbacks* pAllocator,
				VkObjectTableNVX* pObjectTable);

			VKAPI_ATTR void VKAPI_CALL vkDestroyObjectTableNVX(
				VkDevice                                    device,
				VkObjectTableNVX                            objectTable,
				const VkAllocationCallbacks* pAllocator);

			VKAPI_ATTR VkResult VKAPI_CALL vkRegisterObjectsNVX(
				VkDevice                                    device,
				VkObjectTableNVX                            objectTable,
				uint32_t                                    objectCount,
				const VkObjectTableEntryNVX* const* ppObjectTableEntries,
				const uint32_t* pObjectIndices);

			VKAPI_ATTR VkResult VKAPI_CALL vkUnregisterObjectsNVX(
				VkDevice                                    device,
				VkObjectTableNVX                            objectTable,
				uint32_t                                    objectCount,
				const VkObjectEntryTypeNVX* pObjectEntryTypes,
				const uint32_t* pObjectIndices);

			VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceGeneratedCommandsPropertiesNVX(
				VkPhysicalDevice                            physicalDevice,
				VkDeviceGeneratedCommandsFeaturesNVX* pFeatures,
				VkDeviceGeneratedCommandsLimitsNVX* pLimits);
#endif


#define VK_NV_clip_space_w_scaling 1
#define VK_NV_CLIP_SPACE_W_SCALING_SPEC_VERSION 1
#define VK_NV_CLIP_SPACE_W_SCALING_EXTENSION_NAME "VK_NV_clip_space_w_scaling"
			typedef struct VkViewportWScalingNV {
				float    xcoeff;
				float    ycoeff;
			} VkViewportWScalingNV;

			typedef struct VkPipelineViewportWScalingStateCreateInfoNV {
				VkStructureType                sType;
				const void* pNext;
				VkBool32                       viewportWScalingEnable;
				uint32_t                       viewportCount;
				const VkViewportWScalingNV* pViewportWScalings;
			} VkPipelineViewportWScalingStateCreateInfoNV;

			typedef void (VKAPI_PTR* PFN_vkCmdSetViewportWScalingNV)(VkCommandBuffer commandBuffer, uint32_t firstViewport, uint32_t viewportCount, const VkViewportWScalingNV* pViewportWScalings);

#ifndef VK_NO_PROTOTYPES
			VKAPI_ATTR void VKAPI_CALL vkCmdSetViewportWScalingNV(
				VkCommandBuffer                             commandBuffer,
				uint32_t                                    firstViewport,
				uint32_t                                    viewportCount,
				const VkViewportWScalingNV* pViewportWScalings);
#endif


#define VK_EXT_direct_mode_display 1
#define VK_EXT_DIRECT_MODE_DISPLAY_SPEC_VERSION 1
#define VK_EXT_DIRECT_MODE_DISPLAY_EXTENSION_NAME "VK_EXT_direct_mode_display"
			typedef VkResult(VKAPI_PTR * PFN_vkReleaseDisplayEXT)(VkPhysicalDevice physicalDevice, VkDisplayKHR display);

#ifndef VK_NO_PROTOTYPES
			VKAPI_ATTR VkResult VKAPI_CALL vkReleaseDisplayEXT(
				VkPhysicalDevice                            physicalDevice,
				VkDisplayKHR                                display);
#endif


#define VK_EXT_display_surface_counter 1
#define VK_EXT_DISPLAY_SURFACE_COUNTER_SPEC_VERSION 1
#define VK_EXT_DISPLAY_SURFACE_COUNTER_EXTENSION_NAME "VK_EXT_display_surface_counter"

			typedef enum VkSurfaceCounterFlagBitsEXT {
				VK_SURFACE_COUNTER_VBLANK_EXT = 0x00000001,
				VK_SURFACE_COUNTER_FLAG_BITS_MAX_ENUM_EXT = 0x7FFFFFFF
			} VkSurfaceCounterFlagBitsEXT;
			typedef VkFlags VkSurfaceCounterFlagsEXT;
			typedef struct VkSurfaceCapabilities2EXT {
				VkStructureType                  sType;
				void* pNext;
				uint32_t                         minImageCount;
				uint32_t                         maxImageCount;
				VkExtent2D                       currentExtent;
				VkExtent2D                       minImageExtent;
				VkExtent2D                       maxImageExtent;
				uint32_t                         maxImageArrayLayers;
				VkSurfaceTransformFlagsKHR       supportedTransforms;
				VkSurfaceTransformFlagBitsKHR    currentTransform;
				VkCompositeAlphaFlagsKHR         supportedCompositeAlpha;
				VkImageUsageFlags                supportedUsageFlags;
				VkSurfaceCounterFlagsEXT         supportedSurfaceCounters;
			} VkSurfaceCapabilities2EXT;

			typedef VkResult(VKAPI_PTR* PFN_vkGetPhysicalDeviceSurfaceCapabilities2EXT)(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, VkSurfaceCapabilities2EXT* pSurfaceCapabilities);

#ifndef VK_NO_PROTOTYPES
			VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceSurfaceCapabilities2EXT(
				VkPhysicalDevice                            physicalDevice,
				VkSurfaceKHR                                surface,
				VkSurfaceCapabilities2EXT* pSurfaceCapabilities);
#endif


#define VK_EXT_display_control 1
#define VK_EXT_DISPLAY_CONTROL_SPEC_VERSION 1
#define VK_EXT_DISPLAY_CONTROL_EXTENSION_NAME "VK_EXT_display_control"

			typedef enum VkDisplayPowerStateEXT {
				VK_DISPLAY_POWER_STATE_OFF_EXT = 0,
				VK_DISPLAY_POWER_STATE_SUSPEND_EXT = 1,
				VK_DISPLAY_POWER_STATE_ON_EXT = 2,
				VK_DISPLAY_POWER_STATE_BEGIN_RANGE_EXT = VK_DISPLAY_POWER_STATE_OFF_EXT,
				VK_DISPLAY_POWER_STATE_END_RANGE_EXT = VK_DISPLAY_POWER_STATE_ON_EXT,
				VK_DISPLAY_POWER_STATE_RANGE_SIZE_EXT = (VK_DISPLAY_POWER_STATE_ON_EXT - VK_DISPLAY_POWER_STATE_OFF_EXT + 1),
				VK_DISPLAY_POWER_STATE_MAX_ENUM_EXT = 0x7FFFFFFF
			} VkDisplayPowerStateEXT;

			typedef enum VkDeviceEventTypeEXT {
				VK_DEVICE_EVENT_TYPE_DISPLAY_HOTPLUG_EXT = 0,
				VK_DEVICE_EVENT_TYPE_BEGIN_RANGE_EXT = VK_DEVICE_EVENT_TYPE_DISPLAY_HOTPLUG_EXT,
				VK_DEVICE_EVENT_TYPE_END_RANGE_EXT = VK_DEVICE_EVENT_TYPE_DISPLAY_HOTPLUG_EXT,
				VK_DEVICE_EVENT_TYPE_RANGE_SIZE_EXT = (VK_DEVICE_EVENT_TYPE_DISPLAY_HOTPLUG_EXT - VK_DEVICE_EVENT_TYPE_DISPLAY_HOTPLUG_EXT + 1),
				VK_DEVICE_EVENT_TYPE_MAX_ENUM_EXT = 0x7FFFFFFF
			} VkDeviceEventTypeEXT;

			typedef enum VkDisplayEventTypeEXT {
				VK_DISPLAY_EVENT_TYPE_FIRST_PIXEL_OUT_EXT = 0,
				VK_DISPLAY_EVENT_TYPE_BEGIN_RANGE_EXT = VK_DISPLAY_EVENT_TYPE_FIRST_PIXEL_OUT_EXT,
				VK_DISPLAY_EVENT_TYPE_END_RANGE_EXT = VK_DISPLAY_EVENT_TYPE_FIRST_PIXEL_OUT_EXT,
				VK_DISPLAY_EVENT_TYPE_RANGE_SIZE_EXT = (VK_DISPLAY_EVENT_TYPE_FIRST_PIXEL_OUT_EXT - VK_DISPLAY_EVENT_TYPE_FIRST_PIXEL_OUT_EXT + 1),
				VK_DISPLAY_EVENT_TYPE_MAX_ENUM_EXT = 0x7FFFFFFF
			} VkDisplayEventTypeEXT;
			typedef struct VkDisplayPowerInfoEXT {
				VkStructureType           sType;
				const void* pNext;
				VkDisplayPowerStateEXT    powerState;
			} VkDisplayPowerInfoEXT;

			typedef struct VkDeviceEventInfoEXT {
				VkStructureType         sType;
				const void* pNext;
				VkDeviceEventTypeEXT    deviceEvent;
			} VkDeviceEventInfoEXT;

			typedef struct VkDisplayEventInfoEXT {
				VkStructureType          sType;
				const void* pNext;
				VkDisplayEventTypeEXT    displayEvent;
			} VkDisplayEventInfoEXT;

			typedef struct VkSwapchainCounterCreateInfoEXT {
				VkStructureType             sType;
				const void* pNext;
				VkSurfaceCounterFlagsEXT    surfaceCounters;
			} VkSwapchainCounterCreateInfoEXT;

			typedef VkResult(VKAPI_PTR* PFN_vkDisplayPowerControlEXT)(VkDevice device, VkDisplayKHR display, const VkDisplayPowerInfoEXT* pDisplayPowerInfo);
			typedef VkResult(VKAPI_PTR* PFN_vkRegisterDeviceEventEXT)(VkDevice device, const VkDeviceEventInfoEXT* pDeviceEventInfo, const VkAllocationCallbacks* pAllocator, VkFence* pFence);
			typedef VkResult(VKAPI_PTR* PFN_vkRegisterDisplayEventEXT)(VkDevice device, VkDisplayKHR display, const VkDisplayEventInfoEXT* pDisplayEventInfo, const VkAllocationCallbacks* pAllocator, VkFence* pFence);
			typedef VkResult(VKAPI_PTR* PFN_vkGetSwapchainCounterEXT)(VkDevice device, VkSwapchainKHR swapchain, VkSurfaceCounterFlagBitsEXT counter, uint64_t* pCounterValue);

#ifndef VK_NO_PROTOTYPES
			VKAPI_ATTR VkResult VKAPI_CALL vkDisplayPowerControlEXT(
				VkDevice                                    device,
				VkDisplayKHR                                display,
				const VkDisplayPowerInfoEXT* pDisplayPowerInfo);

			VKAPI_ATTR VkResult VKAPI_CALL vkRegisterDeviceEventEXT(
				VkDevice                                    device,
				const VkDeviceEventInfoEXT* pDeviceEventInfo,
				const VkAllocationCallbacks* pAllocator,
				VkFence* pFence);

			VKAPI_ATTR VkResult VKAPI_CALL vkRegisterDisplayEventEXT(
				VkDevice                                    device,
				VkDisplayKHR                                display,
				const VkDisplayEventInfoEXT* pDisplayEventInfo,
				const VkAllocationCallbacks* pAllocator,
				VkFence* pFence);

			VKAPI_ATTR VkResult VKAPI_CALL vkGetSwapchainCounterEXT(
				VkDevice                                    device,
				VkSwapchainKHR                              swapchain,
				VkSurfaceCounterFlagBitsEXT                 counter,
				uint64_t* pCounterValue);
#endif


#define VK_GOOGLE_display_timing 1
#define VK_GOOGLE_DISPLAY_TIMING_SPEC_VERSION 1
#define VK_GOOGLE_DISPLAY_TIMING_EXTENSION_NAME "VK_GOOGLE_display_timing"
			typedef struct VkRefreshCycleDurationGOOGLE {
				uint64_t    refreshDuration;
			} VkRefreshCycleDurationGOOGLE;

			typedef struct VkPastPresentationTimingGOOGLE {
				uint32_t    presentID;
				uint64_t    desiredPresentTime;
				uint64_t    actualPresentTime;
				uint64_t    earliestPresentTime;
				uint64_t    presentMargin;
			} VkPastPresentationTimingGOOGLE;

			typedef struct VkPresentTimeGOOGLE {
				uint32_t    presentID;
				uint64_t    desiredPresentTime;
			} VkPresentTimeGOOGLE;

			typedef struct VkPresentTimesInfoGOOGLE {
				VkStructureType               sType;
				const void* pNext;
				uint32_t                      swapchainCount;
				const VkPresentTimeGOOGLE* pTimes;
			} VkPresentTimesInfoGOOGLE;

			typedef VkResult(VKAPI_PTR* PFN_vkGetRefreshCycleDurationGOOGLE)(VkDevice device, VkSwapchainKHR swapchain, VkRefreshCycleDurationGOOGLE* pDisplayTimingProperties);
			typedef VkResult(VKAPI_PTR* PFN_vkGetPastPresentationTimingGOOGLE)(VkDevice device, VkSwapchainKHR swapchain, uint32_t* pPresentationTimingCount, VkPastPresentationTimingGOOGLE* pPresentationTimings);

#ifndef VK_NO_PROTOTYPES
			VKAPI_ATTR VkResult VKAPI_CALL vkGetRefreshCycleDurationGOOGLE(
				VkDevice                                    device,
				VkSwapchainKHR                              swapchain,
				VkRefreshCycleDurationGOOGLE* pDisplayTimingProperties);

			VKAPI_ATTR VkResult VKAPI_CALL vkGetPastPresentationTimingGOOGLE(
				VkDevice                                    device,
				VkSwapchainKHR                              swapchain,
				uint32_t* pPresentationTimingCount,
				VkPastPresentationTimingGOOGLE* pPresentationTimings);
#endif


#define VK_NV_sample_mask_override_coverage 1
#define VK_NV_SAMPLE_MASK_OVERRIDE_COVERAGE_SPEC_VERSION 1
#define VK_NV_SAMPLE_MASK_OVERRIDE_COVERAGE_EXTENSION_NAME "VK_NV_sample_mask_override_coverage"


#define VK_NV_geometry_shader_passthrough 1
#define VK_NV_GEOMETRY_SHADER_PASSTHROUGH_SPEC_VERSION 1
#define VK_NV_GEOMETRY_SHADER_PASSTHROUGH_EXTENSION_NAME "VK_NV_geometry_shader_passthrough"


#define VK_NV_viewport_array2 1
#define VK_NV_VIEWPORT_ARRAY2_SPEC_VERSION 1
#define VK_NV_VIEWPORT_ARRAY2_EXTENSION_NAME "VK_NV_viewport_array2"


#define VK_NVX_multiview_per_view_attributes 1
#define VK_NVX_MULTIVIEW_PER_VIEW_ATTRIBUTES_SPEC_VERSION 1
#define VK_NVX_MULTIVIEW_PER_VIEW_ATTRIBUTES_EXTENSION_NAME "VK_NVX_multiview_per_view_attributes"
			typedef struct VkPhysicalDeviceMultiviewPerViewAttributesPropertiesNVX {
				VkStructureType    sType;
				void* pNext;
				VkBool32           perViewPositionAllComponents;
			} VkPhysicalDeviceMultiviewPerViewAttributesPropertiesNVX;



#define VK_NV_viewport_swizzle 1
#define VK_NV_VIEWPORT_SWIZZLE_SPEC_VERSION 1
#define VK_NV_VIEWPORT_SWIZZLE_EXTENSION_NAME "VK_NV_viewport_swizzle"

			typedef enum VkViewportCoordinateSwizzleNV {
				VK_VIEWPORT_COORDINATE_SWIZZLE_POSITIVE_X_NV = 0,
				VK_VIEWPORT_COORDINATE_SWIZZLE_NEGATIVE_X_NV = 1,
				VK_VIEWPORT_COORDINATE_SWIZZLE_POSITIVE_Y_NV = 2,
				VK_VIEWPORT_COORDINATE_SWIZZLE_NEGATIVE_Y_NV = 3,
				VK_VIEWPORT_COORDINATE_SWIZZLE_POSITIVE_Z_NV = 4,
				VK_VIEWPORT_COORDINATE_SWIZZLE_NEGATIVE_Z_NV = 5,
				VK_VIEWPORT_COORDINATE_SWIZZLE_POSITIVE_W_NV = 6,
				VK_VIEWPORT_COORDINATE_SWIZZLE_NEGATIVE_W_NV = 7,
				VK_VIEWPORT_COORDINATE_SWIZZLE_BEGIN_RANGE_NV = VK_VIEWPORT_COORDINATE_SWIZZLE_POSITIVE_X_NV,
				VK_VIEWPORT_COORDINATE_SWIZZLE_END_RANGE_NV = VK_VIEWPORT_COORDINATE_SWIZZLE_NEGATIVE_W_NV,
				VK_VIEWPORT_COORDINATE_SWIZZLE_RANGE_SIZE_NV = (VK_VIEWPORT_COORDINATE_SWIZZLE_NEGATIVE_W_NV - VK_VIEWPORT_COORDINATE_SWIZZLE_POSITIVE_X_NV + 1),
				VK_VIEWPORT_COORDINATE_SWIZZLE_MAX_ENUM_NV = 0x7FFFFFFF
			} VkViewportCoordinateSwizzleNV;
			typedef VkFlags VkPipelineViewportSwizzleStateCreateFlagsNV;
			typedef struct VkViewportSwizzleNV {
				VkViewportCoordinateSwizzleNV    x;
				VkViewportCoordinateSwizzleNV    y;
				VkViewportCoordinateSwizzleNV    z;
				VkViewportCoordinateSwizzleNV    w;
			} VkViewportSwizzleNV;

			typedef struct VkPipelineViewportSwizzleStateCreateInfoNV {
				VkStructureType                                sType;
				const void* pNext;
				VkPipelineViewportSwizzleStateCreateFlagsNV    flags;
				uint32_t                                       viewportCount;
				const VkViewportSwizzleNV* pViewportSwizzles;
			} VkPipelineViewportSwizzleStateCreateInfoNV;



#define VK_EXT_discard_rectangles 1
#define VK_EXT_DISCARD_RECTANGLES_SPEC_VERSION 1
#define VK_EXT_DISCARD_RECTANGLES_EXTENSION_NAME "VK_EXT_discard_rectangles"

			typedef enum VkDiscardRectangleModeEXT {
				VK_DISCARD_RECTANGLE_MODE_INCLUSIVE_EXT = 0,
				VK_DISCARD_RECTANGLE_MODE_EXCLUSIVE_EXT = 1,
				VK_DISCARD_RECTANGLE_MODE_BEGIN_RANGE_EXT = VK_DISCARD_RECTANGLE_MODE_INCLUSIVE_EXT,
				VK_DISCARD_RECTANGLE_MODE_END_RANGE_EXT = VK_DISCARD_RECTANGLE_MODE_EXCLUSIVE_EXT,
				VK_DISCARD_RECTANGLE_MODE_RANGE_SIZE_EXT = (VK_DISCARD_RECTANGLE_MODE_EXCLUSIVE_EXT - VK_DISCARD_RECTANGLE_MODE_INCLUSIVE_EXT + 1),
				VK_DISCARD_RECTANGLE_MODE_MAX_ENUM_EXT = 0x7FFFFFFF
			} VkDiscardRectangleModeEXT;
			typedef VkFlags VkPipelineDiscardRectangleStateCreateFlagsEXT;
			typedef struct VkPhysicalDeviceDiscardRectanglePropertiesEXT {
				VkStructureType    sType;
				void* pNext;
				uint32_t           maxDiscardRectangles;
			} VkPhysicalDeviceDiscardRectanglePropertiesEXT;

			typedef struct VkPipelineDiscardRectangleStateCreateInfoEXT {
				VkStructureType                                  sType;
				const void* pNext;
				VkPipelineDiscardRectangleStateCreateFlagsEXT    flags;
				VkDiscardRectangleModeEXT                        discardRectangleMode;
				uint32_t                                         discardRectangleCount;
				const VkRect2D* pDiscardRectangles;
			} VkPipelineDiscardRectangleStateCreateInfoEXT;

			typedef void (VKAPI_PTR* PFN_vkCmdSetDiscardRectangleEXT)(VkCommandBuffer commandBuffer, uint32_t firstDiscardRectangle, uint32_t discardRectangleCount, const VkRect2D* pDiscardRectangles);

#ifndef VK_NO_PROTOTYPES
			VKAPI_ATTR void VKAPI_CALL vkCmdSetDiscardRectangleEXT(
				VkCommandBuffer                             commandBuffer,
				uint32_t                                    firstDiscardRectangle,
				uint32_t                                    discardRectangleCount,
				const VkRect2D* pDiscardRectangles);
#endif


#define VK_EXT_conservative_rasterization 1
#define VK_EXT_CONSERVATIVE_RASTERIZATION_SPEC_VERSION 1
#define VK_EXT_CONSERVATIVE_RASTERIZATION_EXTENSION_NAME "VK_EXT_conservative_rasterization"

			typedef enum VkConservativeRasterizationModeEXT {
				VK_CONSERVATIVE_RASTERIZATION_MODE_DISABLED_EXT = 0,
				VK_CONSERVATIVE_RASTERIZATION_MODE_OVERESTIMATE_EXT = 1,
				VK_CONSERVATIVE_RASTERIZATION_MODE_UNDERESTIMATE_EXT = 2,
				VK_CONSERVATIVE_RASTERIZATION_MODE_BEGIN_RANGE_EXT = VK_CONSERVATIVE_RASTERIZATION_MODE_DISABLED_EXT,
				VK_CONSERVATIVE_RASTERIZATION_MODE_END_RANGE_EXT = VK_CONSERVATIVE_RASTERIZATION_MODE_UNDERESTIMATE_EXT,
				VK_CONSERVATIVE_RASTERIZATION_MODE_RANGE_SIZE_EXT = (VK_CONSERVATIVE_RASTERIZATION_MODE_UNDERESTIMATE_EXT - VK_CONSERVATIVE_RASTERIZATION_MODE_DISABLED_EXT + 1),
				VK_CONSERVATIVE_RASTERIZATION_MODE_MAX_ENUM_EXT = 0x7FFFFFFF
			} VkConservativeRasterizationModeEXT;
			typedef VkFlags VkPipelineRasterizationConservativeStateCreateFlagsEXT;
			typedef struct VkPhysicalDeviceConservativeRasterizationPropertiesEXT {
				VkStructureType    sType;
				void* pNext;
				float              primitiveOverestimationSize;
				float              maxExtraPrimitiveOverestimationSize;
				float              extraPrimitiveOverestimationSizeGranularity;
				VkBool32           primitiveUnderestimation;
				VkBool32           conservativePointAndLineRasterization;
				VkBool32           degenerateTrianglesRasterized;
				VkBool32           degenerateLinesRasterized;
				VkBool32           fullyCoveredFragmentShaderInputVariable;
				VkBool32           conservativeRasterizationPostDepthCoverage;
			} VkPhysicalDeviceConservativeRasterizationPropertiesEXT;

			typedef struct VkPipelineRasterizationConservativeStateCreateInfoEXT {
				VkStructureType                                           sType;
				const void* pNext;
				VkPipelineRasterizationConservativeStateCreateFlagsEXT    flags;
				VkConservativeRasterizationModeEXT                        conservativeRasterizationMode;
				float                                                     extraPrimitiveOverestimationSize;
			} VkPipelineRasterizationConservativeStateCreateInfoEXT;



#define VK_EXT_depth_clip_enable 1
#define VK_EXT_DEPTH_CLIP_ENABLE_SPEC_VERSION 1
#define VK_EXT_DEPTH_CLIP_ENABLE_EXTENSION_NAME "VK_EXT_depth_clip_enable"
			typedef VkFlags VkPipelineRasterizationDepthClipStateCreateFlagsEXT;
			typedef struct VkPhysicalDeviceDepthClipEnableFeaturesEXT {
				VkStructureType    sType;
				void* pNext;
				VkBool32           depthClipEnable;
			} VkPhysicalDeviceDepthClipEnableFeaturesEXT;

			typedef struct VkPipelineRasterizationDepthClipStateCreateInfoEXT {
				VkStructureType                                        sType;
				const void* pNext;
				VkPipelineRasterizationDepthClipStateCreateFlagsEXT    flags;
				VkBool32                                               depthClipEnable;
			} VkPipelineRasterizationDepthClipStateCreateInfoEXT;



#define VK_EXT_swapchain_colorspace 1
#define VK_EXT_SWAPCHAIN_COLOR_SPACE_SPEC_VERSION 4
#define VK_EXT_SWAPCHAIN_COLOR_SPACE_EXTENSION_NAME "VK_EXT_swapchain_colorspace"


#define VK_EXT_hdr_metadata 1
#define VK_EXT_HDR_METADATA_SPEC_VERSION  1
#define VK_EXT_HDR_METADATA_EXTENSION_NAME "VK_EXT_hdr_metadata"
			typedef struct VkXYColorEXT {
				float    x;
				float    y;
			} VkXYColorEXT;

			typedef struct VkHdrMetadataEXT {
				VkStructureType    sType;
				const void* pNext;
				VkXYColorEXT       displayPrimaryRed;
				VkXYColorEXT       displayPrimaryGreen;
				VkXYColorEXT       displayPrimaryBlue;
				VkXYColorEXT       whitePoint;
				float              maxLuminance;
				float              minLuminance;
				float              maxContentLightLevel;
				float              maxFrameAverageLightLevel;
			} VkHdrMetadataEXT;

			typedef void (VKAPI_PTR* PFN_vkSetHdrMetadataEXT)(VkDevice device, uint32_t swapchainCount, const VkSwapchainKHR* pSwapchains, const VkHdrMetadataEXT* pMetadata);

#ifndef VK_NO_PROTOTYPES
			VKAPI_ATTR void VKAPI_CALL vkSetHdrMetadataEXT(
				VkDevice                                    device,
				uint32_t                                    swapchainCount,
				const VkSwapchainKHR* pSwapchains,
				const VkHdrMetadataEXT* pMetadata);
#endif


#define VK_EXT_external_memory_dma_buf 1
#define VK_EXT_EXTERNAL_MEMORY_DMA_BUF_SPEC_VERSION 1
#define VK_EXT_EXTERNAL_MEMORY_DMA_BUF_EXTENSION_NAME "VK_EXT_external_memory_dma_buf"


#define VK_EXT_queue_family_foreign 1
#define VK_EXT_QUEUE_FAMILY_FOREIGN_SPEC_VERSION 1
#define VK_EXT_QUEUE_FAMILY_FOREIGN_EXTENSION_NAME "VK_EXT_queue_family_foreign"
#define VK_QUEUE_FAMILY_FOREIGN_EXT       (~0U-2)


#define VK_EXT_debug_utils 1
			VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkDebugUtilsMessengerEXT)
#define VK_EXT_DEBUG_UTILS_SPEC_VERSION   1
#define VK_EXT_DEBUG_UTILS_EXTENSION_NAME "VK_EXT_debug_utils"
				typedef VkFlags VkDebugUtilsMessengerCallbackDataFlagsEXT;
			typedef VkFlags VkDebugUtilsMessengerCreateFlagsEXT;

			typedef enum VkDebugUtilsMessageSeverityFlagBitsEXT {
				VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT = 0x00000001,
				VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT = 0x00000010,
				VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT = 0x00000100,
				VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT = 0x00001000,
				VK_DEBUG_UTILS_MESSAGE_SEVERITY_FLAG_BITS_MAX_ENUM_EXT = 0x7FFFFFFF
			} VkDebugUtilsMessageSeverityFlagBitsEXT;
			typedef VkFlags VkDebugUtilsMessageSeverityFlagsEXT;

			typedef enum VkDebugUtilsMessageTypeFlagBitsEXT {
				VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT = 0x00000001,
				VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT = 0x00000002,
				VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT = 0x00000004,
				VK_DEBUG_UTILS_MESSAGE_TYPE_FLAG_BITS_MAX_ENUM_EXT = 0x7FFFFFFF
			} VkDebugUtilsMessageTypeFlagBitsEXT;
			typedef VkFlags VkDebugUtilsMessageTypeFlagsEXT;
			typedef struct VkDebugUtilsObjectNameInfoEXT {
				VkStructureType    sType;
				const void* pNext;
				VkObjectType       objectType;
				uint64_t           objectHandle;
				const char* pObjectName;
			} VkDebugUtilsObjectNameInfoEXT;

			typedef struct VkDebugUtilsObjectTagInfoEXT {
				VkStructureType    sType;
				const void* pNext;
				VkObjectType       objectType;
				uint64_t           objectHandle;
				uint64_t           tagName;
				size_t             tagSize;
				const void* pTag;
			} VkDebugUtilsObjectTagInfoEXT;

			typedef struct VkDebugUtilsLabelEXT {
				VkStructureType    sType;
				const void* pNext;
				const char* pLabelName;
				float              color[4];
			} VkDebugUtilsLabelEXT;

			typedef struct VkDebugUtilsMessengerCallbackDataEXT {
				VkStructureType                              sType;
				const void* pNext;
				VkDebugUtilsMessengerCallbackDataFlagsEXT    flags;
				const char* pMessageIdName;
				int32_t                                      messageIdNumber;
				const char* pMessage;
				uint32_t                                     queueLabelCount;
				const VkDebugUtilsLabelEXT* pQueueLabels;
				uint32_t                                     cmdBufLabelCount;
				const VkDebugUtilsLabelEXT* pCmdBufLabels;
				uint32_t                                     objectCount;
				const VkDebugUtilsObjectNameInfoEXT* pObjects;
			} VkDebugUtilsMessengerCallbackDataEXT;

			typedef VkBool32(VKAPI_PTR* PFN_vkDebugUtilsMessengerCallbackEXT)(
				VkDebugUtilsMessageSeverityFlagBitsEXT           messageSeverity,
				VkDebugUtilsMessageTypeFlagsEXT                  messageTypes,
				const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
				void* pUserData);

			typedef struct VkDebugUtilsMessengerCreateInfoEXT {
				VkStructureType                         sType;
				const void* pNext;
				VkDebugUtilsMessengerCreateFlagsEXT     flags;
				VkDebugUtilsMessageSeverityFlagsEXT     messageSeverity;
				VkDebugUtilsMessageTypeFlagsEXT         messageType;
				PFN_vkDebugUtilsMessengerCallbackEXT    pfnUserCallback;
				void* pUserData;
			} VkDebugUtilsMessengerCreateInfoEXT;

			typedef VkResult(VKAPI_PTR* PFN_vkSetDebugUtilsObjectNameEXT)(VkDevice device, const VkDebugUtilsObjectNameInfoEXT* pNameInfo);
			typedef VkResult(VKAPI_PTR* PFN_vkSetDebugUtilsObjectTagEXT)(VkDevice device, const VkDebugUtilsObjectTagInfoEXT* pTagInfo);
			typedef void (VKAPI_PTR* PFN_vkQueueBeginDebugUtilsLabelEXT)(VkQueue queue, const VkDebugUtilsLabelEXT* pLabelInfo);
			typedef void (VKAPI_PTR* PFN_vkQueueEndDebugUtilsLabelEXT)(VkQueue queue);
			typedef void (VKAPI_PTR* PFN_vkQueueInsertDebugUtilsLabelEXT)(VkQueue queue, const VkDebugUtilsLabelEXT* pLabelInfo);
			typedef void (VKAPI_PTR* PFN_vkCmdBeginDebugUtilsLabelEXT)(VkCommandBuffer commandBuffer, const VkDebugUtilsLabelEXT* pLabelInfo);
			typedef void (VKAPI_PTR* PFN_vkCmdEndDebugUtilsLabelEXT)(VkCommandBuffer commandBuffer);
			typedef void (VKAPI_PTR* PFN_vkCmdInsertDebugUtilsLabelEXT)(VkCommandBuffer commandBuffer, const VkDebugUtilsLabelEXT* pLabelInfo);
			typedef VkResult(VKAPI_PTR* PFN_vkCreateDebugUtilsMessengerEXT)(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pMessenger);
			typedef void (VKAPI_PTR* PFN_vkDestroyDebugUtilsMessengerEXT)(VkInstance instance, VkDebugUtilsMessengerEXT messenger, const VkAllocationCallbacks* pAllocator);
			typedef void (VKAPI_PTR* PFN_vkSubmitDebugUtilsMessageEXT)(VkInstance instance, VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageTypes, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData);

#ifndef VK_NO_PROTOTYPES
			VKAPI_ATTR VkResult VKAPI_CALL vkSetDebugUtilsObjectNameEXT(
				VkDevice                                    device,
				const VkDebugUtilsObjectNameInfoEXT* pNameInfo);

			VKAPI_ATTR VkResult VKAPI_CALL vkSetDebugUtilsObjectTagEXT(
				VkDevice                                    device,
				const VkDebugUtilsObjectTagInfoEXT* pTagInfo);

			VKAPI_ATTR void VKAPI_CALL vkQueueBeginDebugUtilsLabelEXT(
				VkQueue                                     queue,
				const VkDebugUtilsLabelEXT* pLabelInfo);

			VKAPI_ATTR void VKAPI_CALL vkQueueEndDebugUtilsLabelEXT(
				VkQueue                                     queue);

			VKAPI_ATTR void VKAPI_CALL vkQueueInsertDebugUtilsLabelEXT(
				VkQueue                                     queue,
				const VkDebugUtilsLabelEXT* pLabelInfo);

			VKAPI_ATTR void VKAPI_CALL vkCmdBeginDebugUtilsLabelEXT(
				VkCommandBuffer                             commandBuffer,
				const VkDebugUtilsLabelEXT* pLabelInfo);

			VKAPI_ATTR void VKAPI_CALL vkCmdEndDebugUtilsLabelEXT(
				VkCommandBuffer                             commandBuffer);

			VKAPI_ATTR void VKAPI_CALL vkCmdInsertDebugUtilsLabelEXT(
				VkCommandBuffer                             commandBuffer,
				const VkDebugUtilsLabelEXT* pLabelInfo);

			VKAPI_ATTR VkResult VKAPI_CALL vkCreateDebugUtilsMessengerEXT(
				VkInstance                                  instance,
				const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
				const VkAllocationCallbacks* pAllocator,
				VkDebugUtilsMessengerEXT* pMessenger);

			VKAPI_ATTR void VKAPI_CALL vkDestroyDebugUtilsMessengerEXT(
				VkInstance                                  instance,
				VkDebugUtilsMessengerEXT                    messenger,
				const VkAllocationCallbacks* pAllocator);

			VKAPI_ATTR void VKAPI_CALL vkSubmitDebugUtilsMessageEXT(
				VkInstance                                  instance,
				VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
				VkDebugUtilsMessageTypeFlagsEXT             messageTypes,
				const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData);
#endif


#define VK_EXT_sampler_filter_minmax 1
#define VK_EXT_SAMPLER_FILTER_MINMAX_SPEC_VERSION 1
#define VK_EXT_SAMPLER_FILTER_MINMAX_EXTENSION_NAME "VK_EXT_sampler_filter_minmax"

			typedef enum VkSamplerReductionModeEXT {
				VK_SAMPLER_REDUCTION_MODE_WEIGHTED_AVERAGE_EXT = 0,
				VK_SAMPLER_REDUCTION_MODE_MIN_EXT = 1,
				VK_SAMPLER_REDUCTION_MODE_MAX_EXT = 2,
				VK_SAMPLER_REDUCTION_MODE_BEGIN_RANGE_EXT = VK_SAMPLER_REDUCTION_MODE_WEIGHTED_AVERAGE_EXT,
				VK_SAMPLER_REDUCTION_MODE_END_RANGE_EXT = VK_SAMPLER_REDUCTION_MODE_MAX_EXT,
				VK_SAMPLER_REDUCTION_MODE_RANGE_SIZE_EXT = (VK_SAMPLER_REDUCTION_MODE_MAX_EXT - VK_SAMPLER_REDUCTION_MODE_WEIGHTED_AVERAGE_EXT + 1),
				VK_SAMPLER_REDUCTION_MODE_MAX_ENUM_EXT = 0x7FFFFFFF
			} VkSamplerReductionModeEXT;
			typedef struct VkSamplerReductionModeCreateInfoEXT {
				VkStructureType              sType;
				const void* pNext;
				VkSamplerReductionModeEXT    reductionMode;
			} VkSamplerReductionModeCreateInfoEXT;

			typedef struct VkPhysicalDeviceSamplerFilterMinmaxPropertiesEXT {
				VkStructureType    sType;
				void* pNext;
				VkBool32           filterMinmaxSingleComponentFormats;
				VkBool32           filterMinmaxImageComponentMapping;
			} VkPhysicalDeviceSamplerFilterMinmaxPropertiesEXT;



#define VK_AMD_gpu_shader_int16 1
#define VK_AMD_GPU_SHADER_INT16_SPEC_VERSION 2
#define VK_AMD_GPU_SHADER_INT16_EXTENSION_NAME "VK_AMD_gpu_shader_int16"


#define VK_AMD_mixed_attachment_samples 1
#define VK_AMD_MIXED_ATTACHMENT_SAMPLES_SPEC_VERSION 1
#define VK_AMD_MIXED_ATTACHMENT_SAMPLES_EXTENSION_NAME "VK_AMD_mixed_attachment_samples"


#define VK_AMD_shader_fragment_mask 1
#define VK_AMD_SHADER_FRAGMENT_MASK_SPEC_VERSION 1
#define VK_AMD_SHADER_FRAGMENT_MASK_EXTENSION_NAME "VK_AMD_shader_fragment_mask"


#define VK_EXT_inline_uniform_block 1
#define VK_EXT_INLINE_UNIFORM_BLOCK_SPEC_VERSION 1
#define VK_EXT_INLINE_UNIFORM_BLOCK_EXTENSION_NAME "VK_EXT_inline_uniform_block"
			typedef struct VkPhysicalDeviceInlineUniformBlockFeaturesEXT {
				VkStructureType    sType;
				void* pNext;
				VkBool32           inlineUniformBlock;
				VkBool32           descriptorBindingInlineUniformBlockUpdateAfterBind;
			} VkPhysicalDeviceInlineUniformBlockFeaturesEXT;

			typedef struct VkPhysicalDeviceInlineUniformBlockPropertiesEXT {
				VkStructureType    sType;
				void* pNext;
				uint32_t           maxInlineUniformBlockSize;
				uint32_t           maxPerStageDescriptorInlineUniformBlocks;
				uint32_t           maxPerStageDescriptorUpdateAfterBindInlineUniformBlocks;
				uint32_t           maxDescriptorSetInlineUniformBlocks;
				uint32_t           maxDescriptorSetUpdateAfterBindInlineUniformBlocks;
			} VkPhysicalDeviceInlineUniformBlockPropertiesEXT;

			typedef struct VkWriteDescriptorSetInlineUniformBlockEXT {
				VkStructureType    sType;
				const void* pNext;
				uint32_t           dataSize;
				const void* pData;
			} VkWriteDescriptorSetInlineUniformBlockEXT;

			typedef struct VkDescriptorPoolInlineUniformBlockCreateInfoEXT {
				VkStructureType    sType;
				const void* pNext;
				uint32_t           maxInlineUniformBlockBindings;
			} VkDescriptorPoolInlineUniformBlockCreateInfoEXT;



#define VK_EXT_shader_stencil_export 1
#define VK_EXT_SHADER_STENCIL_EXPORT_SPEC_VERSION 1
#define VK_EXT_SHADER_STENCIL_EXPORT_EXTENSION_NAME "VK_EXT_shader_stencil_export"


#define VK_EXT_sample_locations 1
#define VK_EXT_SAMPLE_LOCATIONS_SPEC_VERSION 1
#define VK_EXT_SAMPLE_LOCATIONS_EXTENSION_NAME "VK_EXT_sample_locations"
			typedef struct VkSampleLocationEXT {
				float    x;
				float    y;
			} VkSampleLocationEXT;

			typedef struct VkSampleLocationsInfoEXT {
				VkStructureType               sType;
				const void* pNext;
				VkSampleCountFlagBits         sampleLocationsPerPixel;
				VkExtent2D                    sampleLocationGridSize;
				uint32_t                      sampleLocationsCount;
				const VkSampleLocationEXT* pSampleLocations;
			} VkSampleLocationsInfoEXT;

			typedef struct VkAttachmentSampleLocationsEXT {
				uint32_t                    attachmentIndex;
				VkSampleLocationsInfoEXT    sampleLocationsInfo;
			} VkAttachmentSampleLocationsEXT;

			typedef struct VkSubpassSampleLocationsEXT {
				uint32_t                    subpassIndex;
				VkSampleLocationsInfoEXT    sampleLocationsInfo;
			} VkSubpassSampleLocationsEXT;

			typedef struct VkRenderPassSampleLocationsBeginInfoEXT {
				VkStructureType                          sType;
				const void* pNext;
				uint32_t                                 attachmentInitialSampleLocationsCount;
				const VkAttachmentSampleLocationsEXT* pAttachmentInitialSampleLocations;
				uint32_t                                 postSubpassSampleLocationsCount;
				const VkSubpassSampleLocationsEXT* pPostSubpassSampleLocations;
			} VkRenderPassSampleLocationsBeginInfoEXT;

			typedef struct VkPipelineSampleLocationsStateCreateInfoEXT {
				VkStructureType             sType;
				const void* pNext;
				VkBool32                    sampleLocationsEnable;
				VkSampleLocationsInfoEXT    sampleLocationsInfo;
			} VkPipelineSampleLocationsStateCreateInfoEXT;

			typedef struct VkPhysicalDeviceSampleLocationsPropertiesEXT {
				VkStructureType       sType;
				void* pNext;
				VkSampleCountFlags    sampleLocationSampleCounts;
				VkExtent2D            maxSampleLocationGridSize;
				float                 sampleLocationCoordinateRange[2];
				uint32_t              sampleLocationSubPixelBits;
				VkBool32              variableSampleLocations;
			} VkPhysicalDeviceSampleLocationsPropertiesEXT;

			typedef struct VkMultisamplePropertiesEXT {
				VkStructureType    sType;
				void* pNext;
				VkExtent2D         maxSampleLocationGridSize;
			} VkMultisamplePropertiesEXT;

			typedef void (VKAPI_PTR* PFN_vkCmdSetSampleLocationsEXT)(VkCommandBuffer commandBuffer, const VkSampleLocationsInfoEXT* pSampleLocationsInfo);
			typedef void (VKAPI_PTR* PFN_vkGetPhysicalDeviceMultisamplePropertiesEXT)(VkPhysicalDevice physicalDevice, VkSampleCountFlagBits samples, VkMultisamplePropertiesEXT* pMultisampleProperties);

#ifndef VK_NO_PROTOTYPES
			VKAPI_ATTR void VKAPI_CALL vkCmdSetSampleLocationsEXT(
				VkCommandBuffer                             commandBuffer,
				const VkSampleLocationsInfoEXT* pSampleLocationsInfo);

			VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceMultisamplePropertiesEXT(
				VkPhysicalDevice                            physicalDevice,
				VkSampleCountFlagBits                       samples,
				VkMultisamplePropertiesEXT* pMultisampleProperties);
#endif


#define VK_EXT_blend_operation_advanced 1
#define VK_EXT_BLEND_OPERATION_ADVANCED_SPEC_VERSION 2
#define VK_EXT_BLEND_OPERATION_ADVANCED_EXTENSION_NAME "VK_EXT_blend_operation_advanced"

			typedef enum VkBlendOverlapEXT {
				VK_BLEND_OVERLAP_UNCORRELATED_EXT = 0,
				VK_BLEND_OVERLAP_DISJOINT_EXT = 1,
				VK_BLEND_OVERLAP_CONJOINT_EXT = 2,
				VK_BLEND_OVERLAP_BEGIN_RANGE_EXT = VK_BLEND_OVERLAP_UNCORRELATED_EXT,
				VK_BLEND_OVERLAP_END_RANGE_EXT = VK_BLEND_OVERLAP_CONJOINT_EXT,
				VK_BLEND_OVERLAP_RANGE_SIZE_EXT = (VK_BLEND_OVERLAP_CONJOINT_EXT - VK_BLEND_OVERLAP_UNCORRELATED_EXT + 1),
				VK_BLEND_OVERLAP_MAX_ENUM_EXT = 0x7FFFFFFF
			} VkBlendOverlapEXT;
			typedef struct VkPhysicalDeviceBlendOperationAdvancedFeaturesEXT {
				VkStructureType    sType;
				void* pNext;
				VkBool32           advancedBlendCoherentOperations;
			} VkPhysicalDeviceBlendOperationAdvancedFeaturesEXT;

			typedef struct VkPhysicalDeviceBlendOperationAdvancedPropertiesEXT {
				VkStructureType    sType;
				void* pNext;
				uint32_t           advancedBlendMaxColorAttachments;
				VkBool32           advancedBlendIndependentBlend;
				VkBool32           advancedBlendNonPremultipliedSrcColor;
				VkBool32           advancedBlendNonPremultipliedDstColor;
				VkBool32           advancedBlendCorrelatedOverlap;
				VkBool32           advancedBlendAllOperations;
			} VkPhysicalDeviceBlendOperationAdvancedPropertiesEXT;

			typedef struct VkPipelineColorBlendAdvancedStateCreateInfoEXT {
				VkStructureType      sType;
				const void* pNext;
				VkBool32             srcPremultiplied;
				VkBool32             dstPremultiplied;
				VkBlendOverlapEXT    blendOverlap;
			} VkPipelineColorBlendAdvancedStateCreateInfoEXT;



#define VK_NV_fragment_coverage_to_color 1
#define VK_NV_FRAGMENT_COVERAGE_TO_COLOR_SPEC_VERSION 1
#define VK_NV_FRAGMENT_COVERAGE_TO_COLOR_EXTENSION_NAME "VK_NV_fragment_coverage_to_color"
			typedef VkFlags VkPipelineCoverageToColorStateCreateFlagsNV;
			typedef struct VkPipelineCoverageToColorStateCreateInfoNV {
				VkStructureType                                sType;
				const void* pNext;
				VkPipelineCoverageToColorStateCreateFlagsNV    flags;
				VkBool32                                       coverageToColorEnable;
				uint32_t                                       coverageToColorLocation;
			} VkPipelineCoverageToColorStateCreateInfoNV;



#define VK_NV_framebuffer_mixed_samples 1
#define VK_NV_FRAMEBUFFER_MIXED_SAMPLES_SPEC_VERSION 1
#define VK_NV_FRAMEBUFFER_MIXED_SAMPLES_EXTENSION_NAME "VK_NV_framebuffer_mixed_samples"

			typedef enum VkCoverageModulationModeNV {
				VK_COVERAGE_MODULATION_MODE_NONE_NV = 0,
				VK_COVERAGE_MODULATION_MODE_RGB_NV = 1,
				VK_COVERAGE_MODULATION_MODE_ALPHA_NV = 2,
				VK_COVERAGE_MODULATION_MODE_RGBA_NV = 3,
				VK_COVERAGE_MODULATION_MODE_BEGIN_RANGE_NV = VK_COVERAGE_MODULATION_MODE_NONE_NV,
				VK_COVERAGE_MODULATION_MODE_END_RANGE_NV = VK_COVERAGE_MODULATION_MODE_RGBA_NV,
				VK_COVERAGE_MODULATION_MODE_RANGE_SIZE_NV = (VK_COVERAGE_MODULATION_MODE_RGBA_NV - VK_COVERAGE_MODULATION_MODE_NONE_NV + 1),
				VK_COVERAGE_MODULATION_MODE_MAX_ENUM_NV = 0x7FFFFFFF
			} VkCoverageModulationModeNV;
			typedef VkFlags VkPipelineCoverageModulationStateCreateFlagsNV;
			typedef struct VkPipelineCoverageModulationStateCreateInfoNV {
				VkStructureType                                   sType;
				const void* pNext;
				VkPipelineCoverageModulationStateCreateFlagsNV    flags;
				VkCoverageModulationModeNV                        coverageModulationMode;
				VkBool32                                          coverageModulationTableEnable;
				uint32_t                                          coverageModulationTableCount;
				const float* pCoverageModulationTable;
			} VkPipelineCoverageModulationStateCreateInfoNV;



#define VK_NV_fill_rectangle 1
#define VK_NV_FILL_RECTANGLE_SPEC_VERSION 1
#define VK_NV_FILL_RECTANGLE_EXTENSION_NAME "VK_NV_fill_rectangle"


#define VK_NV_shader_sm_builtins 1
#define VK_NV_SHADER_SM_BUILTINS_SPEC_VERSION 1
#define VK_NV_SHADER_SM_BUILTINS_EXTENSION_NAME "VK_NV_shader_sm_builtins"
			typedef struct VkPhysicalDeviceShaderSMBuiltinsPropertiesNV {
				VkStructureType    sType;
				void* pNext;
				uint32_t           shaderSMCount;
				uint32_t           shaderWarpsPerSM;
			} VkPhysicalDeviceShaderSMBuiltinsPropertiesNV;

			typedef struct VkPhysicalDeviceShaderSMBuiltinsFeaturesNV {
				VkStructureType    sType;
				void* pNext;
				VkBool32           shaderSMBuiltins;
			} VkPhysicalDeviceShaderSMBuiltinsFeaturesNV;



#define VK_EXT_post_depth_coverage 1
#define VK_EXT_POST_DEPTH_COVERAGE_SPEC_VERSION 1
#define VK_EXT_POST_DEPTH_COVERAGE_EXTENSION_NAME "VK_EXT_post_depth_coverage"


#define VK_EXT_image_drm_format_modifier 1
#define VK_EXT_IMAGE_DRM_FORMAT_MODIFIER_SPEC_VERSION 1
#define VK_EXT_IMAGE_DRM_FORMAT_MODIFIER_EXTENSION_NAME "VK_EXT_image_drm_format_modifier"
			typedef struct VkDrmFormatModifierPropertiesEXT {
				uint64_t                drmFormatModifier;
				uint32_t                drmFormatModifierPlaneCount;
				VkFormatFeatureFlags    drmFormatModifierTilingFeatures;
			} VkDrmFormatModifierPropertiesEXT;

			typedef struct VkDrmFormatModifierPropertiesListEXT {
				VkStructureType                      sType;
				void* pNext;
				uint32_t                             drmFormatModifierCount;
				VkDrmFormatModifierPropertiesEXT* pDrmFormatModifierProperties;
			} VkDrmFormatModifierPropertiesListEXT;

			typedef struct VkPhysicalDeviceImageDrmFormatModifierInfoEXT {
				VkStructureType    sType;
				const void* pNext;
				uint64_t           drmFormatModifier;
				VkSharingMode      sharingMode;
				uint32_t           queueFamilyIndexCount;
				const uint32_t* pQueueFamilyIndices;
			} VkPhysicalDeviceImageDrmFormatModifierInfoEXT;

			typedef struct VkImageDrmFormatModifierListCreateInfoEXT {
				VkStructureType    sType;
				const void* pNext;
				uint32_t           drmFormatModifierCount;
				const uint64_t* pDrmFormatModifiers;
			} VkImageDrmFormatModifierListCreateInfoEXT;

			typedef struct VkImageDrmFormatModifierExplicitCreateInfoEXT {
				VkStructureType               sType;
				const void* pNext;
				uint64_t                      drmFormatModifier;
				uint32_t                      drmFormatModifierPlaneCount;
				const VkSubresourceLayout* pPlaneLayouts;
			} VkImageDrmFormatModifierExplicitCreateInfoEXT;

			typedef struct VkImageDrmFormatModifierPropertiesEXT {
				VkStructureType    sType;
				void* pNext;
				uint64_t           drmFormatModifier;
			} VkImageDrmFormatModifierPropertiesEXT;

			typedef VkResult(VKAPI_PTR* PFN_vkGetImageDrmFormatModifierPropertiesEXT)(VkDevice device, VkImage image, VkImageDrmFormatModifierPropertiesEXT* pProperties);

#ifndef VK_NO_PROTOTYPES
			VKAPI_ATTR VkResult VKAPI_CALL vkGetImageDrmFormatModifierPropertiesEXT(
				VkDevice                                    device,
				VkImage                                     image,
				VkImageDrmFormatModifierPropertiesEXT* pProperties);
#endif


#define VK_EXT_validation_cache 1
			VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkValidationCacheEXT)
#define VK_EXT_VALIDATION_CACHE_SPEC_VERSION 1
#define VK_EXT_VALIDATION_CACHE_EXTENSION_NAME "VK_EXT_validation_cache"

				typedef enum VkValidationCacheHeaderVersionEXT {
				VK_VALIDATION_CACHE_HEADER_VERSION_ONE_EXT = 1,
				VK_VALIDATION_CACHE_HEADER_VERSION_BEGIN_RANGE_EXT = VK_VALIDATION_CACHE_HEADER_VERSION_ONE_EXT,
				VK_VALIDATION_CACHE_HEADER_VERSION_END_RANGE_EXT = VK_VALIDATION_CACHE_HEADER_VERSION_ONE_EXT,
				VK_VALIDATION_CACHE_HEADER_VERSION_RANGE_SIZE_EXT = (VK_VALIDATION_CACHE_HEADER_VERSION_ONE_EXT - VK_VALIDATION_CACHE_HEADER_VERSION_ONE_EXT + 1),
				VK_VALIDATION_CACHE_HEADER_VERSION_MAX_ENUM_EXT = 0x7FFFFFFF
			} VkValidationCacheHeaderVersionEXT;
			typedef VkFlags VkValidationCacheCreateFlagsEXT;
			typedef struct VkValidationCacheCreateInfoEXT {
				VkStructureType                    sType;
				const void* pNext;
				VkValidationCacheCreateFlagsEXT    flags;
				size_t                             initialDataSize;
				const void* pInitialData;
			} VkValidationCacheCreateInfoEXT;

			typedef struct VkShaderModuleValidationCacheCreateInfoEXT {
				VkStructureType         sType;
				const void* pNext;
				VkValidationCacheEXT    validationCache;
			} VkShaderModuleValidationCacheCreateInfoEXT;

			typedef VkResult(VKAPI_PTR* PFN_vkCreateValidationCacheEXT)(VkDevice device, const VkValidationCacheCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkValidationCacheEXT* pValidationCache);
			typedef void (VKAPI_PTR* PFN_vkDestroyValidationCacheEXT)(VkDevice device, VkValidationCacheEXT validationCache, const VkAllocationCallbacks* pAllocator);
			typedef VkResult(VKAPI_PTR* PFN_vkMergeValidationCachesEXT)(VkDevice device, VkValidationCacheEXT dstCache, uint32_t srcCacheCount, const VkValidationCacheEXT* pSrcCaches);
			typedef VkResult(VKAPI_PTR* PFN_vkGetValidationCacheDataEXT)(VkDevice device, VkValidationCacheEXT validationCache, size_t* pDataSize, void* pData);

#ifndef VK_NO_PROTOTYPES
			VKAPI_ATTR VkResult VKAPI_CALL vkCreateValidationCacheEXT(
				VkDevice                                    device,
				const VkValidationCacheCreateInfoEXT* pCreateInfo,
				const VkAllocationCallbacks* pAllocator,
				VkValidationCacheEXT* pValidationCache);

			VKAPI_ATTR void VKAPI_CALL vkDestroyValidationCacheEXT(
				VkDevice                                    device,
				VkValidationCacheEXT                        validationCache,
				const VkAllocationCallbacks* pAllocator);

			VKAPI_ATTR VkResult VKAPI_CALL vkMergeValidationCachesEXT(
				VkDevice                                    device,
				VkValidationCacheEXT                        dstCache,
				uint32_t                                    srcCacheCount,
				const VkValidationCacheEXT* pSrcCaches);

			VKAPI_ATTR VkResult VKAPI_CALL vkGetValidationCacheDataEXT(
				VkDevice                                    device,
				VkValidationCacheEXT                        validationCache,
				size_t* pDataSize,
				void* pData);
#endif


#define VK_EXT_descriptor_indexing 1
#define VK_EXT_DESCRIPTOR_INDEXING_SPEC_VERSION 2
#define VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME "VK_EXT_descriptor_indexing"

			typedef enum VkDescriptorBindingFlagBitsEXT {
				VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT_EXT = 0x00000001,
				VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT_EXT = 0x00000002,
				VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT = 0x00000004,
				VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT_EXT = 0x00000008,
				VK_DESCRIPTOR_BINDING_FLAG_BITS_MAX_ENUM_EXT = 0x7FFFFFFF
			} VkDescriptorBindingFlagBitsEXT;
			typedef VkFlags VkDescriptorBindingFlagsEXT;
			typedef struct VkDescriptorSetLayoutBindingFlagsCreateInfoEXT {
				VkStructureType                       sType;
				const void* pNext;
				uint32_t                              bindingCount;
				const VkDescriptorBindingFlagsEXT* pBindingFlags;
			} VkDescriptorSetLayoutBindingFlagsCreateInfoEXT;

			typedef struct VkPhysicalDeviceDescriptorIndexingFeaturesEXT {
				VkStructureType    sType;
				void* pNext;
				VkBool32           shaderInputAttachmentArrayDynamicIndexing;
				VkBool32           shaderUniformTexelBufferArrayDynamicIndexing;
				VkBool32           shaderStorageTexelBufferArrayDynamicIndexing;
				VkBool32           shaderUniformBufferArrayNonUniformIndexing;
				VkBool32           shaderSampledImageArrayNonUniformIndexing;
				VkBool32           shaderStorageBufferArrayNonUniformIndexing;
				VkBool32           shaderStorageImageArrayNonUniformIndexing;
				VkBool32           shaderInputAttachmentArrayNonUniformIndexing;
				VkBool32           shaderUniformTexelBufferArrayNonUniformIndexing;
				VkBool32           shaderStorageTexelBufferArrayNonUniformIndexing;
				VkBool32           descriptorBindingUniformBufferUpdateAfterBind;
				VkBool32           descriptorBindingSampledImageUpdateAfterBind;
				VkBool32           descriptorBindingStorageImageUpdateAfterBind;
				VkBool32           descriptorBindingStorageBufferUpdateAfterBind;
				VkBool32           descriptorBindingUniformTexelBufferUpdateAfterBind;
				VkBool32           descriptorBindingStorageTexelBufferUpdateAfterBind;
				VkBool32           descriptorBindingUpdateUnusedWhilePending;
				VkBool32           descriptorBindingPartiallyBound;
				VkBool32           descriptorBindingVariableDescriptorCount;
				VkBool32           runtimeDescriptorArray;
			} VkPhysicalDeviceDescriptorIndexingFeaturesEXT;

			typedef struct VkPhysicalDeviceDescriptorIndexingPropertiesEXT {
				VkStructureType    sType;
				void* pNext;
				uint32_t           maxUpdateAfterBindDescriptorsInAllPools;
				VkBool32           shaderUniformBufferArrayNonUniformIndexingNative;
				VkBool32           shaderSampledImageArrayNonUniformIndexingNative;
				VkBool32           shaderStorageBufferArrayNonUniformIndexingNative;
				VkBool32           shaderStorageImageArrayNonUniformIndexingNative;
				VkBool32           shaderInputAttachmentArrayNonUniformIndexingNative;
				VkBool32           robustBufferAccessUpdateAfterBind;
				VkBool32           quadDivergentImplicitLod;
				uint32_t           maxPerStageDescriptorUpdateAfterBindSamplers;
				uint32_t           maxPerStageDescriptorUpdateAfterBindUniformBuffers;
				uint32_t           maxPerStageDescriptorUpdateAfterBindStorageBuffers;
				uint32_t           maxPerStageDescriptorUpdateAfterBindSampledImages;
				uint32_t           maxPerStageDescriptorUpdateAfterBindStorageImages;
				uint32_t           maxPerStageDescriptorUpdateAfterBindInputAttachments;
				uint32_t           maxPerStageUpdateAfterBindResources;
				uint32_t           maxDescriptorSetUpdateAfterBindSamplers;
				uint32_t           maxDescriptorSetUpdateAfterBindUniformBuffers;
				uint32_t           maxDescriptorSetUpdateAfterBindUniformBuffersDynamic;
				uint32_t           maxDescriptorSetUpdateAfterBindStorageBuffers;
				uint32_t           maxDescriptorSetUpdateAfterBindStorageBuffersDynamic;
				uint32_t           maxDescriptorSetUpdateAfterBindSampledImages;
				uint32_t           maxDescriptorSetUpdateAfterBindStorageImages;
				uint32_t           maxDescriptorSetUpdateAfterBindInputAttachments;
			} VkPhysicalDeviceDescriptorIndexingPropertiesEXT;

			typedef struct VkDescriptorSetVariableDescriptorCountAllocateInfoEXT {
				VkStructureType    sType;
				const void* pNext;
				uint32_t           descriptorSetCount;
				const uint32_t* pDescriptorCounts;
			} VkDescriptorSetVariableDescriptorCountAllocateInfoEXT;

			typedef struct VkDescriptorSetVariableDescriptorCountLayoutSupportEXT {
				VkStructureType    sType;
				void* pNext;
				uint32_t           maxVariableDescriptorCount;
			} VkDescriptorSetVariableDescriptorCountLayoutSupportEXT;



#define VK_EXT_shader_viewport_index_layer 1
#define VK_EXT_SHADER_VIEWPORT_INDEX_LAYER_SPEC_VERSION 1
#define VK_EXT_SHADER_VIEWPORT_INDEX_LAYER_EXTENSION_NAME "VK_EXT_shader_viewport_index_layer"


#define VK_NV_shading_rate_image 1
#define VK_NV_SHADING_RATE_IMAGE_SPEC_VERSION 3
#define VK_NV_SHADING_RATE_IMAGE_EXTENSION_NAME "VK_NV_shading_rate_image"

			typedef enum VkShadingRatePaletteEntryNV {
				VK_SHADING_RATE_PALETTE_ENTRY_NO_INVOCATIONS_NV = 0,
				VK_SHADING_RATE_PALETTE_ENTRY_16_INVOCATIONS_PER_PIXEL_NV = 1,
				VK_SHADING_RATE_PALETTE_ENTRY_8_INVOCATIONS_PER_PIXEL_NV = 2,
				VK_SHADING_RATE_PALETTE_ENTRY_4_INVOCATIONS_PER_PIXEL_NV = 3,
				VK_SHADING_RATE_PALETTE_ENTRY_2_INVOCATIONS_PER_PIXEL_NV = 4,
				VK_SHADING_RATE_PALETTE_ENTRY_1_INVOCATION_PER_PIXEL_NV = 5,
				VK_SHADING_RATE_PALETTE_ENTRY_1_INVOCATION_PER_2X1_PIXELS_NV = 6,
				VK_SHADING_RATE_PALETTE_ENTRY_1_INVOCATION_PER_1X2_PIXELS_NV = 7,
				VK_SHADING_RATE_PALETTE_ENTRY_1_INVOCATION_PER_2X2_PIXELS_NV = 8,
				VK_SHADING_RATE_PALETTE_ENTRY_1_INVOCATION_PER_4X2_PIXELS_NV = 9,
				VK_SHADING_RATE_PALETTE_ENTRY_1_INVOCATION_PER_2X4_PIXELS_NV = 10,
				VK_SHADING_RATE_PALETTE_ENTRY_1_INVOCATION_PER_4X4_PIXELS_NV = 11,
				VK_SHADING_RATE_PALETTE_ENTRY_BEGIN_RANGE_NV = VK_SHADING_RATE_PALETTE_ENTRY_NO_INVOCATIONS_NV,
				VK_SHADING_RATE_PALETTE_ENTRY_END_RANGE_NV = VK_SHADING_RATE_PALETTE_ENTRY_1_INVOCATION_PER_4X4_PIXELS_NV,
				VK_SHADING_RATE_PALETTE_ENTRY_RANGE_SIZE_NV = (VK_SHADING_RATE_PALETTE_ENTRY_1_INVOCATION_PER_4X4_PIXELS_NV - VK_SHADING_RATE_PALETTE_ENTRY_NO_INVOCATIONS_NV + 1),
				VK_SHADING_RATE_PALETTE_ENTRY_MAX_ENUM_NV = 0x7FFFFFFF
			} VkShadingRatePaletteEntryNV;

			typedef enum VkCoarseSampleOrderTypeNV {
				VK_COARSE_SAMPLE_ORDER_TYPE_DEFAULT_NV = 0,
				VK_COARSE_SAMPLE_ORDER_TYPE_CUSTOM_NV = 1,
				VK_COARSE_SAMPLE_ORDER_TYPE_PIXEL_MAJOR_NV = 2,
				VK_COARSE_SAMPLE_ORDER_TYPE_SAMPLE_MAJOR_NV = 3,
				VK_COARSE_SAMPLE_ORDER_TYPE_BEGIN_RANGE_NV = VK_COARSE_SAMPLE_ORDER_TYPE_DEFAULT_NV,
				VK_COARSE_SAMPLE_ORDER_TYPE_END_RANGE_NV = VK_COARSE_SAMPLE_ORDER_TYPE_SAMPLE_MAJOR_NV,
				VK_COARSE_SAMPLE_ORDER_TYPE_RANGE_SIZE_NV = (VK_COARSE_SAMPLE_ORDER_TYPE_SAMPLE_MAJOR_NV - VK_COARSE_SAMPLE_ORDER_TYPE_DEFAULT_NV + 1),
				VK_COARSE_SAMPLE_ORDER_TYPE_MAX_ENUM_NV = 0x7FFFFFFF
			} VkCoarseSampleOrderTypeNV;
			typedef struct VkShadingRatePaletteNV {
				uint32_t                              shadingRatePaletteEntryCount;
				const VkShadingRatePaletteEntryNV* pShadingRatePaletteEntries;
			} VkShadingRatePaletteNV;

			typedef struct VkPipelineViewportShadingRateImageStateCreateInfoNV {
				VkStructureType                  sType;
				const void* pNext;
				VkBool32                         shadingRateImageEnable;
				uint32_t                         viewportCount;
				const VkShadingRatePaletteNV* pShadingRatePalettes;
			} VkPipelineViewportShadingRateImageStateCreateInfoNV;

			typedef struct VkPhysicalDeviceShadingRateImageFeaturesNV {
				VkStructureType    sType;
				void* pNext;
				VkBool32           shadingRateImage;
				VkBool32           shadingRateCoarseSampleOrder;
			} VkPhysicalDeviceShadingRateImageFeaturesNV;

			typedef struct VkPhysicalDeviceShadingRateImagePropertiesNV {
				VkStructureType    sType;
				void* pNext;
				VkExtent2D         shadingRateTexelSize;
				uint32_t           shadingRatePaletteSize;
				uint32_t           shadingRateMaxCoarseSamples;
			} VkPhysicalDeviceShadingRateImagePropertiesNV;

			typedef struct VkCoarseSampleLocationNV {
				uint32_t    pixelX;
				uint32_t    pixelY;
				uint32_t    sample;
			} VkCoarseSampleLocationNV;

			typedef struct VkCoarseSampleOrderCustomNV {
				VkShadingRatePaletteEntryNV        shadingRate;
				uint32_t                           sampleCount;
				uint32_t                           sampleLocationCount;
				const VkCoarseSampleLocationNV* pSampleLocations;
			} VkCoarseSampleOrderCustomNV;

			typedef struct VkPipelineViewportCoarseSampleOrderStateCreateInfoNV {
				VkStructureType                       sType;
				const void* pNext;
				VkCoarseSampleOrderTypeNV             sampleOrderType;
				uint32_t                              customSampleOrderCount;
				const VkCoarseSampleOrderCustomNV* pCustomSampleOrders;
			} VkPipelineViewportCoarseSampleOrderStateCreateInfoNV;

			typedef void (VKAPI_PTR* PFN_vkCmdBindShadingRateImageNV)(VkCommandBuffer commandBuffer, VkImageView imageView, VkImageLayout imageLayout);
			typedef void (VKAPI_PTR* PFN_vkCmdSetViewportShadingRatePaletteNV)(VkCommandBuffer commandBuffer, uint32_t firstViewport, uint32_t viewportCount, const VkShadingRatePaletteNV* pShadingRatePalettes);
			typedef void (VKAPI_PTR* PFN_vkCmdSetCoarseSampleOrderNV)(VkCommandBuffer commandBuffer, VkCoarseSampleOrderTypeNV sampleOrderType, uint32_t customSampleOrderCount, const VkCoarseSampleOrderCustomNV* pCustomSampleOrders);

#ifndef VK_NO_PROTOTYPES
			VKAPI_ATTR void VKAPI_CALL vkCmdBindShadingRateImageNV(
				VkCommandBuffer                             commandBuffer,
				VkImageView                                 imageView,
				VkImageLayout                               imageLayout);

			VKAPI_ATTR void VKAPI_CALL vkCmdSetViewportShadingRatePaletteNV(
				VkCommandBuffer                             commandBuffer,
				uint32_t                                    firstViewport,
				uint32_t                                    viewportCount,
				const VkShadingRatePaletteNV* pShadingRatePalettes);

			VKAPI_ATTR void VKAPI_CALL vkCmdSetCoarseSampleOrderNV(
				VkCommandBuffer                             commandBuffer,
				VkCoarseSampleOrderTypeNV                   sampleOrderType,
				uint32_t                                    customSampleOrderCount,
				const VkCoarseSampleOrderCustomNV* pCustomSampleOrders);
#endif


#define VK_NV_ray_tracing 1
			VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkAccelerationStructureNV)
#define VK_NV_RAY_TRACING_SPEC_VERSION    3
#define VK_NV_RAY_TRACING_EXTENSION_NAME  "VK_NV_ray_tracing"
#define VK_SHADER_UNUSED_NV               (~0U)

				typedef enum VkRayTracingShaderGroupTypeNV {
				VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_NV = 0,
				VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_NV = 1,
				VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_NV = 2,
				VK_RAY_TRACING_SHADER_GROUP_TYPE_BEGIN_RANGE_NV = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_NV,
				VK_RAY_TRACING_SHADER_GROUP_TYPE_END_RANGE_NV = VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_NV,
				VK_RAY_TRACING_SHADER_GROUP_TYPE_RANGE_SIZE_NV = (VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_NV - VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_NV + 1),
				VK_RAY_TRACING_SHADER_GROUP_TYPE_MAX_ENUM_NV = 0x7FFFFFFF
			} VkRayTracingShaderGroupTypeNV;

			typedef enum VkGeometryTypeNV {
				VK_GEOMETRY_TYPE_TRIANGLES_NV = 0,
				VK_GEOMETRY_TYPE_AABBS_NV = 1,
				VK_GEOMETRY_TYPE_BEGIN_RANGE_NV = VK_GEOMETRY_TYPE_TRIANGLES_NV,
				VK_GEOMETRY_TYPE_END_RANGE_NV = VK_GEOMETRY_TYPE_AABBS_NV,
				VK_GEOMETRY_TYPE_RANGE_SIZE_NV = (VK_GEOMETRY_TYPE_AABBS_NV - VK_GEOMETRY_TYPE_TRIANGLES_NV + 1),
				VK_GEOMETRY_TYPE_MAX_ENUM_NV = 0x7FFFFFFF
			} VkGeometryTypeNV;

			typedef enum VkAccelerationStructureTypeNV {
				VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_NV = 0,
				VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_NV = 1,
				VK_ACCELERATION_STRUCTURE_TYPE_BEGIN_RANGE_NV = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_NV,
				VK_ACCELERATION_STRUCTURE_TYPE_END_RANGE_NV = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_NV,
				VK_ACCELERATION_STRUCTURE_TYPE_RANGE_SIZE_NV = (VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_NV - VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_NV + 1),
				VK_ACCELERATION_STRUCTURE_TYPE_MAX_ENUM_NV = 0x7FFFFFFF
			} VkAccelerationStructureTypeNV;

			typedef enum VkCopyAccelerationStructureModeNV {
				VK_COPY_ACCELERATION_STRUCTURE_MODE_CLONE_NV = 0,
				VK_COPY_ACCELERATION_STRUCTURE_MODE_COMPACT_NV = 1,
				VK_COPY_ACCELERATION_STRUCTURE_MODE_BEGIN_RANGE_NV = VK_COPY_ACCELERATION_STRUCTURE_MODE_CLONE_NV,
				VK_COPY_ACCELERATION_STRUCTURE_MODE_END_RANGE_NV = VK_COPY_ACCELERATION_STRUCTURE_MODE_COMPACT_NV,
				VK_COPY_ACCELERATION_STRUCTURE_MODE_RANGE_SIZE_NV = (VK_COPY_ACCELERATION_STRUCTURE_MODE_COMPACT_NV - VK_COPY_ACCELERATION_STRUCTURE_MODE_CLONE_NV + 1),
				VK_COPY_ACCELERATION_STRUCTURE_MODE_MAX_ENUM_NV = 0x7FFFFFFF
			} VkCopyAccelerationStructureModeNV;

			typedef enum VkAccelerationStructureMemoryRequirementsTypeNV {
				VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_OBJECT_NV = 0,
				VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_BUILD_SCRATCH_NV = 1,
				VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_UPDATE_SCRATCH_NV = 2,
				VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_BEGIN_RANGE_NV = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_OBJECT_NV,
				VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_END_RANGE_NV = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_UPDATE_SCRATCH_NV,
				VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_RANGE_SIZE_NV = (VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_UPDATE_SCRATCH_NV - VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_OBJECT_NV + 1),
				VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_MAX_ENUM_NV = 0x7FFFFFFF
			} VkAccelerationStructureMemoryRequirementsTypeNV;

			typedef enum VkGeometryFlagBitsNV {
				VK_GEOMETRY_OPAQUE_BIT_NV = 0x00000001,
				VK_GEOMETRY_NO_DUPLICATE_ANY_HIT_INVOCATION_BIT_NV = 0x00000002,
				VK_GEOMETRY_FLAG_BITS_MAX_ENUM_NV = 0x7FFFFFFF
			} VkGeometryFlagBitsNV;
			typedef VkFlags VkGeometryFlagsNV;

			typedef enum VkGeometryInstanceFlagBitsNV {
				VK_GEOMETRY_INSTANCE_TRIANGLE_CULL_DISABLE_BIT_NV = 0x00000001,
				VK_GEOMETRY_INSTANCE_TRIANGLE_FRONT_COUNTERCLOCKWISE_BIT_NV = 0x00000002,
				VK_GEOMETRY_INSTANCE_FORCE_OPAQUE_BIT_NV = 0x00000004,
				VK_GEOMETRY_INSTANCE_FORCE_NO_OPAQUE_BIT_NV = 0x00000008,
				VK_GEOMETRY_INSTANCE_FLAG_BITS_MAX_ENUM_NV = 0x7FFFFFFF
			} VkGeometryInstanceFlagBitsNV;
			typedef VkFlags VkGeometryInstanceFlagsNV;

			typedef enum VkBuildAccelerationStructureFlagBitsNV {
				VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_NV = 0x00000001,
				VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_NV = 0x00000002,
				VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_NV = 0x00000004,
				VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_BUILD_BIT_NV = 0x00000008,
				VK_BUILD_ACCELERATION_STRUCTURE_LOW_MEMORY_BIT_NV = 0x00000010,
				VK_BUILD_ACCELERATION_STRUCTURE_FLAG_BITS_MAX_ENUM_NV = 0x7FFFFFFF
			} VkBuildAccelerationStructureFlagBitsNV;
			typedef VkFlags VkBuildAccelerationStructureFlagsNV;
			typedef struct VkRayTracingShaderGroupCreateInfoNV {
				VkStructureType                  sType;
				const void* pNext;
				VkRayTracingShaderGroupTypeNV    type;
				uint32_t                         generalShader;
				uint32_t                         closestHitShader;
				uint32_t                         anyHitShader;
				uint32_t                         intersectionShader;
			} VkRayTracingShaderGroupCreateInfoNV;

			typedef struct VkRayTracingPipelineCreateInfoNV {
				VkStructureType                               sType;
				const void* pNext;
				VkPipelineCreateFlags                         flags;
				uint32_t                                      stageCount;
				const VkPipelineShaderStageCreateInfo* pStages;
				uint32_t                                      groupCount;
				const VkRayTracingShaderGroupCreateInfoNV* pGroups;
				uint32_t                                      maxRecursionDepth;
				VkPipelineLayout                              layout;
				VkPipeline                                    basePipelineHandle;
				int32_t                                       basePipelineIndex;
			} VkRayTracingPipelineCreateInfoNV;

			typedef struct VkGeometryTrianglesNV {
				VkStructureType    sType;
				const void* pNext;
				VkBuffer           vertexData;
				VkDeviceSize       vertexOffset;
				uint32_t           vertexCount;
				VkDeviceSize       vertexStride;
				VkFormat           vertexFormat;
				VkBuffer           indexData;
				VkDeviceSize       indexOffset;
				uint32_t           indexCount;
				VkIndexType        indexType;
				VkBuffer           transformData;
				VkDeviceSize       transformOffset;
			} VkGeometryTrianglesNV;

			typedef struct VkGeometryAABBNV {
				VkStructureType    sType;
				const void* pNext;
				VkBuffer           aabbData;
				uint32_t           numAABBs;
				uint32_t           stride;
				VkDeviceSize       offset;
			} VkGeometryAABBNV;

			typedef struct VkGeometryDataNV {
				VkGeometryTrianglesNV    triangles;
				VkGeometryAABBNV         aabbs;
			} VkGeometryDataNV;

			typedef struct VkGeometryNV {
				VkStructureType      sType;
				const void* pNext;
				VkGeometryTypeNV     geometryType;
				VkGeometryDataNV     geometry;
				VkGeometryFlagsNV    flags;
			} VkGeometryNV;

			typedef struct VkAccelerationStructureInfoNV {
				VkStructureType                        sType;
				const void* pNext;
				VkAccelerationStructureTypeNV          type;
				VkBuildAccelerationStructureFlagsNV    flags;
				uint32_t                               instanceCount;
				uint32_t                               geometryCount;
				const VkGeometryNV* pGeometries;
			} VkAccelerationStructureInfoNV;

			typedef struct VkAccelerationStructureCreateInfoNV {
				VkStructureType                  sType;
				const void* pNext;
				VkDeviceSize                     compactedSize;
				VkAccelerationStructureInfoNV    info;
			} VkAccelerationStructureCreateInfoNV;

			typedef struct VkBindAccelerationStructureMemoryInfoNV {
				VkStructureType              sType;
				const void* pNext;
				VkAccelerationStructureNV    accelerationStructure;
				VkDeviceMemory               memory;
				VkDeviceSize                 memoryOffset;
				uint32_t                     deviceIndexCount;
				const uint32_t* pDeviceIndices;
			} VkBindAccelerationStructureMemoryInfoNV;

			typedef struct VkWriteDescriptorSetAccelerationStructureNV {
				VkStructureType                     sType;
				const void* pNext;
				uint32_t                            accelerationStructureCount;
				const VkAccelerationStructureNV* pAccelerationStructures;
			} VkWriteDescriptorSetAccelerationStructureNV;

			typedef struct VkAccelerationStructureMemoryRequirementsInfoNV {
				VkStructureType                                    sType;
				const void* pNext;
				VkAccelerationStructureMemoryRequirementsTypeNV    type;
				VkAccelerationStructureNV                          accelerationStructure;
			} VkAccelerationStructureMemoryRequirementsInfoNV;

			typedef struct VkPhysicalDeviceRayTracingPropertiesNV {
				VkStructureType    sType;
				void* pNext;
				uint32_t           shaderGroupHandleSize;
				uint32_t           maxRecursionDepth;
				uint32_t           maxShaderGroupStride;
				uint32_t           shaderGroupBaseAlignment;
				uint64_t           maxGeometryCount;
				uint64_t           maxInstanceCount;
				uint64_t           maxTriangleCount;
				uint32_t           maxDescriptorSetAccelerationStructures;
			} VkPhysicalDeviceRayTracingPropertiesNV;

			typedef VkResult(VKAPI_PTR* PFN_vkCreateAccelerationStructureNV)(VkDevice device, const VkAccelerationStructureCreateInfoNV* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkAccelerationStructureNV* pAccelerationStructure);
			typedef void (VKAPI_PTR* PFN_vkDestroyAccelerationStructureNV)(VkDevice device, VkAccelerationStructureNV accelerationStructure, const VkAllocationCallbacks* pAllocator);
			typedef void (VKAPI_PTR* PFN_vkGetAccelerationStructureMemoryRequirementsNV)(VkDevice device, const VkAccelerationStructureMemoryRequirementsInfoNV* pInfo, VkMemoryRequirements2KHR* pMemoryRequirements);
			typedef VkResult(VKAPI_PTR* PFN_vkBindAccelerationStructureMemoryNV)(VkDevice device, uint32_t bindInfoCount, const VkBindAccelerationStructureMemoryInfoNV* pBindInfos);
			typedef void (VKAPI_PTR* PFN_vkCmdBuildAccelerationStructureNV)(VkCommandBuffer commandBuffer, const VkAccelerationStructureInfoNV* pInfo, VkBuffer instanceData, VkDeviceSize instanceOffset, VkBool32 update, VkAccelerationStructureNV dst, VkAccelerationStructureNV src, VkBuffer scratch, VkDeviceSize scratchOffset);
			typedef void (VKAPI_PTR* PFN_vkCmdCopyAccelerationStructureNV)(VkCommandBuffer commandBuffer, VkAccelerationStructureNV dst, VkAccelerationStructureNV src, VkCopyAccelerationStructureModeNV mode);
			typedef void (VKAPI_PTR* PFN_vkCmdTraceRaysNV)(VkCommandBuffer commandBuffer, VkBuffer raygenShaderBindingTableBuffer, VkDeviceSize raygenShaderBindingOffset, VkBuffer missShaderBindingTableBuffer, VkDeviceSize missShaderBindingOffset, VkDeviceSize missShaderBindingStride, VkBuffer hitShaderBindingTableBuffer, VkDeviceSize hitShaderBindingOffset, VkDeviceSize hitShaderBindingStride, VkBuffer callableShaderBindingTableBuffer, VkDeviceSize callableShaderBindingOffset, VkDeviceSize callableShaderBindingStride, uint32_t width, uint32_t height, uint32_t depth);
			typedef VkResult(VKAPI_PTR* PFN_vkCreateRayTracingPipelinesNV)(VkDevice device, VkPipelineCache pipelineCache, uint32_t createInfoCount, const VkRayTracingPipelineCreateInfoNV* pCreateInfos, const VkAllocationCallbacks* pAllocator, VkPipeline* pPipelines);
			typedef VkResult(VKAPI_PTR* PFN_vkGetRayTracingShaderGroupHandlesNV)(VkDevice device, VkPipeline pipeline, uint32_t firstGroup, uint32_t groupCount, size_t dataSize, void* pData);
			typedef VkResult(VKAPI_PTR* PFN_vkGetAccelerationStructureHandleNV)(VkDevice device, VkAccelerationStructureNV accelerationStructure, size_t dataSize, void* pData);
			typedef void (VKAPI_PTR* PFN_vkCmdWriteAccelerationStructuresPropertiesNV)(VkCommandBuffer commandBuffer, uint32_t accelerationStructureCount, const VkAccelerationStructureNV* pAccelerationStructures, VkQueryType queryType, VkQueryPool queryPool, uint32_t firstQuery);
			typedef VkResult(VKAPI_PTR* PFN_vkCompileDeferredNV)(VkDevice device, VkPipeline pipeline, uint32_t shader);

#ifndef VK_NO_PROTOTYPES
			VKAPI_ATTR VkResult VKAPI_CALL vkCreateAccelerationStructureNV(
				VkDevice                                    device,
				const VkAccelerationStructureCreateInfoNV* pCreateInfo,
				const VkAllocationCallbacks* pAllocator,
				VkAccelerationStructureNV* pAccelerationStructure);

			VKAPI_ATTR void VKAPI_CALL vkDestroyAccelerationStructureNV(
				VkDevice                                    device,
				VkAccelerationStructureNV                   accelerationStructure,
				const VkAllocationCallbacks* pAllocator);

			VKAPI_ATTR void VKAPI_CALL vkGetAccelerationStructureMemoryRequirementsNV(
				VkDevice                                    device,
				const VkAccelerationStructureMemoryRequirementsInfoNV* pInfo,
				VkMemoryRequirements2KHR* pMemoryRequirements);

			VKAPI_ATTR VkResult VKAPI_CALL vkBindAccelerationStructureMemoryNV(
				VkDevice                                    device,
				uint32_t                                    bindInfoCount,
				const VkBindAccelerationStructureMemoryInfoNV* pBindInfos);

			VKAPI_ATTR void VKAPI_CALL vkCmdBuildAccelerationStructureNV(
				VkCommandBuffer                             commandBuffer,
				const VkAccelerationStructureInfoNV* pInfo,
				VkBuffer                                    instanceData,
				VkDeviceSize                                instanceOffset,
				VkBool32                                    update,
				VkAccelerationStructureNV                   dst,
				VkAccelerationStructureNV                   src,
				VkBuffer                                    scratch,
				VkDeviceSize                                scratchOffset);

			VKAPI_ATTR void VKAPI_CALL vkCmdCopyAccelerationStructureNV(
				VkCommandBuffer                             commandBuffer,
				VkAccelerationStructureNV                   dst,
				VkAccelerationStructureNV                   src,
				VkCopyAccelerationStructureModeNV           mode);

			VKAPI_ATTR void VKAPI_CALL vkCmdTraceRaysNV(
				VkCommandBuffer                             commandBuffer,
				VkBuffer                                    raygenShaderBindingTableBuffer,
				VkDeviceSize                                raygenShaderBindingOffset,
				VkBuffer                                    missShaderBindingTableBuffer,
				VkDeviceSize                                missShaderBindingOffset,
				VkDeviceSize                                missShaderBindingStride,
				VkBuffer                                    hitShaderBindingTableBuffer,
				VkDeviceSize                                hitShaderBindingOffset,
				VkDeviceSize                                hitShaderBindingStride,
				VkBuffer                                    callableShaderBindingTableBuffer,
				VkDeviceSize                                callableShaderBindingOffset,
				VkDeviceSize                                callableShaderBindingStride,
				uint32_t                                    width,
				uint32_t                                    height,
				uint32_t                                    depth);

			VKAPI_ATTR VkResult VKAPI_CALL vkCreateRayTracingPipelinesNV(
				VkDevice                                    device,
				VkPipelineCache                             pipelineCache,
				uint32_t                                    createInfoCount,
				const VkRayTracingPipelineCreateInfoNV* pCreateInfos,
				const VkAllocationCallbacks* pAllocator,
				VkPipeline* pPipelines);

			VKAPI_ATTR VkResult VKAPI_CALL vkGetRayTracingShaderGroupHandlesNV(
				VkDevice                                    device,
				VkPipeline                                  pipeline,
				uint32_t                                    firstGroup,
				uint32_t                                    groupCount,
				size_t                                      dataSize,
				void* pData);

			VKAPI_ATTR VkResult VKAPI_CALL vkGetAccelerationStructureHandleNV(
				VkDevice                                    device,
				VkAccelerationStructureNV                   accelerationStructure,
				size_t                                      dataSize,
				void* pData);

			VKAPI_ATTR void VKAPI_CALL vkCmdWriteAccelerationStructuresPropertiesNV(
				VkCommandBuffer                             commandBuffer,
				uint32_t                                    accelerationStructureCount,
				const VkAccelerationStructureNV* pAccelerationStructures,
				VkQueryType                                 queryType,
				VkQueryPool                                 queryPool,
				uint32_t                                    firstQuery);

			VKAPI_ATTR VkResult VKAPI_CALL vkCompileDeferredNV(
				VkDevice                                    device,
				VkPipeline                                  pipeline,
				uint32_t                                    shader);
#endif


#define VK_NV_representative_fragment_test 1
#define VK_NV_REPRESENTATIVE_FRAGMENT_TEST_SPEC_VERSION 1
#define VK_NV_REPRESENTATIVE_FRAGMENT_TEST_EXTENSION_NAME "VK_NV_representative_fragment_test"
			typedef struct VkPhysicalDeviceRepresentativeFragmentTestFeaturesNV {
				VkStructureType    sType;
				void* pNext;
				VkBool32           representativeFragmentTest;
			} VkPhysicalDeviceRepresentativeFragmentTestFeaturesNV;

			typedef struct VkPipelineRepresentativeFragmentTestStateCreateInfoNV {
				VkStructureType    sType;
				const void* pNext;
				VkBool32           representativeFragmentTestEnable;
			} VkPipelineRepresentativeFragmentTestStateCreateInfoNV;



#define VK_EXT_filter_cubic 1
#define VK_EXT_FILTER_CUBIC_SPEC_VERSION  2
#define VK_EXT_FILTER_CUBIC_EXTENSION_NAME "VK_EXT_filter_cubic"
			typedef struct VkPhysicalDeviceImageViewImageFormatInfoEXT {
				VkStructureType    sType;
				void* pNext;
				VkImageViewType    imageViewType;
			} VkPhysicalDeviceImageViewImageFormatInfoEXT;

			typedef struct VkFilterCubicImageViewImageFormatPropertiesEXT {
				VkStructureType    sType;
				void* pNext;
				VkBool32           filterCubic;
				VkBool32           filterCubicMinmax;
			} VkFilterCubicImageViewImageFormatPropertiesEXT;



#define VK_EXT_global_priority 1
#define VK_EXT_GLOBAL_PRIORITY_SPEC_VERSION 2
#define VK_EXT_GLOBAL_PRIORITY_EXTENSION_NAME "VK_EXT_global_priority"

			typedef enum VkQueueGlobalPriorityEXT {
				VK_QUEUE_GLOBAL_PRIORITY_LOW_EXT = 128,
				VK_QUEUE_GLOBAL_PRIORITY_MEDIUM_EXT = 256,
				VK_QUEUE_GLOBAL_PRIORITY_HIGH_EXT = 512,
				VK_QUEUE_GLOBAL_PRIORITY_REALTIME_EXT = 1024,
				VK_QUEUE_GLOBAL_PRIORITY_BEGIN_RANGE_EXT = VK_QUEUE_GLOBAL_PRIORITY_LOW_EXT,
				VK_QUEUE_GLOBAL_PRIORITY_END_RANGE_EXT = VK_QUEUE_GLOBAL_PRIORITY_REALTIME_EXT,
				VK_QUEUE_GLOBAL_PRIORITY_RANGE_SIZE_EXT = (VK_QUEUE_GLOBAL_PRIORITY_REALTIME_EXT - VK_QUEUE_GLOBAL_PRIORITY_LOW_EXT + 1),
				VK_QUEUE_GLOBAL_PRIORITY_MAX_ENUM_EXT = 0x7FFFFFFF
			} VkQueueGlobalPriorityEXT;
			typedef struct VkDeviceQueueGlobalPriorityCreateInfoEXT {
				VkStructureType             sType;
				const void* pNext;
				VkQueueGlobalPriorityEXT    globalPriority;
			} VkDeviceQueueGlobalPriorityCreateInfoEXT;



#define VK_EXT_external_memory_host 1
#define VK_EXT_EXTERNAL_MEMORY_HOST_SPEC_VERSION 1
#define VK_EXT_EXTERNAL_MEMORY_HOST_EXTENSION_NAME "VK_EXT_external_memory_host"
			typedef struct VkImportMemoryHostPointerInfoEXT {
				VkStructureType                       sType;
				const void* pNext;
				VkExternalMemoryHandleTypeFlagBits    handleType;
				void* pHostPointer;
			} VkImportMemoryHostPointerInfoEXT;

			typedef struct VkMemoryHostPointerPropertiesEXT {
				VkStructureType    sType;
				void* pNext;
				uint32_t           memoryTypeBits;
			} VkMemoryHostPointerPropertiesEXT;

			typedef struct VkPhysicalDeviceExternalMemoryHostPropertiesEXT {
				VkStructureType    sType;
				void* pNext;
				VkDeviceSize       minImportedHostPointerAlignment;
			} VkPhysicalDeviceExternalMemoryHostPropertiesEXT;

			typedef VkResult(VKAPI_PTR* PFN_vkGetMemoryHostPointerPropertiesEXT)(VkDevice device, VkExternalMemoryHandleTypeFlagBits handleType, const void* pHostPointer, VkMemoryHostPointerPropertiesEXT* pMemoryHostPointerProperties);

#ifndef VK_NO_PROTOTYPES
			VKAPI_ATTR VkResult VKAPI_CALL vkGetMemoryHostPointerPropertiesEXT(
				VkDevice                                    device,
				VkExternalMemoryHandleTypeFlagBits          handleType,
				const void* pHostPointer,
				VkMemoryHostPointerPropertiesEXT* pMemoryHostPointerProperties);
#endif


#define VK_AMD_buffer_marker 1
#define VK_AMD_BUFFER_MARKER_SPEC_VERSION 1
#define VK_AMD_BUFFER_MARKER_EXTENSION_NAME "VK_AMD_buffer_marker"
			typedef void (VKAPI_PTR * PFN_vkCmdWriteBufferMarkerAMD)(VkCommandBuffer commandBuffer, VkPipelineStageFlagBits pipelineStage, VkBuffer dstBuffer, VkDeviceSize dstOffset, uint32_t marker);

#ifndef VK_NO_PROTOTYPES
			VKAPI_ATTR void VKAPI_CALL vkCmdWriteBufferMarkerAMD(
				VkCommandBuffer                             commandBuffer,
				VkPipelineStageFlagBits                     pipelineStage,
				VkBuffer                                    dstBuffer,
				VkDeviceSize                                dstOffset,
				uint32_t                                    marker);
#endif


#define VK_EXT_calibrated_timestamps 1
#define VK_EXT_CALIBRATED_TIMESTAMPS_SPEC_VERSION 1
#define VK_EXT_CALIBRATED_TIMESTAMPS_EXTENSION_NAME "VK_EXT_calibrated_timestamps"

			typedef enum VkTimeDomainEXT {
				VK_TIME_DOMAIN_DEVICE_EXT = 0,
				VK_TIME_DOMAIN_CLOCK_MONOTONIC_EXT = 1,
				VK_TIME_DOMAIN_CLOCK_MONOTONIC_RAW_EXT = 2,
				VK_TIME_DOMAIN_QUERY_PERFORMANCE_COUNTER_EXT = 3,
				VK_TIME_DOMAIN_BEGIN_RANGE_EXT = VK_TIME_DOMAIN_DEVICE_EXT,
				VK_TIME_DOMAIN_END_RANGE_EXT = VK_TIME_DOMAIN_QUERY_PERFORMANCE_COUNTER_EXT,
				VK_TIME_DOMAIN_RANGE_SIZE_EXT = (VK_TIME_DOMAIN_QUERY_PERFORMANCE_COUNTER_EXT - VK_TIME_DOMAIN_DEVICE_EXT + 1),
				VK_TIME_DOMAIN_MAX_ENUM_EXT = 0x7FFFFFFF
			} VkTimeDomainEXT;
			typedef struct VkCalibratedTimestampInfoEXT {
				VkStructureType    sType;
				const void* pNext;
				VkTimeDomainEXT    timeDomain;
			} VkCalibratedTimestampInfoEXT;

			typedef VkResult(VKAPI_PTR* PFN_vkGetPhysicalDeviceCalibrateableTimeDomainsEXT)(VkPhysicalDevice physicalDevice, uint32_t* pTimeDomainCount, VkTimeDomainEXT* pTimeDomains);
			typedef VkResult(VKAPI_PTR* PFN_vkGetCalibratedTimestampsEXT)(VkDevice device, uint32_t timestampCount, const VkCalibratedTimestampInfoEXT* pTimestampInfos, uint64_t* pTimestamps, uint64_t* pMaxDeviation);

#ifndef VK_NO_PROTOTYPES
			VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceCalibrateableTimeDomainsEXT(
				VkPhysicalDevice                            physicalDevice,
				uint32_t* pTimeDomainCount,
				VkTimeDomainEXT* pTimeDomains);

			VKAPI_ATTR VkResult VKAPI_CALL vkGetCalibratedTimestampsEXT(
				VkDevice                                    device,
				uint32_t                                    timestampCount,
				const VkCalibratedTimestampInfoEXT* pTimestampInfos,
				uint64_t* pTimestamps,
				uint64_t* pMaxDeviation);
#endif


#define VK_AMD_shader_core_properties 1
#define VK_AMD_SHADER_CORE_PROPERTIES_SPEC_VERSION 1
#define VK_AMD_SHADER_CORE_PROPERTIES_EXTENSION_NAME "VK_AMD_shader_core_properties"
			typedef struct VkPhysicalDeviceShaderCorePropertiesAMD {
				VkStructureType    sType;
				void* pNext;
				uint32_t           shaderEngineCount;
				uint32_t           shaderArraysPerEngineCount;
				uint32_t           computeUnitsPerShaderArray;
				uint32_t           simdPerComputeUnit;
				uint32_t           wavefrontsPerSimd;
				uint32_t           wavefrontSize;
				uint32_t           sgprsPerSimd;
				uint32_t           minSgprAllocation;
				uint32_t           maxSgprAllocation;
				uint32_t           sgprAllocationGranularity;
				uint32_t           vgprsPerSimd;
				uint32_t           minVgprAllocation;
				uint32_t           maxVgprAllocation;
				uint32_t           vgprAllocationGranularity;
			} VkPhysicalDeviceShaderCorePropertiesAMD;



#define VK_AMD_memory_overallocation_behavior 1
#define VK_AMD_MEMORY_OVERALLOCATION_BEHAVIOR_SPEC_VERSION 1
#define VK_AMD_MEMORY_OVERALLOCATION_BEHAVIOR_EXTENSION_NAME "VK_AMD_memory_overallocation_behavior"

			typedef enum VkMemoryOverallocationBehaviorAMD {
				VK_MEMORY_OVERALLOCATION_BEHAVIOR_DEFAULT_AMD = 0,
				VK_MEMORY_OVERALLOCATION_BEHAVIOR_ALLOWED_AMD = 1,
				VK_MEMORY_OVERALLOCATION_BEHAVIOR_DISALLOWED_AMD = 2,
				VK_MEMORY_OVERALLOCATION_BEHAVIOR_BEGIN_RANGE_AMD = VK_MEMORY_OVERALLOCATION_BEHAVIOR_DEFAULT_AMD,
				VK_MEMORY_OVERALLOCATION_BEHAVIOR_END_RANGE_AMD = VK_MEMORY_OVERALLOCATION_BEHAVIOR_DISALLOWED_AMD,
				VK_MEMORY_OVERALLOCATION_BEHAVIOR_RANGE_SIZE_AMD = (VK_MEMORY_OVERALLOCATION_BEHAVIOR_DISALLOWED_AMD - VK_MEMORY_OVERALLOCATION_BEHAVIOR_DEFAULT_AMD + 1),
				VK_MEMORY_OVERALLOCATION_BEHAVIOR_MAX_ENUM_AMD = 0x7FFFFFFF
			} VkMemoryOverallocationBehaviorAMD;
			typedef struct VkDeviceMemoryOverallocationCreateInfoAMD {
				VkStructureType                      sType;
				const void* pNext;
				VkMemoryOverallocationBehaviorAMD    overallocationBehavior;
			} VkDeviceMemoryOverallocationCreateInfoAMD;



#define VK_EXT_vertex_attribute_divisor 1
#define VK_EXT_VERTEX_ATTRIBUTE_DIVISOR_SPEC_VERSION 3
#define VK_EXT_VERTEX_ATTRIBUTE_DIVISOR_EXTENSION_NAME "VK_EXT_vertex_attribute_divisor"
			typedef struct VkPhysicalDeviceVertexAttributeDivisorPropertiesEXT {
				VkStructureType    sType;
				void* pNext;
				uint32_t           maxVertexAttribDivisor;
			} VkPhysicalDeviceVertexAttributeDivisorPropertiesEXT;

			typedef struct VkVertexInputBindingDivisorDescriptionEXT {
				uint32_t    binding;
				uint32_t    divisor;
			} VkVertexInputBindingDivisorDescriptionEXT;

			typedef struct VkPipelineVertexInputDivisorStateCreateInfoEXT {
				VkStructureType                                     sType;
				const void* pNext;
				uint32_t                                            vertexBindingDivisorCount;
				const VkVertexInputBindingDivisorDescriptionEXT* pVertexBindingDivisors;
			} VkPipelineVertexInputDivisorStateCreateInfoEXT;

			typedef struct VkPhysicalDeviceVertexAttributeDivisorFeaturesEXT {
				VkStructureType    sType;
				void* pNext;
				VkBool32           vertexAttributeInstanceRateDivisor;
				VkBool32           vertexAttributeInstanceRateZeroDivisor;
			} VkPhysicalDeviceVertexAttributeDivisorFeaturesEXT;



#define VK_EXT_pipeline_creation_feedback 1
#define VK_EXT_PIPELINE_CREATION_FEEDBACK_SPEC_VERSION 1
#define VK_EXT_PIPELINE_CREATION_FEEDBACK_EXTENSION_NAME "VK_EXT_pipeline_creation_feedback"

			typedef enum VkPipelineCreationFeedbackFlagBitsEXT {
				VK_PIPELINE_CREATION_FEEDBACK_VALID_BIT_EXT = 0x00000001,
				VK_PIPELINE_CREATION_FEEDBACK_APPLICATION_PIPELINE_CACHE_HIT_BIT_EXT = 0x00000002,
				VK_PIPELINE_CREATION_FEEDBACK_BASE_PIPELINE_ACCELERATION_BIT_EXT = 0x00000004,
				VK_PIPELINE_CREATION_FEEDBACK_FLAG_BITS_MAX_ENUM_EXT = 0x7FFFFFFF
			} VkPipelineCreationFeedbackFlagBitsEXT;
			typedef VkFlags VkPipelineCreationFeedbackFlagsEXT;
			typedef struct VkPipelineCreationFeedbackEXT {
				VkPipelineCreationFeedbackFlagsEXT    flags;
				uint64_t                              duration;
			} VkPipelineCreationFeedbackEXT;

			typedef struct VkPipelineCreationFeedbackCreateInfoEXT {
				VkStructureType                   sType;
				const void* pNext;
				VkPipelineCreationFeedbackEXT* pPipelineCreationFeedback;
				uint32_t                          pipelineStageCreationFeedbackCount;
				VkPipelineCreationFeedbackEXT* pPipelineStageCreationFeedbacks;
			} VkPipelineCreationFeedbackCreateInfoEXT;



#define VK_NV_shader_subgroup_partitioned 1
#define VK_NV_SHADER_SUBGROUP_PARTITIONED_SPEC_VERSION 1
#define VK_NV_SHADER_SUBGROUP_PARTITIONED_EXTENSION_NAME "VK_NV_shader_subgroup_partitioned"


#define VK_NV_compute_shader_derivatives 1
#define VK_NV_COMPUTE_SHADER_DERIVATIVES_SPEC_VERSION 1
#define VK_NV_COMPUTE_SHADER_DERIVATIVES_EXTENSION_NAME "VK_NV_compute_shader_derivatives"
			typedef struct VkPhysicalDeviceComputeShaderDerivativesFeaturesNV {
				VkStructureType    sType;
				void* pNext;
				VkBool32           computeDerivativeGroupQuads;
				VkBool32           computeDerivativeGroupLinear;
			} VkPhysicalDeviceComputeShaderDerivativesFeaturesNV;



#define VK_NV_mesh_shader 1
#define VK_NV_MESH_SHADER_SPEC_VERSION    1
#define VK_NV_MESH_SHADER_EXTENSION_NAME  "VK_NV_mesh_shader"
			typedef struct VkPhysicalDeviceMeshShaderFeaturesNV {
				VkStructureType    sType;
				void* pNext;
				VkBool32           taskShader;
				VkBool32           meshShader;
			} VkPhysicalDeviceMeshShaderFeaturesNV;

			typedef struct VkPhysicalDeviceMeshShaderPropertiesNV {
				VkStructureType    sType;
				void* pNext;
				uint32_t           maxDrawMeshTasksCount;
				uint32_t           maxTaskWorkGroupInvocations;
				uint32_t           maxTaskWorkGroupSize[3];
				uint32_t           maxTaskTotalMemorySize;
				uint32_t           maxTaskOutputCount;
				uint32_t           maxMeshWorkGroupInvocations;
				uint32_t           maxMeshWorkGroupSize[3];
				uint32_t           maxMeshTotalMemorySize;
				uint32_t           maxMeshOutputVertices;
				uint32_t           maxMeshOutputPrimitives;
				uint32_t           maxMeshMultiviewViewCount;
				uint32_t           meshOutputPerVertexGranularity;
				uint32_t           meshOutputPerPrimitiveGranularity;
			} VkPhysicalDeviceMeshShaderPropertiesNV;

			typedef struct VkDrawMeshTasksIndirectCommandNV {
				uint32_t    taskCount;
				uint32_t    firstTask;
			} VkDrawMeshTasksIndirectCommandNV;

			typedef void (VKAPI_PTR* PFN_vkCmdDrawMeshTasksNV)(VkCommandBuffer commandBuffer, uint32_t taskCount, uint32_t firstTask);
			typedef void (VKAPI_PTR* PFN_vkCmdDrawMeshTasksIndirectNV)(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride);
			typedef void (VKAPI_PTR* PFN_vkCmdDrawMeshTasksIndirectCountNV)(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, VkBuffer countBuffer, VkDeviceSize countBufferOffset, uint32_t maxDrawCount, uint32_t stride);

#ifndef VK_NO_PROTOTYPES
			VKAPI_ATTR void VKAPI_CALL vkCmdDrawMeshTasksNV(
				VkCommandBuffer                             commandBuffer,
				uint32_t                                    taskCount,
				uint32_t                                    firstTask);

			VKAPI_ATTR void VKAPI_CALL vkCmdDrawMeshTasksIndirectNV(
				VkCommandBuffer                             commandBuffer,
				VkBuffer                                    buffer,
				VkDeviceSize                                offset,
				uint32_t                                    drawCount,
				uint32_t                                    stride);

			VKAPI_ATTR void VKAPI_CALL vkCmdDrawMeshTasksIndirectCountNV(
				VkCommandBuffer                             commandBuffer,
				VkBuffer                                    buffer,
				VkDeviceSize                                offset,
				VkBuffer                                    countBuffer,
				VkDeviceSize                                countBufferOffset,
				uint32_t                                    maxDrawCount,
				uint32_t                                    stride);
#endif


#define VK_NV_fragment_shader_barycentric 1
#define VK_NV_FRAGMENT_SHADER_BARYCENTRIC_SPEC_VERSION 1
#define VK_NV_FRAGMENT_SHADER_BARYCENTRIC_EXTENSION_NAME "VK_NV_fragment_shader_barycentric"
			typedef struct VkPhysicalDeviceFragmentShaderBarycentricFeaturesNV {
				VkStructureType    sType;
				void* pNext;
				VkBool32           fragmentShaderBarycentric;
			} VkPhysicalDeviceFragmentShaderBarycentricFeaturesNV;



#define VK_NV_shader_image_footprint 1
#define VK_NV_SHADER_IMAGE_FOOTPRINT_SPEC_VERSION 1
#define VK_NV_SHADER_IMAGE_FOOTPRINT_EXTENSION_NAME "VK_NV_shader_image_footprint"
			typedef struct VkPhysicalDeviceShaderImageFootprintFeaturesNV {
				VkStructureType    sType;
				void* pNext;
				VkBool32           imageFootprint;
			} VkPhysicalDeviceShaderImageFootprintFeaturesNV;



#define VK_NV_scissor_exclusive 1
#define VK_NV_SCISSOR_EXCLUSIVE_SPEC_VERSION 1
#define VK_NV_SCISSOR_EXCLUSIVE_EXTENSION_NAME "VK_NV_scissor_exclusive"
			typedef struct VkPipelineViewportExclusiveScissorStateCreateInfoNV {
				VkStructureType    sType;
				const void* pNext;
				uint32_t           exclusiveScissorCount;
				const VkRect2D* pExclusiveScissors;
			} VkPipelineViewportExclusiveScissorStateCreateInfoNV;

			typedef struct VkPhysicalDeviceExclusiveScissorFeaturesNV {
				VkStructureType    sType;
				void* pNext;
				VkBool32           exclusiveScissor;
			} VkPhysicalDeviceExclusiveScissorFeaturesNV;

			typedef void (VKAPI_PTR* PFN_vkCmdSetExclusiveScissorNV)(VkCommandBuffer commandBuffer, uint32_t firstExclusiveScissor, uint32_t exclusiveScissorCount, const VkRect2D* pExclusiveScissors);

#ifndef VK_NO_PROTOTYPES
			VKAPI_ATTR void VKAPI_CALL vkCmdSetExclusiveScissorNV(
				VkCommandBuffer                             commandBuffer,
				uint32_t                                    firstExclusiveScissor,
				uint32_t                                    exclusiveScissorCount,
				const VkRect2D* pExclusiveScissors);
#endif


#define VK_NV_device_diagnostic_checkpoints 1
#define VK_NV_DEVICE_DIAGNOSTIC_CHECKPOINTS_SPEC_VERSION 2
#define VK_NV_DEVICE_DIAGNOSTIC_CHECKPOINTS_EXTENSION_NAME "VK_NV_device_diagnostic_checkpoints"
			typedef struct VkQueueFamilyCheckpointPropertiesNV {
				VkStructureType         sType;
				void* pNext;
				VkPipelineStageFlags    checkpointExecutionStageMask;
			} VkQueueFamilyCheckpointPropertiesNV;

			typedef struct VkCheckpointDataNV {
				VkStructureType            sType;
				void* pNext;
				VkPipelineStageFlagBits    stage;
				void* pCheckpointMarker;
			} VkCheckpointDataNV;

			typedef void (VKAPI_PTR* PFN_vkCmdSetCheckpointNV)(VkCommandBuffer commandBuffer, const void* pCheckpointMarker);
			typedef void (VKAPI_PTR* PFN_vkGetQueueCheckpointDataNV)(VkQueue queue, uint32_t* pCheckpointDataCount, VkCheckpointDataNV* pCheckpointData);

#ifndef VK_NO_PROTOTYPES
			VKAPI_ATTR void VKAPI_CALL vkCmdSetCheckpointNV(
				VkCommandBuffer                             commandBuffer,
				const void* pCheckpointMarker);

			VKAPI_ATTR void VKAPI_CALL vkGetQueueCheckpointDataNV(
				VkQueue                                     queue,
				uint32_t* pCheckpointDataCount,
				VkCheckpointDataNV* pCheckpointData);
#endif


#define VK_INTEL_shader_integer_functions2 1
#define VK_INTEL_SHADER_INTEGER_FUNCTIONS2_SPEC_VERSION 1
#define VK_INTEL_SHADER_INTEGER_FUNCTIONS2_EXTENSION_NAME "VK_INTEL_shader_integer_functions2"
			typedef struct VkPhysicalDeviceShaderIntegerFunctions2INTEL {
				VkStructureType    sType;
				void* pNext;
				VkBool32           shaderIntegerFunctions2;
			} VkPhysicalDeviceShaderIntegerFunctions2INTEL;



#define VK_INTEL_performance_query 1
			VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkPerformanceConfigurationINTEL)
#define VK_INTEL_PERFORMANCE_QUERY_SPEC_VERSION 1
#define VK_INTEL_PERFORMANCE_QUERY_EXTENSION_NAME "VK_INTEL_performance_query"

				typedef enum VkPerformanceConfigurationTypeINTEL {
				VK_PERFORMANCE_CONFIGURATION_TYPE_COMMAND_QUEUE_METRICS_DISCOVERY_ACTIVATED_INTEL = 0,
				VK_PERFORMANCE_CONFIGURATION_TYPE_BEGIN_RANGE_INTEL = VK_PERFORMANCE_CONFIGURATION_TYPE_COMMAND_QUEUE_METRICS_DISCOVERY_ACTIVATED_INTEL,
				VK_PERFORMANCE_CONFIGURATION_TYPE_END_RANGE_INTEL = VK_PERFORMANCE_CONFIGURATION_TYPE_COMMAND_QUEUE_METRICS_DISCOVERY_ACTIVATED_INTEL,
				VK_PERFORMANCE_CONFIGURATION_TYPE_RANGE_SIZE_INTEL = (VK_PERFORMANCE_CONFIGURATION_TYPE_COMMAND_QUEUE_METRICS_DISCOVERY_ACTIVATED_INTEL - VK_PERFORMANCE_CONFIGURATION_TYPE_COMMAND_QUEUE_METRICS_DISCOVERY_ACTIVATED_INTEL + 1),
				VK_PERFORMANCE_CONFIGURATION_TYPE_MAX_ENUM_INTEL = 0x7FFFFFFF
			} VkPerformanceConfigurationTypeINTEL;

			typedef enum VkQueryPoolSamplingModeINTEL {
				VK_QUERY_POOL_SAMPLING_MODE_MANUAL_INTEL = 0,
				VK_QUERY_POOL_SAMPLING_MODE_BEGIN_RANGE_INTEL = VK_QUERY_POOL_SAMPLING_MODE_MANUAL_INTEL,
				VK_QUERY_POOL_SAMPLING_MODE_END_RANGE_INTEL = VK_QUERY_POOL_SAMPLING_MODE_MANUAL_INTEL,
				VK_QUERY_POOL_SAMPLING_MODE_RANGE_SIZE_INTEL = (VK_QUERY_POOL_SAMPLING_MODE_MANUAL_INTEL - VK_QUERY_POOL_SAMPLING_MODE_MANUAL_INTEL + 1),
				VK_QUERY_POOL_SAMPLING_MODE_MAX_ENUM_INTEL = 0x7FFFFFFF
			} VkQueryPoolSamplingModeINTEL;

			typedef enum VkPerformanceOverrideTypeINTEL {
				VK_PERFORMANCE_OVERRIDE_TYPE_NULL_HARDWARE_INTEL = 0,
				VK_PERFORMANCE_OVERRIDE_TYPE_FLUSH_GPU_CACHES_INTEL = 1,
				VK_PERFORMANCE_OVERRIDE_TYPE_BEGIN_RANGE_INTEL = VK_PERFORMANCE_OVERRIDE_TYPE_NULL_HARDWARE_INTEL,
				VK_PERFORMANCE_OVERRIDE_TYPE_END_RANGE_INTEL = VK_PERFORMANCE_OVERRIDE_TYPE_FLUSH_GPU_CACHES_INTEL,
				VK_PERFORMANCE_OVERRIDE_TYPE_RANGE_SIZE_INTEL = (VK_PERFORMANCE_OVERRIDE_TYPE_FLUSH_GPU_CACHES_INTEL - VK_PERFORMANCE_OVERRIDE_TYPE_NULL_HARDWARE_INTEL + 1),
				VK_PERFORMANCE_OVERRIDE_TYPE_MAX_ENUM_INTEL = 0x7FFFFFFF
			} VkPerformanceOverrideTypeINTEL;

			typedef enum VkPerformanceParameterTypeINTEL {
				VK_PERFORMANCE_PARAMETER_TYPE_HW_COUNTERS_SUPPORTED_INTEL = 0,
				VK_PERFORMANCE_PARAMETER_TYPE_STREAM_MARKER_VALID_BITS_INTEL = 1,
				VK_PERFORMANCE_PARAMETER_TYPE_BEGIN_RANGE_INTEL = VK_PERFORMANCE_PARAMETER_TYPE_HW_COUNTERS_SUPPORTED_INTEL,
				VK_PERFORMANCE_PARAMETER_TYPE_END_RANGE_INTEL = VK_PERFORMANCE_PARAMETER_TYPE_STREAM_MARKER_VALID_BITS_INTEL,
				VK_PERFORMANCE_PARAMETER_TYPE_RANGE_SIZE_INTEL = (VK_PERFORMANCE_PARAMETER_TYPE_STREAM_MARKER_VALID_BITS_INTEL - VK_PERFORMANCE_PARAMETER_TYPE_HW_COUNTERS_SUPPORTED_INTEL + 1),
				VK_PERFORMANCE_PARAMETER_TYPE_MAX_ENUM_INTEL = 0x7FFFFFFF
			} VkPerformanceParameterTypeINTEL;

			typedef enum VkPerformanceValueTypeINTEL {
				VK_PERFORMANCE_VALUE_TYPE_UINT32_INTEL = 0,
				VK_PERFORMANCE_VALUE_TYPE_UINT64_INTEL = 1,
				VK_PERFORMANCE_VALUE_TYPE_FLOAT_INTEL = 2,
				VK_PERFORMANCE_VALUE_TYPE_BOOL_INTEL = 3,
				VK_PERFORMANCE_VALUE_TYPE_STRING_INTEL = 4,
				VK_PERFORMANCE_VALUE_TYPE_BEGIN_RANGE_INTEL = VK_PERFORMANCE_VALUE_TYPE_UINT32_INTEL,
				VK_PERFORMANCE_VALUE_TYPE_END_RANGE_INTEL = VK_PERFORMANCE_VALUE_TYPE_STRING_INTEL,
				VK_PERFORMANCE_VALUE_TYPE_RANGE_SIZE_INTEL = (VK_PERFORMANCE_VALUE_TYPE_STRING_INTEL - VK_PERFORMANCE_VALUE_TYPE_UINT32_INTEL + 1),
				VK_PERFORMANCE_VALUE_TYPE_MAX_ENUM_INTEL = 0x7FFFFFFF
			} VkPerformanceValueTypeINTEL;
			typedef union VkPerformanceValueDataINTEL {
				uint32_t       value32;
				uint64_t       value64;
				float          valueFloat;
				VkBool32       valueBool;
				const char* valueString;
			} VkPerformanceValueDataINTEL;

			typedef struct VkPerformanceValueINTEL {
				VkPerformanceValueTypeINTEL    type;
				VkPerformanceValueDataINTEL    data;
			} VkPerformanceValueINTEL;

			typedef struct VkInitializePerformanceApiInfoINTEL {
				VkStructureType    sType;
				const void* pNext;
				void* pUserData;
			} VkInitializePerformanceApiInfoINTEL;

			typedef struct VkQueryPoolCreateInfoINTEL {
				VkStructureType                 sType;
				const void* pNext;
				VkQueryPoolSamplingModeINTEL    performanceCountersSampling;
			} VkQueryPoolCreateInfoINTEL;

			typedef struct VkPerformanceMarkerInfoINTEL {
				VkStructureType    sType;
				const void* pNext;
				uint64_t           marker;
			} VkPerformanceMarkerInfoINTEL;

			typedef struct VkPerformanceStreamMarkerInfoINTEL {
				VkStructureType    sType;
				const void* pNext;
				uint32_t           marker;
			} VkPerformanceStreamMarkerInfoINTEL;

			typedef struct VkPerformanceOverrideInfoINTEL {
				VkStructureType                   sType;
				const void* pNext;
				VkPerformanceOverrideTypeINTEL    type;
				VkBool32                          enable;
				uint64_t                          parameter;
			} VkPerformanceOverrideInfoINTEL;

			typedef struct VkPerformanceConfigurationAcquireInfoINTEL {
				VkStructureType                        sType;
				const void* pNext;
				VkPerformanceConfigurationTypeINTEL    type;
			} VkPerformanceConfigurationAcquireInfoINTEL;

			typedef VkResult(VKAPI_PTR* PFN_vkInitializePerformanceApiINTEL)(VkDevice device, const VkInitializePerformanceApiInfoINTEL* pInitializeInfo);
			typedef void (VKAPI_PTR* PFN_vkUninitializePerformanceApiINTEL)(VkDevice device);
			typedef VkResult(VKAPI_PTR* PFN_vkCmdSetPerformanceMarkerINTEL)(VkCommandBuffer commandBuffer, const VkPerformanceMarkerInfoINTEL* pMarkerInfo);
			typedef VkResult(VKAPI_PTR* PFN_vkCmdSetPerformanceStreamMarkerINTEL)(VkCommandBuffer commandBuffer, const VkPerformanceStreamMarkerInfoINTEL* pMarkerInfo);
			typedef VkResult(VKAPI_PTR* PFN_vkCmdSetPerformanceOverrideINTEL)(VkCommandBuffer commandBuffer, const VkPerformanceOverrideInfoINTEL* pOverrideInfo);
			typedef VkResult(VKAPI_PTR* PFN_vkAcquirePerformanceConfigurationINTEL)(VkDevice device, const VkPerformanceConfigurationAcquireInfoINTEL* pAcquireInfo, VkPerformanceConfigurationINTEL* pConfiguration);
			typedef VkResult(VKAPI_PTR* PFN_vkReleasePerformanceConfigurationINTEL)(VkDevice device, VkPerformanceConfigurationINTEL configuration);
			typedef VkResult(VKAPI_PTR* PFN_vkQueueSetPerformanceConfigurationINTEL)(VkQueue queue, VkPerformanceConfigurationINTEL configuration);
			typedef VkResult(VKAPI_PTR* PFN_vkGetPerformanceParameterINTEL)(VkDevice device, VkPerformanceParameterTypeINTEL parameter, VkPerformanceValueINTEL* pValue);

#ifndef VK_NO_PROTOTYPES
			VKAPI_ATTR VkResult VKAPI_CALL vkInitializePerformanceApiINTEL(
				VkDevice                                    device,
				const VkInitializePerformanceApiInfoINTEL* pInitializeInfo);

			VKAPI_ATTR void VKAPI_CALL vkUninitializePerformanceApiINTEL(
				VkDevice                                    device);

			VKAPI_ATTR VkResult VKAPI_CALL vkCmdSetPerformanceMarkerINTEL(
				VkCommandBuffer                             commandBuffer,
				const VkPerformanceMarkerInfoINTEL* pMarkerInfo);

			VKAPI_ATTR VkResult VKAPI_CALL vkCmdSetPerformanceStreamMarkerINTEL(
				VkCommandBuffer                             commandBuffer,
				const VkPerformanceStreamMarkerInfoINTEL* pMarkerInfo);

			VKAPI_ATTR VkResult VKAPI_CALL vkCmdSetPerformanceOverrideINTEL(
				VkCommandBuffer                             commandBuffer,
				const VkPerformanceOverrideInfoINTEL* pOverrideInfo);

			VKAPI_ATTR VkResult VKAPI_CALL vkAcquirePerformanceConfigurationINTEL(
				VkDevice                                    device,
				const VkPerformanceConfigurationAcquireInfoINTEL* pAcquireInfo,
				VkPerformanceConfigurationINTEL* pConfiguration);

			VKAPI_ATTR VkResult VKAPI_CALL vkReleasePerformanceConfigurationINTEL(
				VkDevice                                    device,
				VkPerformanceConfigurationINTEL             configuration);

			VKAPI_ATTR VkResult VKAPI_CALL vkQueueSetPerformanceConfigurationINTEL(
				VkQueue                                     queue,
				VkPerformanceConfigurationINTEL             configuration);

			VKAPI_ATTR VkResult VKAPI_CALL vkGetPerformanceParameterINTEL(
				VkDevice                                    device,
				VkPerformanceParameterTypeINTEL             parameter,
				VkPerformanceValueINTEL* pValue);
#endif


#define VK_EXT_pci_bus_info 1
#define VK_EXT_PCI_BUS_INFO_SPEC_VERSION  2
#define VK_EXT_PCI_BUS_INFO_EXTENSION_NAME "VK_EXT_pci_bus_info"
			typedef struct VkPhysicalDevicePCIBusInfoPropertiesEXT {
				VkStructureType    sType;
				void* pNext;
				uint32_t           pciDomain;
				uint32_t           pciBus;
				uint32_t           pciDevice;
				uint32_t           pciFunction;
			} VkPhysicalDevicePCIBusInfoPropertiesEXT;



#define VK_AMD_display_native_hdr 1
#define VK_AMD_DISPLAY_NATIVE_HDR_SPEC_VERSION 1
#define VK_AMD_DISPLAY_NATIVE_HDR_EXTENSION_NAME "VK_AMD_display_native_hdr"
			typedef struct VkDisplayNativeHdrSurfaceCapabilitiesAMD {
				VkStructureType    sType;
				void* pNext;
				VkBool32           localDimmingSupport;
			} VkDisplayNativeHdrSurfaceCapabilitiesAMD;

			typedef struct VkSwapchainDisplayNativeHdrCreateInfoAMD {
				VkStructureType    sType;
				const void* pNext;
				VkBool32           localDimmingEnable;
			} VkSwapchainDisplayNativeHdrCreateInfoAMD;

			typedef void (VKAPI_PTR* PFN_vkSetLocalDimmingAMD)(VkDevice device, VkSwapchainKHR swapChain, VkBool32 localDimmingEnable);

#ifndef VK_NO_PROTOTYPES
			VKAPI_ATTR void VKAPI_CALL vkSetLocalDimmingAMD(
				VkDevice                                    device,
				VkSwapchainKHR                              swapChain,
				VkBool32                                    localDimmingEnable);
#endif


#define VK_EXT_fragment_density_map 1
#define VK_EXT_FRAGMENT_DENSITY_MAP_SPEC_VERSION 1
#define VK_EXT_FRAGMENT_DENSITY_MAP_EXTENSION_NAME "VK_EXT_fragment_density_map"
			typedef struct VkPhysicalDeviceFragmentDensityMapFeaturesEXT {
				VkStructureType    sType;
				void* pNext;
				VkBool32           fragmentDensityMap;
				VkBool32           fragmentDensityMapDynamic;
				VkBool32           fragmentDensityMapNonSubsampledImages;
			} VkPhysicalDeviceFragmentDensityMapFeaturesEXT;

			typedef struct VkPhysicalDeviceFragmentDensityMapPropertiesEXT {
				VkStructureType    sType;
				void* pNext;
				VkExtent2D         minFragmentDensityTexelSize;
				VkExtent2D         maxFragmentDensityTexelSize;
				VkBool32           fragmentDensityInvocations;
			} VkPhysicalDeviceFragmentDensityMapPropertiesEXT;

			typedef struct VkRenderPassFragmentDensityMapCreateInfoEXT {
				VkStructureType          sType;
				const void* pNext;
				VkAttachmentReference    fragmentDensityMapAttachment;
			} VkRenderPassFragmentDensityMapCreateInfoEXT;
#endif
#if defined(VK_EXT_scalar_block_layout)
			{
				VK_EXT_SCALAR_BLOCK_LAYOUT_EXTENSION_NAME,
				VK_EXT_SCALAR_BLOCK_LAYOUT_SPEC_VERSION,
				true,
				{
				}
			},
#endif
#ifdef XXX
#define VK_GOOGLE_hlsl_functionality1 1
#define VK_GOOGLE_HLSL_FUNCTIONALITY1_SPEC_VERSION 1
#define VK_GOOGLE_HLSL_FUNCTIONALITY1_EXTENSION_NAME "VK_GOOGLE_hlsl_functionality1"


#define VK_GOOGLE_decorate_string 1
#define VK_GOOGLE_DECORATE_STRING_SPEC_VERSION 1
#define VK_GOOGLE_DECORATE_STRING_EXTENSION_NAME "VK_GOOGLE_decorate_string"


#define VK_EXT_subgroup_size_control 1
#define VK_EXT_SUBGROUP_SIZE_CONTROL_SPEC_VERSION 2
#define VK_EXT_SUBGROUP_SIZE_CONTROL_EXTENSION_NAME "VK_EXT_subgroup_size_control"
typedef struct VkPhysicalDeviceSubgroupSizeControlFeaturesEXT {
    VkStructureType    sType;
    void*              pNext;
    VkBool32           subgroupSizeControl;
    VkBool32           computeFullSubgroups;
} VkPhysicalDeviceSubgroupSizeControlFeaturesEXT;

typedef struct VkPhysicalDeviceSubgroupSizeControlPropertiesEXT {
    VkStructureType       sType;
    void*                 pNext;
    uint32_t              minSubgroupSize;
    uint32_t              maxSubgroupSize;
    uint32_t              maxComputeWorkgroupSubgroups;
    VkShaderStageFlags    requiredSubgroupSizeStages;
} VkPhysicalDeviceSubgroupSizeControlPropertiesEXT;

typedef struct VkPipelineShaderStageRequiredSubgroupSizeCreateInfoEXT {
    VkStructureType    sType;
    void*              pNext;
    uint32_t           requiredSubgroupSize;
} VkPipelineShaderStageRequiredSubgroupSizeCreateInfoEXT;



#define VK_AMD_shader_core_properties2 1
#define VK_AMD_SHADER_CORE_PROPERTIES_2_SPEC_VERSION 1
#define VK_AMD_SHADER_CORE_PROPERTIES_2_EXTENSION_NAME "VK_AMD_shader_core_properties2"

typedef enum VkShaderCorePropertiesFlagBitsAMD {
    VK_SHADER_CORE_PROPERTIES_FLAG_BITS_MAX_ENUM_AMD = 0x7FFFFFFF
} VkShaderCorePropertiesFlagBitsAMD;
typedef VkFlags VkShaderCorePropertiesFlagsAMD;
typedef struct VkPhysicalDeviceShaderCoreProperties2AMD {
    VkStructureType                   sType;
    void*                             pNext;
    VkShaderCorePropertiesFlagsAMD    shaderCoreFeatures;
    uint32_t                          activeComputeUnitCount;
} VkPhysicalDeviceShaderCoreProperties2AMD;



#define VK_AMD_device_coherent_memory 1
#define VK_AMD_DEVICE_COHERENT_MEMORY_SPEC_VERSION 1
#define VK_AMD_DEVICE_COHERENT_MEMORY_EXTENSION_NAME "VK_AMD_device_coherent_memory"
typedef struct VkPhysicalDeviceCoherentMemoryFeaturesAMD {
    VkStructureType    sType;
    void*              pNext;
    VkBool32           deviceCoherentMemory;
} VkPhysicalDeviceCoherentMemoryFeaturesAMD;
#endif
#if defined(VK_EXT_memory_budget)
			{
				VK_EXT_MEMORY_BUDGET_EXTENSION_NAME,
				VK_EXT_MEMORY_BUDGET_SPEC_VERSION,
				true,
				{
				}
			},
#endif
#if XXX
#define VK_EXT_memory_priority 1
#define VK_EXT_MEMORY_PRIORITY_SPEC_VERSION 1
#define VK_EXT_MEMORY_PRIORITY_EXTENSION_NAME "VK_EXT_memory_priority"
			typedef struct VkPhysicalDeviceMemoryPriorityFeaturesEXT {
				VkStructureType    sType;
				void* pNext;
				VkBool32           memoryPriority;
			} VkPhysicalDeviceMemoryPriorityFeaturesEXT;

			typedef struct VkMemoryPriorityAllocateInfoEXT {
				VkStructureType    sType;
				const void* pNext;
				float              priority;
			} VkMemoryPriorityAllocateInfoEXT;



#define VK_NV_dedicated_allocation_image_aliasing 1
#define VK_NV_DEDICATED_ALLOCATION_IMAGE_ALIASING_SPEC_VERSION 1
#define VK_NV_DEDICATED_ALLOCATION_IMAGE_ALIASING_EXTENSION_NAME "VK_NV_dedicated_allocation_image_aliasing"
			typedef struct VkPhysicalDeviceDedicatedAllocationImageAliasingFeaturesNV {
				VkStructureType    sType;
				void* pNext;
				VkBool32           dedicatedAllocationImageAliasing;
			} VkPhysicalDeviceDedicatedAllocationImageAliasingFeaturesNV;



#define VK_EXT_buffer_device_address 1
			typedef uint64_t VkDeviceAddress;
#define VK_EXT_BUFFER_DEVICE_ADDRESS_SPEC_VERSION 2
#define VK_EXT_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME "VK_EXT_buffer_device_address"
			typedef struct VkPhysicalDeviceBufferDeviceAddressFeaturesEXT {
				VkStructureType    sType;
				void* pNext;
				VkBool32           bufferDeviceAddress;
				VkBool32           bufferDeviceAddressCaptureReplay;
				VkBool32           bufferDeviceAddressMultiDevice;
			} VkPhysicalDeviceBufferDeviceAddressFeaturesEXT;

			typedef VkPhysicalDeviceBufferDeviceAddressFeaturesEXT VkPhysicalDeviceBufferAddressFeaturesEXT;

			typedef struct VkBufferDeviceAddressInfoEXT {
				VkStructureType    sType;
				const void* pNext;
				VkBuffer           buffer;
			} VkBufferDeviceAddressInfoEXT;

			typedef struct VkBufferDeviceAddressCreateInfoEXT {
				VkStructureType    sType;
				const void* pNext;
				VkDeviceAddress    deviceAddress;
			} VkBufferDeviceAddressCreateInfoEXT;

			typedef VkDeviceAddress(VKAPI_PTR* PFN_vkGetBufferDeviceAddressEXT)(VkDevice device, const VkBufferDeviceAddressInfoEXT* pInfo);

#ifndef VK_NO_PROTOTYPES
			VKAPI_ATTR VkDeviceAddress VKAPI_CALL vkGetBufferDeviceAddressEXT(
				VkDevice                                    device,
				const VkBufferDeviceAddressInfoEXT* pInfo);
#endif


#define VK_EXT_separate_stencil_usage 1
#define VK_EXT_SEPARATE_STENCIL_USAGE_SPEC_VERSION 1
#define VK_EXT_SEPARATE_STENCIL_USAGE_EXTENSION_NAME "VK_EXT_separate_stencil_usage"
			typedef struct VkImageStencilUsageCreateInfoEXT {
				VkStructureType      sType;
				const void* pNext;
				VkImageUsageFlags    stencilUsage;
			} VkImageStencilUsageCreateInfoEXT;



#define VK_EXT_validation_features 1
#define VK_EXT_VALIDATION_FEATURES_SPEC_VERSION 1
#define VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME "VK_EXT_validation_features"

			typedef enum VkValidationFeatureEnableEXT {
				VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT = 0,
				VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_RESERVE_BINDING_SLOT_EXT = 1,
				VK_VALIDATION_FEATURE_ENABLE_BEGIN_RANGE_EXT = VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT,
				VK_VALIDATION_FEATURE_ENABLE_END_RANGE_EXT = VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_RESERVE_BINDING_SLOT_EXT,
				VK_VALIDATION_FEATURE_ENABLE_RANGE_SIZE_EXT = (VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_RESERVE_BINDING_SLOT_EXT - VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT + 1),
				VK_VALIDATION_FEATURE_ENABLE_MAX_ENUM_EXT = 0x7FFFFFFF
			} VkValidationFeatureEnableEXT;

			typedef enum VkValidationFeatureDisableEXT {
				VK_VALIDATION_FEATURE_DISABLE_ALL_EXT = 0,
				VK_VALIDATION_FEATURE_DISABLE_SHADERS_EXT = 1,
				VK_VALIDATION_FEATURE_DISABLE_THREAD_SAFETY_EXT = 2,
				VK_VALIDATION_FEATURE_DISABLE_API_PARAMETERS_EXT = 3,
				VK_VALIDATION_FEATURE_DISABLE_OBJECT_LIFETIMES_EXT = 4,
				VK_VALIDATION_FEATURE_DISABLE_CORE_CHECKS_EXT = 5,
				VK_VALIDATION_FEATURE_DISABLE_UNIQUE_HANDLES_EXT = 6,
				VK_VALIDATION_FEATURE_DISABLE_BEGIN_RANGE_EXT = VK_VALIDATION_FEATURE_DISABLE_ALL_EXT,
				VK_VALIDATION_FEATURE_DISABLE_END_RANGE_EXT = VK_VALIDATION_FEATURE_DISABLE_UNIQUE_HANDLES_EXT,
				VK_VALIDATION_FEATURE_DISABLE_RANGE_SIZE_EXT = (VK_VALIDATION_FEATURE_DISABLE_UNIQUE_HANDLES_EXT - VK_VALIDATION_FEATURE_DISABLE_ALL_EXT + 1),
				VK_VALIDATION_FEATURE_DISABLE_MAX_ENUM_EXT = 0x7FFFFFFF
			} VkValidationFeatureDisableEXT;
			typedef struct VkValidationFeaturesEXT {
				VkStructureType                         sType;
				const void* pNext;
				uint32_t                                enabledValidationFeatureCount;
				const VkValidationFeatureEnableEXT* pEnabledValidationFeatures;
				uint32_t                                disabledValidationFeatureCount;
				const VkValidationFeatureDisableEXT* pDisabledValidationFeatures;
			} VkValidationFeaturesEXT;



#define VK_NV_cooperative_matrix 1
#define VK_NV_COOPERATIVE_MATRIX_SPEC_VERSION 1
#define VK_NV_COOPERATIVE_MATRIX_EXTENSION_NAME "VK_NV_cooperative_matrix"

			typedef enum VkComponentTypeNV {
				VK_COMPONENT_TYPE_FLOAT16_NV = 0,
				VK_COMPONENT_TYPE_FLOAT32_NV = 1,
				VK_COMPONENT_TYPE_FLOAT64_NV = 2,
				VK_COMPONENT_TYPE_SINT8_NV = 3,
				VK_COMPONENT_TYPE_SINT16_NV = 4,
				VK_COMPONENT_TYPE_SINT32_NV = 5,
				VK_COMPONENT_TYPE_SINT64_NV = 6,
				VK_COMPONENT_TYPE_UINT8_NV = 7,
				VK_COMPONENT_TYPE_UINT16_NV = 8,
				VK_COMPONENT_TYPE_UINT32_NV = 9,
				VK_COMPONENT_TYPE_UINT64_NV = 10,
				VK_COMPONENT_TYPE_BEGIN_RANGE_NV = VK_COMPONENT_TYPE_FLOAT16_NV,
				VK_COMPONENT_TYPE_END_RANGE_NV = VK_COMPONENT_TYPE_UINT64_NV,
				VK_COMPONENT_TYPE_RANGE_SIZE_NV = (VK_COMPONENT_TYPE_UINT64_NV - VK_COMPONENT_TYPE_FLOAT16_NV + 1),
				VK_COMPONENT_TYPE_MAX_ENUM_NV = 0x7FFFFFFF
			} VkComponentTypeNV;

			typedef enum VkScopeNV {
				VK_SCOPE_DEVICE_NV = 1,
				VK_SCOPE_WORKGROUP_NV = 2,
				VK_SCOPE_SUBGROUP_NV = 3,
				VK_SCOPE_QUEUE_FAMILY_NV = 5,
				VK_SCOPE_BEGIN_RANGE_NV = VK_SCOPE_DEVICE_NV,
				VK_SCOPE_END_RANGE_NV = VK_SCOPE_QUEUE_FAMILY_NV,
				VK_SCOPE_RANGE_SIZE_NV = (VK_SCOPE_QUEUE_FAMILY_NV - VK_SCOPE_DEVICE_NV + 1),
				VK_SCOPE_MAX_ENUM_NV = 0x7FFFFFFF
			} VkScopeNV;
			typedef struct VkCooperativeMatrixPropertiesNV {
				VkStructureType      sType;
				void* pNext;
				uint32_t             MSize;
				uint32_t             NSize;
				uint32_t             KSize;
				VkComponentTypeNV    AType;
				VkComponentTypeNV    BType;
				VkComponentTypeNV    CType;
				VkComponentTypeNV    DType;
				VkScopeNV            scope;
			} VkCooperativeMatrixPropertiesNV;

			typedef struct VkPhysicalDeviceCooperativeMatrixFeaturesNV {
				VkStructureType    sType;
				void* pNext;
				VkBool32           cooperativeMatrix;
				VkBool32           cooperativeMatrixRobustBufferAccess;
			} VkPhysicalDeviceCooperativeMatrixFeaturesNV;

			typedef struct VkPhysicalDeviceCooperativeMatrixPropertiesNV {
				VkStructureType       sType;
				void* pNext;
				VkShaderStageFlags    cooperativeMatrixSupportedStages;
			} VkPhysicalDeviceCooperativeMatrixPropertiesNV;

			typedef VkResult(VKAPI_PTR* PFN_vkGetPhysicalDeviceCooperativeMatrixPropertiesNV)(VkPhysicalDevice physicalDevice, uint32_t* pPropertyCount, VkCooperativeMatrixPropertiesNV* pProperties);

#ifndef VK_NO_PROTOTYPES
			VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceCooperativeMatrixPropertiesNV(
				VkPhysicalDevice                            physicalDevice,
				uint32_t* pPropertyCount,
				VkCooperativeMatrixPropertiesNV* pProperties);
#endif


#define VK_NV_coverage_reduction_mode 1
#define VK_NV_COVERAGE_REDUCTION_MODE_SPEC_VERSION 1
#define VK_NV_COVERAGE_REDUCTION_MODE_EXTENSION_NAME "VK_NV_coverage_reduction_mode"

			typedef enum VkCoverageReductionModeNV {
				VK_COVERAGE_REDUCTION_MODE_MERGE_NV = 0,
				VK_COVERAGE_REDUCTION_MODE_TRUNCATE_NV = 1,
				VK_COVERAGE_REDUCTION_MODE_BEGIN_RANGE_NV = VK_COVERAGE_REDUCTION_MODE_MERGE_NV,
				VK_COVERAGE_REDUCTION_MODE_END_RANGE_NV = VK_COVERAGE_REDUCTION_MODE_TRUNCATE_NV,
				VK_COVERAGE_REDUCTION_MODE_RANGE_SIZE_NV = (VK_COVERAGE_REDUCTION_MODE_TRUNCATE_NV - VK_COVERAGE_REDUCTION_MODE_MERGE_NV + 1),
				VK_COVERAGE_REDUCTION_MODE_MAX_ENUM_NV = 0x7FFFFFFF
			} VkCoverageReductionModeNV;
			typedef VkFlags VkPipelineCoverageReductionStateCreateFlagsNV;
			typedef struct VkPhysicalDeviceCoverageReductionModeFeaturesNV {
				VkStructureType    sType;
				void* pNext;
				VkBool32           coverageReductionMode;
			} VkPhysicalDeviceCoverageReductionModeFeaturesNV;

			typedef struct VkPipelineCoverageReductionStateCreateInfoNV {
				VkStructureType                                  sType;
				const void* pNext;
				VkPipelineCoverageReductionStateCreateFlagsNV    flags;
				VkCoverageReductionModeNV                        coverageReductionMode;
			} VkPipelineCoverageReductionStateCreateInfoNV;

			typedef struct VkFramebufferMixedSamplesCombinationNV {
				VkStructureType              sType;
				void* pNext;
				VkCoverageReductionModeNV    coverageReductionMode;
				VkSampleCountFlagBits        rasterizationSamples;
				VkSampleCountFlags           depthStencilSamples;
				VkSampleCountFlags           colorSamples;
			} VkFramebufferMixedSamplesCombinationNV;

			typedef VkResult(VKAPI_PTR* PFN_vkGetPhysicalDeviceSupportedFramebufferMixedSamplesCombinationsNV)(VkPhysicalDevice physicalDevice, uint32_t* pCombinationCount, VkFramebufferMixedSamplesCombinationNV* pCombinations);

#ifndef VK_NO_PROTOTYPES
			VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceSupportedFramebufferMixedSamplesCombinationsNV(
				VkPhysicalDevice                            physicalDevice,
				uint32_t* pCombinationCount,
				VkFramebufferMixedSamplesCombinationNV* pCombinations);
#endif


#define VK_EXT_fragment_shader_interlock 1
#define VK_EXT_FRAGMENT_SHADER_INTERLOCK_SPEC_VERSION 1
#define VK_EXT_FRAGMENT_SHADER_INTERLOCK_EXTENSION_NAME "VK_EXT_fragment_shader_interlock"
			typedef struct VkPhysicalDeviceFragmentShaderInterlockFeaturesEXT {
				VkStructureType    sType;
				void* pNext;
				VkBool32           fragmentShaderSampleInterlock;
				VkBool32           fragmentShaderPixelInterlock;
				VkBool32           fragmentShaderShadingRateInterlock;
			} VkPhysicalDeviceFragmentShaderInterlockFeaturesEXT;



#define VK_EXT_ycbcr_image_arrays 1
#define VK_EXT_YCBCR_IMAGE_ARRAYS_SPEC_VERSION 1
#define VK_EXT_YCBCR_IMAGE_ARRAYS_EXTENSION_NAME "VK_EXT_ycbcr_image_arrays"
			typedef struct VkPhysicalDeviceYcbcrImageArraysFeaturesEXT {
				VkStructureType    sType;
				void* pNext;
				VkBool32           ycbcrImageArrays;
			} VkPhysicalDeviceYcbcrImageArraysFeaturesEXT;



#define VK_EXT_headless_surface 1
#define VK_EXT_HEADLESS_SURFACE_SPEC_VERSION 0
#define VK_EXT_HEADLESS_SURFACE_EXTENSION_NAME "VK_EXT_headless_surface"
			typedef VkFlags VkHeadlessSurfaceCreateFlagsEXT;
			typedef struct VkHeadlessSurfaceCreateInfoEXT {
				VkStructureType                    sType;
				const void* pNext;
				VkHeadlessSurfaceCreateFlagsEXT    flags;
			} VkHeadlessSurfaceCreateInfoEXT;

			typedef VkResult(VKAPI_PTR* PFN_vkCreateHeadlessSurfaceEXT)(VkInstance instance, const VkHeadlessSurfaceCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSurfaceKHR* pSurface);

#ifndef VK_NO_PROTOTYPES
			VKAPI_ATTR VkResult VKAPI_CALL vkCreateHeadlessSurfaceEXT(
				VkInstance                                  instance,
				const VkHeadlessSurfaceCreateInfoEXT* pCreateInfo,
				const VkAllocationCallbacks* pAllocator,
				VkSurfaceKHR* pSurface);
#endif


#define VK_EXT_line_rasterization 1
#define VK_EXT_LINE_RASTERIZATION_SPEC_VERSION 1
#define VK_EXT_LINE_RASTERIZATION_EXTENSION_NAME "VK_EXT_line_rasterization"

typedef enum VkLineRasterizationModeEXT {
    VK_LINE_RASTERIZATION_MODE_DEFAULT_EXT = 0,
    VK_LINE_RASTERIZATION_MODE_RECTANGULAR_EXT = 1,
    VK_LINE_RASTERIZATION_MODE_BRESENHAM_EXT = 2,
    VK_LINE_RASTERIZATION_MODE_RECTANGULAR_SMOOTH_EXT = 3,
    VK_LINE_RASTERIZATION_MODE_BEGIN_RANGE_EXT = VK_LINE_RASTERIZATION_MODE_DEFAULT_EXT,
    VK_LINE_RASTERIZATION_MODE_END_RANGE_EXT = VK_LINE_RASTERIZATION_MODE_RECTANGULAR_SMOOTH_EXT,
    VK_LINE_RASTERIZATION_MODE_RANGE_SIZE_EXT = (VK_LINE_RASTERIZATION_MODE_RECTANGULAR_SMOOTH_EXT - VK_LINE_RASTERIZATION_MODE_DEFAULT_EXT + 1),
    VK_LINE_RASTERIZATION_MODE_MAX_ENUM_EXT = 0x7FFFFFFF
} VkLineRasterizationModeEXT;
typedef struct VkPhysicalDeviceLineRasterizationFeaturesEXT {
    VkStructureType    sType;
    void*              pNext;
    VkBool32           rectangularLines;
    VkBool32           bresenhamLines;
    VkBool32           smoothLines;
    VkBool32           stippledRectangularLines;
    VkBool32           stippledBresenhamLines;
    VkBool32           stippledSmoothLines;
} VkPhysicalDeviceLineRasterizationFeaturesEXT;

typedef struct VkPhysicalDeviceLineRasterizationPropertiesEXT {
    VkStructureType    sType;
    void*              pNext;
    uint32_t           lineSubPixelPrecisionBits;
} VkPhysicalDeviceLineRasterizationPropertiesEXT;

typedef struct VkPipelineRasterizationLineStateCreateInfoEXT {
    VkStructureType               sType;
    const void*                   pNext;
    VkLineRasterizationModeEXT    lineRasterizationMode;
    VkBool32                      stippledLineEnable;
    uint32_t                      lineStippleFactor;
    uint16_t                      lineStipplePattern;
} VkPipelineRasterizationLineStateCreateInfoEXT;

typedef void (VKAPI_PTR *PFN_vkCmdSetLineStippleEXT)(VkCommandBuffer commandBuffer, uint32_t lineStippleFactor, uint16_t lineStipplePattern);

#ifndef VK_NO_PROTOTYPES
VKAPI_ATTR void VKAPI_CALL vkCmdSetLineStippleEXT(
    VkCommandBuffer                             commandBuffer,
    uint32_t                                    lineStippleFactor,
    uint16_t                                    lineStipplePattern);
#endif


#define VK_EXT_host_query_reset 1
#define VK_EXT_HOST_QUERY_RESET_SPEC_VERSION 1
#define VK_EXT_HOST_QUERY_RESET_EXTENSION_NAME "VK_EXT_host_query_reset"
			typedef struct VkPhysicalDeviceHostQueryResetFeaturesEXT {
				VkStructureType    sType;
				void* pNext;
				VkBool32           hostQueryReset;
			} VkPhysicalDeviceHostQueryResetFeaturesEXT;

			typedef void (VKAPI_PTR* PFN_vkResetQueryPoolEXT)(VkDevice device, VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount);

#ifndef VK_NO_PROTOTYPES
			VKAPI_ATTR void VKAPI_CALL vkResetQueryPoolEXT(
				VkDevice                                    device,
				VkQueryPool                                 queryPool,
				uint32_t                                    firstQuery,
				uint32_t                                    queryCount);
#endif


#define VK_EXT_index_type_uint8 1
#define VK_EXT_INDEX_TYPE_UINT8_SPEC_VERSION 1
#define VK_EXT_INDEX_TYPE_UINT8_EXTENSION_NAME "VK_EXT_index_type_uint8"
typedef struct VkPhysicalDeviceIndexTypeUint8FeaturesEXT {
    VkStructureType    sType;
    void*              pNext;
    VkBool32           indexTypeUint8;
} VkPhysicalDeviceIndexTypeUint8FeaturesEXT;


#define VK_EXT_shader_demote_to_helper_invocation 1
#define VK_EXT_SHADER_DEMOTE_TO_HELPER_INVOCATION_SPEC_VERSION 1
#define VK_EXT_SHADER_DEMOTE_TO_HELPER_INVOCATION_EXTENSION_NAME "VK_EXT_shader_demote_to_helper_invocation"
			typedef struct VkPhysicalDeviceShaderDemoteToHelperInvocationFeaturesEXT {
				VkStructureType    sType;
				void* pNext;
				VkBool32           shaderDemoteToHelperInvocation;
			} VkPhysicalDeviceShaderDemoteToHelperInvocationFeaturesEXT;



#define VK_EXT_texel_buffer_alignment 1
#define VK_EXT_TEXEL_BUFFER_ALIGNMENT_SPEC_VERSION 1
#define VK_EXT_TEXEL_BUFFER_ALIGNMENT_EXTENSION_NAME "VK_EXT_texel_buffer_alignment"
			typedef struct VkPhysicalDeviceTexelBufferAlignmentFeaturesEXT {
				VkStructureType    sType;
				void* pNext;
				VkBool32           texelBufferAlignment;
			} VkPhysicalDeviceTexelBufferAlignmentFeaturesEXT;

			typedef struct VkPhysicalDeviceTexelBufferAlignmentPropertiesEXT {
				VkStructureType    sType;
				void* pNext;
				VkDeviceSize       storageTexelBufferOffsetAlignmentBytes;
				VkBool32           storageTexelBufferOffsetSingleTexelAlignment;
				VkDeviceSize       uniformTexelBufferOffsetAlignmentBytes;
				VkBool32           uniformTexelBufferOffsetSingleTexelAlignment;
			} VkPhysicalDeviceTexelBufferAlignmentPropertiesEXT;
#endif
#if defined(VK_KHR_win32_surface)
			{
				VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
				VK_KHR_WIN32_SURFACE_SPEC_VERSION,
				false,
				{
					{"vkCreateWin32SurfaceKHR", GET_TRAMPOLINE(Instance, CreateWin32Surface), false},
					{"vkGetPhysicalDeviceWin32PresentationSupportKHR", GET_TRAMPOLINE(PhysicalDevice, GetWin32PresentationSupport), false},
				}
			},
#endif
#if XXXX
#define VK_KHR_external_memory_win32 1
#define VK_KHR_EXTERNAL_MEMORY_WIN32_SPEC_VERSION 1
#define VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME "VK_KHR_external_memory_win32"
	typedef struct VkImportMemoryWin32HandleInfoKHR {
		VkStructureType                       sType;
		const void* pNext;
		VkExternalMemoryHandleTypeFlagBits    handleType;
		HANDLE                                handle;
		LPCWSTR                               name;
	} VkImportMemoryWin32HandleInfoKHR;

	typedef struct VkExportMemoryWin32HandleInfoKHR {
		VkStructureType               sType;
		const void* pNext;
		const SECURITY_ATTRIBUTES* pAttributes;
		DWORD                         dwAccess;
		LPCWSTR                       name;
	} VkExportMemoryWin32HandleInfoKHR;

	typedef struct VkMemoryWin32HandlePropertiesKHR {
		VkStructureType    sType;
		void* pNext;
		uint32_t           memoryTypeBits;
	} VkMemoryWin32HandlePropertiesKHR;

	typedef struct VkMemoryGetWin32HandleInfoKHR {
		VkStructureType                       sType;
		const void* pNext;
		VkDeviceMemory                        memory;
		VkExternalMemoryHandleTypeFlagBits    handleType;
	} VkMemoryGetWin32HandleInfoKHR;

	typedef VkResult(VKAPI_PTR* PFN_vkGetMemoryWin32HandleKHR)(VkDevice device, const VkMemoryGetWin32HandleInfoKHR* pGetWin32HandleInfo, HANDLE* pHandle);
	typedef VkResult(VKAPI_PTR* PFN_vkGetMemoryWin32HandlePropertiesKHR)(VkDevice device, VkExternalMemoryHandleTypeFlagBits handleType, HANDLE handle, VkMemoryWin32HandlePropertiesKHR* pMemoryWin32HandleProperties);

#ifndef VK_NO_PROTOTYPES
	VKAPI_ATTR VkResult VKAPI_CALL vkGetMemoryWin32HandleKHR(
		VkDevice                                    device,
		const VkMemoryGetWin32HandleInfoKHR* pGetWin32HandleInfo,
		HANDLE* pHandle);

	VKAPI_ATTR VkResult VKAPI_CALL vkGetMemoryWin32HandlePropertiesKHR(
		VkDevice                                    device,
		VkExternalMemoryHandleTypeFlagBits          handleType,
		HANDLE                                      handle,
		VkMemoryWin32HandlePropertiesKHR* pMemoryWin32HandleProperties);
#endif


#define VK_KHR_win32_keyed_mutex 1
#define VK_KHR_WIN32_KEYED_MUTEX_SPEC_VERSION 1
#define VK_KHR_WIN32_KEYED_MUTEX_EXTENSION_NAME "VK_KHR_win32_keyed_mutex"
	typedef struct VkWin32KeyedMutexAcquireReleaseInfoKHR {
		VkStructureType          sType;
		const void* pNext;
		uint32_t                 acquireCount;
		const VkDeviceMemory* pAcquireSyncs;
		const uint64_t* pAcquireKeys;
		const uint32_t* pAcquireTimeouts;
		uint32_t                 releaseCount;
		const VkDeviceMemory* pReleaseSyncs;
		const uint64_t* pReleaseKeys;
	} VkWin32KeyedMutexAcquireReleaseInfoKHR;



#define VK_KHR_external_semaphore_win32 1
#define VK_KHR_EXTERNAL_SEMAPHORE_WIN32_SPEC_VERSION 1
#define VK_KHR_EXTERNAL_SEMAPHORE_WIN32_EXTENSION_NAME "VK_KHR_external_semaphore_win32"
	typedef struct VkImportSemaphoreWin32HandleInfoKHR {
		VkStructureType                          sType;
		const void* pNext;
		VkSemaphore                              semaphore;
		VkSemaphoreImportFlags                   flags;
		VkExternalSemaphoreHandleTypeFlagBits    handleType;
		HANDLE                                   handle;
		LPCWSTR                                  name;
	} VkImportSemaphoreWin32HandleInfoKHR;

	typedef struct VkExportSemaphoreWin32HandleInfoKHR {
		VkStructureType               sType;
		const void* pNext;
		const SECURITY_ATTRIBUTES* pAttributes;
		DWORD                         dwAccess;
		LPCWSTR                       name;
	} VkExportSemaphoreWin32HandleInfoKHR;

	typedef struct VkD3D12FenceSubmitInfoKHR {
		VkStructureType    sType;
		const void* pNext;
		uint32_t           waitSemaphoreValuesCount;
		const uint64_t* pWaitSemaphoreValues;
		uint32_t           signalSemaphoreValuesCount;
		const uint64_t* pSignalSemaphoreValues;
	} VkD3D12FenceSubmitInfoKHR;

	typedef struct VkSemaphoreGetWin32HandleInfoKHR {
		VkStructureType                          sType;
		const void* pNext;
		VkSemaphore                              semaphore;
		VkExternalSemaphoreHandleTypeFlagBits    handleType;
	} VkSemaphoreGetWin32HandleInfoKHR;

	typedef VkResult(VKAPI_PTR* PFN_vkImportSemaphoreWin32HandleKHR)(VkDevice device, const VkImportSemaphoreWin32HandleInfoKHR* pImportSemaphoreWin32HandleInfo);
	typedef VkResult(VKAPI_PTR* PFN_vkGetSemaphoreWin32HandleKHR)(VkDevice device, const VkSemaphoreGetWin32HandleInfoKHR* pGetWin32HandleInfo, HANDLE* pHandle);

#ifndef VK_NO_PROTOTYPES
	VKAPI_ATTR VkResult VKAPI_CALL vkImportSemaphoreWin32HandleKHR(
		VkDevice                                    device,
		const VkImportSemaphoreWin32HandleInfoKHR* pImportSemaphoreWin32HandleInfo);

	VKAPI_ATTR VkResult VKAPI_CALL vkGetSemaphoreWin32HandleKHR(
		VkDevice                                    device,
		const VkSemaphoreGetWin32HandleInfoKHR* pGetWin32HandleInfo,
		HANDLE* pHandle);
#endif


#define VK_KHR_external_fence_win32 1
#define VK_KHR_EXTERNAL_FENCE_WIN32_SPEC_VERSION 1
#define VK_KHR_EXTERNAL_FENCE_WIN32_EXTENSION_NAME "VK_KHR_external_fence_win32"
	typedef struct VkImportFenceWin32HandleInfoKHR {
		VkStructureType                      sType;
		const void* pNext;
		VkFence                              fence;
		VkFenceImportFlags                   flags;
		VkExternalFenceHandleTypeFlagBits    handleType;
		HANDLE                               handle;
		LPCWSTR                              name;
	} VkImportFenceWin32HandleInfoKHR;

	typedef struct VkExportFenceWin32HandleInfoKHR {
		VkStructureType               sType;
		const void* pNext;
		const SECURITY_ATTRIBUTES* pAttributes;
		DWORD                         dwAccess;
		LPCWSTR                       name;
	} VkExportFenceWin32HandleInfoKHR;

	typedef struct VkFenceGetWin32HandleInfoKHR {
		VkStructureType                      sType;
		const void* pNext;
		VkFence                              fence;
		VkExternalFenceHandleTypeFlagBits    handleType;
	} VkFenceGetWin32HandleInfoKHR;

	typedef VkResult(VKAPI_PTR* PFN_vkImportFenceWin32HandleKHR)(VkDevice device, const VkImportFenceWin32HandleInfoKHR* pImportFenceWin32HandleInfo);
	typedef VkResult(VKAPI_PTR* PFN_vkGetFenceWin32HandleKHR)(VkDevice device, const VkFenceGetWin32HandleInfoKHR* pGetWin32HandleInfo, HANDLE* pHandle);

#ifndef VK_NO_PROTOTYPES
	VKAPI_ATTR VkResult VKAPI_CALL vkImportFenceWin32HandleKHR(
		VkDevice                                    device,
		const VkImportFenceWin32HandleInfoKHR* pImportFenceWin32HandleInfo);

	VKAPI_ATTR VkResult VKAPI_CALL vkGetFenceWin32HandleKHR(
		VkDevice                                    device,
		const VkFenceGetWin32HandleInfoKHR* pGetWin32HandleInfo,
		HANDLE* pHandle);
#endif


#define VK_NV_external_memory_win32 1
#define VK_NV_EXTERNAL_MEMORY_WIN32_SPEC_VERSION 1
#define VK_NV_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME "VK_NV_external_memory_win32"
	typedef struct VkImportMemoryWin32HandleInfoNV {
		VkStructureType                      sType;
		const void* pNext;
		VkExternalMemoryHandleTypeFlagsNV    handleType;
		HANDLE                               handle;
	} VkImportMemoryWin32HandleInfoNV;

	typedef struct VkExportMemoryWin32HandleInfoNV {
		VkStructureType               sType;
		const void* pNext;
		const SECURITY_ATTRIBUTES* pAttributes;
		DWORD                         dwAccess;
	} VkExportMemoryWin32HandleInfoNV;

	typedef VkResult(VKAPI_PTR* PFN_vkGetMemoryWin32HandleNV)(VkDevice device, VkDeviceMemory memory, VkExternalMemoryHandleTypeFlagsNV handleType, HANDLE* pHandle);

#ifndef VK_NO_PROTOTYPES
	VKAPI_ATTR VkResult VKAPI_CALL vkGetMemoryWin32HandleNV(
		VkDevice                                    device,
		VkDeviceMemory                              memory,
		VkExternalMemoryHandleTypeFlagsNV           handleType,
		HANDLE* pHandle);
#endif


#define VK_NV_win32_keyed_mutex 1
#define VK_NV_WIN32_KEYED_MUTEX_SPEC_VERSION 1
#define VK_NV_WIN32_KEYED_MUTEX_EXTENSION_NAME "VK_NV_win32_keyed_mutex"
	typedef struct VkWin32KeyedMutexAcquireReleaseInfoNV {
		VkStructureType          sType;
		const void* pNext;
		uint32_t                 acquireCount;
		const VkDeviceMemory* pAcquireSyncs;
		const uint64_t* pAcquireKeys;
		const uint32_t* pAcquireTimeoutMilliseconds;
		uint32_t                 releaseCount;
		const VkDeviceMemory* pReleaseSyncs;
		const uint64_t* pReleaseKeys;
	} VkWin32KeyedMutexAcquireReleaseInfoNV;



#define VK_EXT_full_screen_exclusive 1
#define VK_EXT_FULL_SCREEN_EXCLUSIVE_SPEC_VERSION 3
#define VK_EXT_FULL_SCREEN_EXCLUSIVE_EXTENSION_NAME "VK_EXT_full_screen_exclusive"

	typedef enum VkFullScreenExclusiveEXT {
		VK_FULL_SCREEN_EXCLUSIVE_DEFAULT_EXT = 0,
		VK_FULL_SCREEN_EXCLUSIVE_ALLOWED_EXT = 1,
		VK_FULL_SCREEN_EXCLUSIVE_DISALLOWED_EXT = 2,
		VK_FULL_SCREEN_EXCLUSIVE_APPLICATION_CONTROLLED_EXT = 3,
		VK_FULL_SCREEN_EXCLUSIVE_BEGIN_RANGE_EXT = VK_FULL_SCREEN_EXCLUSIVE_DEFAULT_EXT,
		VK_FULL_SCREEN_EXCLUSIVE_END_RANGE_EXT = VK_FULL_SCREEN_EXCLUSIVE_APPLICATION_CONTROLLED_EXT,
		VK_FULL_SCREEN_EXCLUSIVE_RANGE_SIZE_EXT = (VK_FULL_SCREEN_EXCLUSIVE_APPLICATION_CONTROLLED_EXT - VK_FULL_SCREEN_EXCLUSIVE_DEFAULT_EXT + 1),
		VK_FULL_SCREEN_EXCLUSIVE_MAX_ENUM_EXT = 0x7FFFFFFF
	} VkFullScreenExclusiveEXT;
	typedef struct VkSurfaceFullScreenExclusiveInfoEXT {
		VkStructureType             sType;
		void* pNext;
		VkFullScreenExclusiveEXT    fullScreenExclusive;
	} VkSurfaceFullScreenExclusiveInfoEXT;

	typedef struct VkSurfaceCapabilitiesFullScreenExclusiveEXT {
		VkStructureType    sType;
		void* pNext;
		VkBool32           fullScreenExclusiveSupported;
	} VkSurfaceCapabilitiesFullScreenExclusiveEXT;

	typedef struct VkSurfaceFullScreenExclusiveWin32InfoEXT {
		VkStructureType    sType;
		const void* pNext;
		HMONITOR           hmonitor;
	} VkSurfaceFullScreenExclusiveWin32InfoEXT;

	typedef VkResult(VKAPI_PTR* PFN_vkGetPhysicalDeviceSurfacePresentModes2EXT)(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceSurfaceInfo2KHR* pSurfaceInfo, uint32_t* pPresentModeCount, VkPresentModeKHR* pPresentModes);
	typedef VkResult(VKAPI_PTR* PFN_vkAcquireFullScreenExclusiveModeEXT)(VkDevice device, VkSwapchainKHR swapchain);
	typedef VkResult(VKAPI_PTR* PFN_vkReleaseFullScreenExclusiveModeEXT)(VkDevice device, VkSwapchainKHR swapchain);
	typedef VkResult(VKAPI_PTR* PFN_vkGetDeviceGroupSurfacePresentModes2EXT)(VkDevice device, const VkPhysicalDeviceSurfaceInfo2KHR* pSurfaceInfo, VkDeviceGroupPresentModeFlagsKHR* pModes);

#ifndef VK_NO_PROTOTYPES
	VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceSurfacePresentModes2EXT(
		VkPhysicalDevice                            physicalDevice,
		const VkPhysicalDeviceSurfaceInfo2KHR* pSurfaceInfo,
		uint32_t* pPresentModeCount,
		VkPresentModeKHR* pPresentModes);

	VKAPI_ATTR VkResult VKAPI_CALL vkAcquireFullScreenExclusiveModeEXT(
		VkDevice                                    device,
		VkSwapchainKHR                              swapchain);

	VKAPI_ATTR VkResult VKAPI_CALL vkReleaseFullScreenExclusiveModeEXT(
		VkDevice                                    device,
		VkSwapchainKHR                              swapchain);

	VKAPI_ATTR VkResult VKAPI_CALL vkGetDeviceGroupSurfacePresentModes2EXT(
		VkDevice                                    device,
		const VkPhysicalDeviceSurfaceInfo2KHR* pSurfaceInfo,
		VkDeviceGroupPresentModeFlagsKHR* pModes);
#endif
#endif
#if defined(VK_KHR_xcb_surface)
            {
                VK_KHR_XCB_SURFACE_EXTENSION_NAME,
                VK_KHR_XCB_SURFACE_SPEC_VERSION,
                false,
                {
                    {"vkCreateXcbSurfaceKHR", GET_TRAMPOLINE(Instance, CreateXcbSurface), false},
                    {"vkGetPhysicalDeviceXcbPresentationSupportKHR", GET_TRAMPOLINE(PhysicalDevice, GetPhysicalDeviceXcbPresentationSupport), false},
                }
            },
#endif
#if defined(VK_KHR_xlib_surface)
             {
                 VK_KHR_XLIB_SURFACE_EXTENSION_NAME,
                 VK_KHR_XLIB_SURFACE_SPEC_VERSION,
                 false,
                 {
                     {"vkCreateXlibSurfaceKHR", GET_TRAMPOLINE(Instance, CreateXlibSurface), false},
                     {"vkGetPhysicalDeviceXlibPresentationSupportKHR", GET_TRAMPOLINE(PhysicalDevice, GetPhysicalDeviceXlibPresentationSupport), false},
                 }
             },
#endif
#if defined(VK_EXT_acquire_xlib_display)
             {
                 VK_EXT_ACQUIRE_XLIB_DISPLAY_EXTENSION_NAME,
                 VK_EXT_ACQUIRE_XLIB_DISPLAY_SPEC_VERSION,
                 false,
                 {
                     {"vkAcquireXlibDisplayEXT", GET_TRAMPOLINE(PhysicalDevice, AcquireXlibDisplay), false},
                     {"vkGetRandROutputDisplayEXT", GET_TRAMPOLINE(PhysicalDevice, GetRandROutputDisplay), false},
                 }
             },
#endif
		}
	};
	return result;
}
