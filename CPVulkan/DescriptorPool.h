#pragma once
#include "Base.h"

class DescriptorPool final
{
public:
	DescriptorPool() = default;
	DescriptorPool(const DescriptorPool&) = delete;
	DescriptorPool(DescriptorPool&&) = delete;
	~DescriptorPool() = default;

	DescriptorPool& operator=(const DescriptorPool&) = delete;
	DescriptorPool&& operator=(const DescriptorPool&&) = delete;
	
	VkResult Reset(VkFlags flags);

	static VkResult Create(const VkDescriptorPoolCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDescriptorPool* pDescriptorPool);
};