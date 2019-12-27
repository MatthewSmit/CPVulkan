#pragma once
#include "Base.h"

#include <PipelineState.h>

class RenderPass final
{
public:
	RenderPass() = default;
	RenderPass(const RenderPass&) = delete;
	RenderPass(RenderPass&&) = delete;
	~RenderPass() = default;

	RenderPass& operator=(const RenderPass&) = delete;
	RenderPass&& operator=(const RenderPass&&) = delete;

	void OnDelete(const VkAllocationCallbacks*)
	{
	}

	static VkResult Create(const VkRenderPassCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkRenderPass* pRenderPass);
	
#if defined(VK_KHR_create_renderpass2)
	static VkResult Create(const VkRenderPassCreateInfo2KHR* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkRenderPass* pRenderPass);
#endif

	[[nodiscard]] const std::vector<AttachmentDescription>& getAttachments() const { return attachments; }
	[[nodiscard]] const std::vector<SubpassDescription>& getSubpasses() const { return subpasses; }
	[[nodiscard]] const std::vector<SubpassDependency>& getDependencies() const { return dependencies; }

	[[nodiscard]] VkExtent2D getRenderAreaGranularity() const;

private:
	std::vector<AttachmentDescription> attachments;
	std::vector<SubpassDescription> subpasses;
	std::vector<SubpassDependency> dependencies;
};
