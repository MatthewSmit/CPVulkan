#pragma once
#include "Base.h"

#include <vector>

struct AttachmentDescription
{
	VkAttachmentDescriptionFlags flags;
	VkFormat format;
	VkSampleCountFlagBits samples;
	VkAttachmentLoadOp loadOp;
	VkAttachmentStoreOp storeOp;
	VkAttachmentLoadOp stencilLoadOp;
	VkAttachmentStoreOp stencilStoreOp;
	VkImageLayout initialLayout;
	VkImageLayout finalLayout;
};

struct AttachmentReference
{
	uint32_t attachment;
	VkImageLayout layout;
	VkImageAspectFlags aspectMask;
};

struct SubpassDescription
{
	VkSubpassDescriptionFlags flags;
	VkPipelineBindPoint pipelineBindPoint;
	uint32_t viewMask;
	std::vector<AttachmentReference> inputAttachments;
	std::vector<AttachmentReference> colourAttachments;
	std::vector<AttachmentReference> resolveAttachments;
	AttachmentReference depthStencilAttachment;
	std::vector<uint32_t> preserveAttachments;
};

struct SubpassDependency
{
	uint32_t srcSubpass;
	uint32_t dstSubpass;
	VkPipelineStageFlags srcStageMask;
	VkPipelineStageFlags dstStageMask;
	VkAccessFlags srcAccessMask;
	VkAccessFlags dstAccessMask;
	VkDependencyFlags dependencyFlags;
	int32_t viewOffset;
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