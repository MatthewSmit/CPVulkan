#include "Framebuffer.h"

#include "Device.h"
#include "ImageView.h"

#include <cassert>

VkResult Framebuffer::Create(const VkFramebufferCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkFramebuffer* pFramebuffer)
{
	assert(pCreateInfo->sType == VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO);

	auto framebuffer = Allocate<Framebuffer>(pAllocator, VK_SYSTEM_ALLOCATION_SCOPE_DEVICE);
	if (!framebuffer)
	{
		return VK_ERROR_OUT_OF_HOST_MEMORY;
	}

	auto next = pCreateInfo->pNext;
	while (next)
	{
		const auto type = static_cast<const VkBaseInStructure*>(next)->sType;
		switch (type)
		{
		case VK_STRUCTURE_TYPE_FRAMEBUFFER_ATTACHMENTS_CREATE_INFO_KHR:
			TODO_ERROR();
		}
		next = static_cast<const VkBaseInStructure*>(next)->pNext;
	}

	if (pCreateInfo->flags)
	{
		TODO_ERROR();
	}

	framebuffer->attachments = std::vector<ImageView*>(pCreateInfo->attachmentCount);
	for (auto i = 0u; i < pCreateInfo->attachmentCount; i++)
	{
		framebuffer->attachments[i] = UnwrapVulkan<ImageView>(pCreateInfo->pAttachments[i]);
	}

	framebuffer->width = pCreateInfo->width;
	framebuffer->height = pCreateInfo->height;
	framebuffer->layers = pCreateInfo->layers;

	WrapVulkan(framebuffer, pFramebuffer);
	return VK_SUCCESS;
}

VkResult Device::CreateFramebuffer(const VkFramebufferCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkFramebuffer* pFramebuffer)
{
	return Framebuffer::Create(pCreateInfo, pAllocator, pFramebuffer);
}

void Device::DestroyFramebuffer(VkFramebuffer framebuffer, const VkAllocationCallbacks* pAllocator)
{
	if (framebuffer)
	{
		Free(UnwrapVulkan<Framebuffer>(framebuffer), pAllocator);
	}
}
