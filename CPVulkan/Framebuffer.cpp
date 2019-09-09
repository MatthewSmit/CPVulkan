#include "Framebuffer.h"

#include "ImageView.h"

#include <cassert>

VkResult Framebuffer::Create(const VkFramebufferCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkFramebuffer* pFramebuffer)
{
	assert(pCreateInfo->sType == VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO);

	auto framebuffer = Allocate<Framebuffer>(pAllocator);

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
