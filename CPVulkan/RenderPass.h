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

class RenderPass final : public VulkanBase
{
public:
	~RenderPass() override = default;
	
	static VkResult Create(const VkRenderPassCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkRenderPass* pRenderPass);

	const std::vector<VkAttachmentDescription>& getAttachments() const { return attachments; }
	const std::vector<Subpass>& getSubpasses() const { return subpasses; }
	const std::vector<VkSubpassDependency>& getDependencies() const { return dependencies; }

private:
	std::vector<VkAttachmentDescription> attachments;
	std::vector<Subpass> subpasses;
	std::vector<VkSubpassDependency> dependencies;
};