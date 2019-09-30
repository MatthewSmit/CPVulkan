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
#if defined(VK_AMD_gcn_shader)
			{
				VK_AMD_GCN_SHADER_EXTENSION_NAME,
				VK_AMD_GCN_SHADER_SPEC_VERSION,
				true,
				{
				}
			},
#endif
#if defined(VK_NV_dedicated_allocation)
			{
				VK_NV_DEDICATED_ALLOCATION_EXTENSION_NAME,
				VK_NV_DEDICATED_ALLOCATION_SPEC_VERSION,
				true,
				{
				}
			},
#endif
#if defined(VK_EXT_transform_feedback)
			{
				VK_EXT_TRANSFORM_FEEDBACK_EXTENSION_NAME,
				VK_EXT_TRANSFORM_FEEDBACK_SPEC_VERSION,
				true,
				{
					{"vkCmdBindTransformFeedbackBuffersEXT", GET_TRAMPOLINE(CommandBuffer, BindTransformFeedbackBuffers), false},
					{"vkCmdBeginTransformFeedbackEXT", GET_TRAMPOLINE(CommandBuffer, BeginTransformFeedback), false},
					{"vkCmdEndTransformFeedbackEXT", GET_TRAMPOLINE(CommandBuffer, EndTransformFeedback), false},
					{"vkCmdBeginQueryIndexedEXT", GET_TRAMPOLINE(CommandBuffer, BeginQueryIndexed), false},
					{"vkCmdEndQueryIndexedEXT", GET_TRAMPOLINE(CommandBuffer, EndQueryIndexed), false},
					{"vkCmdDrawIndirectByteCountEXT", GET_TRAMPOLINE(CommandBuffer, DrawIndirectByteCount), false},
				}
			},
#endif
#if defined(VK_NVX_image_view_handle)
			{
				VK_NVX_IMAGE_VIEW_HANDLE_EXTENSION_NAME,
				VK_NVX_IMAGE_VIEW_HANDLE_SPEC_VERSION,
				true,
				{
					{"vkGetImageViewHandleNVX", GET_TRAMPOLINE(Device, GetImageViewHandleNVX), false},
				}
			},
#endif
#if defined(VK_AMD_draw_indirect_count)
			{
				VK_AMD_DRAW_INDIRECT_COUNT_EXTENSION_NAME,
				VK_AMD_DRAW_INDIRECT_COUNT_SPEC_VERSION,
				true,
				{
					{"vkCmdDrawIndirectCountAMD", GET_TRAMPOLINE(CommandBuffer, DrawIndirectCount), false},
				}
			},
#endif
#if defined(VK_AMD_negative_viewport_height)
			{
				VK_AMD_NEGATIVE_VIEWPORT_HEIGHT_EXTENSION_NAME,
				VK_AMD_NEGATIVE_VIEWPORT_HEIGHT_SPEC_VERSION,
				true,
				{
				}
			},
#endif
#if defined(VK_AMD_gpu_shader_half_float)
			{
				VK_AMD_GPU_SHADER_HALF_FLOAT_EXTENSION_NAME,
				VK_AMD_GPU_SHADER_HALF_FLOAT_SPEC_VERSION,
				true,
				{
				}
			},
#endif
#if defined(VK_AMD_shader_ballot)
			{
				VK_AMD_SHADER_BALLOT_EXTENSION_NAME,
				VK_AMD_SHADER_BALLOT_SPEC_VERSION,
				true,
				{
				}
			},
#endif
#if defined(VK_AMD_texture_gather_bias_lod)
			{
				VK_AMD_TEXTURE_GATHER_BIAS_LOD_EXTENSION_NAME,
				VK_AMD_TEXTURE_GATHER_BIAS_LOD_SPEC_VERSION,
				true,
				{
				}
			},
#endif
#if defined(VK_AMD_shader_info)
			{
				VK_AMD_SHADER_INFO_EXTENSION_NAME,
				VK_AMD_SHADER_INFO_SPEC_VERSION,
				true,
				{
					{"vkGetShaderInfoAMD", GET_TRAMPOLINE(Device, GetShaderInfoAMD), false},
				}
			},
#endif
#if defined(VK_AMD_shader_image_load_store_lod)
			{
				VK_AMD_SHADER_IMAGE_LOAD_STORE_LOD_EXTENSION_NAME,
				VK_AMD_SHADER_IMAGE_LOAD_STORE_LOD_SPEC_VERSION,
				true,
				{
				}
			},
#endif
#if defined(VK_NV_corner_sampled_image)
			{
				VK_NV_CORNER_SAMPLED_IMAGE_EXTENSION_NAME,
				VK_NV_CORNER_SAMPLED_IMAGE_SPEC_VERSION,
				true,
				{
				}
			},
#endif
#if defined(VK_IMG_format_pvrtc)
			{
				VK_IMG_FORMAT_PVRTC_EXTENSION_NAME,
				VK_IMG_FORMAT_PVRTC_SPEC_VERSION,
				true,
				{
				}
			},
#endif
#if defined(VK_NV_external_memory_capabilities)
			{
				VK_NV_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME,
				VK_NV_EXTERNAL_MEMORY_CAPABILITIES_SPEC_VERSION,
				false,
				{
					{"vkGetPhysicalDeviceExternalImageFormatPropertiesNV", GET_TRAMPOLINE(PhysicalDevice, GetExternalImageFormatPropertiesNV), false},
				}
			},
#endif
#if defined(VK_NV_external_memory)
			{
				VK_NV_EXTERNAL_MEMORY_EXTENSION_NAME,
				VK_NV_EXTERNAL_MEMORY_SPEC_VERSION,
				true,
				{
				}
			},
#endif
#if defined(VK_EXT_validation_flags)
			{
				VK_EXT_VALIDATION_FLAGS_EXTENSION_NAME,
				VK_EXT_VALIDATION_FLAGS_SPEC_VERSION,
				false,
				{
				}
			},
#endif
#if defined(VK_EXT_shader_subgroup_ballot)
			{
				VK_EXT_SHADER_SUBGROUP_BALLOT_EXTENSION_NAME,
				VK_EXT_SHADER_SUBGROUP_BALLOT_SPEC_VERSION,
				true,
				{
				}
			},
#endif
#if defined(VK_EXT_shader_subgroup_vote)
			{
				VK_EXT_SHADER_SUBGROUP_VOTE_EXTENSION_NAME,
				VK_EXT_SHADER_SUBGROUP_VOTE_SPEC_VERSION,
				true,
				{
				}
			},
#endif
#if defined(VK_EXT_texture_compression_astc_hdr)
			{
				VK_EXT_TEXTURE_COMPRESSION_ASTC_HDR_EXTENSION_NAME,
				VK_EXT_TEXTURE_COMPRESSION_ASTC_HDR_SPEC_VERSION,
				true,
				{
				}
			},
#endif
#if defined(VK_EXT_astc_decode_mode)
			{
				VK_EXT_ASTC_DECODE_MODE_EXTENSION_NAME,
				VK_EXT_ASTC_DECODE_MODE_SPEC_VERSION,
				true,
				{
				}
			},
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
#if defined(VK_NVX_device_generated_commands)
			{
				VK_NVX_DEVICE_GENERATED_COMMANDS_EXTENSION_NAME,
				VK_NVX_DEVICE_GENERATED_COMMANDS_SPEC_VERSION,
				true,
				{
					{"vkCmdProcessCommandsNVX", GET_TRAMPOLINE(CommandBuffer, ProcessCommands), false},
					{"vkCmdReserveSpaceForCommandsNVX", GET_TRAMPOLINE(CommandBuffer, ReserveSpaceForCommands), false},
					{"vkCreateIndirectCommandsLayoutNVX", GET_TRAMPOLINE(Device, CreateIndirectCommandsLayoutNVX), false},
					{"vkDestroyIndirectCommandsLayoutNVX", GET_TRAMPOLINE(Device, DestroyIndirectCommandsLayoutNVX), false},
					{"vkCreateObjectTableNVX", GET_TRAMPOLINE(Device, CreateObjectTableNVX), false},
					{"vkDestroyObjectTableNVX", GET_TRAMPOLINE(Device, DestroyObjectTableNVX), false},
					{"vkRegisterObjectsNVX", GET_TRAMPOLINE(Device, RegisterObjectsNVX), false},
					{"vkUnregisterObjectsNVX", GET_TRAMPOLINE(Device, UnregisterObjectsNVX), false},
					{"vkGetPhysicalDeviceGeneratedCommandsPropertiesNVX", GET_TRAMPOLINE(PhysicalDevice, GetGeneratedCommandsPropertiesNVX), false},
				}
			},
#endif
#if defined(VK_NV_clip_space_w_scaling)
			{
				VK_NV_CLIP_SPACE_W_SCALING_EXTENSION_NAME,
				VK_NV_CLIP_SPACE_W_SCALING_SPEC_VERSION,
				true,
				{
					{"vkCmdSetViewportWScalingNV", GET_TRAMPOLINE(CommandBuffer, SetViewportWScaling), false},
				}
			},
#endif
#if defined(VK_EXT_direct_mode_display)
			{
				VK_EXT_DIRECT_MODE_DISPLAY_EXTENSION_NAME,
				VK_EXT_DIRECT_MODE_DISPLAY_SPEC_VERSION,
				false,
				{
					{"vkReleaseDisplayEXT", GET_TRAMPOLINE(PhysicalDevice, ReleaseDisplay), false},
				}
			},
#endif
#if defined(VK_EXT_display_surface_counter)
			{
				VK_EXT_DISPLAY_SURFACE_COUNTER_EXTENSION_NAME,
				VK_EXT_DISPLAY_SURFACE_COUNTER_SPEC_VERSION,
				false,
				{
					{"vkGetPhysicalDeviceSurfaceCapabilities2EXT", GET_TRAMPOLINE(PhysicalDevice, GetSurfaceCapabilities2), false},
				}
			},
#endif
#if defined(VK_EXT_display_control)
			{
				VK_EXT_DISPLAY_CONTROL_EXTENSION_NAME,
				VK_EXT_DISPLAY_CONTROL_SPEC_VERSION,
				true,
				{
					{"vkDisplayPowerControlEXT", GET_TRAMPOLINE(Device, DisplayPowerControl), false},
					{"vkRegisterDeviceEventEXT", GET_TRAMPOLINE(Device, RegisterDeviceEvent), false},
					{"vkRegisterDisplayEventEXT", GET_TRAMPOLINE(Device, RegisterDisplayEvent), false},
					{"vkGetSwapchainCounterEXT", GET_TRAMPOLINE(Device, GetSwapchainCounter), false},
				}
			},
#endif
#if defined(VK_GOOGLE_display_timing)
			{
				VK_GOOGLE_DISPLAY_TIMING_EXTENSION_NAME,
				VK_GOOGLE_DISPLAY_TIMING_SPEC_VERSION,
				true,
				{
					{"vkGetRefreshCycleDurationGOOGLE", GET_TRAMPOLINE(Device, GetRefreshCycleDurationGOOGLE), false},
					{"vkGetPastPresentationTimingGOOGLE", GET_TRAMPOLINE(Device, GetPastPresentationTimingGOOGLE), false},
				}
			},
#endif
#if defined(VK_NV_sample_mask_override_coverage)
			{
				VK_NV_SAMPLE_MASK_OVERRIDE_COVERAGE_EXTENSION_NAME,
				VK_NV_SAMPLE_MASK_OVERRIDE_COVERAGE_SPEC_VERSION,
				true,
				{
				}
			},
#endif
#if defined(VK_NV_geometry_shader_passthrough)
			{
				VK_NV_GEOMETRY_SHADER_PASSTHROUGH_EXTENSION_NAME,
				VK_NV_GEOMETRY_SHADER_PASSTHROUGH_SPEC_VERSION,
				true,
				{
				}
			},
#endif
#if defined(VK_NV_viewport_array2)
			{
				VK_NV_VIEWPORT_ARRAY2_EXTENSION_NAME,
				VK_NV_VIEWPORT_ARRAY2_SPEC_VERSION,
				true,
				{
				}
			},
#endif
#if defined(VK_NVX_multiview_per_view_attributes)
			{
				VK_NVX_MULTIVIEW_PER_VIEW_ATTRIBUTES_EXTENSION_NAME,
				VK_NVX_MULTIVIEW_PER_VIEW_ATTRIBUTES_SPEC_VERSION,
				true,
				{
				}
			},
#endif
#if defined(VK_NV_viewport_swizzle)
			{
				VK_NV_VIEWPORT_SWIZZLE_EXTENSION_NAME,
				VK_NV_VIEWPORT_SWIZZLE_SPEC_VERSION,
				true,
				{
				}
			},
#endif
#if defined(VK_EXT_discard_rectangles)
			{
				VK_EXT_DISCARD_RECTANGLES_EXTENSION_NAME,
				VK_EXT_DISCARD_RECTANGLES_SPEC_VERSION,
				true,
				{
					{"vkCmdSetDiscardRectangleEXT", GET_TRAMPOLINE(CommandBuffer, SetDiscardRectangle), false},
				}
			},
#endif
#if defined(VK_EXT_conservative_rasterization)
			{
				VK_EXT_CONSERVATIVE_RASTERIZATION_EXTENSION_NAME,
				VK_EXT_CONSERVATIVE_RASTERIZATION_SPEC_VERSION,
				true,
				{
				}
			},
#endif
#if defined(VK_EXT_depth_clip_enable)
			{
				VK_EXT_DEPTH_CLIP_ENABLE_EXTENSION_NAME,
				VK_EXT_DEPTH_CLIP_ENABLE_SPEC_VERSION,
				true,
				{
				}
			},
#endif
#if defined(VK_EXT_swapchain_colorspace)
			{
				VK_EXT_SWAPCHAIN_COLOR_SPACE_EXTENSION_NAME,
				VK_EXT_SWAPCHAIN_COLOR_SPACE_SPEC_VERSION,
				false,
				{
				}
			},
#endif
#if defined(VK_EXT_hdr_metadata)
			{
				VK_EXT_HDR_METADATA_EXTENSION_NAME,
				VK_EXT_HDR_METADATA_SPEC_VERSION,
				true,
				{
					{"vkSetHdrMetadataEXT", GET_TRAMPOLINE(Device, SetHdrMetadata), false},
				}
			},
#endif
#if defined(VK_EXT_external_memory_dma_buf)
			{
				VK_EXT_EXTERNAL_MEMORY_DMA_BUF_EXTENSION_NAME,
				VK_EXT_EXTERNAL_MEMORY_DMA_BUF_SPEC_VERSION,
				true,
				{
				}
			},
#endif
#if defined(VK_EXT_queue_family_foreign)
			{
				VK_EXT_QUEUE_FAMILY_FOREIGN_EXTENSION_NAME,
				VK_EXT_QUEUE_FAMILY_FOREIGN_SPEC_VERSION,
				true,
				{
				}
			},
#endif
#if defined(VK_EXT_debug_utils)
			{
				VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
				VK_EXT_DEBUG_UTILS_SPEC_VERSION,
				false,
				{
					{"vkSetDebugUtilsObjectNameEXT", GET_TRAMPOLINE(Device, SetDebugUtilsObjectName), true},
					{"vkSetDebugUtilsObjectTagEXT", GET_TRAMPOLINE(Device, SetDebugUtilsObjectTag), true},
					{"vkQueueBeginDebugUtilsLabelEXT", GET_TRAMPOLINE(Queue, BeginDebugUtilsLabel), true},
					{"vkQueueEndDebugUtilsLabelEXT", GET_TRAMPOLINE(Queue, EndDebugUtilsLabel), true},
					{"vkQueueInsertDebugUtilsLabelEXT", GET_TRAMPOLINE(Queue, InsertDebugUtilsLabel), true},
					{"vkCmdBeginDebugUtilsLabelEXT", GET_TRAMPOLINE(CommandBuffer, BeginDebugUtilsLabel), true},
					{"vkCmdEndDebugUtilsLabelEXT", GET_TRAMPOLINE(CommandBuffer, EndDebugUtilsLabel), true},
					{"vkCmdInsertDebugUtilsLabelEXT", GET_TRAMPOLINE(CommandBuffer, InsertDebugUtilsLabel), true},
					{"vkCreateDebugUtilsMessengerEXT", GET_TRAMPOLINE(Instance, CreateDebugUtilsMessenger), false},
					{"vkDestroyDebugUtilsMessengerEXT", GET_TRAMPOLINE(Instance, DestroyDebugUtilsMessenger), false},
					{"vkSubmitDebugUtilsMessageEXT", GET_TRAMPOLINE(Instance, SubmitDebugUtilsMessage), false},
				}
			},
#endif
#if defined(VK_EXT_sampler_filter_minmax)
			{
				VK_EXT_SAMPLER_FILTER_MINMAX_EXTENSION_NAME,
				VK_EXT_SAMPLER_FILTER_MINMAX_SPEC_VERSION,
				true,
				{
				}
			},
#endif
#if defined(VK_AMD_gpu_shader_int16)
			{
				VK_AMD_GPU_SHADER_INT16_EXTENSION_NAME,
				VK_AMD_GPU_SHADER_INT16_SPEC_VERSION,
				true,
				{
				}
			},
#endif
#if defined(VK_AMD_mixed_attachment_samples)
			{
				VK_AMD_MIXED_ATTACHMENT_SAMPLES_EXTENSION_NAME,
				VK_AMD_MIXED_ATTACHMENT_SAMPLES_SPEC_VERSION,
				true,
				{
				}
			},
#endif
#if defined(VK_AMD_shader_fragment_mask)
			{
				VK_AMD_SHADER_FRAGMENT_MASK_EXTENSION_NAME,
				VK_AMD_SHADER_FRAGMENT_MASK_SPEC_VERSION,
				true,
				{
				}
			},
#endif
#if defined(VK_EXT_inline_uniform_block)
			{
				VK_EXT_INLINE_UNIFORM_BLOCK_EXTENSION_NAME,
				VK_EXT_INLINE_UNIFORM_BLOCK_SPEC_VERSION,
				true,
				{
				}
			},
#endif
#if defined(VK_EXT_shader_stencil_export)
			{
				VK_EXT_SHADER_STENCIL_EXPORT_EXTENSION_NAME,
				VK_EXT_SHADER_STENCIL_EXPORT_SPEC_VERSION,
				true,
				{
				}
			},
#endif
#if defined(VK_EXT_sample_locations)
			{
				VK_EXT_SAMPLE_LOCATIONS_EXTENSION_NAME,
				VK_EXT_SAMPLE_LOCATIONS_SPEC_VERSION,
				true,
				{
					{"vkCmdSetSampleLocationsEXT", GET_TRAMPOLINE(CommandBuffer, SetSampleLocations), false},
					{"vkGetPhysicalDeviceMultisamplePropertiesEXT", GET_TRAMPOLINE(PhysicalDevice, GetMultisampleProperties), false},
				}
			},
#endif
#if defined(VK_EXT_blend_operation_advanced)
			{
				VK_EXT_BLEND_OPERATION_ADVANCED_EXTENSION_NAME,
				VK_EXT_BLEND_OPERATION_ADVANCED_SPEC_VERSION,
				true,
				{
				}
			},
#endif
#if defined(VK_NV_fragment_coverage_to_color)
			{
				VK_NV_FRAGMENT_COVERAGE_TO_COLOR_EXTENSION_NAME,
				VK_NV_FRAGMENT_COVERAGE_TO_COLOR_SPEC_VERSION,
				true,
				{
				}
			},
#endif
#if defined(VK_NV_framebuffer_mixed_samples)
			{
				VK_NV_FRAMEBUFFER_MIXED_SAMPLES_EXTENSION_NAME,
				VK_NV_FRAMEBUFFER_MIXED_SAMPLES_SPEC_VERSION,
				true,
				{
				}
			},
#endif
#if defined(VK_NV_fill_rectangle)
			{
				VK_NV_FILL_RECTANGLE_EXTENSION_NAME,
				VK_NV_FILL_RECTANGLE_SPEC_VERSION,
				true,
				{
				}
			},
#endif
#if defined(VK_NV_shader_sm_builtins)
			{
				VK_NV_SHADER_SM_BUILTINS_EXTENSION_NAME,
				VK_NV_SHADER_SM_BUILTINS_SPEC_VERSION,
				true,
				{
				}
			},
#endif
#if defined(VK_EXT_post_depth_coverage)
			{
				VK_EXT_POST_DEPTH_COVERAGE_EXTENSION_NAME,
				VK_EXT_POST_DEPTH_COVERAGE_SPEC_VERSION,
				true,
				{
				}
			},
#endif
#if defined(VK_EXT_image_drm_format_modifier)
			{
				VK_EXT_IMAGE_DRM_FORMAT_MODIFIER_EXTENSION_NAME,
				VK_EXT_IMAGE_DRM_FORMAT_MODIFIER_SPEC_VERSION,
				true,
				{
					{"vkGetImageDrmFormatModifierPropertiesEXT", GET_TRAMPOLINE(Device, GetImageDrmFormatModifierProperties), false},
				}
			},
#endif
#if defined(VK_EXT_validation_cache)
			{
				VK_EXT_VALIDATION_CACHE_EXTENSION_NAME,
				VK_EXT_VALIDATION_CACHE_SPEC_VERSION,
				true,
				{
					{"vkCreateValidationCacheEXT", GET_TRAMPOLINE(Device, CreateValidationCache), false},
					{"vkDestroyValidationCacheEXT", GET_TRAMPOLINE(Device, DestroyValidationCache), false},
					{"vkMergeValidationCachesEXT", GET_TRAMPOLINE(Device, MergeValidationCaches), false},
					{"vkGetValidationCacheDataEXT", GET_TRAMPOLINE(Device, GetValidationCacheData), false},
				}
			},
#endif
#if defined(VK_EXT_descriptor_indexing)
			{
				VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
				VK_EXT_DESCRIPTOR_INDEXING_SPEC_VERSION,
				true,
				{
				}
			},
#endif
#if defined(VK_EXT_shader_viewport_index_layer)
			{
				VK_EXT_SHADER_VIEWPORT_INDEX_LAYER_EXTENSION_NAME,
				VK_EXT_SHADER_VIEWPORT_INDEX_LAYER_SPEC_VERSION,
				true,
				{
				}
			},
#endif
#if defined(VK_NV_shading_rate_image)
			{
				VK_NV_SHADING_RATE_IMAGE_EXTENSION_NAME,
				VK_NV_SHADING_RATE_IMAGE_SPEC_VERSION,
				true,
				{
					{"vkCmdBindShadingRateImageNV", GET_TRAMPOLINE(CommandBuffer, BindShadingRateImage), false},
					{"vkCmdSetViewportShadingRatePaletteNV", GET_TRAMPOLINE(CommandBuffer, SetViewportShadingRatePalette), false},
					{"vkCmdSetCoarseSampleOrderNV", GET_TRAMPOLINE(CommandBuffer, SetCoarseSampleOrder), false},
				}
			},
#endif
#if defined(VK_NV_ray_tracing)
			{
				VK_NV_RAY_TRACING_EXTENSION_NAME,
				VK_NV_RAY_TRACING_SPEC_VERSION,
				true,
				{
					{"vkCreateAccelerationStructureNV", GET_TRAMPOLINE(Device, CreateAccelerationStructureNV), false},
					{"vkDestroyAccelerationStructureNV", GET_TRAMPOLINE(Device, DestroyAccelerationStructureNV), false},
					{"vkGetAccelerationStructureMemoryRequirementsNV", GET_TRAMPOLINE(Device, GetAccelerationStructureMemoryRequirementsNV), false},
					{"vkBindAccelerationStructureMemoryNV", GET_TRAMPOLINE(Device, BindAccelerationStructureMemoryNV), false},
					{"vkCmdBuildAccelerationStructureNV", GET_TRAMPOLINE(CommandBuffer, BuildAccelerationStructure), false},
					{"vkCmdCopyAccelerationStructureNV", GET_TRAMPOLINE(CommandBuffer, CopyAccelerationStructure), false},
					{"vkCmdTraceRaysNV", GET_TRAMPOLINE(CommandBuffer, TraceRays), false},
					{"vkCreateRayTracingPipelinesNV", GET_TRAMPOLINE(Device, CreateRayTracingPipelinesNV), false},
					{"vkGetRayTracingShaderGroupHandlesNV", GET_TRAMPOLINE(Device, GetRayTracingShaderGroupHandlesNV), false},
					{"vkGetAccelerationStructureHandleNV", GET_TRAMPOLINE(Device, GetAccelerationStructureHandleNV), false},
					{"vkCmdWriteAccelerationStructuresPropertiesNV", GET_TRAMPOLINE(CommandBuffer, WriteAccelerationStructuresProperties), false},
					{"vkCompileDeferredNV", GET_TRAMPOLINE(Device, CompileDeferredNV), false},
				}
			},
#endif
#if defined(VK_NV_representative_fragment_test)
			{
				VK_NV_REPRESENTATIVE_FRAGMENT_TEST_EXTENSION_NAME,
				VK_NV_REPRESENTATIVE_FRAGMENT_TEST_SPEC_VERSION,
				true,
				{
				}
			},
#endif
#if defined(VK_EXT_filter_cubic)
			{
				VK_EXT_FILTER_CUBIC_EXTENSION_NAME,
				VK_EXT_FILTER_CUBIC_SPEC_VERSION,
				true,
				{
				}
			},
#endif
#if defined(VK_EXT_global_priority)
			{
				VK_EXT_GLOBAL_PRIORITY_EXTENSION_NAME,
				VK_EXT_GLOBAL_PRIORITY_SPEC_VERSION,
				true,
				{
				}
			},
#endif
#if defined(VK_EXT_external_memory_host)
			{
				VK_EXT_EXTERNAL_MEMORY_HOST_EXTENSION_NAME,
				VK_EXT_EXTERNAL_MEMORY_HOST_SPEC_VERSION,
				true,
				{
					{"vkGetMemoryHostPointerPropertiesEXT", GET_TRAMPOLINE(Device, GetMemoryHostPointerProperties), false},
				}
			},
#endif
#if defined(VK_AMD_buffer_marker)
			{
				VK_AMD_BUFFER_MARKER_EXTENSION_NAME,
				VK_AMD_BUFFER_MARKER_SPEC_VERSION,
				true,
				{
					{"vkCmdWriteBufferMarkerAMD", GET_TRAMPOLINE(CommandBuffer, WriteBufferMarker), false},
				}
			},
#endif
#if defined(VK_EXT_calibrated_timestamps)
			{
				VK_EXT_CALIBRATED_TIMESTAMPS_EXTENSION_NAME,
				VK_EXT_CALIBRATED_TIMESTAMPS_SPEC_VERSION,
				true,
				{
					{"vkGetPhysicalDeviceCalibrateableTimeDomainsEXT", GET_TRAMPOLINE(PhysicalDevice, GetCalibrateableTimeDomains), false},
					{"vkGetCalibratedTimestampsEXT", GET_TRAMPOLINE(Device, GetCalibratedTimestamps), false},
				}
			},
#endif
#if defined(VK_AMD_shader_core_properties)
			{
				VK_AMD_SHADER_CORE_PROPERTIES_EXTENSION_NAME,
				VK_AMD_SHADER_CORE_PROPERTIES_SPEC_VERSION,
				true,
				{
				}
			},
#endif
#if defined(VK_AMD_memory_overallocation_behavior)
			{
				VK_AMD_MEMORY_OVERALLOCATION_BEHAVIOR_EXTENSION_NAME,
				VK_AMD_MEMORY_OVERALLOCATION_BEHAVIOR_SPEC_VERSION,
				true,
				{
				}
			},
#endif
#if defined(VK_EXT_vertex_attribute_divisor)
			{
				VK_EXT_VERTEX_ATTRIBUTE_DIVISOR_EXTENSION_NAME,
				VK_EXT_VERTEX_ATTRIBUTE_DIVISOR_SPEC_VERSION,
				true,
				{
				}
			},
#endif
#if defined(VK_EXT_pipeline_creation_feedback)
			{
				VK_EXT_PIPELINE_CREATION_FEEDBACK_EXTENSION_NAME,
				VK_EXT_PIPELINE_CREATION_FEEDBACK_SPEC_VERSION,
				true,
				{
				}
			},
#endif
#if defined(VK_NV_shader_subgroup_partitioned)
			{
				VK_NV_SHADER_SUBGROUP_PARTITIONED_EXTENSION_NAME,
				VK_NV_SHADER_SUBGROUP_PARTITIONED_SPEC_VERSION,
				true,
				{
				}
			},
#endif
#if defined(VK_NV_compute_shader_derivatives)
			{
				VK_NV_COMPUTE_SHADER_DERIVATIVES_EXTENSION_NAME,
				VK_NV_COMPUTE_SHADER_DERIVATIVES_SPEC_VERSION,
				true,
				{
				}
			},
#endif
#if defined(VK_NV_mesh_shader)
			{
				VK_NV_MESH_SHADER_EXTENSION_NAME,
				VK_NV_MESH_SHADER_SPEC_VERSION,
				true,
				{
					{"vkCmdDrawMeshTasksNV", GET_TRAMPOLINE(CommandBuffer, DrawMeshTasks), false},
					{"vkCmdDrawMeshTasksIndirectNV", GET_TRAMPOLINE(CommandBuffer, DrawMeshTasksIndirect), false},
					{"vkCmdDrawMeshTasksIndirectCountNV", GET_TRAMPOLINE(CommandBuffer, DrawMeshTasksIndirectCount), false},
				}
			},
#endif
#if defined(VK_NV_fragment_shader_barycentric)
			{
				VK_NV_FRAGMENT_SHADER_BARYCENTRIC_EXTENSION_NAME,
				VK_NV_FRAGMENT_SHADER_BARYCENTRIC_SPEC_VERSION,
				true,
				{
				}
			},
#endif
#if defined(VK_NV_shader_image_footprint)
			{
				VK_NV_SHADER_IMAGE_FOOTPRINT_EXTENSION_NAME,
				VK_NV_SHADER_IMAGE_FOOTPRINT_SPEC_VERSION,
				true,
				{
				}
			},
#endif
#if defined(VK_NV_scissor_exclusive)
			{
				VK_NV_SCISSOR_EXCLUSIVE_EXTENSION_NAME,
				VK_NV_SCISSOR_EXCLUSIVE_SPEC_VERSION,
				true,
				{
					{"vkCmdSetExclusiveScissorNV", GET_TRAMPOLINE(CommandBuffer, SetExclusiveScissor), false},
				}
			},
#endif
#if defined(VK_NV_device_diagnostic_checkpoints)
			{
				VK_NV_DEVICE_DIAGNOSTIC_CHECKPOINTS_EXTENSION_NAME,
				VK_NV_DEVICE_DIAGNOSTIC_CHECKPOINTS_SPEC_VERSION,
				true,
				{
					{"vkCmdSetCheckpointNV", GET_TRAMPOLINE(CommandBuffer, SetCheckpoint), false},
					{"vkGetQueueCheckpointDataNV", GET_TRAMPOLINE(Queue, GetCheckpointData), false},
				}
			},
#endif
#if defined(VK_INTEL_shader_integer_functions2)
			{
				VK_INTEL_SHADER_INTEGER_FUNCTIONS_2_EXTENSION_NAME,
				VK_INTEL_SHADER_INTEGER_FUNCTIONS_2_SPEC_VERSION,
				true,
				{
				}
			},
#endif
#if defined(VK_INTEL_performance_query)
			{
				VK_INTEL_PERFORMANCE_QUERY_EXTENSION_NAME,
				VK_INTEL_PERFORMANCE_QUERY_SPEC_VERSION,
				true,
				{
					{"vkInitializePerformanceApiINTEL", GET_TRAMPOLINE(Device, InitializePerformanceApiINTEL), false},
					{"vkUninitializePerformanceApiINTEL", GET_TRAMPOLINE(Device, UninitializePerformanceApiINTEL), false},
					{"vkCmdSetPerformanceMarkerINTEL", GET_TRAMPOLINE(CommandBuffer, SetPerformanceMarker), false},
					{"vkCmdSetPerformanceStreamMarkerINTEL", GET_TRAMPOLINE(CommandBuffer, SetPerformanceStreamMarker), false},
					{"vkCmdSetPerformanceOverrideINTEL", GET_TRAMPOLINE(CommandBuffer, SetPerformanceOverride), false},
					{"vkAcquirePerformanceConfigurationINTEL", GET_TRAMPOLINE(Device, AcquirePerformanceConfigurationINTEL), false},
					{"vkReleasePerformanceConfigurationINTEL", GET_TRAMPOLINE(Device, ReleasePerformanceConfigurationINTEL), false},
					{"vkQueueSetPerformanceConfigurationINTEL", GET_TRAMPOLINE(Queue, SetPerformanceConfiguration), false},
					{"vkGetPerformanceParameterINTEL", GET_TRAMPOLINE(Device, GetPerformanceParameterINTEL), false},
				}
			},
#endif
#if defined(VK_EXT_pci_bus_info)
			{
				VK_EXT_PCI_BUS_INFO_EXTENSION_NAME,
				VK_EXT_PCI_BUS_INFO_SPEC_VERSION,
				true,
				{
				}
			},
#endif
#if defined(VK_AMD_display_native_hdr)
			{
				VK_AMD_DISPLAY_NATIVE_HDR_EXTENSION_NAME,
				VK_AMD_DISPLAY_NATIVE_HDR_SPEC_VERSION,
				true,
				{
					{"vkSetLocalDimmingAMD", GET_TRAMPOLINE(Device, SetLocalDimmingAMD), false},
				}
			},
#endif
#if defined(VK_EXT_fragment_density_map)
			{
				VK_EXT_FRAGMENT_DENSITY_MAP_EXTENSION_NAME,
				VK_EXT_FRAGMENT_DENSITY_MAP_SPEC_VERSION,
				true,
				{
				}
			},
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
#if defined(VK_GOOGLE_hlsl_functionality1)
			{
				VK_GOOGLE_HLSL_FUNCTIONALITY1_EXTENSION_NAME,
				VK_GOOGLE_HLSL_FUNCTIONALITY1_SPEC_VERSION,
				true,
				{
				}
			},
#endif
#if defined(VK_GOOGLE_decorate_string)
			{
				VK_GOOGLE_DECORATE_STRING_EXTENSION_NAME,
				VK_GOOGLE_DECORATE_STRING_SPEC_VERSION,
				true,
				{
				}
			},
#endif
#if defined(VK_EXT_subgroup_size_control)
			{
				VK_EXT_SUBGROUP_SIZE_CONTROL_EXTENSION_NAME,
				VK_EXT_SUBGROUP_SIZE_CONTROL_SPEC_VERSION,
				true,
				{
				}
			},
#endif
#if defined(VK_AMD_shader_core_properties2)
			{
				VK_AMD_SHADER_CORE_PROPERTIES_2_EXTENSION_NAME,
				VK_AMD_SHADER_CORE_PROPERTIES_2_SPEC_VERSION,
				true,
				{
				}
			},
#endif
#if defined(VK_AMD_device_coherent_memory)
			{
				VK_AMD_DEVICE_COHERENT_MEMORY_EXTENSION_NAME,
				VK_AMD_DEVICE_COHERENT_MEMORY_SPEC_VERSION,
				true,
				{
				}
			},
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
#if defined(VK_EXT_memory_priority)
			{
				VK_EXT_MEMORY_PRIORITY_EXTENSION_NAME,
				VK_EXT_MEMORY_PRIORITY_SPEC_VERSION,
				true,
				{
				}
			},
#endif
#if defined(VK_NV_dedicated_allocation_image_aliasing)
			{
				VK_NV_DEDICATED_ALLOCATION_IMAGE_ALIASING_EXTENSION_NAME,
				VK_NV_DEDICATED_ALLOCATION_IMAGE_ALIASING_SPEC_VERSION,
				true,
				{
				}
			},
#endif
#if defined(VK_EXT_buffer_device_address)
			{
				VK_EXT_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
				VK_EXT_BUFFER_DEVICE_ADDRESS_SPEC_VERSION,
				true,
				{
					{"vkGetBufferDeviceAddressEXT", GET_TRAMPOLINE(Device, GetBufferDeviceAddress), false},
				}
			},
#endif
#if defined(VK_EXT_separate_stencil_usage)
			{
				VK_EXT_SEPARATE_STENCIL_USAGE_EXTENSION_NAME,
				VK_EXT_SEPARATE_STENCIL_USAGE_SPEC_VERSION,
				true,
				{
				}
			},
#endif
#if defined(VK_EXT_validation_features)
			{
				VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME,
				VK_EXT_VALIDATION_FEATURES_SPEC_VERSION,
				false,
				{
				}
			},
#endif
#if defined(VK_NV_cooperative_matrix)
			{
				VK_NV_COOPERATIVE_MATRIX_EXTENSION_NAME,
				VK_NV_COOPERATIVE_MATRIX_SPEC_VERSION,
				true,
				{
					{"vkGetPhysicalDeviceCooperativeMatrixPropertiesNV", GET_TRAMPOLINE(PhysicalDevice, GetCooperativeMatrixPropertiesNV), false},
				}
			},
#endif
#if defined(VK_NV_coverage_reduction_mode)
			{
				VK_NV_COVERAGE_REDUCTION_MODE_EXTENSION_NAME,
				VK_NV_COVERAGE_REDUCTION_MODE_SPEC_VERSION,
				true,
				{
					{"vkGetPhysicalDeviceSupportedFramebufferMixedSamplesCombinationsNV", GET_TRAMPOLINE(PhysicalDevice, GetSupportedFramebufferMixedSamplesCombinationsNV), false},
				}
			},
#endif
#if defined(VK_EXT_fragment_shader_interlock)
			{
				VK_EXT_FRAGMENT_SHADER_INTERLOCK_EXTENSION_NAME,
				VK_EXT_FRAGMENT_SHADER_INTERLOCK_SPEC_VERSION,
				true,
				{
				}
			},
#endif
#if defined(VK_EXT_ycbcr_image_arrays)
			{
				VK_EXT_YCBCR_IMAGE_ARRAYS_EXTENSION_NAME,
				VK_EXT_YCBCR_IMAGE_ARRAYS_SPEC_VERSION,
				true,
				{
				}
			},
#endif
#if defined(VK_EXT_headless_surface)
			{
				VK_EXT_HEADLESS_SURFACE_EXTENSION_NAME,
				VK_EXT_HEADLESS_SURFACE_SPEC_VERSION,
				false,
				{
					{"vkCreateHeadlessSurfaceEXT", GET_TRAMPOLINE(Instance, CreateHeadlessSurface), false},
				}
			},
#endif
#if defined(VK_EXT_line_rasterization)
			{
				VK_EXT_LINE_RASTERIZATION_EXTENSION_NAME,
				VK_EXT_LINE_RASTERIZATION_SPEC_VERSION,
				true,
				{
					{"vkCmdSetLineStippleEXT", GET_TRAMPOLINE(CommandBuffer, SetLineStipple), false},
				}
			},
#endif
#if defined(VK_EXT_host_query_reset)
			{
				VK_EXT_HOST_QUERY_RESET_EXTENSION_NAME,
				VK_EXT_HOST_QUERY_RESET_SPEC_VERSION,
				true,
				{
					{"vkResetQueryPoolEXT", GET_TRAMPOLINE(Device, ResetQueryPool), false},
				}
			},
#endif
#if defined(VK_EXT_index_type_uint8)
			{
				VK_EXT_INDEX_TYPE_UINT8_EXTENSION_NAME,
				VK_EXT_INDEX_TYPE_UINT8_SPEC_VERSION,
				true,
				{
				}
			},
#endif
#if defined(VK_EXT_shader_demote_to_helper_invocation)
			{
				VK_EXT_SHADER_DEMOTE_TO_HELPER_INVOCATION_EXTENSION_NAME,
				VK_EXT_SHADER_DEMOTE_TO_HELPER_INVOCATION_SPEC_VERSION,
				true,
				{
				}
			},
#endif
#if defined(VK_EXT_texel_buffer_alignment)
			{
				VK_EXT_TEXEL_BUFFER_ALIGNMENT_EXTENSION_NAME,
				VK_EXT_TEXEL_BUFFER_ALIGNMENT_SPEC_VERSION,
				true,
				{
				}
			},
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
#if defined(VK_KHR_external_memory_win32)
			{
				VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME,
				VK_KHR_EXTERNAL_MEMORY_WIN32_SPEC_VERSION,
				true,
				{
					{"vkGetMemoryWin32HandleKHR", GET_TRAMPOLINE(Device, GetMemoryWin32Handle), false},
					{"vkGetMemoryWin32HandlePropertiesKHR", GET_TRAMPOLINE(Device, GetMemoryWin32HandleProperties), false},
				}
			},
#endif
#if defined(VK_KHR_win32_keyed_mutex)
			{
				VK_KHR_WIN32_KEYED_MUTEX_EXTENSION_NAME,
				VK_KHR_WIN32_KEYED_MUTEX_SPEC_VERSION,
				true,
				{
				}
			},
#endif
#if defined(VK_KHR_external_semaphore_win32)
			{
				VK_KHR_EXTERNAL_SEMAPHORE_WIN32_EXTENSION_NAME,
				VK_KHR_EXTERNAL_SEMAPHORE_WIN32_SPEC_VERSION,
				true,
				{
					{"vkImportSemaphoreWin32HandleKHR", GET_TRAMPOLINE(Device, ImportSemaphoreWin32Handle), false},
					{"vkGetSemaphoreWin32HandleKHR", GET_TRAMPOLINE(Device, GetSemaphoreWin32Handle), false},
				}
			},
#endif
#if defined(VK_KHR_external_fence_win32)
			{
				VK_KHR_EXTERNAL_FENCE_WIN32_EXTENSION_NAME,
				VK_KHR_EXTERNAL_FENCE_WIN32_SPEC_VERSION,
				true,
				{
					{"vkImportFenceWin32HandleKHR", GET_TRAMPOLINE(Device, ImportFenceWin32Handle), false},
					{"vkGetFenceWin32HandleKHR", GET_TRAMPOLINE(Device, GetFenceWin32Handle), false},
				}
			},
#endif
#if defined(VK_NV_external_memory_win32)
			{
				VK_NV_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME,
				VK_NV_EXTERNAL_MEMORY_WIN32_SPEC_VERSION,
				true,
				{
					{"vkGetMemoryWin32HandleNV", GET_TRAMPOLINE(Device, GetMemoryWin32HandleNV), false},
				}
			},
#endif
#if defined(VK_NV_win32_keyed_mutex)
			{
				VK_NV_WIN32_KEYED_MUTEX_EXTENSION_NAME,
				VK_NV_WIN32_KEYED_MUTEX_SPEC_VERSION,
				true,
				{
				}
			},
#endif
#if defined(VK_EXT_full_screen_exclusive)
			{
				VK_EXT_FULL_SCREEN_EXCLUSIVE_EXTENSION_NAME,
				VK_EXT_FULL_SCREEN_EXCLUSIVE_SPEC_VERSION,
				true,
				{
					{"vkGetPhysicalDeviceSurfacePresentModes2EXT", GET_TRAMPOLINE(PhysicalDevice, GetSurfacePresentModes2), false},
					{"vkAcquireFullScreenExclusiveModeEXT", GET_TRAMPOLINE(Device, AcquireFullScreenExclusiveMode), false},
					{"vkReleaseFullScreenExclusiveModeEXT", GET_TRAMPOLINE(Device, ReleaseFullScreenExclusiveMode), false},
					{"vkGetDeviceGroupSurfacePresentModes2EXT", GET_TRAMPOLINE(Device, GetDeviceGroupSurfacePresentModes2), false},
				}
			},
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
