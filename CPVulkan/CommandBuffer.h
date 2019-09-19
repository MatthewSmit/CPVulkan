#pragma once
#include "Base.h"

#include <memory>

struct DeviceState;

class Command
{
public:
	virtual ~Command() = default;
	
	virtual void Process(DeviceState* deviceState) = 0;
};

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

	VKAPI_ATTR VkResult VKAPI_PTR Begin(const VkCommandBufferBeginInfo* pBeginInfo);
	VKAPI_ATTR VkResult VKAPI_PTR End();
	VKAPI_ATTR VkResult VKAPI_PTR Reset(VkCommandBufferResetFlags flags);
	VKAPI_ATTR void VKAPI_PTR BindPipeline(VkPipelineBindPoint pipelineBindPoint, VkPipeline pipeline);
	VKAPI_ATTR void VKAPI_PTR SetViewport(uint32_t firstViewport, uint32_t viewportCount, const VkViewport* pViewports);
	VKAPI_ATTR void VKAPI_PTR SetScissor(uint32_t firstScissor, uint32_t scissorCount, const VkRect2D* pScissors);
	VKAPI_ATTR void VKAPI_PTR SetLineWidth(float lineWidth) { FATAL_ERROR(); } 
	VKAPI_ATTR void VKAPI_PTR SetDepthBias(float depthBiasConstantFactor, float depthBiasClamp, float depthBiasSlopeFactor) { FATAL_ERROR(); } 
	VKAPI_ATTR void VKAPI_PTR SetBlendConstants(const float blendConstants[4]) { FATAL_ERROR(); } 
	VKAPI_ATTR void VKAPI_PTR SetDepthBounds(float minDepthBounds, float maxDepthBounds) { FATAL_ERROR(); } 
	VKAPI_ATTR void VKAPI_PTR SetStencilCompareMask(VkStencilFaceFlags faceMask, uint32_t compareMask) { FATAL_ERROR(); } 
	VKAPI_ATTR void VKAPI_PTR SetStencilWriteMask(VkStencilFaceFlags faceMask, uint32_t writeMask) { FATAL_ERROR(); } 
	VKAPI_ATTR void VKAPI_PTR SetStencilReference(VkStencilFaceFlags faceMask, uint32_t reference) { FATAL_ERROR(); } 
	VKAPI_ATTR void VKAPI_PTR BindDescriptorSets(VkPipelineBindPoint pipelineBindPoint, VkPipelineLayout layout, uint32_t firstSet, uint32_t descriptorSetCount, const VkDescriptorSet* pDescriptorSets, uint32_t dynamicOffsetCount, const uint32_t* pDynamicOffsets);
	VKAPI_ATTR void VKAPI_PTR BindIndexBuffer(VkBuffer buffer, VkDeviceSize offset, VkIndexType indexType) { FATAL_ERROR(); } 
	VKAPI_ATTR void VKAPI_PTR BindVertexBuffers(uint32_t firstBinding, uint32_t bindingCount, const VkBuffer* pBuffers, const VkDeviceSize* pOffsets);
	VKAPI_ATTR void VKAPI_PTR Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance);
	VKAPI_ATTR void VKAPI_PTR DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) { FATAL_ERROR(); } 
	VKAPI_ATTR void VKAPI_PTR DrawIndirect(VkBuffer buffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride) { FATAL_ERROR(); } 
	VKAPI_ATTR void VKAPI_PTR DrawIndexedIndirect(VkBuffer buffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride) { FATAL_ERROR(); } 
	VKAPI_ATTR void VKAPI_PTR Dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) { FATAL_ERROR(); } 
	VKAPI_ATTR void VKAPI_PTR DispatchIndirect(VkBuffer buffer, VkDeviceSize offset) { FATAL_ERROR(); } 
	VKAPI_ATTR void VKAPI_PTR CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, uint32_t regionCount, const VkBufferCopy* pRegions) { FATAL_ERROR(); } 
	VKAPI_ATTR void VKAPI_PTR CopyImage(VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageCopy* pRegions);
	VKAPI_ATTR void VKAPI_PTR BlitImage(VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageBlit* pRegions, VkFilter filter) { FATAL_ERROR(); } 
	VKAPI_ATTR void VKAPI_PTR CopyBufferToImage(VkBuffer srcBuffer, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkBufferImageCopy* pRegions) { FATAL_ERROR(); } 
	VKAPI_ATTR void VKAPI_PTR CopyImageToBuffer(VkImage srcImage, VkImageLayout srcImageLayout, VkBuffer dstBuffer, uint32_t regionCount, const VkBufferImageCopy* pRegions);
	VKAPI_ATTR void VKAPI_PTR UpdateBuffer(VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize dataSize, const void* pData) { FATAL_ERROR(); } 
	VKAPI_ATTR void VKAPI_PTR FillBuffer(VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize size, uint32_t data) { FATAL_ERROR(); } 
	VKAPI_ATTR void VKAPI_PTR ClearColorImage(VkImage image, VkImageLayout imageLayout, const VkClearColorValue* pColor, uint32_t rangeCount, const VkImageSubresourceRange* pRanges);
	VKAPI_ATTR void VKAPI_PTR ClearDepthStencilImage(VkImage image, VkImageLayout imageLayout, const VkClearDepthStencilValue* pDepthStencil, uint32_t rangeCount, const VkImageSubresourceRange* pRanges) { FATAL_ERROR(); } 
	VKAPI_ATTR void VKAPI_PTR ClearAttachments(uint32_t attachmentCount, const VkClearAttachment* pAttachments, uint32_t rectCount, const VkClearRect* pRects);
	VKAPI_ATTR void VKAPI_PTR ResolveImage(VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageResolve* pRegions) { FATAL_ERROR(); } 
	VKAPI_ATTR void VKAPI_PTR SetEvent(VkEvent event, VkPipelineStageFlags stageMask);
	VKAPI_ATTR void VKAPI_PTR ResetEvent(VkEvent event, VkPipelineStageFlags stageMask);
	VKAPI_ATTR void VKAPI_PTR WaitEvents(uint32_t eventCount, const VkEvent* pEvents, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, uint32_t memoryBarrierCount, const VkMemoryBarrier* pMemoryBarriers, uint32_t bufferMemoryBarrierCount, const VkBufferMemoryBarrier* pBufferMemoryBarriers, uint32_t imageMemoryBarrierCount, const VkImageMemoryBarrier* pImageMemoryBarriers);
	VKAPI_ATTR void VKAPI_PTR PipelineBarrier(VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags, uint32_t memoryBarrierCount, const VkMemoryBarrier* pMemoryBarriers, uint32_t bufferMemoryBarrierCount, const VkBufferMemoryBarrier* pBufferMemoryBarriers, uint32_t imageMemoryBarrierCount, const VkImageMemoryBarrier* pImageMemoryBarriers);
	VKAPI_ATTR void VKAPI_PTR BeginQuery(VkQueryPool queryPool, uint32_t query, VkQueryControlFlags flags) { FATAL_ERROR(); } 
	VKAPI_ATTR void VKAPI_PTR EndQuery(VkQueryPool queryPool, uint32_t query) { FATAL_ERROR(); } 
	VKAPI_ATTR void VKAPI_PTR ResetQueryPool(VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount) { FATAL_ERROR(); } 
	VKAPI_ATTR void VKAPI_PTR WriteTimestamp(VkPipelineStageFlagBits pipelineStage, VkQueryPool queryPool, uint32_t query) { FATAL_ERROR(); } 
	VKAPI_ATTR void VKAPI_PTR CopyQueryPoolResults(VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount, VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize stride, VkQueryResultFlags flags) { FATAL_ERROR(); } 
	VKAPI_ATTR void VKAPI_PTR PushConstants(VkPipelineLayout layout, VkShaderStageFlags stageFlags, uint32_t offset, uint32_t size, const void* pValues) { FATAL_ERROR(); } 
	VKAPI_ATTR void VKAPI_PTR BeginRenderPass(const VkRenderPassBeginInfo* pRenderPassBegin, VkSubpassContents contents);
	VKAPI_ATTR void VKAPI_PTR NextSubpass(VkSubpassContents contents) { FATAL_ERROR(); } 
	VKAPI_ATTR void VKAPI_PTR EndRenderPass();
	VKAPI_ATTR void VKAPI_PTR ExecuteCommands(uint32_t commandBufferCount, const VkCommandBuffer* pCommandBuffers);

	VKAPI_ATTR void VKAPI_PTR SetDeviceMask(uint32_t deviceMask) { FATAL_ERROR(); } 
	VKAPI_ATTR void VKAPI_PTR DispatchBase(uint32_t baseGroupX, uint32_t baseGroupY, uint32_t baseGroupZ, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) { FATAL_ERROR(); } 

	VKAPI_ATTR void VKAPI_PTR PushDescriptorSet(VkPipelineBindPoint pipelineBindPoint, VkPipelineLayout layout, uint32_t set, uint32_t descriptorWriteCount, const VkWriteDescriptorSet* pDescriptorWrites) { FATAL_ERROR(); } 
	VKAPI_ATTR void VKAPI_PTR PushDescriptorSetWithTemplate(VkDescriptorUpdateTemplate descriptorUpdateTemplate, VkPipelineLayout layout, uint32_t set, const void* pData) { FATAL_ERROR(); } 

	VKAPI_ATTR void VKAPI_PTR BeginRenderPass2(const VkRenderPassBeginInfo* pRenderPassBegin, const VkSubpassBeginInfoKHR* pSubpassBeginInfo) { FATAL_ERROR(); } 
	VKAPI_ATTR void VKAPI_PTR NextSubpass2(const VkSubpassBeginInfoKHR* pSubpassBeginInfo, const VkSubpassEndInfoKHR* pSubpassEndInfo) { FATAL_ERROR(); } 
	VKAPI_ATTR void VKAPI_PTR EndRenderPass2(const VkSubpassEndInfoKHR* pSubpassEndInfo) { FATAL_ERROR(); } 

	VKAPI_ATTR void VKAPI_PTR DrawIndirectCount(VkBuffer buffer, VkDeviceSize offset, VkBuffer countBuffer, VkDeviceSize countBufferOffset, uint32_t maxDrawCount, uint32_t stride) { FATAL_ERROR(); } 
	VKAPI_ATTR void VKAPI_PTR DrawIndexedIndirectCount(VkBuffer buffer, VkDeviceSize offset, VkBuffer countBuffer, VkDeviceSize countBufferOffset, uint32_t maxDrawCount, uint32_t stride) { FATAL_ERROR(); } 

	VKAPI_ATTR void VKAPI_PTR DebugMarkerBegin(const VkDebugMarkerMarkerInfoEXT* pMarkerInfo) { FATAL_ERROR(); } 
	VKAPI_ATTR void VKAPI_PTR DebugMarkerEnd() { FATAL_ERROR(); } 
	VKAPI_ATTR void VKAPI_PTR DebugMarkerInsert(const VkDebugMarkerMarkerInfoEXT* pMarkerInfo) { FATAL_ERROR(); } 

	VKAPI_ATTR void VKAPI_PTR BindTransformFeedbackBuffers(uint32_t firstBinding, uint32_t bindingCount, const VkBuffer* pBuffers, const VkDeviceSize* pOffsets, const VkDeviceSize* pSizes) { FATAL_ERROR(); } 
	VKAPI_ATTR void VKAPI_PTR BeginTransformFeedback(uint32_t firstCounterBuffer, uint32_t counterBufferCount, const VkBuffer* pCounterBuffers, const VkDeviceSize* pCounterBufferOffsets) { FATAL_ERROR(); } 
	VKAPI_ATTR void VKAPI_PTR EndTransformFeedback(uint32_t firstCounterBuffer, uint32_t counterBufferCount, const VkBuffer* pCounterBuffers, const VkDeviceSize* pCounterBufferOffsets) { FATAL_ERROR(); } 
	VKAPI_ATTR void VKAPI_PTR BeginQueryIndexed(VkQueryPool queryPool, uint32_t query, VkQueryControlFlags flags, uint32_t index) { FATAL_ERROR(); } 
	VKAPI_ATTR void VKAPI_PTR EndQueryIndexed(VkQueryPool queryPool, uint32_t query, uint32_t index) { FATAL_ERROR(); } 
	VKAPI_ATTR void VKAPI_PTR DrawIndirectByteCount(uint32_t instanceCount, uint32_t firstInstance, VkBuffer counterBuffer, VkDeviceSize counterBufferOffset, uint32_t counterOffset, uint32_t vertexStride) { FATAL_ERROR(); } 

	VKAPI_ATTR void VKAPI_PTR BeginConditionalRendering(const VkConditionalRenderingBeginInfoEXT* pConditionalRenderingBegin) { FATAL_ERROR(); } 
	VKAPI_ATTR void VKAPI_PTR EndConditionalRendering() { FATAL_ERROR(); } 

	VKAPI_ATTR void VKAPI_PTR ProcessCommands(const VkCmdProcessCommandsInfoNVX* pProcessCommandsInfo) { FATAL_ERROR(); } 
	VKAPI_ATTR void VKAPI_PTR ReserveSpaceForCommands(const VkCmdReserveSpaceForCommandsInfoNVX* pReserveSpaceInfo) { FATAL_ERROR(); } 
	
	VKAPI_ATTR void VKAPI_PTR SetViewportWScaling(uint32_t firstViewport, uint32_t viewportCount, const VkViewportWScalingNV* pViewportWScalings) { FATAL_ERROR(); } 

	VKAPI_ATTR void VKAPI_PTR SetDiscardRectangle(uint32_t firstDiscardRectangle, uint32_t discardRectangleCount, const VkRect2D* pDiscardRectangles) { FATAL_ERROR(); } 

	VKAPI_ATTR void VKAPI_PTR BeginDebugUtilsLabel(const VkDebugUtilsLabelEXT* pLabelInfo) { FATAL_ERROR(); } 
	VKAPI_ATTR void VKAPI_PTR EndDebugUtilsLabel() { FATAL_ERROR(); } 
	VKAPI_ATTR void VKAPI_PTR InsertDebugUtilsLabel(const VkDebugUtilsLabelEXT* pLabelInfo) { FATAL_ERROR(); } 
	
	VKAPI_ATTR void VKAPI_PTR SetSampleLocations(const VkSampleLocationsInfoEXT* pSampleLocationsInfo) { FATAL_ERROR(); } 
	
	VKAPI_ATTR void VKAPI_PTR BindShadingRateImage(VkImageView imageView, VkImageLayout imageLayout) { FATAL_ERROR(); } 
	VKAPI_ATTR void VKAPI_PTR SetViewportShadingRatePalette(uint32_t firstViewport, uint32_t viewportCount, const VkShadingRatePaletteNV* pShadingRatePalettes) { FATAL_ERROR(); } 
	VKAPI_ATTR void VKAPI_PTR SetCoarseSampleOrder(VkCoarseSampleOrderTypeNV sampleOrderType, uint32_t customSampleOrderCount, const VkCoarseSampleOrderCustomNV* pCustomSampleOrders) { FATAL_ERROR(); } 

	VKAPI_ATTR void VKAPI_PTR BuildAccelerationStructure(const VkAccelerationStructureInfoNV* pInfo, VkBuffer instanceData, VkDeviceSize instanceOffset, VkBool32 update, VkAccelerationStructureNV dst, VkAccelerationStructureNV src, VkBuffer scratch, VkDeviceSize scratchOffset) { FATAL_ERROR(); } 
	VKAPI_ATTR void VKAPI_PTR CopyAccelerationStructure(VkAccelerationStructureNV dst, VkAccelerationStructureNV src, VkCopyAccelerationStructureModeNV mode) { FATAL_ERROR(); } 
	VKAPI_ATTR void VKAPI_PTR TraceRays(VkBuffer raygenShaderBindingTableBuffer, VkDeviceSize raygenShaderBindingOffset, VkBuffer missShaderBindingTableBuffer, VkDeviceSize missShaderBindingOffset, VkDeviceSize missShaderBindingStride, VkBuffer hitShaderBindingTableBuffer, VkDeviceSize hitShaderBindingOffset, VkDeviceSize hitShaderBindingStride, VkBuffer callableShaderBindingTableBuffer, VkDeviceSize callableShaderBindingOffset, VkDeviceSize callableShaderBindingStride, uint32_t width, uint32_t height, uint32_t depth) { FATAL_ERROR(); } 
	VKAPI_ATTR void VKAPI_PTR WriteAccelerationStructuresProperties(uint32_t accelerationStructureCount, const VkAccelerationStructureNV* pAccelerationStructures, VkQueryType queryType, VkQueryPool queryPool, uint32_t firstQuery) { FATAL_ERROR(); } 
	
	VKAPI_ATTR void VKAPI_PTR WriteBufferMarker(VkPipelineStageFlagBits pipelineStage, VkBuffer dstBuffer, VkDeviceSize dstOffset, uint32_t marker) { FATAL_ERROR(); } 

	VKAPI_ATTR void VKAPI_PTR DrawMeshTasks(uint32_t taskCount, uint32_t firstTask) { FATAL_ERROR(); } 
	VKAPI_ATTR void VKAPI_PTR DrawMeshTasksIndirect(VkBuffer buffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride) { FATAL_ERROR(); } 
	VKAPI_ATTR void VKAPI_PTR DrawMeshTasksIndirectCount(VkBuffer buffer, VkDeviceSize offset, VkBuffer countBuffer, VkDeviceSize countBufferOffset, uint32_t maxDrawCount, uint32_t stride) { FATAL_ERROR(); } 

	VKAPI_ATTR void VKAPI_PTR SetExclusiveScissor(uint32_t firstExclusiveScissor, uint32_t exclusiveScissorCount, const VkRect2D* pExclusiveScissors) { FATAL_ERROR(); } 

	VKAPI_ATTR void VKAPI_PTR SetCheckpoint(const void* pCheckpointMarker) { FATAL_ERROR(); } 

	VKAPI_ATTR VkResult VKAPI_PTR SetPerformanceMarker(const VkPerformanceMarkerInfoINTEL* pMarkerInfo) { FATAL_ERROR(); } 
	VKAPI_ATTR VkResult VKAPI_PTR SetPerformanceStreamMarker(const VkPerformanceStreamMarkerInfoINTEL* pMarkerInfo) { FATAL_ERROR(); } 
	VKAPI_ATTR VkResult VKAPI_PTR SetPerformanceOverride(const VkPerformanceOverrideInfoINTEL* pOverrideInfo) { FATAL_ERROR(); }

	VKAPI_ATTR void VKAPI_PTR SetLineStipple(uint32_t lineStippleFactor, uint16_t lineStipplePattern) { FATAL_ERROR(); }

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