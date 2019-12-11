#include "DescriptorPool.h"

#include "DescriptorSet.h"
#include "Device.h"

#include <cassert>

DescriptorPool::~DescriptorPool()
{
	Reset();
}

VkResult DescriptorPool::Reset()
{
	for (auto descriptorSet : usedSets)
	{
		Free(descriptorSet, allocator);
	}
	usedSets.clear();
	return VK_SUCCESS;
}

DescriptorSet* DescriptorPool::createDescriptorPool()
{
	// TODO: Lock?
	const auto descriptorSet = Allocate<DescriptorSet>(allocator, VK_SYSTEM_ALLOCATION_SCOPE_DEVICE);
	usedSets.push_back(descriptorSet);
	return descriptorSet;
}

void DescriptorPool::freeDescriptorPool(DescriptorSet* descriptorSet)
{
	// TODO: Lock?
	const auto element = std::find(usedSets.begin(), usedSets.end(), descriptorSet);
	if (element == usedSets.end())
	{
		FATAL_ERROR();
	}
	usedSets.erase(element);
	Free(descriptorSet, allocator);
}

VkResult Device::ResetDescriptorPool(VkDescriptorPool descriptorPool, VkDescriptorPoolResetFlags)
{
	return UnwrapVulkan<DescriptorPool>(descriptorPool)->Reset();
}

VkResult DescriptorPool::Create(const VkDescriptorPoolCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDescriptorPool* pDescriptorPool)
{
	assert(pCreateInfo->sType == VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO);

	const auto descriptorPool = Allocate<DescriptorPool>(pAllocator, VK_SYSTEM_ALLOCATION_SCOPE_DEVICE);
	if (!descriptorPool)
	{
		return VK_ERROR_OUT_OF_HOST_MEMORY;
	}

	auto next = static_cast<const VkBaseInStructure*>(pCreateInfo->pNext);
	while (next)
	{
		const auto type = next->sType;
		switch (type)
		{
		case VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_INLINE_UNIFORM_BLOCK_CREATE_INFO_EXT:
			TODO_ERROR();
		}
		next = next->pNext;
	}

	descriptorPool->allocator = pAllocator;

	WrapVulkan(descriptorPool, pDescriptorPool);
	return VK_SUCCESS;
}

VkResult Device::CreateDescriptorPool(const VkDescriptorPoolCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDescriptorPool* pDescriptorPool)
{
	return DescriptorPool::Create(pCreateInfo, pAllocator, pDescriptorPool);
}

void Device::DestroyDescriptorPool(VkDescriptorPool descriptorPool, const VkAllocationCallbacks* pAllocator)
{
	if (descriptorPool)
	{
		// TODO: Destroying a pool object implicitly frees all objects allocated from that pool. Specifically, destroying VkCommandPool frees all VkCommandBuffer objects that were allocated from it, and destroying VkDescriptorPool frees all VkDescriptorSet objects that were allocated from it.
		Free(UnwrapVulkan<DescriptorPool>(descriptorPool), pAllocator);
	}
}
