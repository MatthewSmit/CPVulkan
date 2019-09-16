#include "Framebuffer.h"

#include "Device.h"
#include "ImageView.h"

#include <cassert>

VkResult Framebuffer::Create(const VkFramebufferCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkFramebuffer* pFramebuffer)
{
	assert(pCreateInfo->sType == VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO);

	auto framebuffer = Allocate<Framebuffer>(pAllocator, VK_SYSTEM_ALLOCATION_SCOPE_DEVICE);

	auto next = pCreateInfo->pNext;
	while (next)
	{
		const auto type = static_cast<const VkBaseInStructure*>(next)->sType;
		switch (type)
		{
		case VK_STRUCTURE_TYPE_FRAMEBUFFER_ATTACHMENTS_CREATE_INFO_KHR:
			FATAL_ERROR();

		default:
			next = static_cast<const VkBaseInStructure*>(next)->pNext;
			break;
		}
	}

	if (pCreateInfo->flags)
	{
		FATAL_ERROR();
	}

	framebuffer->attachments = std::vector<ImageView*>(pCreateInfo->attachmentCount);
	for (auto i = 0u; i < pCreateInfo->attachmentCount; i++)
	{
		framebuffer->attachments[i] = reinterpret_cast<ImageView*>(pCreateInfo->pAttachments[i]);
	}

	framebuffer->width = pCreateInfo->width;
	framebuffer->height = pCreateInfo->height;

	if (pCreateInfo->layers != 1)
	{
		FATAL_ERROR();
	}

	*pFramebuffer = reinterpret_cast<VkFramebuffer>(framebuffer);
	return VK_SUCCESS;
}

VkResult Device::CreateFramebuffer(const VkFramebufferCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkFramebuffer* pFramebuffer)
{
	return Framebuffer::Create(pCreateInfo, pAllocator, pFramebuffer);
}

void Device::DestroyFramebuffer(VkFramebuffer framebuffer, const VkAllocationCallbacks* pAllocator)
{
	Free(reinterpret_cast<Framebuffer*>(framebuffer), pAllocator);
}
