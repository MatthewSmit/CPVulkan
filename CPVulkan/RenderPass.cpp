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

	auto next = reinterpret_cast<const VkBaseInStructure*>(pCreateInfo)->pNext;
	while (next)
	{
		const auto type = next->sType;
		switch (type)
		{
		case VK_STRUCTURE_TYPE_RENDER_PASS_FRAGMENT_DENSITY_MAP_CREATE_INFO_EXT:
			TODO_ERROR();

		case VK_STRUCTURE_TYPE_RENDER_PASS_INPUT_ATTACHMENT_ASPECT_CREATE_INFO:
			TODO_ERROR();

		case VK_STRUCTURE_TYPE_RENDER_PASS_MULTIVIEW_CREATE_INFO:
			TODO_ERROR();

		default:
			break;
		}
		next = next->pNext;
	}

	if (pCreateInfo->flags)
	{
		TODO_ERROR();
	}

	renderPass->attachments = std::vector<AttachmentDescription>(pCreateInfo->attachmentCount);
	renderPass->subpasses = std::vector<SubpassDescription>(pCreateInfo->subpassCount);
	renderPass->dependencies = std::vector<SubpassDependency>(pCreateInfo->dependencyCount);

	for (auto i = 0u; i < pCreateInfo->attachmentCount; i++)
	{
		renderPass->attachments[i].flags = pCreateInfo->pAttachments[i].flags;
		renderPass->attachments[i].format = pCreateInfo->pAttachments[i].format;
		renderPass->attachments[i].samples = pCreateInfo->pAttachments[i].samples;
		renderPass->attachments[i].loadOp = pCreateInfo->pAttachments[i].loadOp;
		renderPass->attachments[i].storeOp = pCreateInfo->pAttachments[i].storeOp;
		renderPass->attachments[i].stencilLoadOp = pCreateInfo->pAttachments[i].stencilLoadOp;
		renderPass->attachments[i].stencilStoreOp = pCreateInfo->pAttachments[i].stencilStoreOp;
		renderPass->attachments[i].initialLayout = pCreateInfo->pAttachments[i].initialLayout;
		renderPass->attachments[i].finalLayout = pCreateInfo->pAttachments[i].finalLayout;
	}

	for (auto i = 0u; i < pCreateInfo->subpassCount; i++)
	{
		renderPass->subpasses[i].flags = pCreateInfo->pSubpasses[i].flags;
		renderPass->subpasses[i].pipelineBindPoint = pCreateInfo->pSubpasses[i].pipelineBindPoint;
		renderPass->subpasses[i].viewMask = 0;
		
		renderPass->subpasses[i].inputAttachments = std::vector<AttachmentReference>(pCreateInfo->pSubpasses[i].inputAttachmentCount);
		for (auto j = 0u; j < pCreateInfo->pSubpasses[i].inputAttachmentCount; j++)
		{
			renderPass->subpasses[i].inputAttachments[j].attachment = pCreateInfo->pSubpasses[i].pInputAttachments[j].attachment;
			renderPass->subpasses[i].inputAttachments[j].layout = pCreateInfo->pSubpasses[i].pInputAttachments[j].layout;
			renderPass->subpasses[i].inputAttachments[j].aspectMask = 0;
		}
		
		renderPass->subpasses[i].colourAttachments = std::vector<AttachmentReference>(pCreateInfo->pSubpasses[i].colorAttachmentCount);
		for (auto j = 0u; j < pCreateInfo->pSubpasses[i].colorAttachmentCount; j++)
		{
			renderPass->subpasses[i].colourAttachments[j].attachment = pCreateInfo->pSubpasses[i].pColorAttachments[j].attachment;
			renderPass->subpasses[i].colourAttachments[j].layout = pCreateInfo->pSubpasses[i].pColorAttachments[j].layout;
			renderPass->subpasses[i].colourAttachments[j].aspectMask = 0;
		}
		
		if (pCreateInfo->pSubpasses[i].pResolveAttachments)
		{
			renderPass->subpasses[i].resolveAttachments = std::vector<AttachmentReference>(pCreateInfo->pSubpasses[i].colorAttachmentCount);
			for (auto j = 0u; j < pCreateInfo->pSubpasses[i].colorAttachmentCount; j++)
			{
				renderPass->subpasses[i].resolveAttachments[j].attachment = pCreateInfo->pSubpasses[i].pResolveAttachments[j].attachment;
				renderPass->subpasses[i].resolveAttachments[j].layout = pCreateInfo->pSubpasses[i].pResolveAttachments[j].layout;
				renderPass->subpasses[i].resolveAttachments[j].aspectMask = 0;
			}
		}
		
		if (pCreateInfo->pSubpasses[i].pDepthStencilAttachment)
		{
			renderPass->subpasses[i].depthStencilAttachment.attachment = pCreateInfo->pSubpasses[i].pDepthStencilAttachment->attachment;
			renderPass->subpasses[i].depthStencilAttachment.layout = pCreateInfo->pSubpasses[i].pDepthStencilAttachment->layout;
			renderPass->subpasses[i].depthStencilAttachment.aspectMask = 0;
		}
		else
		{
			renderPass->subpasses[i].depthStencilAttachment.attachment = VK_ATTACHMENT_UNUSED;
			renderPass->subpasses[i].depthStencilAttachment.layout = VK_IMAGE_LAYOUT_UNDEFINED;
			renderPass->subpasses[i].depthStencilAttachment.aspectMask = 0;
		}
		
		renderPass->subpasses[i].preserveAttachments = std::vector<uint32_t>(pCreateInfo->pSubpasses[i].preserveAttachmentCount);
		memcpy(renderPass->subpasses[i].preserveAttachments.data(), pCreateInfo->pSubpasses[i].pPreserveAttachments, sizeof(uint32_t) * pCreateInfo->pSubpasses[i].preserveAttachmentCount);
	}

	for (auto i = 0u; i < pCreateInfo->dependencyCount; i++)
	{
		renderPass->dependencies[i].srcSubpass = pCreateInfo->pDependencies[i].srcSubpass;
		renderPass->dependencies[i].dstSubpass = pCreateInfo->pDependencies[i].dstSubpass;
		renderPass->dependencies[i].srcStageMask = pCreateInfo->pDependencies[i].srcStageMask;
		renderPass->dependencies[i].dstStageMask = pCreateInfo->pDependencies[i].dstStageMask;
		renderPass->dependencies[i].srcAccessMask = pCreateInfo->pDependencies[i].srcAccessMask;
		renderPass->dependencies[i].dstAccessMask = pCreateInfo->pDependencies[i].dstAccessMask;
		renderPass->dependencies[i].dependencyFlags = pCreateInfo->pDependencies[i].dependencyFlags;
		renderPass->dependencies[i].viewOffset = 0;
	}

	WrapVulkan(renderPass, pRenderPass);
	return VK_SUCCESS;
}

VkResult Device::CreateRenderPass(const VkRenderPassCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkRenderPass* pRenderPass)
{
	return RenderPass::Create(pCreateInfo, pAllocator, pRenderPass);
}

#if defined(VK_KHR_create_renderpass2)
VkResult RenderPass::Create(const VkRenderPassCreateInfo2KHR* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkRenderPass* pRenderPass)
{
	assert(pCreateInfo->sType == VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO_2_KHR);

	auto renderPass = Allocate<RenderPass>(pAllocator, VK_SYSTEM_ALLOCATION_SCOPE_DEVICE);
	if (!renderPass)
	{
		return VK_ERROR_OUT_OF_HOST_MEMORY;
	}

	auto next = static_cast<const VkBaseInStructure*>(pCreateInfo->pNext);
	while (next)
	{
		const auto type = next->sType;
		switch (type)
		{
		case VK_STRUCTURE_TYPE_RENDER_PASS_FRAGMENT_DENSITY_MAP_CREATE_INFO_EXT:
			TODO_ERROR();

		default:
			break;
		}
		next = next->pNext;
	}

	renderPass->attachments = std::vector<AttachmentDescription>(pCreateInfo->attachmentCount);
	renderPass->subpasses = std::vector<SubpassDescription>(pCreateInfo->subpassCount);
	renderPass->dependencies = std::vector<SubpassDependency>(pCreateInfo->dependencyCount);

	for (auto i = 0u; i < pCreateInfo->attachmentCount; i++)
	{
		renderPass->attachments[i].flags = pCreateInfo->pAttachments[i].flags;
		renderPass->attachments[i].format = pCreateInfo->pAttachments[i].format;
		renderPass->attachments[i].samples = pCreateInfo->pAttachments[i].samples;
		renderPass->attachments[i].loadOp = pCreateInfo->pAttachments[i].loadOp;
		renderPass->attachments[i].storeOp = pCreateInfo->pAttachments[i].storeOp;
		renderPass->attachments[i].stencilLoadOp = pCreateInfo->pAttachments[i].stencilLoadOp;
		renderPass->attachments[i].stencilStoreOp = pCreateInfo->pAttachments[i].stencilStoreOp;
		renderPass->attachments[i].initialLayout = pCreateInfo->pAttachments[i].initialLayout;
		renderPass->attachments[i].finalLayout = pCreateInfo->pAttachments[i].finalLayout;

		next = static_cast<const VkBaseInStructure*>(pCreateInfo->pAttachments[i].pNext);
		while (next)
		{
			const auto type = next->sType;
			switch (type)
			{
#if defined(VK_KHR_separate_depth_stencil_layouts)
			case VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_STENCIL_LAYOUT_KHR:
				TODO_ERROR();
#endif

			default:
				break;
			}
			next = next->pNext;
		}
	}

	for (auto i = 0u; i < pCreateInfo->subpassCount; i++)
	{
		renderPass->subpasses[i].flags = pCreateInfo->pSubpasses[i].flags;
		renderPass->subpasses[i].pipelineBindPoint = pCreateInfo->pSubpasses[i].pipelineBindPoint;
		renderPass->subpasses[i].viewMask = 0;

		renderPass->subpasses[i].inputAttachments = std::vector<AttachmentReference>(pCreateInfo->pSubpasses[i].inputAttachmentCount);
		for (auto j = 0u; j < pCreateInfo->pSubpasses[i].inputAttachmentCount; j++)
		{
			renderPass->subpasses[i].inputAttachments[j].attachment = pCreateInfo->pSubpasses[i].pInputAttachments[j].attachment;
			renderPass->subpasses[i].inputAttachments[j].layout = pCreateInfo->pSubpasses[i].pInputAttachments[j].layout;
			renderPass->subpasses[i].inputAttachments[j].aspectMask = pCreateInfo->pSubpasses[i].pInputAttachments[j].aspectMask;

			next = static_cast<const VkBaseInStructure*>(pCreateInfo->pSubpasses[i].pInputAttachments[j].pNext);
			while (next)
			{
				const auto type = next->sType;
				switch (type)
				{
#if defined(VK_KHR_separate_depth_stencil_layouts)
				case VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_STENCIL_LAYOUT_KHR:
					TODO_ERROR();
#endif

				default:
					break;
				}
				next = next->pNext;
			}
		}

		renderPass->subpasses[i].colourAttachments = std::vector<AttachmentReference>(pCreateInfo->pSubpasses[i].colorAttachmentCount);
		for (auto j = 0u; j < pCreateInfo->pSubpasses[i].colorAttachmentCount; j++)
		{
			renderPass->subpasses[i].colourAttachments[j].attachment = pCreateInfo->pSubpasses[i].pColorAttachments[j].attachment;
			renderPass->subpasses[i].colourAttachments[j].layout = pCreateInfo->pSubpasses[i].pColorAttachments[j].layout;
			renderPass->subpasses[i].colourAttachments[j].aspectMask = pCreateInfo->pSubpasses[i].pColorAttachments[j].aspectMask;

			next = static_cast<const VkBaseInStructure*>(pCreateInfo->pSubpasses[i].pColorAttachments[j].pNext);
			while (next)
			{
				const auto type = next->sType;
				switch (type)
				{
#if defined(VK_KHR_separate_depth_stencil_layouts)
				case VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_STENCIL_LAYOUT_KHR:
					TODO_ERROR();
#endif

				default:
					break;
				}
				next = next->pNext;
			}
		}

		if (pCreateInfo->pSubpasses[i].pResolveAttachments)
		{
			renderPass->subpasses[i].resolveAttachments = std::vector<AttachmentReference>(pCreateInfo->pSubpasses[i].colorAttachmentCount);
			for (auto j = 0u; j < pCreateInfo->pSubpasses[i].colorAttachmentCount; j++)
			{
				renderPass->subpasses[i].resolveAttachments[j].attachment = pCreateInfo->pSubpasses[i].pResolveAttachments[j].attachment;
				renderPass->subpasses[i].resolveAttachments[j].layout = pCreateInfo->pSubpasses[i].pResolveAttachments[j].layout;
				renderPass->subpasses[i].resolveAttachments[j].aspectMask = pCreateInfo->pSubpasses[i].pResolveAttachments[j].aspectMask;

				next = static_cast<const VkBaseInStructure*>(pCreateInfo->pSubpasses[i].pResolveAttachments[j].pNext);
				while (next)
				{
					const auto type = next->sType;
					switch (type)
					{
#if defined(VK_KHR_separate_depth_stencil_layouts)
					case VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_STENCIL_LAYOUT_KHR:
						TODO_ERROR();
#endif

					default:
						break;
					}
					next = next->pNext;
				}
			}
		}

		if (pCreateInfo->pSubpasses[i].pDepthStencilAttachment)
		{
			renderPass->subpasses[i].depthStencilAttachment.attachment = pCreateInfo->pSubpasses[i].pDepthStencilAttachment->attachment;
			renderPass->subpasses[i].depthStencilAttachment.layout = pCreateInfo->pSubpasses[i].pDepthStencilAttachment->layout;
			renderPass->subpasses[i].depthStencilAttachment.aspectMask = pCreateInfo->pSubpasses[i].pDepthStencilAttachment->aspectMask;

			next = static_cast<const VkBaseInStructure*>(pCreateInfo->pSubpasses[i].pDepthStencilAttachment->pNext);
			while (next)
			{
				const auto type = next->sType;
				switch (type)
				{
#if defined(VK_KHR_separate_depth_stencil_layouts)
				case VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_STENCIL_LAYOUT_KHR:
					TODO_ERROR();
#endif

				default:
					break;
				}
				next = next->pNext;
			}
		}
		else
		{
			renderPass->subpasses[i].depthStencilAttachment.attachment = VK_ATTACHMENT_UNUSED;
			renderPass->subpasses[i].depthStencilAttachment.layout = VK_IMAGE_LAYOUT_UNDEFINED;
			renderPass->subpasses[i].depthStencilAttachment.aspectMask = 0;
		}

		renderPass->subpasses[i].preserveAttachments = std::vector<uint32_t>(pCreateInfo->pSubpasses[i].preserveAttachmentCount);
		memcpy(renderPass->subpasses[i].preserveAttachments.data(), pCreateInfo->pSubpasses[i].pPreserveAttachments, sizeof(uint32_t) * pCreateInfo->pSubpasses[i].preserveAttachmentCount);

		next = static_cast<const VkBaseInStructure*>(pCreateInfo->pSubpasses[i].pNext);
		while (next)
		{
			const auto type = next->sType;
			switch (type)
			{
#if defined(VK_KHR_depth_stencil_resolve)
			case VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_DEPTH_STENCIL_RESOLVE_KHR:
				TODO_ERROR();
#endif

			default:
				break;
			}
			next = next->pNext;
		}
	}

	for (auto i = 0u; i < pCreateInfo->dependencyCount; i++)
	{
		renderPass->dependencies[i].srcSubpass = pCreateInfo->pDependencies[i].srcSubpass;
		renderPass->dependencies[i].dstSubpass = pCreateInfo->pDependencies[i].dstSubpass;
		renderPass->dependencies[i].srcStageMask = pCreateInfo->pDependencies[i].srcStageMask;
		renderPass->dependencies[i].dstStageMask = pCreateInfo->pDependencies[i].dstStageMask;
		renderPass->dependencies[i].srcAccessMask = pCreateInfo->pDependencies[i].srcAccessMask;
		renderPass->dependencies[i].dstAccessMask = pCreateInfo->pDependencies[i].dstAccessMask;
		renderPass->dependencies[i].dependencyFlags = pCreateInfo->pDependencies[i].dependencyFlags;
		renderPass->dependencies[i].viewOffset = pCreateInfo->pDependencies[i].viewOffset;
	}

	WrapVulkan(renderPass, pRenderPass);
	return VK_SUCCESS;
}

VkResult Device::CreateRenderPass2(const VkRenderPassCreateInfo2KHR* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkRenderPass* pRenderPass)
{
	return RenderPass::Create(pCreateInfo, pAllocator, pRenderPass);
}
#endif


VkExtent2D RenderPass::getRenderAreaGranularity() const
{
	return VkExtent2D{1, 1};
}

void Device::GetRenderAreaGranularity(VkRenderPass renderPass, VkExtent2D* pGranularity)
{
	*pGranularity = UnwrapVulkan<RenderPass>(renderPass)->getRenderAreaGranularity();
}

void Device::DestroyRenderPass(VkRenderPass renderPass, const VkAllocationCallbacks* pAllocator)
{
	if (renderPass)
	{
		Free(UnwrapVulkan<RenderPass>(renderPass), pAllocator);
	}
}