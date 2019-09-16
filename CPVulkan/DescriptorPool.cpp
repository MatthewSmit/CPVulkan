#include "DescriptorPool.h"

#include "Device.h"

#include <cassert>

VkResult DescriptorPool::Create(const VkDescriptorPoolCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDescriptorPool* pDescriptorPool)
{
	assert(pCreateInfo->sType == VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO);

	auto descriptorPool = Allocate<DescriptorPool>(pAllocator, VK_SYSTEM_ALLOCATION_SCOPE_DEVICE);

	auto next = pCreateInfo->pNext;
	while (next)
	{
		const auto type = static_cast<const VkBaseInStructure*>(next)->sType;
		switch (type)
		{
		case VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_INLINE_UNIFORM_BLOCK_CREATE_INFO_EXT:
			FATAL_ERROR();
		}
		next = static_cast<const VkBaseInStructure*>(next)->pNext;
	}

	*pDescriptorPool = reinterpret_cast<VkDescriptorPool>(descriptorPool);
	return VK_SUCCESS;
}

VkResult Device::CreateDescriptorPool(const VkDescriptorPoolCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDescriptorPool* pDescriptorPool)
{
	return DescriptorPool::Create(pCreateInfo, pAllocator, pDescriptorPool);
}

void Device::DestroyDescriptorPool(VkDescriptorPool descriptorPool, const VkAllocationCallbacks* pAllocator)
{
	Free(reinterpret_cast<DescriptorPool*>(descriptorPool), pAllocator);
}
