#pragma once
#include "Base.h"

#include <vector>

class DescriptorSetLayout final
{
public:
	DescriptorSetLayout() = default;
	DescriptorSetLayout(const DescriptorSetLayout&) = delete;
	DescriptorSetLayout(DescriptorSetLayout&&) = delete;
	~DescriptorSetLayout();

	DescriptorSetLayout& operator=(const DescriptorSetLayout&) = delete;
	DescriptorSetLayout&& operator=(const DescriptorSetLayout&&) = delete;

	void OnDelete(const VkAllocationCallbacks*)
	{
	}

	static VkResult Create(const VkDescriptorSetLayoutCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDescriptorSetLayout* pSetLayout);

	[[nodiscard]] const std::vector<VkDescriptorSetLayoutBinding>& getBindings() const { return bindings; }

private:
	std::vector<VkDescriptorSetLayoutBinding> bindings{};
};