#pragma once
#include "Base.h"

class DescriptorSet final : public VulkanBase
{
public:
	~DescriptorSet() override = default;
	
	static VkResult Create(VkDescriptorPool descriptorPool, VkDescriptorSetLayout pSetLayout, VkDescriptorSet* pDescriptorSet);
};