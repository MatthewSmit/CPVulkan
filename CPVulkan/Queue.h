#pragma once
#include "Base.h"

class Queue final
{
public:
	Queue() = default;
	Queue(const Queue&) = delete;
	Queue(Queue&&) = delete;
	~Queue() = default;

	Queue& operator=(const Queue&) = delete;
	Queue&& operator=(const Queue&&) = delete;

	VKAPI_ATTR VkResult VKAPI_PTR Submit(uint32_t submitCount, const VkSubmitInfo* pSubmits, VkFence fence);
	VKAPI_ATTR VkResult VKAPI_PTR WaitIdle();
	VKAPI_ATTR VkResult VKAPI_PTR BindSparse(uint32_t bindInfoCount, const VkBindSparseInfo* pBindInfo, VkFence fence);
	
	VKAPI_ATTR VkResult VKAPI_PTR Present(const VkPresentInfoKHR* pPresentInfo);
	
	VKAPI_ATTR void VKAPI_PTR BeginDebugUtilsLabel(const VkDebugUtilsLabelEXT* pLabelInfo) { FATAL_ERROR(); } 
	VKAPI_ATTR void VKAPI_PTR EndDebugUtilsLabel() { FATAL_ERROR(); } 
	VKAPI_ATTR void VKAPI_PTR InsertDebugUtilsLabel(const VkDebugUtilsLabelEXT* pLabelInfo) { FATAL_ERROR(); } 
	
	VKAPI_ATTR void VKAPI_PTR GetCheckpointData(uint32_t* pCheckpointDataCount, VkCheckpointDataNV* pCheckpointData) { FATAL_ERROR(); } 

	VKAPI_ATTR VkResult VKAPI_PTR SetPerformanceConfiguration(VkPerformanceConfigurationINTEL configuration) { FATAL_ERROR(); }

	static Queue* Create(const VkDeviceQueueCreateInfo& vkDeviceQueueCreateInfo, const VkAllocationCallbacks* pAllocator);

	[[nodiscard]] VkDeviceQueueCreateFlags getFlags() const { return flags; }

private:
	VkDeviceQueueCreateFlags flags{};
};