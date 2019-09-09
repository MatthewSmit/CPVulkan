#pragma once
#include "Base.h"
#include "ImageView.h"

class Framebuffer final : public VulkanBase
{
public:
	~Framebuffer() override = default;

	static VkResult Create(const VkFramebufferCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkFramebuffer* pFramebuffer);

	const std::vector<ImageView*>& getAttachments() const { return attachments; }

private:
	std::vector<ImageView*> attachments;
	uint32_t width;
	uint32_t height;
};
