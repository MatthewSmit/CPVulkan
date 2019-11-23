#include "RenderPass.h"

#include "Device.h"

#include <cassert>

VkResult RenderPass::Create(const VkRenderPassCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkRenderPass* pRenderPass)
{
	assert(pCreateInfo->sType == VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO);

	auto renderPass = Allocate<RenderPass>(pAllocator, VK_SYSTEM_ALLOCATION_SCOPE_DEVICE);
	if (!renderPass)
	{
		return VK_ERROR_OUT_OF_HOST_MEMORY;
	}

	auto next = pCreateInfo->pNext;
	while (next)
	{
		const auto type = static_cast<const VkBaseInStructure*>(next)->sType;
		switch (type)
		{
		case VK_STRUCTURE_TYPE_RENDER_PASS_FRAGMENT_DENSITY_MAP_CREATE_INFO_EXT:
			FATAL_ERROR();

		case VK_STRUCTURE_TYPE_RENDER_PASS_INPUT_ATTACHMENT_ASPECT_CREATE_INFO:
			FATAL_ERROR();

		case VK_STRUCTURE_TYPE_RENDER_PASS_MULTIVIEW_CREATE_INFO:
			FATAL_ERROR();
		}
		next = static_cast<const VkBaseInStructure*>(next)->pNext;
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

	WrapVulkan(renderPass, pRenderPass);
	return VK_SUCCESS;
}

VkExtent2D RenderPass::getRenderAreaGranularity() const
{
	return VkExtent2D{1, 1};
}

void Device::GetRenderAreaGranularity(VkRenderPass renderPass, VkExtent2D* pGranularity)
{
	*pGranularity = UnwrapVulkan<RenderPass>(renderPass)->getRenderAreaGranularity();
}

VkResult Device::CreateRenderPass(const VkRenderPassCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkRenderPass* pRenderPass)
{
	return RenderPass::Create(pCreateInfo, pAllocator, pRenderPass);
}

void Device::DestroyRenderPass(VkRenderPass renderPass, const VkAllocationCallbacks* pAllocator)
{
	if (renderPass)
	{
		Free(UnwrapVulkan<RenderPass>(renderPass), pAllocator);
	}
}

#if defined(VK_KHR_create_renderpass2)
VkResult Device::CreateRenderPass2(const VkRenderPassCreateInfo2KHR* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkRenderPass* pRenderPass)
{
	FATAL_ERROR();
}
#endif
