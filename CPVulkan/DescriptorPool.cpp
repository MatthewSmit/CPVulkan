#include "DescriptorPool.h"

#include "Device.h"

#include <cassert>

VkResult DescriptorPool::Reset(VkFlags flags)
{
	FATAL_ERROR();
}

VkResult Device::ResetDescriptorPool(VkDescriptorPool descriptorPool, VkDescriptorPoolResetFlags flags)
{
	return UnwrapVulkan<DescriptorPool>(descriptorPool)->Reset(flags);
}

VkResult DescriptorPool::Create(const VkDescriptorPoolCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDescriptorPool* pDescriptorPool)
{
	assert(pCreateInfo->sType == VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO);

	const auto descriptorPool = Allocate<DescriptorPool>(pAllocator, VK_SYSTEM_ALLOCATION_SCOPE_DEVICE);
	if (!descriptorPool)
	{
		return VK_ERROR_OUT_OF_HOST_MEMORY;
	}

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

	WrapVulkan(descriptorPool, pDescriptorPool);
	return VK_SUCCESS;
}

VkResult Device::CreateDescriptorPool(const VkDescriptorPoolCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDescriptorPool* pDescriptorPool)
{
	return DescriptorPool::Create(pCreateInfo, pAllocator, pDescriptorPool);
}

void Device::DestroyDescriptorPool(VkDescriptorPool descriptorPool, const VkAllocationCallbacks* pAllocator)
{
	Free(UnwrapVulkan<DescriptorPool>(descriptorPool), pAllocator);
}
