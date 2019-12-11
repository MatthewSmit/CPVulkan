#include "Buffer.h"

#include "Device.h"
#include "Util.h"

#include <cassert>

VkResult Buffer::BindMemory(DeviceMemory* memory, uint64_t memoryOffset)
{
	const auto memorySpan = memory->getSpan();
	data = memorySpan.subspan(memoryOffset, size);
	return VK_SUCCESS;
}

VkResult Device::BindBufferMemory(VkBuffer buffer, VkDeviceMemory memory, VkDeviceSize memoryOffset)
{
	return UnwrapVulkan<Buffer>(buffer)->BindMemory(UnwrapVulkan<DeviceMemory>(memory), memoryOffset);
}

void Buffer::GetMemoryRequirements(VkMemoryRequirements* pMemoryRequirements) const
{
	pMemoryRequirements->size = size;
	if (usage & (VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT | 
		VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT |
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT))
	{
		pMemoryRequirements->alignment = 256;
	}
	else
	{
		pMemoryRequirements->alignment = 16;
	}
	pMemoryRequirements->memoryTypeBits = 1;
}

void Buffer::GetMemoryRequirements(VkMemoryRequirements2* pMemoryRequirements) const
{
	auto next = static_cast<VkBaseOutStructure*>(pMemoryRequirements->pNext);
	while (next)
	{
		const auto type = next->sType;
		switch (type)
		{
		case VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS:
			{
				const auto memoryRequirements = reinterpret_cast<VkMemoryDedicatedRequirements*>(next);
				memoryRequirements->prefersDedicatedAllocation = false;
				memoryRequirements->requiresDedicatedAllocation = false;
				break;
			}
			
		default:
			break;
		}
		next = next->pNext;
	}

	GetMemoryRequirements(&pMemoryRequirements->memoryRequirements);
}

void Device::GetBufferMemoryRequirements(VkBuffer buffer, VkMemoryRequirements* pMemoryRequirements) const
{
	UnwrapVulkan<Buffer>(buffer)->GetMemoryRequirements(pMemoryRequirements);
}

void Device::GetBufferMemoryRequirements2(const VkBufferMemoryRequirementsInfo2* pInfo, VkMemoryRequirements2* pMemoryRequirements) const
{
	assert(pInfo->sType == VK_STRUCTURE_TYPE_BUFFER_MEMORY_REQUIREMENTS_INFO_2);
	assert(pMemoryRequirements->sType == VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2);

	UnwrapVulkan<Buffer>(pInfo->buffer)->GetMemoryRequirements(pMemoryRequirements);
}

VkResult Buffer::Create(gsl::not_null<const VkBufferCreateInfo*> pCreateInfo, const VkAllocationCallbacks* pAllocator, VkBuffer* pBuffer)
{
	assert(pCreateInfo->sType == VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO);

	auto next = static_cast<const VkBaseInStructure*>(pCreateInfo->pNext);
	while (next)
	{
		const auto type = next->sType;
		switch (type)
		{
		case VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_CREATE_INFO_EXT:
			TODO_ERROR();
			
		case VK_STRUCTURE_TYPE_DEDICATED_ALLOCATION_BUFFER_CREATE_INFO_NV:
			TODO_ERROR();
			
		case VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_BUFFER_CREATE_INFO:
			break;

		default:
			break;
		}
		next = next->pNext;
	}

	if (pCreateInfo->flags & VK_BUFFER_CREATE_SPARSE_BINDING_BIT)
	{
		TODO_ERROR();
	}

	const auto buffer = Allocate<Buffer>(pAllocator, VK_SYSTEM_ALLOCATION_SCOPE_DEVICE);
	if (!buffer)
	{
		return VK_ERROR_OUT_OF_HOST_MEMORY;
	}

	buffer->flags = pCreateInfo->flags;
	buffer->size = pCreateInfo->size;
	buffer->usage = pCreateInfo->usage;
	
	if (pCreateInfo->sharingMode != VK_SHARING_MODE_EXCLUSIVE)
	{
		TODO_ERROR();
	}
	
	WrapVulkan(buffer, pBuffer);
	return VK_SUCCESS;
}

VkResult Device::CreateBuffer(const VkBufferCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkBuffer* pBuffer)
{
	return Buffer::Create(pCreateInfo, pAllocator, pBuffer);
}

void Device::DestroyBuffer(VkBuffer buffer, const VkAllocationCallbacks* pAllocator)
{
	if (buffer)
	{
		Free(UnwrapVulkan<Buffer>(buffer), pAllocator);
	}
}

VkResult Device::BindBufferMemory2(uint32_t bindInfoCount, const VkBindBufferMemoryInfo* pBindInfos)
{
	for (auto i = 0u; i < bindInfoCount; i++)
	{
		assert(pBindInfos[i].sType == VK_STRUCTURE_TYPE_BIND_BUFFER_MEMORY_INFO);
		
		auto next = static_cast<const VkBaseInStructure*>(pBindInfos[i].pNext);
		while (next)
		{
			const auto type = next->sType;
			switch (type)
			{
			case VK_STRUCTURE_TYPE_BIND_BUFFER_MEMORY_DEVICE_GROUP_INFO:
				TODO_ERROR();

			default:
				break;
			}
			next = next->pNext;
		}

		const auto result = UnwrapVulkan<Buffer>(pBindInfos[i].buffer)->BindMemory(UnwrapVulkan<DeviceMemory>(pBindInfos[i].memory), pBindInfos[i].memoryOffset);
		if (result != VK_SUCCESS)
		{
			return result;
		}
	}

	return VK_SUCCESS;
}

#if defined(VK_EXT_buffer_device_address)
VkDeviceAddress Device::GetBufferDeviceAddress(const VkBufferDeviceAddressInfoEXT* pInfo)
{
	return reinterpret_cast<VkDeviceAddress>(UnwrapVulkan<Buffer>(pInfo->buffer)->getData().data());
}
#endif