#pragma once
#include "Base.h"

class ImageView;

class Framebuffer final
{
public:
	Framebuffer() = default;
	Framebuffer(const Framebuffer&) = delete;
	Framebuffer(Framebuffer&&) = delete;
	~Framebuffer() = default;

	Framebuffer& operator=(const Framebuffer&) = delete;
	Framebuffer&& operator=(const Framebuffer&&) = delete;

	static VkResult Create(const VkFramebufferCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkFramebuffer* pFramebuffer);

	[[nodiscard]] const std::vector<ImageView*>& getAttachments() const { return attachments; }

private:
	std::vector<ImageView*> attachments;
	uint32_t width;
	uint32_t height;
	uint32_t layers;
};
