#pragma once
#include "Base.h"

class DescriptorSetLayout final : public VulkanBase
{
public:
	~DescriptorSetLayout() override = default;
	
	static VkResult Create(const VkDescriptorSetLayoutCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDescriptorSetLayout* pSetLayout);

private:
	std::vector<VkDescriptorSetLayoutBinding> bindings{};
};