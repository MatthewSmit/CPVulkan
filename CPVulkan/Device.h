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
	~Device();

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
	VKAPI_ATTR void VKAPI_PTR GetDeviceMemoryCommitment(VkDeviceMemory memory, VkDeviceSize* pCommittedMemoryInBytes) { FATAL_ERROR(); } 
	VKAPI_ATTR VkResult VKAPI_PTR BindBufferMemory(VkBuffer buffer, VkDeviceMemory memory, VkDeviceSize memoryOffset);
	VKAPI_ATTR VkResult VKAPI_PTR BindImageMemory(VkImage image, VkDeviceMemory memory, VkDeviceSize memoryOffset);
	VKAPI_ATTR void VKAPI_PTR GetBufferMemoryRequirements(VkBuffer buffer, VkMemoryRequirements* pMemoryRequirements);
	VKAPI_ATTR void VKAPI_PTR GetImageMemoryRequirements(VkImage image, VkMemoryRequirements* pMemoryRequirements);
	VKAPI_ATTR void VKAPI_PTR GetImageSparseMemoryRequirements(VkImage image, uint32_t* pSparseMemoryRequirementCount, VkSparseImageMemoryRequirements* pSparseMemoryRequirements) { FATAL_ERROR(); } 
	VKAPI_ATTR VkResult VKAPI_PTR CreateFence(const VkFenceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkFence* pFence);
	VKAPI_ATTR void VKAPI_PTR DestroyFence(VkFence fence, const VkAllocationCallbacks* pAllocator);
	VKAPI_ATTR VkResult VKAPI_PTR ResetFences(uint32_t fenceCount, const VkFence* pFences) { FATAL_ERROR(); } 
	VKAPI_ATTR VkResult VKAPI_PTR GetFenceStatus(VkFence fence) { FATAL_ERROR(); } 
	VKAPI_ATTR VkResult VKAPI_PTR WaitForFences(uint32_t fenceCount, const VkFence* pFences, VkBool32 waitAll, uint64_t timeout);
	VKAPI_ATTR VkResult VKAPI_PTR CreateSemaphore(const VkSemaphoreCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSemaphore* pSemaphore);
	VKAPI_ATTR void VKAPI_PTR DestroySemaphore(VkSemaphore semaphore, const VkAllocationCallbacks* pAllocator);
	VKAPI_ATTR VkResult VKAPI_PTR CreateEvent(const VkEventCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkEvent* pEvent);
	VKAPI_ATTR void VKAPI_PTR DestroyEvent(VkEvent event, const VkAllocationCallbacks* pAllocator);
	VKAPI_ATTR VkResult VKAPI_PTR GetEventStatus(VkEvent event);
	VKAPI_ATTR VkResult VKAPI_PTR SetEvent(VkEvent event) { FATAL_ERROR(); } 
	VKAPI_ATTR VkResult VKAPI_PTR ResetEvent(VkEvent event);
	VKAPI_ATTR VkResult VKAPI_PTR CreateQueryPool(const VkQueryPoolCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkQueryPool* pQueryPool);
	VKAPI_ATTR void VKAPI_PTR DestroyQueryPool(VkQueryPool queryPool, const VkAllocationCallbacks* pAllocator);
	VKAPI_ATTR VkResult VKAPI_PTR GetQueryPoolResults(VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount, size_t dataSize, void* pData, VkDeviceSize stride, VkQueryResultFlags flags) { FATAL_ERROR(); } 
	VKAPI_ATTR VkResult VKAPI_PTR CreateBuffer(const VkBufferCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkBuffer* pBuffer);
	VKAPI_ATTR void VKAPI_PTR DestroyBuffer(VkBuffer buffer, const VkAllocationCallbacks* pAllocator);
	VKAPI_ATTR VkResult VKAPI_PTR CreateBufferView(const VkBufferViewCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkBufferView* pView);
	VKAPI_ATTR void VKAPI_PTR DestroyBufferView(VkBufferView bufferView, const VkAllocationCallbacks* pAllocator);
	VKAPI_ATTR VkResult VKAPI_PTR CreateImage(const VkImageCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkImage* pImage);
	VKAPI_ATTR void VKAPI_PTR DestroyImage(VkImage image, const VkAllocationCallbacks* pAllocator);
	VKAPI_ATTR void VKAPI_PTR GetImageSubresourceLayout(VkImage image, const VkImageSubresource* pSubresource, VkSubresourceLayout* pLayout) { FATAL_ERROR(); } 
	VKAPI_ATTR VkResult VKAPI_PTR CreateImageView(const VkImageViewCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkImageView* pView);
	VKAPI_ATTR void VKAPI_PTR DestroyImageView(VkImageView imageView, const VkAllocationCallbacks* pAllocator);
	VKAPI_ATTR VkResult VKAPI_PTR CreateShaderModule(const VkShaderModuleCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkShaderModule* pShaderModule);
	VKAPI_ATTR void VKAPI_PTR DestroyShaderModule(VkShaderModule shaderModule, const VkAllocationCallbacks* pAllocator);
	VKAPI_ATTR VkResult VKAPI_PTR CreatePipelineCache(const VkPipelineCacheCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkPipelineCache* pPipelineCache);
	VKAPI_ATTR void VKAPI_PTR DestroyPipelineCache(VkPipelineCache pipelineCache, const VkAllocationCallbacks* pAllocator);
	VKAPI_ATTR VkResult VKAPI_PTR GetPipelineCacheData(VkPipelineCache pipelineCache, size_t* pDataSize, void* pData) { FATAL_ERROR(); } 
	VKAPI_ATTR VkResult VKAPI_PTR MergePipelineCaches(VkPipelineCache dstCache, uint32_t srcCacheCount, const VkPipelineCache* pSrcCaches) { FATAL_ERROR(); } 
	VKAPI_ATTR VkResult VKAPI_PTR CreateGraphicsPipelines(VkPipelineCache pipelineCache, uint32_t createInfoCount, const VkGraphicsPipelineCreateInfo* pCreateInfos, const VkAllocationCallbacks* pAllocator, VkPipeline* pPipelines);
	VKAPI_ATTR VkResult VKAPI_PTR CreateComputePipelines(VkPipelineCache pipelineCache, uint32_t createInfoCount, const VkComputePipelineCreateInfo* pCreateInfos, const VkAllocationCallbacks* pAllocator, VkPipeline* pPipelines) { FATAL_ERROR(); } 
	VKAPI_ATTR void VKAPI_PTR DestroyPipeline(VkPipeline pipeline, const VkAllocationCallbacks* pAllocator);
	VKAPI_ATTR VkResult VKAPI_PTR CreatePipelineLayout(const VkPipelineLayoutCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkPipelineLayout* pPipelineLayout);
	VKAPI_ATTR void VKAPI_PTR DestroyPipelineLayout(VkPipelineLayout pipelineLayout, const VkAllocationCallbacks* pAllocator);
	VKAPI_ATTR VkResult VKAPI_PTR CreateSampler(const VkSamplerCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSampler* pSampler);
	VKAPI_ATTR void VKAPI_PTR DestroySampler(VkSampler sampler, const VkAllocationCallbacks* pAllocator);
	VKAPI_ATTR VkResult VKAPI_PTR CreateDescriptorSetLayout(const VkDescriptorSetLayoutCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDescriptorSetLayout* pSetLayout);
	VKAPI_ATTR void VKAPI_PTR DestroyDescriptorSetLayout(VkDescriptorSetLayout descriptorSetLayout, const VkAllocationCallbacks* pAllocator);
	VKAPI_ATTR VkResult VKAPI_PTR CreateDescriptorPool(const VkDescriptorPoolCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDescriptorPool* pDescriptorPool);
	VKAPI_ATTR void VKAPI_PTR DestroyDescriptorPool(VkDescriptorPool descriptorPool, const VkAllocationCallbacks* pAllocator);
	VKAPI_ATTR VkResult VKAPI_PTR ResetDescriptorPool(VkDescriptorPool descriptorPool, VkDescriptorPoolResetFlags flags) { FATAL_ERROR(); } 
	VKAPI_ATTR VkResult VKAPI_PTR AllocateDescriptorSets(const VkDescriptorSetAllocateInfo* pAllocateInfo, VkDescriptorSet* pDescriptorSets);
	VKAPI_ATTR VkResult VKAPI_PTR FreeDescriptorSets(VkDescriptorPool descriptorPool, uint32_t descriptorSetCount, const VkDescriptorSet* pDescriptorSets);
	VKAPI_ATTR void VKAPI_PTR UpdateDescriptorSets(uint32_t descriptorWriteCount, const VkWriteDescriptorSet* pDescriptorWrites, uint32_t descriptorCopyCount, const VkCopyDescriptorSet* pDescriptorCopies);
	VKAPI_ATTR VkResult VKAPI_PTR CreateFramebuffer(const VkFramebufferCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkFramebuffer* pFramebuffer);
	VKAPI_ATTR void VKAPI_PTR DestroyFramebuffer(VkFramebuffer framebuffer, const VkAllocationCallbacks* pAllocator);
	VKAPI_ATTR VkResult VKAPI_PTR CreateRenderPass(const VkRenderPassCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkRenderPass* pRenderPass);
	VKAPI_ATTR void VKAPI_PTR DestroyRenderPass(VkRenderPass renderPass, const VkAllocationCallbacks* pAllocator);
	VKAPI_ATTR void VKAPI_PTR GetRenderAreaGranularity(VkRenderPass renderPass, VkExtent2D* pGranularity) { FATAL_ERROR(); }
	VKAPI_ATTR VkResult VKAPI_PTR CreateCommandPool(const VkCommandPoolCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkCommandPool* pCommandPool);
	VKAPI_ATTR void VKAPI_PTR DestroyCommandPool(VkCommandPool commandPool, const VkAllocationCallbacks* pAllocator);
	VKAPI_ATTR VkResult VKAPI_PTR ResetCommandPool(VkCommandPool commandPool, VkCommandPoolResetFlags flags);
	VKAPI_ATTR VkResult VKAPI_PTR AllocateCommandBuffers(const VkCommandBufferAllocateInfo* pAllocateInfo, VkCommandBuffer* pCommandBuffers);
	VKAPI_ATTR void VKAPI_PTR FreeCommandBuffers(VkCommandPool commandPool, uint32_t commandBufferCount, const VkCommandBuffer* pCommandBuffers);
	
	VKAPI_ATTR VkResult VKAPI_PTR BindBufferMemory2(uint32_t bindInfoCount, const VkBindBufferMemoryInfo* pBindInfos) { FATAL_ERROR(); } 
	VKAPI_ATTR VkResult VKAPI_PTR BindImageMemory2(uint32_t bindInfoCount, const VkBindImageMemoryInfo* pBindInfos) { FATAL_ERROR(); } 
	VKAPI_ATTR void VKAPI_PTR GetDeviceGroupPeerMemoryFeatures(uint32_t heapIndex, uint32_t localDeviceIndex, uint32_t remoteDeviceIndex, VkPeerMemoryFeatureFlags* pPeerMemoryFeatures) { FATAL_ERROR(); } 
	VKAPI_ATTR void VKAPI_PTR GetImageMemoryRequirements2(const VkImageMemoryRequirementsInfo2* pInfo, VkMemoryRequirements2* pMemoryRequirements) { FATAL_ERROR(); } 
	VKAPI_ATTR void VKAPI_PTR GetBufferMemoryRequirements2(const VkBufferMemoryRequirementsInfo2* pInfo, VkMemoryRequirements2* pMemoryRequirements);
	VKAPI_ATTR void VKAPI_PTR GetImageSparseMemoryRequirements2(const VkImageSparseMemoryRequirementsInfo2* pInfo, uint32_t* pSparseMemoryRequirementCount, VkSparseImageMemoryRequirements2* pSparseMemoryRequirements) { FATAL_ERROR(); } 
	VKAPI_ATTR void VKAPI_PTR TrimCommandPool(VkCommandPool commandPool, VkCommandPoolTrimFlags flags);
	VKAPI_ATTR void VKAPI_PTR GetDeviceQueue2(const VkDeviceQueueInfo2* pQueueInfo, VkQueue* pQueue);
	VKAPI_ATTR VkResult VKAPI_PTR CreateSamplerYcbcrConversion(const VkSamplerYcbcrConversionCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSamplerYcbcrConversion* pYcbcrConversion) { FATAL_ERROR(); } 
	VKAPI_ATTR void VKAPI_PTR DestroySamplerYcbcrConversion(VkSamplerYcbcrConversion ycbcrConversion, const VkAllocationCallbacks* pAllocator) { FATAL_ERROR(); } 
	VKAPI_ATTR VkResult VKAPI_PTR CreateDescriptorUpdateTemplate(const VkDescriptorUpdateTemplateCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDescriptorUpdateTemplate* pDescriptorUpdateTemplate) { FATAL_ERROR(); } 
	VKAPI_ATTR void VKAPI_PTR DestroyDescriptorUpdateTemplate(VkDescriptorUpdateTemplate descriptorUpdateTemplate, const VkAllocationCallbacks* pAllocator) { FATAL_ERROR(); } 
	VKAPI_ATTR void VKAPI_PTR UpdateDescriptorSetWithTemplate(VkDescriptorSet descriptorSet, VkDescriptorUpdateTemplate descriptorUpdateTemplate, const void* pData) { FATAL_ERROR(); } 
	VKAPI_ATTR void VKAPI_PTR GetDescriptorSetLayoutSupport(const VkDescriptorSetLayoutCreateInfo* pCreateInfo, VkDescriptorSetLayoutSupport* pSupport) { FATAL_ERROR(); } 

	VKAPI_ATTR VkResult VKAPI_PTR CreateSwapchain(const VkSwapchainCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSwapchainKHR* pSwapchain);
	VKAPI_ATTR void VKAPI_PTR DestroySwapchain(VkSwapchainKHR swapchain, const VkAllocationCallbacks* pAllocator);
	VKAPI_ATTR VkResult VKAPI_PTR GetSwapchainImages(VkSwapchainKHR swapchain, uint32_t* pSwapchainImageCount, VkImage* pSwapchainImages);
	VKAPI_ATTR VkResult VKAPI_PTR AcquireNextImage(VkSwapchainKHR swapchain, uint64_t timeout, VkSemaphore semaphore, VkFence fence, uint32_t* pImageIndex);
	VKAPI_ATTR VkResult VKAPI_PTR GetDeviceGroupPresentCapabilities(VkDeviceGroupPresentCapabilitiesKHR* pDeviceGroupPresentCapabilities) { FATAL_ERROR(); } 
	VKAPI_ATTR VkResult VKAPI_PTR GetDeviceGroupSurfacePresentModes(VkSurfaceKHR surface, VkDeviceGroupPresentModeFlagsKHR* pModes) { FATAL_ERROR(); } 
	VKAPI_ATTR VkResult VKAPI_PTR AcquireNextImage2(const VkAcquireNextImageInfoKHR* pAcquireInfo, uint32_t* pImageIndex) { FATAL_ERROR(); } 

	VKAPI_ATTR VkResult VKAPI_PTR CreateSharedSwapchains(uint32_t swapchainCount, const VkSwapchainCreateInfoKHR* pCreateInfos, const VkAllocationCallbacks* pAllocator, VkSwapchainKHR* pSwapchains) { FATAL_ERROR(); } 

	VKAPI_ATTR VkResult VKAPI_PTR GetMemoryFd(const VkMemoryGetFdInfoKHR* pGetFdInfo, int* pFd) { FATAL_ERROR(); } 
	VKAPI_ATTR VkResult VKAPI_PTR GetMemoryFdProperties(VkExternalMemoryHandleTypeFlagBits handleType, int fd, VkMemoryFdPropertiesKHR* pMemoryFdProperties) { FATAL_ERROR(); } 

	VKAPI_ATTR VkResult VKAPI_PTR ImportSemaphoreFd(const VkImportSemaphoreFdInfoKHR* pImportSemaphoreFdInfo) { FATAL_ERROR(); } 
	VKAPI_ATTR VkResult VKAPI_PTR GetSemaphoreFd(const VkSemaphoreGetFdInfoKHR* pGetFdInfo, int* pFd) { FATAL_ERROR(); } 

	VKAPI_ATTR VkResult VKAPI_PTR CreateRenderPass2(const VkRenderPassCreateInfo2KHR* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkRenderPass* pRenderPass) { FATAL_ERROR(); } 
	
	VKAPI_ATTR VkResult VKAPI_PTR GetSwapchainStatus(VkSwapchainKHR swapchain) { FATAL_ERROR(); } 

	VKAPI_ATTR VkResult VKAPI_PTR ImportFenceFd(const VkImportFenceFdInfoKHR* pImportFenceFdInfo) { FATAL_ERROR(); } 
	VKAPI_ATTR VkResult VKAPI_PTR GetFenceFd(const VkFenceGetFdInfoKHR* pGetFdInfo, int* pFd) { FATAL_ERROR(); }

	VKAPI_ATTR VkResult VKAPI_PTR DebugMarkerSetObjectTag(const VkDebugMarkerObjectTagInfoEXT* pTagInfo) { FATAL_ERROR(); } 
	VKAPI_ATTR VkResult VKAPI_PTR DebugMarkerSetObjectName(const VkDebugMarkerObjectNameInfoEXT* pNameInfo) { FATAL_ERROR(); } 
	
	VKAPI_ATTR uint32_t VKAPI_PTR GetImageViewHandleNVX(const VkImageViewHandleInfoNVX* pInfo) { FATAL_ERROR(); } 

	VKAPI_ATTR VkResult VKAPI_PTR GetShaderInfoAMD(VkPipeline pipeline, VkShaderStageFlagBits shaderStage, VkShaderInfoTypeAMD infoType, size_t* pInfoSize, void* pInfo) { FATAL_ERROR(); } 

	VKAPI_ATTR VkResult VKAPI_PTR CreateIndirectCommandsLayoutNVX(const VkIndirectCommandsLayoutCreateInfoNVX* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkIndirectCommandsLayoutNVX* pIndirectCommandsLayout) { FATAL_ERROR(); } 
	VKAPI_ATTR void VKAPI_PTR DestroyIndirectCommandsLayoutNVX(VkIndirectCommandsLayoutNVX indirectCommandsLayout, const VkAllocationCallbacks* pAllocator) { FATAL_ERROR(); } 
	VKAPI_ATTR VkResult VKAPI_PTR CreateObjectTableNVX(const VkObjectTableCreateInfoNVX* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkObjectTableNVX* pObjectTable) { FATAL_ERROR(); } 
	VKAPI_ATTR void VKAPI_PTR DestroyObjectTableNVX(VkObjectTableNVX objectTable, const VkAllocationCallbacks* pAllocator) { FATAL_ERROR(); } 
	VKAPI_ATTR VkResult VKAPI_PTR RegisterObjectsNVX(VkObjectTableNVX objectTable, uint32_t objectCount, const VkObjectTableEntryNVX* const* ppObjectTableEntries, const uint32_t* pObjectIndices) { FATAL_ERROR(); } 
	VKAPI_ATTR VkResult VKAPI_PTR UnregisterObjectsNVX(VkObjectTableNVX objectTable, uint32_t objectCount, const VkObjectEntryTypeNVX* pObjectEntryTypes, const uint32_t* pObjectIndices) { FATAL_ERROR(); } 
	
	VKAPI_ATTR VkResult VKAPI_PTR DisplayPowerControl(VkDisplayKHR display, const VkDisplayPowerInfoEXT* pDisplayPowerInfo) { FATAL_ERROR(); } 
	VKAPI_ATTR VkResult VKAPI_PTR RegisterDeviceEvent(const VkDeviceEventInfoEXT* pDeviceEventInfo, const VkAllocationCallbacks* pAllocator, VkFence* pFence) { FATAL_ERROR(); } 
	VKAPI_ATTR VkResult VKAPI_PTR RegisterDisplayEvent(VkDisplayKHR display, const VkDisplayEventInfoEXT* pDisplayEventInfo, const VkAllocationCallbacks* pAllocator, VkFence* pFence) { FATAL_ERROR(); } 
	VKAPI_ATTR VkResult VKAPI_PTR GetSwapchainCounter(VkSwapchainKHR swapchain, VkSurfaceCounterFlagBitsEXT counter, uint64_t* pCounterValue) { FATAL_ERROR(); } 

	VKAPI_ATTR VkResult VKAPI_PTR GetRefreshCycleDurationGOOGLE(VkSwapchainKHR swapchain, VkRefreshCycleDurationGOOGLE* pDisplayTimingProperties) { FATAL_ERROR(); } 
	VKAPI_ATTR VkResult VKAPI_PTR GetPastPresentationTimingGOOGLE(VkSwapchainKHR swapchain, uint32_t* pPresentationTimingCount, VkPastPresentationTimingGOOGLE* pPresentationTimings) { FATAL_ERROR(); } 

	VKAPI_ATTR void VKAPI_PTR SetHdrMetadata(uint32_t swapchainCount, const VkSwapchainKHR* pSwapchains, const VkHdrMetadataEXT* pMetadata) { FATAL_ERROR(); } 

	VKAPI_ATTR VkResult VKAPI_PTR SetDebugUtilsObjectName(const VkDebugUtilsObjectNameInfoEXT* pNameInfo) { FATAL_ERROR(); } 
	VKAPI_ATTR VkResult VKAPI_PTR SetDebugUtilsObjectTag(const VkDebugUtilsObjectTagInfoEXT* pTagInfo) { FATAL_ERROR(); } 
	
	VKAPI_ATTR VkResult VKAPI_PTR GetImageDrmFormatModifierProperties(VkImage image, VkImageDrmFormatModifierPropertiesEXT* pProperties) { FATAL_ERROR(); } 

	VKAPI_ATTR VkResult VKAPI_PTR CreateValidationCache(const VkValidationCacheCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkValidationCacheEXT* pValidationCache) { FATAL_ERROR(); } 
	VKAPI_ATTR void VKAPI_PTR DestroyValidationCache(VkValidationCacheEXT validationCache, const VkAllocationCallbacks* pAllocator) { FATAL_ERROR(); } 
	VKAPI_ATTR VkResult VKAPI_PTR MergeValidationCaches(VkValidationCacheEXT dstCache, uint32_t srcCacheCount, const VkValidationCacheEXT* pSrcCaches) { FATAL_ERROR(); } 
	VKAPI_ATTR VkResult VKAPI_PTR GetValidationCacheData(VkValidationCacheEXT validationCache, size_t* pDataSize, void* pData) { FATAL_ERROR(); } 

	VKAPI_ATTR VkResult VKAPI_PTR CreateAccelerationStructureNV(const VkAccelerationStructureCreateInfoNV* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkAccelerationStructureNV* pAccelerationStructure) { FATAL_ERROR(); } 
	VKAPI_ATTR void VKAPI_PTR DestroyAccelerationStructureNV(VkAccelerationStructureNV accelerationStructure, const VkAllocationCallbacks* pAllocator) { FATAL_ERROR(); } 
	VKAPI_ATTR void VKAPI_PTR GetAccelerationStructureMemoryRequirementsNV(const VkAccelerationStructureMemoryRequirementsInfoNV* pInfo, VkMemoryRequirements2KHR* pMemoryRequirements) { FATAL_ERROR(); } 
	VKAPI_ATTR VkResult VKAPI_PTR BindAccelerationStructureMemoryNV(uint32_t bindInfoCount, const VkBindAccelerationStructureMemoryInfoNV* pBindInfos) { FATAL_ERROR(); } 
	VKAPI_ATTR VkResult VKAPI_PTR CreateRayTracingPipelinesNV(VkPipelineCache pipelineCache, uint32_t createInfoCount, const VkRayTracingPipelineCreateInfoNV* pCreateInfos, const VkAllocationCallbacks* pAllocator, VkPipeline* pPipelines) { FATAL_ERROR(); } 
	VKAPI_ATTR VkResult VKAPI_PTR GetRayTracingShaderGroupHandlesNV(VkPipeline pipeline, uint32_t firstGroup, uint32_t groupCount, size_t dataSize, void* pData) { FATAL_ERROR(); } 
	VKAPI_ATTR VkResult VKAPI_PTR GetAccelerationStructureHandleNV(VkAccelerationStructureNV accelerationStructure, size_t dataSize, void* pData) { FATAL_ERROR(); } 
	VKAPI_ATTR VkResult VKAPI_PTR CompileDeferredNV(VkPipeline pipeline, uint32_t shader) { FATAL_ERROR(); } 

	VKAPI_ATTR VkResult VKAPI_PTR GetMemoryHostPointerProperties(VkExternalMemoryHandleTypeFlagBits handleType, const void* pHostPointer, VkMemoryHostPointerPropertiesEXT* pMemoryHostPointerProperties) { FATAL_ERROR(); } 

	VKAPI_ATTR VkResult VKAPI_PTR GetCalibratedTimestamps(uint32_t timestampCount, const VkCalibratedTimestampInfoEXT* pTimestampInfos, uint64_t* pTimestamps, uint64_t* pMaxDeviation) { FATAL_ERROR(); } 

	VKAPI_ATTR VkResult VKAPI_PTR InitializePerformanceApiINTEL(const VkInitializePerformanceApiInfoINTEL* pInitializeInfo) { FATAL_ERROR(); } 
	VKAPI_ATTR void VKAPI_PTR UninitializePerformanceApiINTEL() { FATAL_ERROR(); } 
	VKAPI_ATTR VkResult VKAPI_PTR AcquirePerformanceConfigurationINTEL(const VkPerformanceConfigurationAcquireInfoINTEL* pAcquireInfo, VkPerformanceConfigurationINTEL* pConfiguration) { FATAL_ERROR(); } 
	VKAPI_ATTR VkResult VKAPI_PTR ReleasePerformanceConfigurationINTEL(VkPerformanceConfigurationINTEL configuration) { FATAL_ERROR(); } 
	VKAPI_ATTR VkResult VKAPI_PTR GetPerformanceParameterINTEL(VkPerformanceParameterTypeINTEL parameter, VkPerformanceValueINTEL* pValue) { FATAL_ERROR(); } 

	VKAPI_ATTR void VKAPI_PTR SetLocalDimmingAMD(VkSwapchainKHR swapChain, VkBool32 localDimmingEnable) { FATAL_ERROR(); } 

	VKAPI_ATTR VkDeviceAddress VKAPI_PTR GetBufferDeviceAddress(const VkBufferDeviceAddressInfoEXT* pInfo) { FATAL_ERROR(); } 

	VKAPI_ATTR void VKAPI_PTR ResetQueryPoolEXT(VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount) { FATAL_ERROR(); }

#if defined(VK_USE_PLATFORM_WIN32_KHR)
	VKAPI_ATTR VkResult VKAPI_PTR GetMemoryWin32Handle(const VkMemoryGetWin32HandleInfoKHR* pGetWin32HandleInfo, HANDLE* pHandle) { FATAL_ERROR(); }
	VKAPI_ATTR VkResult VKAPI_PTR GetMemoryWin32HandleProperties(VkExternalMemoryHandleTypeFlagBits handleType, HANDLE handle, VkMemoryWin32HandlePropertiesKHR* pMemoryWin32HandleProperties) { FATAL_ERROR(); }
								  
	VKAPI_ATTR VkResult VKAPI_PTR ImportSemaphoreWin32Handle(const VkImportSemaphoreWin32HandleInfoKHR* pImportSemaphoreWin32HandleInfo) { FATAL_ERROR(); }
	VKAPI_ATTR VkResult VKAPI_PTR GetSemaphoreWin32Handle(const VkSemaphoreGetWin32HandleInfoKHR* pGetWin32HandleInfo, HANDLE* pHandle) { FATAL_ERROR(); }
								  
	VKAPI_ATTR VkResult VKAPI_PTR ImportFenceWin32Handle(const VkImportFenceWin32HandleInfoKHR* pImportFenceWin32HandleInfo) { FATAL_ERROR(); }
	VKAPI_ATTR VkResult VKAPI_PTR GetFenceWin32Handle(const VkFenceGetWin32HandleInfoKHR* pGetWin32HandleInfo, HANDLE* pHandle) { FATAL_ERROR(); }
								  
	VKAPI_ATTR VkResult VKAPI_PTR GetMemoryWin32HandleNV(VkDeviceMemory memory, VkExternalMemoryHandleTypeFlagsNV handleType, HANDLE* pHandle) { FATAL_ERROR(); }
								  
	VKAPI_ATTR VkResult VKAPI_PTR AcquireFullScreenExclusiveMode(VkSwapchainKHR swapchain) { FATAL_ERROR(); }
	VKAPI_ATTR VkResult VKAPI_PTR ReleaseFullScreenExclusiveMode(VkSwapchainKHR swapchain) { FATAL_ERROR(); }
	VKAPI_ATTR VkResult VKAPI_PTR GetDeviceGroupSurfacePresentModes2(const VkPhysicalDeviceSurfaceInfo2KHR* pSurfaceInfo, VkDeviceGroupPresentModeFlagsKHR* pModes) { FATAL_ERROR(); }
#endif

	static VkResult Create(Instance* instance, const VkDeviceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDevice* pDevice);

private:
	void* magic;
	std::unique_ptr<DeviceState> state;
	ExtensionGroup enabledExtensions{};
	
	std::vector<std::unique_ptr<Queue>> queues;
};
