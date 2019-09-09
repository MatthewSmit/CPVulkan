#include "DescriptorPool.h"

#include <cassert>

VkResult DescriptorPool::Create(const VkDescriptorPoolCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDescriptorPool* pDescriptorPool)
{
	assert(pCreateInfo->sType == VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO);

	auto descriptorPool = Allocate<DescriptorPool>(pAllocator);

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

	// TODO

	*pDescriptorPool = reinterpret_cast<VkDescriptorPool>(descriptorPool);
	return VK_SUCCESS;
}
