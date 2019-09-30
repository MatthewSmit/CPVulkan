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
};