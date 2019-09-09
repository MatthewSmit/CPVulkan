#pragma once
#include "Base.h"

class DescriptorPool final : public VulkanBase
{
public:
	~DescriptorPool() override = default;
	
	static VkResult Create(const VkDescriptorPoolCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDescriptorPool* pDescriptorPool);
};