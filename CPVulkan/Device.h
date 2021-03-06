#pragma once
#include "Base.h"

#include "Extensions.h"

#include <memory>

struct DeviceState;

class Instance;
class Queue;

class Device final
{
public:
	Device();
	Device(const Device&) = delete;
	Device(Device&&) = delete;
	~Device();

	Device& operator=(const Device&) = delete;
	Device&& operator=(const Device&&) = delete;

	void OnDelete(const VkAllocationCallbacks* pAllocator);

	VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL GetProcAddress(const char* pName) const;
	VKAPI_ATTR void VKAPI_PTR DestroyDevice(const VkAllocationCallbacks* pAllocator);
	VKAPI_ATTR void VKAPI_PTR GetDeviceQueue(uint32_t queueFamilyIndex, uint32_t queueIndex, VkQueue* pQueue);
	VKAPI_ATTR VkResult VKAPI_PTR WaitIdle();
	VKAPI_ATTR VkResult VKAPI_PTR AllocateMemory(const VkMemoryAllocateInfo* pAllocateInfo, const VkAllocationCallbacks* pAllocator, VkDeviceMemory* pMemory);
	VKAPI_ATTR void VKAPI_PTR FreeMemory(VkDeviceMemory memory, const VkAllocationCallbacks* pAllocator);
	VKAPI_ATTR VkResult VKAPI_PTR MapMemory(VkDeviceMemory memory, VkDeviceSize offset, VkDeviceSize size, VkMemoryMapFlags flags, void** ppData);
	VKAPI_ATTR void VKAPI_PTR UnmapMemory(VkDeviceMemory memory);
	VKAPI_ATTR VkResult VKAPI_PTR FlushMappedMemoryRanges(uint32_t memoryRangeCount, const VkMappedMemoryRange* pMemoryRanges);
	VKAPI_ATTR VkResult VKAPI_PTR InvalidateMappedMemoryRanges(uint32_t memoryRangeCount, const VkMappedMemoryRange* pMemoryRanges);
	VKAPI_ATTR void VKAPI_PTR GetDeviceMemoryCommitment(VkDeviceMemory memory, VkDeviceSize* pCommittedMemoryInBytes) const;
	VKAPI_ATTR VkResult VKAPI_PTR BindBufferMemory(VkBuffer buffer, VkDeviceMemory memory, VkDeviceSize memoryOffset);
	VKAPI_ATTR VkResult VKAPI_PTR BindImageMemory(VkImage image, VkDeviceMemory memory, VkDeviceSize memoryOffset);
	VKAPI_ATTR void VKAPI_PTR GetBufferMemoryRequirements(VkBuffer buffer, VkMemoryRequirements* pMemoryRequirements) const;
	VKAPI_ATTR void VKAPI_PTR GetImageMemoryRequirements(VkImage image, VkMemoryRequirements* pMemoryRequirements);
	VKAPI_ATTR void VKAPI_PTR GetImageSparseMemoryRequirements(VkImage image, uint32_t* pSparseMemoryRequirementCount, VkSparseImageMemoryRequirements* pSparseMemoryRequirements);
	VKAPI_ATTR VkResult VKAPI_PTR CreateFence(const VkFenceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkFence* pFence);
	VKAPI_ATTR void VKAPI_PTR DestroyFence(VkFence fence, const VkAllocationCallbacks* pAllocator);
	VKAPI_ATTR VkResult VKAPI_PTR ResetFences(uint32_t fenceCount, const VkFence* pFences);
	VKAPI_ATTR VkResult VKAPI_PTR GetFenceStatus(VkFence fence) const;
	VKAPI_ATTR VkResult VKAPI_PTR WaitForFences(uint32_t fenceCount, const VkFence* pFences, VkBool32 waitAll, uint64_t timeout);
	VKAPI_ATTR VkResult VKAPI_PTR CreateSemaphore(const VkSemaphoreCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSemaphore* pSemaphore);
	VKAPI_ATTR void VKAPI_PTR DestroySemaphore(VkSemaphore semaphore, const VkAllocationCallbacks* pAllocator);
	VKAPI_ATTR VkResult VKAPI_PTR CreateEvent(const VkEventCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkEvent* pEvent);
	VKAPI_ATTR void VKAPI_PTR DestroyEvent(VkEvent event, const VkAllocationCallbacks* pAllocator);
	VKAPI_ATTR VkResult VKAPI_PTR GetEventStatus(VkEvent event);
	VKAPI_ATTR VkResult VKAPI_PTR SetEvent(VkEvent event);
	VKAPI_ATTR VkResult VKAPI_PTR ResetEvent(VkEvent event);
	VKAPI_ATTR VkResult VKAPI_PTR CreateQueryPool(const VkQueryPoolCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkQueryPool* pQueryPool);
	VKAPI_ATTR void VKAPI_PTR DestroyQueryPool(VkQueryPool queryPool, const VkAllocationCallbacks* pAllocator);
	VKAPI_ATTR VkResult VKAPI_PTR GetQueryPoolResults(VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount, size_t dataSize, void* pData, VkDeviceSize stride, VkQueryResultFlags flags);
	VKAPI_ATTR VkResult VKAPI_PTR CreateBuffer(const VkBufferCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkBuffer* pBuffer);
	VKAPI_ATTR void VKAPI_PTR DestroyBuffer(VkBuffer buffer, const VkAllocationCallbacks* pAllocator);
	VKAPI_ATTR VkResult VKAPI_PTR CreateBufferView(const VkBufferViewCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkBufferView* pView);
	VKAPI_ATTR void VKAPI_PTR DestroyBufferView(VkBufferView bufferView, const VkAllocationCallbacks* pAllocator);
	VKAPI_ATTR VkResult VKAPI_PTR CreateImage(const VkImageCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkImage* pImage);
	VKAPI_ATTR void VKAPI_PTR DestroyImage(VkImage image, const VkAllocationCallbacks* pAllocator);
	VKAPI_ATTR void VKAPI_PTR GetImageSubresourceLayout(VkImage image, const VkImageSubresource* pSubresource, VkSubresourceLayout* pLayout);
	VKAPI_ATTR VkResult VKAPI_PTR CreateImageView(const VkImageViewCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkImageView* pView);
	VKAPI_ATTR void VKAPI_PTR DestroyImageView(VkImageView imageView, const VkAllocationCallbacks* pAllocator);
	VKAPI_ATTR VkResult VKAPI_PTR CreateShaderModule(const VkShaderModuleCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkShaderModule* pShaderModule);
	VKAPI_ATTR void VKAPI_PTR DestroyShaderModule(VkShaderModule shaderModule, const VkAllocationCallbacks* pAllocator);
	VKAPI_ATTR VkResult VKAPI_PTR CreatePipelineCache(const VkPipelineCacheCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkPipelineCache* pPipelineCache);
	VKAPI_ATTR void VKAPI_PTR DestroyPipelineCache(VkPipelineCache pipelineCache, const VkAllocationCallbacks* pAllocator);
	VKAPI_ATTR VkResult VKAPI_PTR GetPipelineCacheData(VkPipelineCache pipelineCache, size_t* pDataSize, void* pData);
	VKAPI_ATTR VkResult VKAPI_PTR MergePipelineCaches(VkPipelineCache dstCache, uint32_t srcCacheCount, const VkPipelineCache* pSrcCaches);
	VKAPI_ATTR VkResult VKAPI_PTR CreateGraphicsPipelines(VkPipelineCache pipelineCache, uint32_t createInfoCount, const VkGraphicsPipelineCreateInfo* pCreateInfos, const VkAllocationCallbacks* pAllocator, VkPipeline* pPipelines);
	VKAPI_ATTR VkResult VKAPI_PTR CreateComputePipelines(VkPipelineCache pipelineCache, uint32_t createInfoCount, const VkComputePipelineCreateInfo* pCreateInfos, const VkAllocationCallbacks* pAllocator, VkPipeline* pPipelines);
	VKAPI_ATTR void VKAPI_PTR DestroyPipeline(VkPipeline pipeline, const VkAllocationCallbacks* pAllocator);
	VKAPI_ATTR VkResult VKAPI_PTR CreatePipelineLayout(const VkPipelineLayoutCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkPipelineLayout* pPipelineLayout);
	VKAPI_ATTR void VKAPI_PTR DestroyPipelineLayout(VkPipelineLayout pipelineLayout, const VkAllocationCallbacks* pAllocator);
	VKAPI_ATTR VkResult VKAPI_PTR CreateSampler(const VkSamplerCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSampler* pSampler);
	VKAPI_ATTR void VKAPI_PTR DestroySampler(VkSampler sampler, const VkAllocationCallbacks* pAllocator);
	VKAPI_ATTR VkResult VKAPI_PTR CreateDescriptorSetLayout(const VkDescriptorSetLayoutCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDescriptorSetLayout* pSetLayout);
	VKAPI_ATTR void VKAPI_PTR DestroyDescriptorSetLayout(VkDescriptorSetLayout descriptorSetLayout, const VkAllocationCallbacks* pAllocator);
	VKAPI_ATTR VkResult VKAPI_PTR CreateDescriptorPool(const VkDescriptorPoolCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDescriptorPool* pDescriptorPool);
	VKAPI_ATTR void VKAPI_PTR DestroyDescriptorPool(VkDescriptorPool descriptorPool, const VkAllocationCallbacks* pAllocator);
	VKAPI_ATTR VkResult VKAPI_PTR ResetDescriptorPool(VkDescriptorPool descriptorPool, VkDescriptorPoolResetFlags flags);
	VKAPI_ATTR VkResult VKAPI_PTR AllocateDescriptorSets(const VkDescriptorSetAllocateInfo* pAllocateInfo, VkDescriptorSet* pDescriptorSets);
	VKAPI_ATTR VkResult VKAPI_PTR FreeDescriptorSets(VkDescriptorPool descriptorPool, uint32_t descriptorSetCount, const VkDescriptorSet* pDescriptorSets);
	VKAPI_ATTR void VKAPI_PTR UpdateDescriptorSets(uint32_t descriptorWriteCount, const VkWriteDescriptorSet* pDescriptorWrites, uint32_t descriptorCopyCount, const VkCopyDescriptorSet* pDescriptorCopies);
	VKAPI_ATTR VkResult VKAPI_PTR CreateFramebuffer(const VkFramebufferCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkFramebuffer* pFramebuffer);
	VKAPI_ATTR void VKAPI_PTR DestroyFramebuffer(VkFramebuffer framebuffer, const VkAllocationCallbacks* pAllocator);
	VKAPI_ATTR VkResult VKAPI_PTR CreateRenderPass(const VkRenderPassCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkRenderPass* pRenderPass);
	VKAPI_ATTR void VKAPI_PTR DestroyRenderPass(VkRenderPass renderPass, const VkAllocationCallbacks* pAllocator);
	VKAPI_ATTR void VKAPI_PTR GetRenderAreaGranularity(VkRenderPass renderPass, VkExtent2D* pGranularity);
	VKAPI_ATTR VkResult VKAPI_PTR CreateCommandPool(const VkCommandPoolCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkCommandPool* pCommandPool);
	VKAPI_ATTR void VKAPI_PTR DestroyCommandPool(VkCommandPool commandPool, const VkAllocationCallbacks* pAllocator);
	VKAPI_ATTR VkResult VKAPI_PTR ResetCommandPool(VkCommandPool commandPool, VkCommandPoolResetFlags flags);
	VKAPI_ATTR VkResult VKAPI_PTR AllocateCommandBuffers(const VkCommandBufferAllocateInfo* pAllocateInfo, VkCommandBuffer* pCommandBuffers);
	VKAPI_ATTR void VKAPI_PTR FreeCommandBuffers(VkCommandPool commandPool, uint32_t commandBufferCount, const VkCommandBuffer* pCommandBuffers);
	
	VKAPI_ATTR VkResult VKAPI_PTR BindBufferMemory2(uint32_t bindInfoCount, const VkBindBufferMemoryInfo* pBindInfos);
	VKAPI_ATTR VkResult VKAPI_PTR BindImageMemory2(uint32_t bindInfoCount, const VkBindImageMemoryInfo* pBindInfos);
	VKAPI_ATTR void VKAPI_PTR GetDeviceGroupPeerMemoryFeatures(uint32_t heapIndex, uint32_t localDeviceIndex, uint32_t remoteDeviceIndex, VkPeerMemoryFeatureFlags* pPeerMemoryFeatures);
	VKAPI_ATTR void VKAPI_PTR GetImageMemoryRequirements2(const VkImageMemoryRequirementsInfo2* pInfo, VkMemoryRequirements2* pMemoryRequirements);
	VKAPI_ATTR void VKAPI_PTR GetBufferMemoryRequirements2(const VkBufferMemoryRequirementsInfo2* pInfo, VkMemoryRequirements2* pMemoryRequirements) const;
	VKAPI_ATTR void VKAPI_PTR GetImageSparseMemoryRequirements2(const VkImageSparseMemoryRequirementsInfo2* pInfo, uint32_t* pSparseMemoryRequirementCount, VkSparseImageMemoryRequirements2* pSparseMemoryRequirements);
	VKAPI_ATTR void VKAPI_PTR TrimCommandPool(VkCommandPool commandPool, VkCommandPoolTrimFlags flags);
	VKAPI_ATTR void VKAPI_PTR GetDeviceQueue2(const VkDeviceQueueInfo2* pQueueInfo, VkQueue* pQueue);
	VKAPI_ATTR VkResult VKAPI_PTR CreateSamplerYcbcrConversion(const VkSamplerYcbcrConversionCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSamplerYcbcrConversion* pYcbcrConversion);
	VKAPI_ATTR void VKAPI_PTR DestroySamplerYcbcrConversion(VkSamplerYcbcrConversion ycbcrConversion, const VkAllocationCallbacks* pAllocator);
	VKAPI_ATTR VkResult VKAPI_PTR CreateDescriptorUpdateTemplate(const VkDescriptorUpdateTemplateCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDescriptorUpdateTemplate* pDescriptorUpdateTemplate);
	VKAPI_ATTR void VKAPI_PTR DestroyDescriptorUpdateTemplate(VkDescriptorUpdateTemplate descriptorUpdateTemplate, const VkAllocationCallbacks* pAllocator);
	VKAPI_ATTR void VKAPI_PTR UpdateDescriptorSetWithTemplate(VkDescriptorSet descriptorSet, VkDescriptorUpdateTemplate descriptorUpdateTemplate, const void* pData);
	VKAPI_ATTR void VKAPI_PTR GetDescriptorSetLayoutSupport(const VkDescriptorSetLayoutCreateInfo* pCreateInfo, VkDescriptorSetLayoutSupport* pSupport);

	VKAPI_ATTR VkResult VKAPI_PTR CreateSwapchain(const VkSwapchainCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSwapchainKHR* pSwapchain);
	VKAPI_ATTR void VKAPI_PTR DestroySwapchain(VkSwapchainKHR swapchain, const VkAllocationCallbacks* pAllocator);
	VKAPI_ATTR VkResult VKAPI_PTR GetSwapchainImages(VkSwapchainKHR swapchain, uint32_t* pSwapchainImageCount, VkImage* pSwapchainImages);
	VKAPI_ATTR VkResult VKAPI_PTR AcquireNextImage(VkSwapchainKHR swapchain, uint64_t timeout, VkSemaphore semaphore, VkFence fence, uint32_t* pImageIndex);
	VKAPI_ATTR VkResult VKAPI_PTR GetDeviceGroupPresentCapabilities(VkDeviceGroupPresentCapabilitiesKHR* pDeviceGroupPresentCapabilities);
	VKAPI_ATTR VkResult VKAPI_PTR GetDeviceGroupSurfacePresentModes(VkSurfaceKHR surface, VkDeviceGroupPresentModeFlagsKHR* pModes);
	VKAPI_ATTR VkResult VKAPI_PTR AcquireNextImage2(const VkAcquireNextImageInfoKHR* pAcquireInfo, uint32_t* pImageIndex);

	VKAPI_ATTR VkResult VKAPI_PTR CreateSharedSwapchains(uint32_t swapchainCount, const VkSwapchainCreateInfoKHR* pCreateInfos, const VkAllocationCallbacks* pAllocator, VkSwapchainKHR* pSwapchains);

	VKAPI_ATTR VkResult VKAPI_PTR GetMemoryFd(const VkMemoryGetFdInfoKHR* pGetFdInfo, int* pFd);
	VKAPI_ATTR VkResult VKAPI_PTR GetMemoryFdProperties(VkExternalMemoryHandleTypeFlagBits handleType, int fd, VkMemoryFdPropertiesKHR* pMemoryFdProperties);

#if defined(VK_KHR_external_semaphore_fd)
	VKAPI_ATTR VkResult VKAPI_PTR ImportSemaphoreFd(const VkImportSemaphoreFdInfoKHR* pImportSemaphoreFdInfo);
	VKAPI_ATTR VkResult VKAPI_PTR GetSemaphoreFd(const VkSemaphoreGetFdInfoKHR* pGetFdInfo, int* pFd);
#endif

#if defined(VK_KHR_create_renderpass2)
	VKAPI_ATTR VkResult VKAPI_PTR CreateRenderPass2(const VkRenderPassCreateInfo2KHR* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkRenderPass* pRenderPass);
#endif
	
	VKAPI_ATTR VkResult VKAPI_PTR GetSwapchainStatus(VkSwapchainKHR swapchain);

	VKAPI_ATTR VkResult VKAPI_PTR ImportFenceFd(const VkImportFenceFdInfoKHR* pImportFenceFdInfo);
	VKAPI_ATTR VkResult VKAPI_PTR GetFenceFd(const VkFenceGetFdInfoKHR* pGetFdInfo, int* pFd);

#if defined(VK_KHR_pipeline_executable_properties)
	VKAPI_ATTR VkResult VKAPI_PTR GetPipelineExecutableProperties(const VkPipelineInfoKHR* pPipelineInfo, uint32_t* pExecutableCount, VkPipelineExecutablePropertiesKHR* pProperties);
	VKAPI_ATTR VkResult VKAPI_PTR GetPipelineExecutableStatistics(const VkPipelineExecutableInfoKHR* pExecutableInfo, uint32_t* pStatisticCount, VkPipelineExecutableStatisticKHR* pStatistics);
	VKAPI_ATTR VkResult VKAPI_PTR GetPipelineExecutableInternalRepresentations(const VkPipelineExecutableInfoKHR* pExecutableInfo, uint32_t* pInternalRepresentationCount, VkPipelineExecutableInternalRepresentationKHR* pInternalRepresentations);
#endif

	VKAPI_ATTR VkResult VKAPI_PTR DebugMarkerSetObjectTag(const VkDebugMarkerObjectTagInfoEXT* pTagInfo);
	VKAPI_ATTR VkResult VKAPI_PTR DebugMarkerSetObjectName(const VkDebugMarkerObjectNameInfoEXT* pNameInfo);
	
	VKAPI_ATTR uint32_t VKAPI_PTR GetImageViewHandleNVX(const VkImageViewHandleInfoNVX* pInfo);

	VKAPI_ATTR VkResult VKAPI_PTR GetShaderInfoAMD(VkPipeline pipeline, VkShaderStageFlagBits shaderStage, VkShaderInfoTypeAMD infoType, size_t* pInfoSize, void* pInfo);

	VKAPI_ATTR VkResult VKAPI_PTR CreateIndirectCommandsLayoutNVX(const VkIndirectCommandsLayoutCreateInfoNVX* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkIndirectCommandsLayoutNVX* pIndirectCommandsLayout);
	VKAPI_ATTR void VKAPI_PTR DestroyIndirectCommandsLayoutNVX(VkIndirectCommandsLayoutNVX indirectCommandsLayout, const VkAllocationCallbacks* pAllocator);
	VKAPI_ATTR VkResult VKAPI_PTR CreateObjectTableNVX(const VkObjectTableCreateInfoNVX* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkObjectTableNVX* pObjectTable);
	VKAPI_ATTR void VKAPI_PTR DestroyObjectTableNVX(VkObjectTableNVX objectTable, const VkAllocationCallbacks* pAllocator);
	VKAPI_ATTR VkResult VKAPI_PTR RegisterObjectsNVX(VkObjectTableNVX objectTable, uint32_t objectCount, const VkObjectTableEntryNVX* const* ppObjectTableEntries, const uint32_t* pObjectIndices);
	VKAPI_ATTR VkResult VKAPI_PTR UnregisterObjectsNVX(VkObjectTableNVX objectTable, uint32_t objectCount, const VkObjectEntryTypeNVX* pObjectEntryTypes, const uint32_t* pObjectIndices);
	
	VKAPI_ATTR VkResult VKAPI_PTR DisplayPowerControl(VkDisplayKHR display, const VkDisplayPowerInfoEXT* pDisplayPowerInfo);
	VKAPI_ATTR VkResult VKAPI_PTR RegisterDeviceEvent(const VkDeviceEventInfoEXT* pDeviceEventInfo, const VkAllocationCallbacks* pAllocator, VkFence* pFence);
	VKAPI_ATTR VkResult VKAPI_PTR RegisterDisplayEvent(VkDisplayKHR display, const VkDisplayEventInfoEXT* pDisplayEventInfo, const VkAllocationCallbacks* pAllocator, VkFence* pFence);
	VKAPI_ATTR VkResult VKAPI_PTR GetSwapchainCounter(VkSwapchainKHR swapchain, VkSurfaceCounterFlagBitsEXT counter, uint64_t* pCounterValue);

	VKAPI_ATTR VkResult VKAPI_PTR GetRefreshCycleDurationGOOGLE(VkSwapchainKHR swapchain, VkRefreshCycleDurationGOOGLE* pDisplayTimingProperties);
	VKAPI_ATTR VkResult VKAPI_PTR GetPastPresentationTimingGOOGLE(VkSwapchainKHR swapchain, uint32_t* pPresentationTimingCount, VkPastPresentationTimingGOOGLE* pPresentationTimings);

#if defined(VK_EXT_hdr_metadata)
	VKAPI_ATTR void VKAPI_PTR SetHdrMetadata(uint32_t swapchainCount, const VkSwapchainKHR* pSwapchains, const VkHdrMetadataEXT* pMetadata);
#endif

	VKAPI_ATTR VkResult VKAPI_PTR SetDebugUtilsObjectName(const VkDebugUtilsObjectNameInfoEXT* pNameInfo);
	VKAPI_ATTR VkResult VKAPI_PTR SetDebugUtilsObjectTag(const VkDebugUtilsObjectTagInfoEXT* pTagInfo);
	
	VKAPI_ATTR VkResult VKAPI_PTR GetImageDrmFormatModifierProperties(VkImage image, VkImageDrmFormatModifierPropertiesEXT* pProperties);

	VKAPI_ATTR VkResult VKAPI_PTR CreateValidationCache(const VkValidationCacheCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkValidationCacheEXT* pValidationCache);
	VKAPI_ATTR void VKAPI_PTR DestroyValidationCache(VkValidationCacheEXT validationCache, const VkAllocationCallbacks* pAllocator);
	VKAPI_ATTR VkResult VKAPI_PTR MergeValidationCaches(VkValidationCacheEXT dstCache, uint32_t srcCacheCount, const VkValidationCacheEXT* pSrcCaches);
	VKAPI_ATTR VkResult VKAPI_PTR GetValidationCacheData(VkValidationCacheEXT validationCache, size_t* pDataSize, void* pData);

	VKAPI_ATTR VkResult VKAPI_PTR CreateAccelerationStructureNV(const VkAccelerationStructureCreateInfoNV* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkAccelerationStructureNV* pAccelerationStructure);
	VKAPI_ATTR void VKAPI_PTR DestroyAccelerationStructureNV(VkAccelerationStructureNV accelerationStructure, const VkAllocationCallbacks* pAllocator);
	VKAPI_ATTR void VKAPI_PTR GetAccelerationStructureMemoryRequirementsNV(const VkAccelerationStructureMemoryRequirementsInfoNV* pInfo, VkMemoryRequirements2KHR* pMemoryRequirements);
	VKAPI_ATTR VkResult VKAPI_PTR BindAccelerationStructureMemoryNV(uint32_t bindInfoCount, const VkBindAccelerationStructureMemoryInfoNV* pBindInfos);
	VKAPI_ATTR VkResult VKAPI_PTR CreateRayTracingPipelinesNV(VkPipelineCache pipelineCache, uint32_t createInfoCount, const VkRayTracingPipelineCreateInfoNV* pCreateInfos, const VkAllocationCallbacks* pAllocator, VkPipeline* pPipelines);
	VKAPI_ATTR VkResult VKAPI_PTR GetRayTracingShaderGroupHandlesNV(VkPipeline pipeline, uint32_t firstGroup, uint32_t groupCount, size_t dataSize, void* pData);
	VKAPI_ATTR VkResult VKAPI_PTR GetAccelerationStructureHandleNV(VkAccelerationStructureNV accelerationStructure, size_t dataSize, void* pData);
	VKAPI_ATTR VkResult VKAPI_PTR CompileDeferredNV(VkPipeline pipeline, uint32_t shader);

	VKAPI_ATTR VkResult VKAPI_PTR GetMemoryHostPointerProperties(VkExternalMemoryHandleTypeFlagBits handleType, const void* pHostPointer, VkMemoryHostPointerPropertiesEXT* pMemoryHostPointerProperties);

	VKAPI_ATTR VkResult VKAPI_PTR GetCalibratedTimestamps(uint32_t timestampCount, const VkCalibratedTimestampInfoEXT* pTimestampInfos, uint64_t* pTimestamps, uint64_t* pMaxDeviation);

	VKAPI_ATTR VkResult VKAPI_PTR InitializePerformanceApiINTEL(const VkInitializePerformanceApiInfoINTEL* pInitializeInfo);
	VKAPI_ATTR void VKAPI_PTR UninitializePerformanceApiINTEL();
	VKAPI_ATTR VkResult VKAPI_PTR AcquirePerformanceConfigurationINTEL(const VkPerformanceConfigurationAcquireInfoINTEL* pAcquireInfo, VkPerformanceConfigurationINTEL* pConfiguration);
	VKAPI_ATTR VkResult VKAPI_PTR ReleasePerformanceConfigurationINTEL(VkPerformanceConfigurationINTEL configuration);
	VKAPI_ATTR VkResult VKAPI_PTR GetPerformanceParameterINTEL(VkPerformanceParameterTypeINTEL parameter, VkPerformanceValueINTEL* pValue);

	VKAPI_ATTR void VKAPI_PTR SetLocalDimmingAMD(VkSwapchainKHR swapChain, VkBool32 localDimmingEnable);

#if defined(VK_EXT_buffer_device_address)
	VKAPI_ATTR VkDeviceAddress VKAPI_PTR GetBufferDeviceAddress(const VkBufferDeviceAddressInfoEXT* pInfo);
#endif

	VKAPI_ATTR void VKAPI_PTR ResetQueryPool(VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount);

#if defined(VK_KHR_timeline_semaphore)
	VKAPI_ATTR VkResult VKAPI_PTR GetSemaphoreCounterValue(VkSemaphore semaphore, uint64_t* pValue);
	VKAPI_ATTR VkResult VKAPI_PTR WaitSemaphores(const VkSemaphoreWaitInfoKHR* pWaitInfo, uint64_t timeout);
	VKAPI_ATTR VkResult VKAPI_PTR SignalSemaphore(const VkSemaphoreSignalInfoKHR* pSignalInfo);
#endif

#if defined(VK_KHR_external_memory_win32)
	VKAPI_ATTR VkResult VKAPI_PTR GetMemoryWin32Handle(const VkMemoryGetWin32HandleInfoKHR* pGetWin32HandleInfo, HANDLE* pHandle);
	VKAPI_ATTR VkResult VKAPI_PTR GetMemoryWin32HandleProperties(VkExternalMemoryHandleTypeFlagBits handleType, HANDLE handle, VkMemoryWin32HandlePropertiesKHR* pMemoryWin32HandleProperties);
#endif

#if defined(VK_KHR_external_semaphore_win32)
	VKAPI_ATTR VkResult VKAPI_PTR ImportSemaphoreWin32Handle(const VkImportSemaphoreWin32HandleInfoKHR* pImportSemaphoreWin32HandleInfo);
	VKAPI_ATTR VkResult VKAPI_PTR GetSemaphoreWin32Handle(const VkSemaphoreGetWin32HandleInfoKHR* pGetWin32HandleInfo, HANDLE* pHandle);
#endif

#if defined(VK_KHR_external_fence_win32)
	VKAPI_ATTR VkResult VKAPI_PTR ImportFenceWin32Handle(const VkImportFenceWin32HandleInfoKHR* pImportFenceWin32HandleInfo);
	VKAPI_ATTR VkResult VKAPI_PTR GetFenceWin32Handle(const VkFenceGetWin32HandleInfoKHR* pGetWin32HandleInfo, HANDLE* pHandle);
#endif

#if defined(VK_NV_external_memory_win32)
	VKAPI_ATTR VkResult VKAPI_PTR GetMemoryWin32HandleNV(VkDeviceMemory memory, VkExternalMemoryHandleTypeFlagsNV handleType, HANDLE* pHandle);
#endif
								  
	VKAPI_ATTR VkResult VKAPI_PTR AcquireFullScreenExclusiveMode(VkSwapchainKHR swapchain);
	VKAPI_ATTR VkResult VKAPI_PTR ReleaseFullScreenExclusiveMode(VkSwapchainKHR swapchain);
	VKAPI_ATTR VkResult VKAPI_PTR GetDeviceGroupSurfacePresentModes2(const VkPhysicalDeviceSurfaceInfo2KHR* pSurfaceInfo, VkDeviceGroupPresentModeFlagsKHR* pModes);

	static VkResult Create(const Instance* instance, const VkDeviceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDevice* pDevice);

	[[nodiscard]] DeviceState* getState() { return state.get(); }
	[[nodiscard]] const DeviceState* getState() const { return state.get(); }

private:
	std::unique_ptr<DeviceState> state;
	ExtensionGroup enabledExtensions{};
	
	std::vector<std::unique_ptr<Queue>> queues;
};
