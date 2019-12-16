// ReSharper disable once CppMissingIncludeGuard
VULKAN_FUNCTION(DestroyInstance, void, Instance, void*, const VkAllocationCallbacks*)
VULKAN_FUNCTION(EnumeratePhysicalDevices, VkResult, Instance, void*, uint32_t*, VkPhysicalDevice*)
VULKAN_FUNCTION(GetFeatures, void, PhysicalDevice, void*, VkPhysicalDeviceFeatures*)
VULKAN_FUNCTION(GetFormatProperties, void, PhysicalDevice, void*, VkFormat, VkFormatProperties*)
VULKAN_FUNCTION(GetImageFormatProperties, VkResult, PhysicalDevice, void*, VkFormat, VkImageType, VkImageTiling, VkImageUsageFlags, VkImageCreateFlags, VkImageFormatProperties*)
VULKAN_FUNCTION(GetProperties, void, PhysicalDevice, void*, VkPhysicalDeviceProperties*)
VULKAN_FUNCTION(GetQueueFamilyProperties, void, PhysicalDevice, void*, uint32_t*, VkQueueFamilyProperties*)
VULKAN_FUNCTION(GetMemoryProperties, void, PhysicalDevice, void*, VkPhysicalDeviceMemoryProperties*)
VULKAN_FUNCTION(GetProcAddress, PFN_vkVoidFunction, Instance, void*, const char*)
VULKAN_FUNCTION(GetProcAddress, PFN_vkVoidFunction, Device, void*, const char*)
VULKAN_FUNCTION(CreateDevice, VkResult, PhysicalDevice, void*, const VkDeviceCreateInfo*, const VkAllocationCallbacks*, VkDevice*)
VULKAN_FUNCTION(DestroyDevice, void, Device, void*, const VkAllocationCallbacks*)
VULKAN_FUNCTION(EnumerateDeviceExtensionProperties, VkResult, PhysicalDevice, void*, const char*, uint32_t*, VkExtensionProperties*)
VULKAN_FUNCTION(EnumerateDeviceLayerProperties, VkResult, PhysicalDevice, void*, uint32_t*, VkLayerProperties*)
VULKAN_FUNCTION(GetDeviceQueue, void, Device, void*, uint32_t, uint32_t, VkQueue*)
VULKAN_FUNCTION(Submit, VkResult, Queue, void*, uint32_t, const VkSubmitInfo*, VkFence)
VULKAN_FUNCTION(WaitIdle, VkResult, Queue, void*)
VULKAN_FUNCTION(WaitIdle, VkResult, Device, void*)
VULKAN_FUNCTION(AllocateMemory, VkResult, Device, void*, const VkMemoryAllocateInfo*, const VkAllocationCallbacks*, VkDeviceMemory*)
VULKAN_FUNCTION(FreeMemory, void, Device, void*, VkDeviceMemory, const VkAllocationCallbacks*)
VULKAN_FUNCTION(MapMemory, VkResult, Device, void*, VkDeviceMemory, VkDeviceSize, VkDeviceSize, VkMemoryMapFlags, void**)
VULKAN_FUNCTION(UnmapMemory, void, Device, void*, VkDeviceMemory)
VULKAN_FUNCTION(FlushMappedMemoryRanges, VkResult, Device, void*, uint32_t, const VkMappedMemoryRange*)
VULKAN_FUNCTION(InvalidateMappedMemoryRanges, VkResult, Device, void*, uint32_t, const VkMappedMemoryRange*)
VULKAN_FUNCTION(GetDeviceMemoryCommitment, void, Device, void*, VkDeviceMemory, VkDeviceSize*)
VULKAN_FUNCTION(BindBufferMemory, VkResult, Device, void*, VkBuffer, VkDeviceMemory, VkDeviceSize)
VULKAN_FUNCTION(BindImageMemory, VkResult, Device, void*, VkImage, VkDeviceMemory, VkDeviceSize)
VULKAN_FUNCTION(GetBufferMemoryRequirements, void, Device, void*, VkBuffer, VkMemoryRequirements*)
VULKAN_FUNCTION(GetImageMemoryRequirements, void, Device, void*, VkImage, VkMemoryRequirements*)
VULKAN_FUNCTION(GetImageSparseMemoryRequirements, void, Device, void*, VkImage, uint32_t*, VkSparseImageMemoryRequirements*)
VULKAN_FUNCTION(GetSparseImageFormatProperties, void, PhysicalDevice, void*, VkFormat, VkImageType, VkSampleCountFlagBits, VkImageUsageFlags, VkImageTiling, uint32_t*, VkSparseImageFormatProperties*)
VULKAN_FUNCTION(BindSparse, VkResult, Queue, void*, uint32_t, const VkBindSparseInfo*, VkFence)
VULKAN_FUNCTION(CreateFence, VkResult, Device, void*, const VkFenceCreateInfo*, const VkAllocationCallbacks*, VkFence*)
VULKAN_FUNCTION(DestroyFence, void, Device, void*, VkFence, const VkAllocationCallbacks*)
VULKAN_FUNCTION(ResetFences, VkResult, Device, void*, uint32_t, const VkFence*)
VULKAN_FUNCTION(GetFenceStatus, VkResult, Device, void*, VkFence)
VULKAN_FUNCTION(WaitForFences, VkResult, Device, void*, uint32_t, const VkFence*, VkBool32, uint64_t)
VULKAN_FUNCTION(CreateSemaphore, VkResult, Device, void*, const VkSemaphoreCreateInfo*, const VkAllocationCallbacks*, VkSemaphore*)
VULKAN_FUNCTION(DestroySemaphore, void, Device, void*, VkSemaphore, const VkAllocationCallbacks*)
VULKAN_FUNCTION(CreateEvent, VkResult, Device, void*, const VkEventCreateInfo*, const VkAllocationCallbacks*, VkEvent*)
VULKAN_FUNCTION(DestroyEvent, void, Device, void*, VkEvent, const VkAllocationCallbacks*)
VULKAN_FUNCTION(GetEventStatus, VkResult, Device, void*, VkEvent)
VULKAN_FUNCTION(SetEvent, VkResult, Device, void*, VkEvent)
VULKAN_FUNCTION(ResetEvent, VkResult, Device, void*, VkEvent)
VULKAN_FUNCTION(CreateQueryPool, VkResult, Device, void*, const VkQueryPoolCreateInfo*, const VkAllocationCallbacks*, VkQueryPool*)
VULKAN_FUNCTION(DestroyQueryPool, void, Device, void*, VkQueryPool, const VkAllocationCallbacks*)
VULKAN_FUNCTION(GetQueryPoolResults, VkResult, Device, void*, VkQueryPool, uint32_t, uint32_t, size_t, void*, VkDeviceSize, VkQueryResultFlags)
VULKAN_FUNCTION(CreateBuffer, VkResult, Device, void*, const VkBufferCreateInfo*, const VkAllocationCallbacks*, VkBuffer*)
VULKAN_FUNCTION(DestroyBuffer, void, Device, void*, VkBuffer, const VkAllocationCallbacks*)
VULKAN_FUNCTION(CreateBufferView, VkResult, Device, void*, const VkBufferViewCreateInfo*, const VkAllocationCallbacks*, VkBufferView*)
VULKAN_FUNCTION(DestroyBufferView, void, Device, void*, VkBufferView, const VkAllocationCallbacks*)
VULKAN_FUNCTION(CreateImage, VkResult, Device, void*, const VkImageCreateInfo*, const VkAllocationCallbacks*, VkImage*)
VULKAN_FUNCTION(DestroyImage, void, Device, void*, VkImage, const VkAllocationCallbacks*)
VULKAN_FUNCTION(GetImageSubresourceLayout, void, Device, void*, VkImage, const VkImageSubresource*, VkSubresourceLayout*)
VULKAN_FUNCTION(CreateImageView, VkResult, Device, void*, const VkImageViewCreateInfo*, const VkAllocationCallbacks*, VkImageView*)
VULKAN_FUNCTION(DestroyImageView, void, Device, void*, VkImageView, const VkAllocationCallbacks*)
VULKAN_FUNCTION(CreateShaderModule, VkResult, Device, void*, const VkShaderModuleCreateInfo*, const VkAllocationCallbacks*, VkShaderModule*)
VULKAN_FUNCTION(DestroyShaderModule, void, Device, void*, VkShaderModule, const VkAllocationCallbacks*)
VULKAN_FUNCTION(CreatePipelineCache, VkResult, Device, void*, const VkPipelineCacheCreateInfo*, const VkAllocationCallbacks*, VkPipelineCache*)
VULKAN_FUNCTION(DestroyPipelineCache, void, Device, void*, VkPipelineCache, const VkAllocationCallbacks*)
VULKAN_FUNCTION(GetPipelineCacheData, VkResult, Device, void*, VkPipelineCache, size_t*, void*)
VULKAN_FUNCTION(MergePipelineCaches, VkResult, Device, void*, VkPipelineCache, uint32_t, const VkPipelineCache*)
VULKAN_FUNCTION(CreateGraphicsPipelines, VkResult, Device, void*, VkPipelineCache, uint32_t, const VkGraphicsPipelineCreateInfo*, const VkAllocationCallbacks*, VkPipeline*)
VULKAN_FUNCTION(CreateComputePipelines, VkResult, Device, void*, VkPipelineCache, uint32_t, const VkComputePipelineCreateInfo*, const VkAllocationCallbacks*, VkPipeline*)
VULKAN_FUNCTION(DestroyPipeline, void, Device, void*, VkPipeline, const VkAllocationCallbacks*)
VULKAN_FUNCTION(CreatePipelineLayout, VkResult, Device, void*, const VkPipelineLayoutCreateInfo*, const VkAllocationCallbacks*, VkPipelineLayout*)
VULKAN_FUNCTION(DestroyPipelineLayout, void, Device, void*, VkPipelineLayout, const VkAllocationCallbacks*)
VULKAN_FUNCTION(CreateSampler, VkResult, Device, void*, const VkSamplerCreateInfo*, const VkAllocationCallbacks*, VkSampler*)
VULKAN_FUNCTION(DestroySampler, void, Device, void*, VkSampler, const VkAllocationCallbacks*)
VULKAN_FUNCTION(CreateDescriptorSetLayout, VkResult, Device, void*, const VkDescriptorSetLayoutCreateInfo*, const VkAllocationCallbacks*, VkDescriptorSetLayout*)
VULKAN_FUNCTION(DestroyDescriptorSetLayout, void, Device, void*, VkDescriptorSetLayout, const VkAllocationCallbacks*)
VULKAN_FUNCTION(CreateDescriptorPool, VkResult, Device, void*, const VkDescriptorPoolCreateInfo*, const VkAllocationCallbacks*, VkDescriptorPool*)
VULKAN_FUNCTION(DestroyDescriptorPool, void, Device, void*, VkDescriptorPool, const VkAllocationCallbacks*)
VULKAN_FUNCTION(ResetDescriptorPool, VkResult, Device, void*, VkDescriptorPool, VkDescriptorPoolResetFlags)
VULKAN_FUNCTION(AllocateDescriptorSets, VkResult, Device, void*, const VkDescriptorSetAllocateInfo*, VkDescriptorSet*)
VULKAN_FUNCTION(FreeDescriptorSets, VkResult, Device, void*, VkDescriptorPool, uint32_t, const VkDescriptorSet*)
VULKAN_FUNCTION(UpdateDescriptorSets, void, Device, void*, uint32_t, const VkWriteDescriptorSet*, uint32_t, const VkCopyDescriptorSet*)
VULKAN_FUNCTION(CreateFramebuffer, VkResult, Device, void*, const VkFramebufferCreateInfo*, const VkAllocationCallbacks*, VkFramebuffer*)
VULKAN_FUNCTION(DestroyFramebuffer, void, Device, void*, VkFramebuffer, const VkAllocationCallbacks*)
VULKAN_FUNCTION(CreateRenderPass, VkResult, Device, void*, const VkRenderPassCreateInfo*, const VkAllocationCallbacks*, VkRenderPass*)
VULKAN_FUNCTION(DestroyRenderPass, void, Device, void*, VkRenderPass, const VkAllocationCallbacks*)
VULKAN_FUNCTION(GetRenderAreaGranularity, void, Device, void*, VkRenderPass, VkExtent2D*)
VULKAN_FUNCTION(CreateCommandPool, VkResult, Device, void*, const VkCommandPoolCreateInfo*, const VkAllocationCallbacks*, VkCommandPool*)
VULKAN_FUNCTION(DestroyCommandPool, void, Device, void*, VkCommandPool, const VkAllocationCallbacks*)
VULKAN_FUNCTION(ResetCommandPool, VkResult, Device, void*, VkCommandPool, VkCommandPoolResetFlags)
VULKAN_FUNCTION(AllocateCommandBuffers, VkResult, Device, void*, const VkCommandBufferAllocateInfo*, VkCommandBuffer*)
VULKAN_FUNCTION(FreeCommandBuffers, void, Device, void*, VkCommandPool, uint32_t, const VkCommandBuffer*)
VULKAN_FUNCTION(Begin, VkResult, CommandBuffer, void*, const VkCommandBufferBeginInfo*)
VULKAN_FUNCTION(End, VkResult, CommandBuffer, void*)
VULKAN_FUNCTION(Reset, VkResult, CommandBuffer, void*, VkCommandBufferResetFlags)
VULKAN_FUNCTION(BindPipeline, void, CommandBuffer, void*, VkPipelineBindPoint, VkPipeline)
VULKAN_FUNCTION(SetViewport, void, CommandBuffer, void*, uint32_t, uint32_t, const VkViewport*)
VULKAN_FUNCTION(SetScissor, void, CommandBuffer, void*, uint32_t, uint32_t, const VkRect2D*)
VULKAN_FUNCTION(SetLineWidth, void, CommandBuffer, void*, float)
VULKAN_FUNCTION(SetDepthBias, void, CommandBuffer, void*, float, float, float)
VULKAN_FUNCTION(SetBlendConstants, void, CommandBuffer, void*, const float*)
VULKAN_FUNCTION(SetDepthBounds, void, CommandBuffer, void*, float, float)
VULKAN_FUNCTION(SetStencilCompareMask, void, CommandBuffer, void*, VkStencilFaceFlags, uint32_t)
VULKAN_FUNCTION(SetStencilWriteMask, void, CommandBuffer, void*, VkStencilFaceFlags, uint32_t)
VULKAN_FUNCTION(SetStencilReference, void, CommandBuffer, void*, VkStencilFaceFlags, uint32_t)
VULKAN_FUNCTION(BindDescriptorSets, void, CommandBuffer, void*, VkPipelineBindPoint, VkPipelineLayout, uint32_t, uint32_t, const VkDescriptorSet*, uint32_t, const uint32_t*)
VULKAN_FUNCTION(BindIndexBuffer, void, CommandBuffer, void*, VkBuffer, VkDeviceSize, VkIndexType)
VULKAN_FUNCTION(BindVertexBuffers, void, CommandBuffer, void*, uint32_t, uint32_t, const VkBuffer*, const VkDeviceSize*)
VULKAN_FUNCTION(Draw, void, CommandBuffer, void*, uint32_t, uint32_t, uint32_t, uint32_t)
VULKAN_FUNCTION(DrawIndexed, void, CommandBuffer, void*, uint32_t, uint32_t, uint32_t, int32_t, uint32_t)
VULKAN_FUNCTION(DrawIndirect, void, CommandBuffer, void*, VkBuffer, VkDeviceSize, uint32_t, uint32_t)
VULKAN_FUNCTION(DrawIndexedIndirect, void, CommandBuffer, void*, VkBuffer, VkDeviceSize, uint32_t, uint32_t)
VULKAN_FUNCTION(Dispatch, void, CommandBuffer, void*, uint32_t, uint32_t, uint32_t)
VULKAN_FUNCTION(DispatchIndirect, void, CommandBuffer, void*, VkBuffer, VkDeviceSize)
VULKAN_FUNCTION(CopyBuffer, void, CommandBuffer, void*, VkBuffer, VkBuffer, uint32_t, const VkBufferCopy*)
VULKAN_FUNCTION(CopyImage, void, CommandBuffer, void*, VkImage, VkImageLayout, VkImage, VkImageLayout, uint32_t, const VkImageCopy*)
VULKAN_FUNCTION(BlitImage, void, CommandBuffer, void*, VkImage, VkImageLayout, VkImage, VkImageLayout, uint32_t, const VkImageBlit*, VkFilter)
VULKAN_FUNCTION(CopyBufferToImage, void, CommandBuffer, void*, VkBuffer, VkImage, VkImageLayout, uint32_t, const VkBufferImageCopy*)
VULKAN_FUNCTION(CopyImageToBuffer, void, CommandBuffer, void*, VkImage, VkImageLayout, VkBuffer, uint32_t, const VkBufferImageCopy*)
VULKAN_FUNCTION(UpdateBuffer, void, CommandBuffer, void*, VkBuffer, VkDeviceSize, VkDeviceSize, const void*)
VULKAN_FUNCTION(FillBuffer, void, CommandBuffer, void*, VkBuffer, VkDeviceSize, VkDeviceSize, uint32_t)
VULKAN_FUNCTION(ClearColorImage, void, CommandBuffer, void*, VkImage, VkImageLayout, const VkClearColorValue*, uint32_t, const VkImageSubresourceRange*)
VULKAN_FUNCTION(ClearDepthStencilImage, void, CommandBuffer, void*, VkImage, VkImageLayout, const VkClearDepthStencilValue*, uint32_t, const VkImageSubresourceRange*)
VULKAN_FUNCTION(ClearAttachments, void, CommandBuffer, void*, uint32_t, const VkClearAttachment*, uint32_t, const VkClearRect*)
VULKAN_FUNCTION(ResolveImage, void, CommandBuffer, void*, VkImage, VkImageLayout, VkImage, VkImageLayout, uint32_t, const VkImageResolve*)
VULKAN_FUNCTION(SetEvent, void, CommandBuffer, void*, VkEvent, VkPipelineStageFlags)
VULKAN_FUNCTION(ResetEvent, void, CommandBuffer, void*, VkEvent, VkPipelineStageFlags)
VULKAN_FUNCTION(WaitEvents, void, CommandBuffer, void*, uint32_t, const VkEvent*, VkPipelineStageFlags, VkPipelineStageFlags, uint32_t, const VkMemoryBarrier*, uint32_t, const VkBufferMemoryBarrier*, uint32_t, const VkImageMemoryBarrier*)
VULKAN_FUNCTION(PipelineBarrier, void, CommandBuffer, void*, VkPipelineStageFlags, VkPipelineStageFlags, VkDependencyFlags, uint32_t, const VkMemoryBarrier*, uint32_t, const VkBufferMemoryBarrier*, uint32_t, const VkImageMemoryBarrier*)
VULKAN_FUNCTION(BeginQuery, void, CommandBuffer, void*, VkQueryPool, uint32_t, VkQueryControlFlags)
VULKAN_FUNCTION(EndQuery, void, CommandBuffer, void*, VkQueryPool, uint32_t)
VULKAN_FUNCTION(ResetQueryPool, void, CommandBuffer, void*, VkQueryPool, uint32_t, uint32_t)
VULKAN_FUNCTION(WriteTimestamp, void, CommandBuffer, void*, VkPipelineStageFlagBits, VkQueryPool, uint32_t)
VULKAN_FUNCTION(CopyQueryPoolResults, void, CommandBuffer, void*, VkQueryPool, uint32_t, uint32_t, VkBuffer, VkDeviceSize, VkDeviceSize, VkQueryResultFlags)
VULKAN_FUNCTION(PushConstants, void, CommandBuffer, void*, VkPipelineLayout, VkShaderStageFlags, uint32_t, uint32_t, const void*)
VULKAN_FUNCTION(BeginRenderPass, void, CommandBuffer, void*, const VkRenderPassBeginInfo*, VkSubpassContents)
VULKAN_FUNCTION(NextSubpass, void, CommandBuffer, void*, VkSubpassContents)
VULKAN_FUNCTION(EndRenderPass, void, CommandBuffer, void*)
VULKAN_FUNCTION(ExecuteCommands, void, CommandBuffer, void*, uint32_t, const VkCommandBuffer*)
VULKAN_FUNCTION(BindBufferMemory2, VkResult, Device, void*, uint32_t, const VkBindBufferMemoryInfo*)
VULKAN_FUNCTION(BindImageMemory2, VkResult, Device, void*, uint32_t, const VkBindImageMemoryInfo*)
VULKAN_FUNCTION(GetDeviceGroupPeerMemoryFeatures, void, Device, void*, uint32_t, uint32_t, uint32_t, VkPeerMemoryFeatureFlags*)
VULKAN_FUNCTION(SetDeviceMask, void, CommandBuffer, void*, uint32_t)
VULKAN_FUNCTION(DispatchBase, void, CommandBuffer, void*, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t)
VULKAN_FUNCTION(EnumeratePhysicalDeviceGroups, VkResult, Instance, void*, uint32_t*, VkPhysicalDeviceGroupProperties*)
VULKAN_FUNCTION(GetImageMemoryRequirements2, void, Device, void*, const VkImageMemoryRequirementsInfo2*, VkMemoryRequirements2*)
VULKAN_FUNCTION(GetBufferMemoryRequirements2, void, Device, void*, const VkBufferMemoryRequirementsInfo2*, VkMemoryRequirements2*)
VULKAN_FUNCTION(GetImageSparseMemoryRequirements2, void, Device, void*, const VkImageSparseMemoryRequirementsInfo2*, uint32_t*, VkSparseImageMemoryRequirements2*)
VULKAN_FUNCTION(GetSparseImageFormatProperties2, void, PhysicalDevice, void*, const VkPhysicalDeviceSparseImageFormatInfo2*, uint32_t*, VkSparseImageFormatProperties2*)
VULKAN_FUNCTION(TrimCommandPool, void, Device, void*, VkCommandPool, VkCommandPoolTrimFlags)
VULKAN_FUNCTION(GetDeviceQueue2, void, Device, void*, const VkDeviceQueueInfo2*, VkQueue*)
VULKAN_FUNCTION(CreateSamplerYcbcrConversion, VkResult, Device, void*, const VkSamplerYcbcrConversionCreateInfo*, const VkAllocationCallbacks*, VkSamplerYcbcrConversion*)
VULKAN_FUNCTION(DestroySamplerYcbcrConversion, void, Device, void*, VkSamplerYcbcrConversion, const VkAllocationCallbacks*)
VULKAN_FUNCTION(CreateDescriptorUpdateTemplate, VkResult, Device, void*, const VkDescriptorUpdateTemplateCreateInfo*, const VkAllocationCallbacks*, VkDescriptorUpdateTemplate*)
VULKAN_FUNCTION(DestroyDescriptorUpdateTemplate, void, Device, void*, VkDescriptorUpdateTemplate, const VkAllocationCallbacks*)
VULKAN_FUNCTION(UpdateDescriptorSetWithTemplate, void, Device, void*, VkDescriptorSet, VkDescriptorUpdateTemplate, const void*)
VULKAN_FUNCTION(GetExternalBufferProperties, void, PhysicalDevice, void*, const VkPhysicalDeviceExternalBufferInfo*, VkExternalBufferProperties*)
VULKAN_FUNCTION(GetExternalFenceProperties, void, PhysicalDevice, void*, const VkPhysicalDeviceExternalFenceInfo*, VkExternalFenceProperties*)
VULKAN_FUNCTION(GetExternalSemaphoreProperties, void, PhysicalDevice, void*, const VkPhysicalDeviceExternalSemaphoreInfo*, VkExternalSemaphoreProperties*)
VULKAN_FUNCTION(GetDescriptorSetLayoutSupport, void, Device, void*, const VkDescriptorSetLayoutCreateInfo*, VkDescriptorSetLayoutSupport*)
VULKAN_FUNCTION(DestroySurface, void, Instance, void*, VkSurfaceKHR, const VkAllocationCallbacks*)
VULKAN_FUNCTION(GetSurfaceSupport, VkResult, PhysicalDevice, void*, uint32_t, VkSurfaceKHR, VkBool32*)
VULKAN_FUNCTION(GetSurfaceCapabilities, VkResult, PhysicalDevice, void*, VkSurfaceKHR, VkSurfaceCapabilitiesKHR*)
VULKAN_FUNCTION(GetSurfaceFormats, VkResult, PhysicalDevice, void*, VkSurfaceKHR, uint32_t*, VkSurfaceFormatKHR*)
VULKAN_FUNCTION(GetSurfacePresentModes, VkResult, PhysicalDevice, void*, VkSurfaceKHR, uint32_t*, VkPresentModeKHR*)
VULKAN_FUNCTION(CreateSwapchain, VkResult, Device, void*, const VkSwapchainCreateInfoKHR*, const VkAllocationCallbacks*, VkSwapchainKHR*)
VULKAN_FUNCTION(DestroySwapchain, void, Device, void*, VkSwapchainKHR, const VkAllocationCallbacks*)
VULKAN_FUNCTION(GetSwapchainImages, VkResult, Device, void*, VkSwapchainKHR, uint32_t*, VkImage*)
VULKAN_FUNCTION(AcquireNextImage, VkResult, Device, void*, VkSwapchainKHR, uint64_t, VkSemaphore, VkFence, uint32_t*)
VULKAN_FUNCTION(Present, VkResult, Queue, void*, const VkPresentInfoKHR*)
VULKAN_FUNCTION(GetDeviceGroupPresentCapabilities, VkResult, Device, void*, VkDeviceGroupPresentCapabilitiesKHR*)
VULKAN_FUNCTION(GetDeviceGroupSurfacePresentModes, VkResult, Device, void*, VkSurfaceKHR, VkDeviceGroupPresentModeFlagsKHR*)
VULKAN_FUNCTION(GetPresentRectangles, VkResult, PhysicalDevice, void*, VkSurfaceKHR, uint32_t*, VkRect2D*)
VULKAN_FUNCTION(AcquireNextImage2, VkResult, Device, void*, const VkAcquireNextImageInfoKHR*, uint32_t*)

#if defined(VK_KHR_display)
VULKAN_FUNCTION(GetDisplayProperties, VkResult, PhysicalDevice, void*, uint32_t*, VkDisplayPropertiesKHR*)
VULKAN_FUNCTION(GetDisplayPlaneProperties, VkResult, PhysicalDevice, void*, uint32_t*, VkDisplayPlanePropertiesKHR*)
VULKAN_FUNCTION(GetDisplayPlaneSupportedDisplays, VkResult, PhysicalDevice, void*, uint32_t, uint32_t*, VkDisplayKHR*)
VULKAN_FUNCTION(GetDisplayModeProperties, VkResult, PhysicalDevice, void*, VkDisplayKHR, uint32_t*, VkDisplayModePropertiesKHR*)
VULKAN_FUNCTION(CreateDisplayMode, VkResult, PhysicalDevice, void*, VkDisplayKHR, const VkDisplayModeCreateInfoKHR*, const VkAllocationCallbacks*, VkDisplayModeKHR*)
VULKAN_FUNCTION(GetDisplayPlaneCapabilities, VkResult, PhysicalDevice, void*, VkDisplayModeKHR, uint32_t, VkDisplayPlaneCapabilitiesKHR*)
#endif

VULKAN_FUNCTION(CreateDisplayPlaneSurface, VkResult, Instance, void*, const VkDisplaySurfaceCreateInfoKHR*, const VkAllocationCallbacks*, VkSurfaceKHR*)
VULKAN_FUNCTION(CreateSharedSwapchains, VkResult, Device, void*, uint32_t, const VkSwapchainCreateInfoKHR*, const VkAllocationCallbacks*, VkSwapchainKHR*)
VULKAN_FUNCTION(GetFeatures2, void, PhysicalDevice, void*, VkPhysicalDeviceFeatures2*)
VULKAN_FUNCTION(GetProperties2, void, PhysicalDevice, void*, VkPhysicalDeviceProperties2*)
VULKAN_FUNCTION(GetFormatProperties2, void, PhysicalDevice, void*, VkFormat, VkFormatProperties2*)
VULKAN_FUNCTION(GetImageFormatProperties2, VkResult, PhysicalDevice, void*, const VkPhysicalDeviceImageFormatInfo2*, VkImageFormatProperties2*)
VULKAN_FUNCTION(GetQueueFamilyProperties2, void, PhysicalDevice, void*, uint32_t*, VkQueueFamilyProperties2*)
VULKAN_FUNCTION(GetMemoryProperties2, void, PhysicalDevice, void*, VkPhysicalDeviceMemoryProperties2*)
VULKAN_FUNCTION(GetMemoryFd, VkResult, Device, void*, const VkMemoryGetFdInfoKHR*, int*)
VULKAN_FUNCTION(GetMemoryFdProperties, VkResult, Device, void*, VkExternalMemoryHandleTypeFlagBits, int, VkMemoryFdPropertiesKHR*)

#if defined(VK_KHR_external_semaphore_fd)
VULKAN_FUNCTION(ImportSemaphoreFd, VkResult, Device, void*, const VkImportSemaphoreFdInfoKHR*)
VULKAN_FUNCTION(GetSemaphoreFd, VkResult, Device, void*, const VkSemaphoreGetFdInfoKHR*, int*)
#endif

#if defined(VK_KHR_push_descriptor)
VULKAN_FUNCTION(PushDescriptorSet, void, CommandBuffer, void*, VkPipelineBindPoint, VkPipelineLayout, uint32_t, uint32_t, const VkWriteDescriptorSet*)
VULKAN_FUNCTION(PushDescriptorSetWithTemplate, void, CommandBuffer, void*, VkDescriptorUpdateTemplate, VkPipelineLayout, uint32_t, const void*)
#endif

VULKAN_FUNCTION(CreateRenderPass2, VkResult, Device, void*, const VkRenderPassCreateInfo2KHR*, const VkAllocationCallbacks*, VkRenderPass*)
VULKAN_FUNCTION(BeginRenderPass2, void, CommandBuffer, void*, const VkRenderPassBeginInfo*, const VkSubpassBeginInfoKHR*)
VULKAN_FUNCTION(NextSubpass2, void, CommandBuffer, void*, const VkSubpassBeginInfoKHR*, const VkSubpassEndInfoKHR*)
VULKAN_FUNCTION(EndRenderPass2, void, CommandBuffer, void*, const VkSubpassEndInfoKHR*)
VULKAN_FUNCTION(GetSwapchainStatus, VkResult, Device, void*, VkSwapchainKHR)
VULKAN_FUNCTION(ImportFenceFd, VkResult, Device, void*, const VkImportFenceFdInfoKHR*)
VULKAN_FUNCTION(GetFenceFd, VkResult, Device, void*, const VkFenceGetFdInfoKHR*, int*)
VULKAN_FUNCTION(GetSurfaceCapabilities2, VkResult, PhysicalDevice, void*, const VkPhysicalDeviceSurfaceInfo2KHR*, VkSurfaceCapabilities2KHR*)
VULKAN_FUNCTION(GetSurfaceFormats2, VkResult, PhysicalDevice, void*, const VkPhysicalDeviceSurfaceInfo2KHR*, uint32_t*, VkSurfaceFormat2KHR*)
VULKAN_FUNCTION(GetDisplayProperties2, VkResult, PhysicalDevice, void*, uint32_t*, VkDisplayProperties2KHR*)
VULKAN_FUNCTION(GetDisplayPlaneProperties2, VkResult, PhysicalDevice, void*, uint32_t*, VkDisplayPlaneProperties2KHR*)
VULKAN_FUNCTION(GetDisplayModeProperties2, VkResult, PhysicalDevice, void*, VkDisplayKHR, uint32_t*, VkDisplayModeProperties2KHR*)
VULKAN_FUNCTION(GetDisplayPlaneCapabilities2, VkResult, PhysicalDevice, void*, const VkDisplayPlaneInfo2KHR*, VkDisplayPlaneCapabilities2KHR*)
VULKAN_FUNCTION(DrawIndirectCount, void, CommandBuffer, void*, VkBuffer, VkDeviceSize, VkBuffer, VkDeviceSize, uint32_t, uint32_t)
VULKAN_FUNCTION(DrawIndexedIndirectCount, void, CommandBuffer, void*, VkBuffer, VkDeviceSize, VkBuffer, VkDeviceSize, uint32_t, uint32_t)
VULKAN_FUNCTION(GetPipelineExecutableProperties, VkResult, Device, void*, const VkPipelineInfoKHR*, uint32_t*, VkPipelineExecutablePropertiesKHR*)
VULKAN_FUNCTION(GetPipelineExecutableStatistics, VkResult, Device, void*, const VkPipelineExecutableInfoKHR*, uint32_t*, VkPipelineExecutableStatisticKHR*)
VULKAN_FUNCTION(GetPipelineExecutableInternalRepresentations, VkResult, Device, void*, const VkPipelineExecutableInfoKHR*, uint32_t*, VkPipelineExecutableInternalRepresentationKHR*)
VULKAN_FUNCTION(CreateDebugReportCallback, VkResult, Instance, void*, const VkDebugReportCallbackCreateInfoEXT*, const VkAllocationCallbacks*, VkDebugReportCallbackEXT*)
VULKAN_FUNCTION(DestroyDebugReportCallback, void, Instance, void*, VkDebugReportCallbackEXT, const VkAllocationCallbacks*)
VULKAN_FUNCTION(DebugReportMessage, void, Instance, void*, VkDebugReportFlagsEXT, VkDebugReportObjectTypeEXT, uint64_t, size_t, int32_t, const char*, const char*)
VULKAN_FUNCTION(DebugMarkerSetObjectTag, VkResult, Device, void*, const VkDebugMarkerObjectTagInfoEXT*)
VULKAN_FUNCTION(DebugMarkerSetObjectName, VkResult, Device, void*, const VkDebugMarkerObjectNameInfoEXT*)
VULKAN_FUNCTION(DebugMarkerBegin, void, CommandBuffer, void*, const VkDebugMarkerMarkerInfoEXT*)
VULKAN_FUNCTION(DebugMarkerEnd, void, CommandBuffer, void*)
VULKAN_FUNCTION(DebugMarkerInsert, void, CommandBuffer, void*, const VkDebugMarkerMarkerInfoEXT*)
VULKAN_FUNCTION(BindTransformFeedbackBuffers, void, CommandBuffer, void*, uint32_t, uint32_t, const VkBuffer*, const VkDeviceSize*, const VkDeviceSize*)
VULKAN_FUNCTION(BeginTransformFeedback, void, CommandBuffer, void*, uint32_t, uint32_t, const VkBuffer*, const VkDeviceSize*)
VULKAN_FUNCTION(EndTransformFeedback, void, CommandBuffer, void*, uint32_t, uint32_t, const VkBuffer*, const VkDeviceSize*)
VULKAN_FUNCTION(BeginQueryIndexed, void, CommandBuffer, void*, VkQueryPool, uint32_t, VkQueryControlFlags, uint32_t)
VULKAN_FUNCTION(EndQueryIndexed, void, CommandBuffer, void*, VkQueryPool, uint32_t, uint32_t)
VULKAN_FUNCTION(DrawIndirectByteCount, void, CommandBuffer, void*, uint32_t, uint32_t, VkBuffer, VkDeviceSize, uint32_t, uint32_t)
VULKAN_FUNCTION(GetImageViewHandleNVX, uint32_t, Device, void*, const VkImageViewHandleInfoNVX*)
VULKAN_FUNCTION(GetShaderInfoAMD, VkResult, Device, void*, VkPipeline, VkShaderStageFlagBits, VkShaderInfoTypeAMD, size_t*, void*)
VULKAN_FUNCTION(GetExternalImageFormatPropertiesNV, VkResult, PhysicalDevice, void*, VkFormat, VkImageType, VkImageTiling, VkImageUsageFlags, VkImageCreateFlags, VkExternalMemoryHandleTypeFlagsNV, VkExternalImageFormatPropertiesNV*)
VULKAN_FUNCTION(BeginConditionalRendering, void, CommandBuffer, void*, const VkConditionalRenderingBeginInfoEXT*)
VULKAN_FUNCTION(EndConditionalRendering, void, CommandBuffer, void*)
VULKAN_FUNCTION(ProcessCommands, void, CommandBuffer, void*, const VkCmdProcessCommandsInfoNVX*)
VULKAN_FUNCTION(ReserveSpaceForCommands, void, CommandBuffer, void*, const VkCmdReserveSpaceForCommandsInfoNVX*)
VULKAN_FUNCTION(CreateIndirectCommandsLayoutNVX, VkResult, Device, void*, const VkIndirectCommandsLayoutCreateInfoNVX*, const VkAllocationCallbacks*, VkIndirectCommandsLayoutNVX*)
VULKAN_FUNCTION(DestroyIndirectCommandsLayoutNVX, void, Device, void*, VkIndirectCommandsLayoutNVX, const VkAllocationCallbacks*)
VULKAN_FUNCTION(CreateObjectTableNVX, VkResult, Device, void*, const VkObjectTableCreateInfoNVX*, const VkAllocationCallbacks*, VkObjectTableNVX*)
VULKAN_FUNCTION(DestroyObjectTableNVX, void, Device, void*, VkObjectTableNVX, const VkAllocationCallbacks*)
VULKAN_FUNCTION(RegisterObjectsNVX, VkResult, Device, void*, VkObjectTableNVX, uint32_t, const VkObjectTableEntryNVX* const*, const uint32_t*)
VULKAN_FUNCTION(UnregisterObjectsNVX, VkResult, Device, void*, VkObjectTableNVX, uint32_t, const VkObjectEntryTypeNVX*, const uint32_t*)
VULKAN_FUNCTION(GetGeneratedCommandsPropertiesNVX, void, PhysicalDevice, void*, VkDeviceGeneratedCommandsFeaturesNVX*, VkDeviceGeneratedCommandsLimitsNVX*)
VULKAN_FUNCTION(SetViewportWScaling, void, CommandBuffer, void*, uint32_t, uint32_t, const VkViewportWScalingNV*)
VULKAN_FUNCTION(ReleaseDisplay, VkResult, PhysicalDevice, void*, VkDisplayKHR)
VULKAN_FUNCTION(GetSurfaceCapabilities2KHR, VkResult, PhysicalDevice, void*, const VkPhysicalDeviceSurfaceInfo2KHR*, VkSurfaceCapabilities2KHR*)
VULKAN_FUNCTION(DisplayPowerControl, VkResult, Device, void*, VkDisplayKHR, const VkDisplayPowerInfoEXT*)
VULKAN_FUNCTION(RegisterDeviceEvent, VkResult, Device, void*, const VkDeviceEventInfoEXT*, const VkAllocationCallbacks*, VkFence*)
VULKAN_FUNCTION(RegisterDisplayEvent, VkResult, Device, void*, VkDisplayKHR, const VkDisplayEventInfoEXT*, const VkAllocationCallbacks*, VkFence*)
VULKAN_FUNCTION(GetSwapchainCounter, VkResult, Device, void*, VkSwapchainKHR, VkSurfaceCounterFlagBitsEXT, uint64_t*)
VULKAN_FUNCTION(GetRefreshCycleDurationGOOGLE, VkResult, Device, void*, VkSwapchainKHR, VkRefreshCycleDurationGOOGLE*)
VULKAN_FUNCTION(GetPastPresentationTimingGOOGLE, VkResult, Device, void*, VkSwapchainKHR, uint32_t*, VkPastPresentationTimingGOOGLE*)
VULKAN_FUNCTION(SetDiscardRectangle, void, CommandBuffer, void*, uint32_t, uint32_t, const VkRect2D*)

#if defined(VK_EXT_hdr_metadata)
VULKAN_FUNCTION(SetHdrMetadata, void, Device, void*, uint32_t, const VkSwapchainKHR*, const VkHdrMetadataEXT*)
#endif

VULKAN_FUNCTION(SetDebugUtilsObjectName, VkResult, Device, void*, const VkDebugUtilsObjectNameInfoEXT*)
VULKAN_FUNCTION(SetDebugUtilsObjectTag, VkResult, Device, void*, const VkDebugUtilsObjectTagInfoEXT*)
VULKAN_FUNCTION(BeginDebugUtilsLabel, void, Queue, void*, const VkDebugUtilsLabelEXT*)
VULKAN_FUNCTION(EndDebugUtilsLabel, void, Queue, void*)
VULKAN_FUNCTION(InsertDebugUtilsLabel, void, Queue, void*, const VkDebugUtilsLabelEXT*)
VULKAN_FUNCTION(BeginDebugUtilsLabel, void, CommandBuffer, void*, const VkDebugUtilsLabelEXT*)
VULKAN_FUNCTION(EndDebugUtilsLabel, void, CommandBuffer, void*)
VULKAN_FUNCTION(InsertDebugUtilsLabel, void, CommandBuffer, void*, const VkDebugUtilsLabelEXT*)
VULKAN_FUNCTION(CreateDebugUtilsMessenger, VkResult, Instance, void*, const VkDebugUtilsMessengerCreateInfoEXT*, const VkAllocationCallbacks*, VkDebugUtilsMessengerEXT*)
VULKAN_FUNCTION(DestroyDebugUtilsMessenger, void, Instance, void*, VkDebugUtilsMessengerEXT, const VkAllocationCallbacks*)
VULKAN_FUNCTION(SubmitDebugUtilsMessage, void, Instance, void*, VkDebugUtilsMessageSeverityFlagBitsEXT, VkDebugUtilsMessageTypeFlagsEXT, const VkDebugUtilsMessengerCallbackDataEXT*)
VULKAN_FUNCTION(SetSampleLocations, void, CommandBuffer, void*, const VkSampleLocationsInfoEXT*)
VULKAN_FUNCTION(GetMultisampleProperties, void, PhysicalDevice, void*, VkSampleCountFlagBits, VkMultisamplePropertiesEXT*)
VULKAN_FUNCTION(GetImageDrmFormatModifierProperties, VkResult, Device, void*, VkImage, VkImageDrmFormatModifierPropertiesEXT*)
VULKAN_FUNCTION(CreateValidationCache, VkResult, Device, void*, const VkValidationCacheCreateInfoEXT*, const VkAllocationCallbacks*, VkValidationCacheEXT*)
VULKAN_FUNCTION(DestroyValidationCache, void, Device, void*, VkValidationCacheEXT, const VkAllocationCallbacks*)
VULKAN_FUNCTION(MergeValidationCaches, VkResult, Device, void*, VkValidationCacheEXT, uint32_t, const VkValidationCacheEXT*)
VULKAN_FUNCTION(GetValidationCacheData, VkResult, Device, void*, VkValidationCacheEXT, size_t*, void*)
VULKAN_FUNCTION(BindShadingRateImage, void, CommandBuffer, void*, VkImageView, VkImageLayout)
VULKAN_FUNCTION(SetViewportShadingRatePalette, void, CommandBuffer, void*, uint32_t, uint32_t, const VkShadingRatePaletteNV*)
VULKAN_FUNCTION(SetCoarseSampleOrder, void, CommandBuffer, void*, VkCoarseSampleOrderTypeNV, uint32_t, const VkCoarseSampleOrderCustomNV*)
VULKAN_FUNCTION(CreateAccelerationStructureNV, VkResult, Device, void*, const VkAccelerationStructureCreateInfoNV*, const VkAllocationCallbacks*, VkAccelerationStructureNV*)
VULKAN_FUNCTION(DestroyAccelerationStructureNV, void, Device, void*, VkAccelerationStructureNV, const VkAllocationCallbacks*)
VULKAN_FUNCTION(GetAccelerationStructureMemoryRequirementsNV, void, Device, void*, const VkAccelerationStructureMemoryRequirementsInfoNV*, VkMemoryRequirements2KHR*)
VULKAN_FUNCTION(BindAccelerationStructureMemoryNV, VkResult, Device, void*, uint32_t, const VkBindAccelerationStructureMemoryInfoNV*)
VULKAN_FUNCTION(BuildAccelerationStructure, void, CommandBuffer, void*, const VkAccelerationStructureInfoNV*, VkBuffer, VkDeviceSize, VkBool32, VkAccelerationStructureNV, VkAccelerationStructureNV, VkBuffer, VkDeviceSize)
VULKAN_FUNCTION(CopyAccelerationStructure, void, CommandBuffer, void*, VkAccelerationStructureNV, VkAccelerationStructureNV, VkCopyAccelerationStructureModeNV)
VULKAN_FUNCTION(TraceRays, void, CommandBuffer, void*, VkBuffer, VkDeviceSize, VkBuffer, VkDeviceSize, VkDeviceSize, VkBuffer, VkDeviceSize, VkDeviceSize, VkBuffer, VkDeviceSize, VkDeviceSize, uint32_t, uint32_t, uint32_t)
VULKAN_FUNCTION(CreateRayTracingPipelinesNV, VkResult, Device, void*, VkPipelineCache, uint32_t, const VkRayTracingPipelineCreateInfoNV*, const VkAllocationCallbacks*, VkPipeline*)
VULKAN_FUNCTION(GetRayTracingShaderGroupHandlesNV, VkResult, Device, void*, VkPipeline, uint32_t, uint32_t, size_t, void*)
VULKAN_FUNCTION(GetAccelerationStructureHandleNV, VkResult, Device, void*, VkAccelerationStructureNV, size_t, void*)
VULKAN_FUNCTION(WriteAccelerationStructuresProperties, void, CommandBuffer, void*, uint32_t, const VkAccelerationStructureNV*, VkQueryType, VkQueryPool, uint32_t)
VULKAN_FUNCTION(CompileDeferredNV, VkResult, Device, void*, VkPipeline, uint32_t)
VULKAN_FUNCTION(GetMemoryHostPointerProperties, VkResult, Device, void*, VkExternalMemoryHandleTypeFlagBits, const void*, VkMemoryHostPointerPropertiesEXT*)
VULKAN_FUNCTION(WriteBufferMarker, void, CommandBuffer, void*, VkPipelineStageFlagBits, VkBuffer, VkDeviceSize, uint32_t)
VULKAN_FUNCTION(GetCalibrateableTimeDomains, VkResult, PhysicalDevice, void*, uint32_t*, VkTimeDomainEXT*)
VULKAN_FUNCTION(GetCalibratedTimestamps, VkResult, Device, void*, uint32_t, const VkCalibratedTimestampInfoEXT*, uint64_t*, uint64_t*)
VULKAN_FUNCTION(DrawMeshTasks, void, CommandBuffer, void*, uint32_t, uint32_t)
VULKAN_FUNCTION(DrawMeshTasksIndirect, void, CommandBuffer, void*, VkBuffer, VkDeviceSize, uint32_t, uint32_t)
VULKAN_FUNCTION(DrawMeshTasksIndirectCount, void, CommandBuffer, void*, VkBuffer, VkDeviceSize, VkBuffer, VkDeviceSize, uint32_t, uint32_t)
VULKAN_FUNCTION(SetExclusiveScissor, void, CommandBuffer, void*, uint32_t, uint32_t, const VkRect2D*)
VULKAN_FUNCTION(SetCheckpoint, void, CommandBuffer, void*, const void*)
VULKAN_FUNCTION(GetCheckpointData, void, Queue, void*, uint32_t*, VkCheckpointDataNV*)
VULKAN_FUNCTION(InitializePerformanceApiINTEL, VkResult, Device, void*, const VkInitializePerformanceApiInfoINTEL*)
VULKAN_FUNCTION(UninitializePerformanceApiINTEL, void, Device, void*)
VULKAN_FUNCTION(SetPerformanceMarker, VkResult, CommandBuffer, void*, const VkPerformanceMarkerInfoINTEL*)
VULKAN_FUNCTION(SetPerformanceStreamMarker, VkResult, CommandBuffer, void*, const VkPerformanceStreamMarkerInfoINTEL*)
VULKAN_FUNCTION(SetPerformanceOverride, VkResult, CommandBuffer, void*, const VkPerformanceOverrideInfoINTEL*)
VULKAN_FUNCTION(AcquirePerformanceConfigurationINTEL, VkResult, Device, void*, const VkPerformanceConfigurationAcquireInfoINTEL*, VkPerformanceConfigurationINTEL*)
VULKAN_FUNCTION(ReleasePerformanceConfigurationINTEL, VkResult, Device, void*, VkPerformanceConfigurationINTEL)
VULKAN_FUNCTION(SetPerformanceConfiguration, VkResult, Queue, void*, VkPerformanceConfigurationINTEL)
VULKAN_FUNCTION(GetPerformanceParameterINTEL, VkResult, Device, void*, VkPerformanceParameterTypeINTEL, VkPerformanceValueINTEL*)
VULKAN_FUNCTION(SetLocalDimmingAMD, void, Device, void*, VkSwapchainKHR, VkBool32)
VULKAN_FUNCTION(GetBufferDeviceAddress, VkDeviceAddress, Device, void*, const VkBufferDeviceAddressInfoEXT*)
VULKAN_FUNCTION(GetCooperativeMatrixPropertiesNV, VkResult, PhysicalDevice, void*, uint32_t*, VkCooperativeMatrixPropertiesNV*)
VULKAN_FUNCTION(GetSupportedFramebufferMixedSamplesCombinationsNV, VkResult, PhysicalDevice, void*, uint32_t*, VkFramebufferMixedSamplesCombinationNV*)
VULKAN_FUNCTION(CreateHeadlessSurface, VkResult, Instance, void*, const VkHeadlessSurfaceCreateInfoEXT*, const VkAllocationCallbacks*, VkSurfaceKHR*)
VULKAN_FUNCTION(SetLineStipple, void, CommandBuffer, void*, uint32_t, uint16_t)
VULKAN_FUNCTION(ResetQueryPool, void, Device, void*, VkQueryPool, uint32_t, uint32_t)

#if defined(VK_KHR_timeline_semaphore)
VULKAN_FUNCTION(GetSemaphoreCounterValue, VkResult, Device, void*, VkSemaphore, uint64_t*)
VULKAN_FUNCTION(WaitSemaphores, VkResult, Device, void*, const VkSemaphoreWaitInfoKHR*, uint64_t)
VULKAN_FUNCTION(SignalSemaphore, VkResult, Device, void*, const VkSemaphoreSignalInfoKHR*)
#endif

#if defined(VK_KHR_win32_surface)
VULKAN_FUNCTION(CreateWin32Surface, VkResult, Instance, void*, const VkWin32SurfaceCreateInfoKHR*, const VkAllocationCallbacks*, VkSurfaceKHR*)
VULKAN_FUNCTION(GetWin32PresentationSupport, VkBool32, PhysicalDevice, void*, uint32_t)
#endif

#if defined(VK_KHR_external_memory_win32)
VULKAN_FUNCTION(GetMemoryWin32Handle, VkResult, Device, void*, const VkMemoryGetWin32HandleInfoKHR*, HANDLE*)
VULKAN_FUNCTION(GetMemoryWin32HandleProperties, VkResult, Device, void*, VkExternalMemoryHandleTypeFlagBits, HANDLE, VkMemoryWin32HandlePropertiesKHR*)
#endif

#if defined(VK_KHR_external_semaphore_win32)
VULKAN_FUNCTION(ImportSemaphoreWin32Handle, VkResult, Device, void*, const VkImportSemaphoreWin32HandleInfoKHR*)
VULKAN_FUNCTION(GetSemaphoreWin32Handle, VkResult, Device, void*, const VkSemaphoreGetWin32HandleInfoKHR*, HANDLE*)
#endif

#if defined(VK_KHR_external_fence_win32)
VULKAN_FUNCTION(ImportFenceWin32Handle, VkResult, Device, void*, const VkImportFenceWin32HandleInfoKHR*)
VULKAN_FUNCTION(GetFenceWin32Handle, VkResult, Device, void*, const VkFenceGetWin32HandleInfoKHR*, HANDLE*)
#endif

#if defined(VK_NV_external_memory_win32)
VULKAN_FUNCTION(GetMemoryWin32HandleNV, VkResult, Device, void*, VkDeviceMemory, VkExternalMemoryHandleTypeFlagsNV, HANDLE*)
#endif

#if defined(VK_EXT_full_screen_exclusive)
VULKAN_FUNCTION(GetSurfacePresentModes2, VkResult, PhysicalDevice, void*, const VkPhysicalDeviceSurfaceInfo2KHR*, uint32_t*, VkPresentModeKHR*)
VULKAN_FUNCTION(AcquireFullScreenExclusiveMode, VkResult, Device, void*, VkSwapchainKHR)
VULKAN_FUNCTION(ReleaseFullScreenExclusiveMode, VkResult, Device, void*, VkSwapchainKHR)
VULKAN_FUNCTION(GetDeviceGroupSurfacePresentModes2, VkResult, Device, void*, const VkPhysicalDeviceSurfaceInfo2KHR*, VkDeviceGroupPresentModeFlagsKHR*)
#endif

#if defined(VK_KHR_xcb_surface)
VULKAN_FUNCTION(CreateXcbSurface, VkResult, Instance, void*, const VkXcbSurfaceCreateInfoKHR*, const VkAllocationCallbacks*, VkSurfaceKHR*)
VULKAN_FUNCTION(GetPhysicalDeviceXcbPresentationSupport, VkBool32, PhysicalDevice, void*, uint32_t, xcb_connection_t*, xcb_visualid_t)
#endif

#if defined(VK_KHR_xlib_surface)
VULKAN_FUNCTION(CreateXlibSurface, VkResult, Instance, void*, const VkXlibSurfaceCreateInfoKHR*, const VkAllocationCallbacks*, VkSurfaceKHR*)
VULKAN_FUNCTION(GetPhysicalDeviceXlibPresentationSupport, VkBool32, PhysicalDevice, void*, uint32_t, Display*, VisualID)
#endif

#if defined(VK_USE_PLATFORM_XLIB_XRANDR_EXT) && defined(VK_EXT_acquire_xlib_display)
VULKAN_FUNCTION(AcquireXlibDisplay, VkResult, PhysicalDevice, void*, Display*, VkDisplayKHR)
VULKAN_FUNCTION(GetRandROutputDisplay, VkResult, PhysicalDevice, void*, Display*, RROutput, VkDisplayKHR*)
#endif
