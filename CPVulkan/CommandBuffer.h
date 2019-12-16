#pragma once
#include "Base.h"

#include <memory>
#include <vector>

struct DeviceState;

class Command;
class ExecuteCommandsCommand;

enum class State
{
	Initial,
	Recording,
	Executable,
	Pending,
	Invalid,
};

class CommandBuffer final
{
public:
	CommandBuffer(DeviceState* deviceState, VkCommandBufferLevel level, VkCommandPoolCreateFlags poolFlags);
	~CommandBuffer();

	void OnDelete(const VkAllocationCallbacks*)
	{
	}

	VKAPI_ATTR VkResult VKAPI_PTR Begin(const VkCommandBufferBeginInfo* pBeginInfo);
	VKAPI_ATTR VkResult VKAPI_PTR End();
	VKAPI_ATTR VkResult VKAPI_PTR Reset(VkCommandBufferResetFlags flags);
	VKAPI_ATTR void VKAPI_PTR BindPipeline(VkPipelineBindPoint pipelineBindPoint, VkPipeline pipeline);
	VKAPI_ATTR void VKAPI_PTR SetViewport(uint32_t firstViewport, uint32_t viewportCount, const VkViewport* pViewports);
	VKAPI_ATTR void VKAPI_PTR SetScissor(uint32_t firstScissor, uint32_t scissorCount, const VkRect2D* pScissors);
	VKAPI_ATTR void VKAPI_PTR SetLineWidth(float lineWidth) { TODO_ERROR(); } 
	VKAPI_ATTR void VKAPI_PTR SetDepthBias(float depthBiasConstantFactor, float depthBiasClamp, float depthBiasSlopeFactor) { TODO_ERROR(); } 
	VKAPI_ATTR void VKAPI_PTR SetBlendConstants(const float blendConstants[4]) { TODO_ERROR(); } 
	VKAPI_ATTR void VKAPI_PTR SetDepthBounds(float minDepthBounds, float maxDepthBounds) { TODO_ERROR(); } 
	VKAPI_ATTR void VKAPI_PTR SetStencilCompareMask(VkStencilFaceFlags faceMask, uint32_t compareMask) { TODO_ERROR(); } 
	VKAPI_ATTR void VKAPI_PTR SetStencilWriteMask(VkStencilFaceFlags faceMask, uint32_t writeMask) { TODO_ERROR(); } 
	VKAPI_ATTR void VKAPI_PTR SetStencilReference(VkStencilFaceFlags faceMask, uint32_t reference) { TODO_ERROR(); } 
	VKAPI_ATTR void VKAPI_PTR BindDescriptorSets(VkPipelineBindPoint pipelineBindPoint, VkPipelineLayout layout, uint32_t firstSet, uint32_t descriptorSetCount, const VkDescriptorSet* pDescriptorSets, uint32_t dynamicOffsetCount, const uint32_t* pDynamicOffsets);
	VKAPI_ATTR void VKAPI_PTR BindIndexBuffer(VkBuffer buffer, VkDeviceSize offset, VkIndexType indexType);
	VKAPI_ATTR void VKAPI_PTR BindVertexBuffers(uint32_t firstBinding, uint32_t bindingCount, const VkBuffer* pBuffers, const VkDeviceSize* pOffsets);
	VKAPI_ATTR void VKAPI_PTR Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance);
	VKAPI_ATTR void VKAPI_PTR DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance);
	VKAPI_ATTR void VKAPI_PTR DrawIndirect(VkBuffer buffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride);
	VKAPI_ATTR void VKAPI_PTR DrawIndexedIndirect(VkBuffer buffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride);
	VKAPI_ATTR void VKAPI_PTR Dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ);
	VKAPI_ATTR void VKAPI_PTR DispatchIndirect(VkBuffer buffer, VkDeviceSize offset) { TODO_ERROR(); } 
	VKAPI_ATTR void VKAPI_PTR CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, uint32_t regionCount, const VkBufferCopy* pRegions);
	VKAPI_ATTR void VKAPI_PTR CopyImage(VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageCopy* pRegions);
	VKAPI_ATTR void VKAPI_PTR BlitImage(VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageBlit* pRegions, VkFilter filter);
	VKAPI_ATTR void VKAPI_PTR CopyBufferToImage(VkBuffer srcBuffer, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkBufferImageCopy* pRegions);
	VKAPI_ATTR void VKAPI_PTR CopyImageToBuffer(VkImage srcImage, VkImageLayout srcImageLayout, VkBuffer dstBuffer, uint32_t regionCount, const VkBufferImageCopy* pRegions);
	VKAPI_ATTR void VKAPI_PTR UpdateBuffer(VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize dataSize, const void* pData);
	VKAPI_ATTR void VKAPI_PTR FillBuffer(VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize size, uint32_t data);
	VKAPI_ATTR void VKAPI_PTR ClearColorImage(VkImage image, VkImageLayout imageLayout, const VkClearColorValue* pColor, uint32_t rangeCount, const VkImageSubresourceRange* pRanges);
	VKAPI_ATTR void VKAPI_PTR ClearDepthStencilImage(VkImage image, VkImageLayout imageLayout, const VkClearDepthStencilValue* pDepthStencil, uint32_t rangeCount, const VkImageSubresourceRange* pRanges);
	VKAPI_ATTR void VKAPI_PTR ClearAttachments(uint32_t attachmentCount, const VkClearAttachment* pAttachments, uint32_t rectCount, const VkClearRect* pRects);
	VKAPI_ATTR void VKAPI_PTR ResolveImage(VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageResolve* pRegions);
	VKAPI_ATTR void VKAPI_PTR SetEvent(VkEvent event, VkPipelineStageFlags stageMask);
	VKAPI_ATTR void VKAPI_PTR ResetEvent(VkEvent event, VkPipelineStageFlags stageMask);
	VKAPI_ATTR void VKAPI_PTR WaitEvents(uint32_t eventCount, const VkEvent* pEvents, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, uint32_t memoryBarrierCount, const VkMemoryBarrier* pMemoryBarriers, uint32_t bufferMemoryBarrierCount, const VkBufferMemoryBarrier* pBufferMemoryBarriers, uint32_t imageMemoryBarrierCount, const VkImageMemoryBarrier* pImageMemoryBarriers);
	VKAPI_ATTR void VKAPI_PTR PipelineBarrier(VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags, uint32_t memoryBarrierCount, const VkMemoryBarrier* pMemoryBarriers, uint32_t bufferMemoryBarrierCount, const VkBufferMemoryBarrier* pBufferMemoryBarriers, uint32_t imageMemoryBarrierCount, const VkImageMemoryBarrier* pImageMemoryBarriers);
	VKAPI_ATTR void VKAPI_PTR BeginQuery(VkQueryPool queryPool, uint32_t query, VkQueryControlFlags flags);
	VKAPI_ATTR void VKAPI_PTR EndQuery(VkQueryPool queryPool, uint32_t query);
	VKAPI_ATTR void VKAPI_PTR ResetQueryPool(VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount);
	VKAPI_ATTR void VKAPI_PTR WriteTimestamp(VkPipelineStageFlagBits pipelineStage, VkQueryPool queryPool, uint32_t query);
	VKAPI_ATTR void VKAPI_PTR CopyQueryPoolResults(VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount, VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize stride, VkQueryResultFlags flags);
	VKAPI_ATTR void VKAPI_PTR PushConstants(VkPipelineLayout layout, VkShaderStageFlags stageFlags, uint32_t offset, uint32_t size, const void* pValues);
	VKAPI_ATTR void VKAPI_PTR BeginRenderPass(const VkRenderPassBeginInfo* pRenderPassBegin, VkSubpassContents contents);
	VKAPI_ATTR void VKAPI_PTR NextSubpass(VkSubpassContents contents);
	VKAPI_ATTR void VKAPI_PTR EndRenderPass();
	VKAPI_ATTR void VKAPI_PTR ExecuteCommands(uint32_t commandBufferCount, const VkCommandBuffer* pCommandBuffers);

	VKAPI_ATTR void VKAPI_PTR SetDeviceMask(uint32_t deviceMask) { TODO_ERROR(); } 
	VKAPI_ATTR void VKAPI_PTR DispatchBase(uint32_t baseGroupX, uint32_t baseGroupY, uint32_t baseGroupZ, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) { TODO_ERROR(); } 

#if defined(VK_KHR_push_descriptor)
	VKAPI_ATTR void VKAPI_PTR PushDescriptorSet(VkPipelineBindPoint pipelineBindPoint, VkPipelineLayout layout, uint32_t set, uint32_t descriptorWriteCount, const VkWriteDescriptorSet* pDescriptorWrites);
	VKAPI_ATTR void VKAPI_PTR PushDescriptorSetWithTemplate(VkDescriptorUpdateTemplate descriptorUpdateTemplate, VkPipelineLayout layout, uint32_t set, const void* pData) { TODO_ERROR(); }
#endif

#if defined(VK_KHR_create_renderpass2)
	VKAPI_ATTR void VKAPI_PTR BeginRenderPass2(const VkRenderPassBeginInfo* pRenderPassBegin, const VkSubpassBeginInfoKHR* pSubpassBeginInfo) { TODO_ERROR(); } 
	VKAPI_ATTR void VKAPI_PTR NextSubpass2(const VkSubpassBeginInfoKHR* pSubpassBeginInfo, const VkSubpassEndInfoKHR* pSubpassEndInfo) { TODO_ERROR(); } 
	VKAPI_ATTR void VKAPI_PTR EndRenderPass2(const VkSubpassEndInfoKHR* pSubpassEndInfo) { TODO_ERROR(); }
#endif

#if defined(VK_KHR_draw_indirect_count) || defined(VK_AMD_draw_indirect_count)
	VKAPI_ATTR void VKAPI_PTR DrawIndirectCount(VkBuffer buffer, VkDeviceSize offset, VkBuffer countBuffer, VkDeviceSize countBufferOffset, uint32_t maxDrawCount, uint32_t stride);
	VKAPI_ATTR void VKAPI_PTR DrawIndexedIndirectCount(VkBuffer buffer, VkDeviceSize offset, VkBuffer countBuffer, VkDeviceSize countBufferOffset, uint32_t maxDrawCount, uint32_t stride);
#endif

	VKAPI_ATTR void VKAPI_PTR DebugMarkerBegin(const VkDebugMarkerMarkerInfoEXT* pMarkerInfo) { TODO_ERROR(); } 
	VKAPI_ATTR void VKAPI_PTR DebugMarkerEnd() { TODO_ERROR(); } 
	VKAPI_ATTR void VKAPI_PTR DebugMarkerInsert(const VkDebugMarkerMarkerInfoEXT* pMarkerInfo) { TODO_ERROR(); } 

	VKAPI_ATTR void VKAPI_PTR BindTransformFeedbackBuffers(uint32_t firstBinding, uint32_t bindingCount, const VkBuffer* pBuffers, const VkDeviceSize* pOffsets, const VkDeviceSize* pSizes) { TODO_ERROR(); } 
	VKAPI_ATTR void VKAPI_PTR BeginTransformFeedback(uint32_t firstCounterBuffer, uint32_t counterBufferCount, const VkBuffer* pCounterBuffers, const VkDeviceSize* pCounterBufferOffsets) { TODO_ERROR(); } 
	VKAPI_ATTR void VKAPI_PTR EndTransformFeedback(uint32_t firstCounterBuffer, uint32_t counterBufferCount, const VkBuffer* pCounterBuffers, const VkDeviceSize* pCounterBufferOffsets) { TODO_ERROR(); } 
	VKAPI_ATTR void VKAPI_PTR BeginQueryIndexed(VkQueryPool queryPool, uint32_t query, VkQueryControlFlags flags, uint32_t index) { TODO_ERROR(); } 
	VKAPI_ATTR void VKAPI_PTR EndQueryIndexed(VkQueryPool queryPool, uint32_t query, uint32_t index) { TODO_ERROR(); } 
	VKAPI_ATTR void VKAPI_PTR DrawIndirectByteCount(uint32_t instanceCount, uint32_t firstInstance, VkBuffer counterBuffer, VkDeviceSize counterBufferOffset, uint32_t counterOffset, uint32_t vertexStride) { TODO_ERROR(); } 

	VKAPI_ATTR void VKAPI_PTR BeginConditionalRendering(const VkConditionalRenderingBeginInfoEXT* pConditionalRenderingBegin) { TODO_ERROR(); } 
	VKAPI_ATTR void VKAPI_PTR EndConditionalRendering() { TODO_ERROR(); } 

	VKAPI_ATTR void VKAPI_PTR ProcessCommands(const VkCmdProcessCommandsInfoNVX* pProcessCommandsInfo) { TODO_ERROR(); } 
	VKAPI_ATTR void VKAPI_PTR ReserveSpaceForCommands(const VkCmdReserveSpaceForCommandsInfoNVX* pReserveSpaceInfo) { TODO_ERROR(); } 
	
	VKAPI_ATTR void VKAPI_PTR SetViewportWScaling(uint32_t firstViewport, uint32_t viewportCount, const VkViewportWScalingNV* pViewportWScalings) { TODO_ERROR(); } 

	VKAPI_ATTR void VKAPI_PTR SetDiscardRectangle(uint32_t firstDiscardRectangle, uint32_t discardRectangleCount, const VkRect2D* pDiscardRectangles) { TODO_ERROR(); } 

	VKAPI_ATTR void VKAPI_PTR BeginDebugUtilsLabel(const VkDebugUtilsLabelEXT* pLabelInfo) { TODO_ERROR(); } 
	VKAPI_ATTR void VKAPI_PTR EndDebugUtilsLabel() { TODO_ERROR(); } 
	VKAPI_ATTR void VKAPI_PTR InsertDebugUtilsLabel(const VkDebugUtilsLabelEXT* pLabelInfo) { TODO_ERROR(); } 
	
	VKAPI_ATTR void VKAPI_PTR SetSampleLocations(const VkSampleLocationsInfoEXT* pSampleLocationsInfo) { TODO_ERROR(); } 
	
	VKAPI_ATTR void VKAPI_PTR BindShadingRateImage(VkImageView imageView, VkImageLayout imageLayout) { TODO_ERROR(); } 
	VKAPI_ATTR void VKAPI_PTR SetViewportShadingRatePalette(uint32_t firstViewport, uint32_t viewportCount, const VkShadingRatePaletteNV* pShadingRatePalettes) { TODO_ERROR(); } 
	VKAPI_ATTR void VKAPI_PTR SetCoarseSampleOrder(VkCoarseSampleOrderTypeNV sampleOrderType, uint32_t customSampleOrderCount, const VkCoarseSampleOrderCustomNV* pCustomSampleOrders) { TODO_ERROR(); } 

	VKAPI_ATTR void VKAPI_PTR BuildAccelerationStructure(const VkAccelerationStructureInfoNV* pInfo, VkBuffer instanceData, VkDeviceSize instanceOffset, VkBool32 update, VkAccelerationStructureNV dst, VkAccelerationStructureNV src, VkBuffer scratch, VkDeviceSize scratchOffset) { TODO_ERROR(); } 
	VKAPI_ATTR void VKAPI_PTR CopyAccelerationStructure(VkAccelerationStructureNV dst, VkAccelerationStructureNV src, VkCopyAccelerationStructureModeNV mode) { TODO_ERROR(); } 
	VKAPI_ATTR void VKAPI_PTR TraceRays(VkBuffer raygenShaderBindingTableBuffer, VkDeviceSize raygenShaderBindingOffset, VkBuffer missShaderBindingTableBuffer, VkDeviceSize missShaderBindingOffset, VkDeviceSize missShaderBindingStride, VkBuffer hitShaderBindingTableBuffer, VkDeviceSize hitShaderBindingOffset, VkDeviceSize hitShaderBindingStride, VkBuffer callableShaderBindingTableBuffer, VkDeviceSize callableShaderBindingOffset, VkDeviceSize callableShaderBindingStride, uint32_t width, uint32_t height, uint32_t depth) { TODO_ERROR(); } 
	VKAPI_ATTR void VKAPI_PTR WriteAccelerationStructuresProperties(uint32_t accelerationStructureCount, const VkAccelerationStructureNV* pAccelerationStructures, VkQueryType queryType, VkQueryPool queryPool, uint32_t firstQuery) { TODO_ERROR(); } 
	
	VKAPI_ATTR void VKAPI_PTR WriteBufferMarker(VkPipelineStageFlagBits pipelineStage, VkBuffer dstBuffer, VkDeviceSize dstOffset, uint32_t marker) { TODO_ERROR(); } 

	VKAPI_ATTR void VKAPI_PTR DrawMeshTasks(uint32_t taskCount, uint32_t firstTask) { TODO_ERROR(); } 
	VKAPI_ATTR void VKAPI_PTR DrawMeshTasksIndirect(VkBuffer buffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride) { TODO_ERROR(); } 
	VKAPI_ATTR void VKAPI_PTR DrawMeshTasksIndirectCount(VkBuffer buffer, VkDeviceSize offset, VkBuffer countBuffer, VkDeviceSize countBufferOffset, uint32_t maxDrawCount, uint32_t stride) { TODO_ERROR(); } 

	VKAPI_ATTR void VKAPI_PTR SetExclusiveScissor(uint32_t firstExclusiveScissor, uint32_t exclusiveScissorCount, const VkRect2D* pExclusiveScissors) { TODO_ERROR(); } 

	VKAPI_ATTR void VKAPI_PTR SetCheckpoint(const void* pCheckpointMarker) { TODO_ERROR(); } 

	VKAPI_ATTR VkResult VKAPI_PTR SetPerformanceMarker(const VkPerformanceMarkerInfoINTEL* pMarkerInfo) { TODO_ERROR(); } 
	VKAPI_ATTR VkResult VKAPI_PTR SetPerformanceStreamMarker(const VkPerformanceStreamMarkerInfoINTEL* pMarkerInfo) { TODO_ERROR(); } 
	VKAPI_ATTR VkResult VKAPI_PTR SetPerformanceOverride(const VkPerformanceOverrideInfoINTEL* pOverrideInfo) { TODO_ERROR(); }

	VKAPI_ATTR void VKAPI_PTR SetLineStipple(uint32_t lineStippleFactor, uint16_t lineStipplePattern) { TODO_ERROR(); }

	void ForceReset();
	VkResult Submit();

	friend class ExecuteCommandsCommand;

private:
	DeviceState* deviceState{};
	VkCommandBufferLevel level;
	VkCommandPoolCreateFlags poolFlags;
	
	State state{State::Initial};
	std::vector<std::unique_ptr<Command>> commands;
};