#include "Buffer.h"

#include "Device.h"

#include <cassert>

VkResult Buffer::BindMemory(VkDeviceMemory memory, uint64_t memoryOffset)
{
	// TODO: Bounds check
	data = reinterpret_cast<DeviceMemory*>(memory)->Data + memoryOffset;
	return VK_SUCCESS;
}

VkResult Device::BindBufferMemory(VkBuffer buffer, VkDeviceMemory memory, VkDeviceSize memoryOffset)
{
	return reinterpret_cast<Buffer*>(buffer)->BindMemory(memory, memoryOffset);
}

void Buffer::GetMemoryRequirements(VkMemoryRequirements* pMemoryRequirements) const
{
	pMemoryRequirements->size = size;
	pMemoryRequirements->alignment = 16;
	pMemoryRequirements->memoryTypeBits = 1;
}

void Buffer::GetMemoryRequirements(VkMemoryRequirements2* pMemoryRequirements) const
{
	auto next = pMemoryRequirements->pNext;
	while (next)
	{
		const auto type = static_cast<const VkBaseInStructure*>(next)->sType;
		switch (type)
		{
		case VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS:
			{
				auto memoryRequirements = reinterpret_cast<VkMemoryDedicatedRequirements*>(next);
				memoryRequirements->prefersDedicatedAllocation = false;
				memoryRequirements->requiresDedicatedAllocation = false;
				break;
			}
		}
		next = static_cast<VkMemoryRequirements2*>(next)->pNext;
	}

	GetMemoryRequirements(&pMemoryRequirements->memoryRequirements);
}

void Device::GetBufferMemoryRequirements(VkBuffer buffer, VkMemoryRequirements* pMemoryRequirements)
{
	reinterpret_cast<Buffer*>(buffer)->GetMemoryRequirements(pMemoryRequirements);
}

void Device::GetBufferMemoryRequirements2(const VkBufferMemoryRequirementsInfo2* pInfo, VkMemoryRequirements2* pMemoryRequirements)
{
	assert(pInfo->sType == VK_STRUCTURE_TYPE_BUFFER_MEMORY_REQUIREMENTS_INFO_2);
	assert(pMemoryRequirements->sType == VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2);

	reinterpret_cast<Buffer*>(pInfo->buffer)->GetMemoryRequirements(pMemoryRequirements);
}

VkResult Buffer::Create(gsl::not_null<const VkBufferCreateInfo*> pCreateInfo, const VkAllocationCallbacks* pAllocator, gsl::not_null<VkBuffer*> pBuffer)
{
	assert(pCreateInfo->sType == VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO);

	auto next = pCreateInfo->pNext;
	while (next)
	{
		const auto type = static_cast<const VkBaseInStructure*>(next)->sType;
		switch (type)
		{
		case VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_CREATE_INFO_EXT:
			FATAL_ERROR();
			
		case VK_STRUCTURE_TYPE_DEDICATED_ALLOCATION_BUFFER_CREATE_INFO_NV:
			FATAL_ERROR();
			
		case VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_BUFFER_CREATE_INFO:
			FATAL_ERROR();
		}
		next = static_cast<const VkBaseInStructure*>(next)->pNext;
	}

	Buffer* buffer;

	if (pCreateInfo->flags & VK_BUFFER_CREATE_SPARSE_BINDING_BIT)
	{
		buffer = Allocate<SparseBuffer>(pAllocator, VK_SYSTEM_ALLOCATION_SCOPE_DEVICE);
		const auto pages = (pCreateInfo->size + 4095) / 4096;
		reinterpret_cast<SparseBuffer*>(buffer)->setPages(pages);
	}
	else
	{
		buffer = Allocate<Buffer>(pAllocator, VK_SYSTEM_ALLOCATION_SCOPE_DEVICE);
	}

	buffer->flags = pCreateInfo->flags;
	buffer->size = pCreateInfo->size;
	buffer->usage = pCreateInfo->usage;
	
	if (pCreateInfo->sharingMode != VK_SHARING_MODE_EXCLUSIVE)
	{
		FATAL_ERROR();
	}
	
	*pBuffer = reinterpret_cast<VkBuffer>(buffer);
	return VK_SUCCESS;
}

VkResult SparseBuffer::BindMemory(VkDeviceMemory memory, uint64_t memoryOffset)
{
	FATAL_ERROR();
}

void SparseBuffer::SparseBindMemory(uint32_t bindCount, const VkSparseMemoryBind* pBinds)
{
	for (auto i = 0u; i < bindCount; i++)
	{
		const auto& bind = pBinds[i];

		if (bind.flags)
		{
			FATAL_ERROR();
		}
		
		auto page = (bind.resourceOffset + 4095) / 4096;
		for (auto j = 0u; j < bind.size; j += 4096)
		{
			if (bind.memory)
			{
				this->pointers[page] = reinterpret_cast<DeviceMemory*>(bind.memory)->Data + bind.memoryOffset;
			}
			else
			{
				this->pointers[page] = nullptr;
			}
			page++;
		}
	}
}

void SparseBuffer::GetMemoryRequirements(VkMemoryRequirements* pMemoryRequirements) const
{
	pMemoryRequirements->size = (size + 4095) / 4096 * 4096;
	pMemoryRequirements->alignment = 4096;
	pMemoryRequirements->memoryTypeBits = 1;
}

uint8_t* SparseBuffer::getData(uint64_t offset, uint64_t size) const noexcept
{
	FATAL_ERROR();
}

VkResult Device::CreateBuffer(const VkBufferCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkBuffer* pBuffer)
{
	return Buffer::Create(pCreateInfo, pAllocator, pBuffer);
}

void Device::DestroyBuffer(VkBuffer buffer, const VkAllocationCallbacks* pAllocator)
{
	Free(reinterpret_cast<Buffer*>(buffer), pAllocator);
}
