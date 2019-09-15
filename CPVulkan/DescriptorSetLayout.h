#pragma once
#include "Base.h"

class DescriptorSetLayout final : public VulkanBase
{
public:
	~DescriptorSetLayout() override = default;
	
	static VkResult Create(const VkDescriptorSetLayoutCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDescriptorSetLayout* pSetLayout);

	[[nodiscard]] const std::vector<VkDescriptorSetLayoutBinding>& getBindings() const { return bindings; }

private:
	std::vector<VkDescriptorSetLayoutBinding> bindings{};
};