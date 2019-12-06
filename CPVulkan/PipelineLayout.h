#pragma once
#include "Base.h"

class PipelineLayout final
{
public:
	PipelineLayout() = default;
	PipelineLayout(const PipelineLayout&) = delete;
	PipelineLayout(PipelineLayout&&) = delete;
	~PipelineLayout() = default;

	PipelineLayout& operator=(const PipelineLayout&) = delete;
	PipelineLayout&& operator=(const PipelineLayout&&) = delete;

	void OnDelete(const VkAllocationCallbacks*)
	{
	}

	static VkResult Create(const VkPipelineLayoutCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkPipelineLayout* pPipelineLayout);

	[[nodiscard]] const std::vector<DescriptorSetLayout*>& getDescriptorSetLayouts() const { return descriptorSetLayouts; }
	[[nodiscard]] const std::vector<VkPushConstantRange>& getPushConstantRanges() const { return pushConstantRanges; }

private:
	std::vector<DescriptorSetLayout*> descriptorSetLayouts{};
	std::vector<VkPushConstantRange> pushConstantRanges{};
};