#pragma once
#include "Base.h"

class DescriptorPool final
{
public:
	DescriptorPool() = default;
	DescriptorPool(const DescriptorPool&) = delete;
	DescriptorPool(DescriptorPool&&) = delete;
	~DescriptorPool();

	DescriptorPool& operator=(const DescriptorPool&) = delete;
	DescriptorPool&& operator=(const DescriptorPool&&) = delete;

	void OnDelete(const VkAllocationCallbacks*)
	{
	}
	
	VkResult Reset();
	
	DescriptorSet* createDescriptorPool();
	void freeDescriptorPool(DescriptorSet* descriptorSet);

	static VkResult Create(const VkDescriptorPoolCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDescriptorPool* pDescriptorPool);

private:
	std::vector<DescriptorSet*> usedSets{};
	
	const VkAllocationCallbacks* allocator;
};