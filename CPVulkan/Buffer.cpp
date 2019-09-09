#include "Buffer.h"

#include <cassert>

VkResult Buffer::BindMemory(VkDeviceMemory memory, uint64_t memoryOffset)
{
	// TODO: Bounds check
	data = reinterpret_cast<DeviceMemory*>(memory)->Data + memoryOffset;
	return VK_SUCCESS;
}

void Buffer::GetMemoryRequirements(VkMemoryRequirements* pMemoryRequirements)
{
	pMemoryRequirements->size = size;
	pMemoryRequirements->alignment = 16;
	pMemoryRequirements->memoryTypeBits = 1;
}

VkResult Buffer::Create(const VkBufferCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkBuffer* pBuffer)
{
	assert(pCreateInfo->sType == VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO);

	auto buffer = Allocate<Buffer>(pAllocator);

	auto next = pCreateInfo->pNext;
	while (next)
	{
		auto type = *static_cast<const VkStructureType*>(next);
		switch (type)
		{
		default:
			FATAL_ERROR();
		}
	}

	if (pCreateInfo->flags)
	{
		FATAL_ERROR();
	}

	buffer->size = pCreateInfo->size;
	buffer->usage = pCreateInfo->usage;
	
	if (pCreateInfo->sharingMode != VK_SHARING_MODE_EXCLUSIVE)
	{
		FATAL_ERROR();
	}
	
	*pBuffer = reinterpret_cast<VkBuffer>(buffer);
	return VK_SUCCESS;
}
