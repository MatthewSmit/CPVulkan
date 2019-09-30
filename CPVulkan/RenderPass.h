#pragma once
#include "Base.h"

struct Subpass
{
	VkSubpassDescriptionFlags Flags;
	VkPipelineBindPoint PipelineBindPoint;
	std::vector<VkAttachmentReference> InputAttachments;
	std::vector<VkAttachmentReference> ColourAttachments;
	std::vector<VkAttachmentReference> ResolveAttachments;
	VkAttachmentReference DepthStencilAttachment;
	std::vector<uint32_t> PreserveAttachments;
};

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

	[[nodiscard]] const std::vector<VkAttachmentDescription>& getAttachments() const { return attachments; }
	[[nodiscard]] const std::vector<Subpass>& getSubpasses() const { return subpasses; }
	[[nodiscard]] const std::vector<VkSubpassDependency>& getDependencies() const { return dependencies; }

	[[nodiscard]] VkExtent2D getRenderAreaGranularity() const;

private:
	std::vector<VkAttachmentDescription> attachments;
	std::vector<Subpass> subpasses;
	std::vector<VkSubpassDependency> dependencies;
};