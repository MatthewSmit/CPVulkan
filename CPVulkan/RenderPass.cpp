#include "RenderPass.h"

#include <cassert>

VkResult RenderPass::Create(const VkRenderPassCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkRenderPass* pRenderPass)
{
	assert(pCreateInfo->sType == VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO);

	auto renderPass = Allocate<RenderPass>(pAllocator);

	auto next = pCreateInfo->pNext;
	while (next)
	{
		auto type = *static_cast<const VkStructureType*>(next);
		switch (type)
		{
		default:
			FATAL_ERROR();
		}
	}

	if (pCreateInfo->flags)
	{
		FATAL_ERROR();
	}

	renderPass->attachments = std::vector<VkAttachmentDescription>(pCreateInfo->attachmentCount);
	memcpy(renderPass->attachments.data(), pCreateInfo->pAttachments, sizeof(VkAttachmentDescription) * pCreateInfo->attachmentCount);

	renderPass->subpasses = std::vector<Subpass>(pCreateInfo->subpassCount);
	for (auto i = 0u; i < pCreateInfo->subpassCount; i++)
	{
		renderPass->subpasses[i].Flags = pCreateInfo->pSubpasses[i].flags;
		renderPass->subpasses[i].PipelineBindPoint = pCreateInfo->pSubpasses[i].pipelineBindPoint;
		renderPass->subpasses[i].InputAttachments = std::vector<VkAttachmentReference>(pCreateInfo->pSubpasses[i].inputAttachmentCount);
		memcpy(renderPass->subpasses[i].InputAttachments.data(), pCreateInfo->pSubpasses[i].pInputAttachments, sizeof(VkAttachmentReference) * pCreateInfo->pSubpasses[i].inputAttachmentCount);

		renderPass->subpasses[i].ColourAttachments = std::vector<VkAttachmentReference>(pCreateInfo->pSubpasses[i].colorAttachmentCount);
		memcpy(renderPass->subpasses[i].ColourAttachments.data(), pCreateInfo->pSubpasses[i].pColorAttachments, sizeof(VkAttachmentReference) * pCreateInfo->pSubpasses[i].colorAttachmentCount);

		if (pCreateInfo->pSubpasses[i].pResolveAttachments)
		{
			renderPass->subpasses[i].ResolveAttachments = std::vector<VkAttachmentReference>(pCreateInfo->pSubpasses[i].colorAttachmentCount);
			memcpy(renderPass->subpasses[i].ResolveAttachments.data(), pCreateInfo->pSubpasses[i].pResolveAttachments, sizeof(VkAttachmentReference) * pCreateInfo->pSubpasses[i].colorAttachmentCount);
		}

		if (pCreateInfo->pSubpasses[i].pDepthStencilAttachment)
		{
			renderPass->subpasses[i].DepthStencilAttachment = *pCreateInfo->pSubpasses[i].pDepthStencilAttachment;
		}
		else
		{
			renderPass->subpasses[i].DepthStencilAttachment.attachment = VK_ATTACHMENT_UNUSED;
			renderPass->subpasses[i].DepthStencilAttachment.layout = VK_IMAGE_LAYOUT_UNDEFINED;
		}

		renderPass->subpasses[i].PreserveAttachments = std::vector<uint32_t>(pCreateInfo->pSubpasses[i].preserveAttachmentCount);
		memcpy(renderPass->subpasses[i].PreserveAttachments.data(), pCreateInfo->pSubpasses[i].pPreserveAttachments, sizeof(uint32_t) * pCreateInfo->pSubpasses[i].preserveAttachmentCount);
	}

	renderPass->dependencies = std::vector<VkSubpassDependency>(pCreateInfo->dependencyCount);
	memcpy(renderPass->dependencies.data(), pCreateInfo->pDependencies, sizeof(VkSubpassDependency) * pCreateInfo->dependencyCount);

	*pRenderPass = reinterpret_cast<VkRenderPass>(renderPass);
	return VK_SUCCESS;
}
