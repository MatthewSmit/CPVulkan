#pragma once
#include "Base.h"

class Buffer;

class BufferView final : public VulkanBase
{
public:
	BufferView() = default;
	BufferView(const BufferView&) = delete;
	BufferView(BufferView&&) = delete;
	~BufferView() override = default;

	BufferView& operator=(const BufferView&) = delete;
	BufferView&& operator=(const BufferView&&) = delete;
	
	static VkResult Create(gsl::not_null<const VkBufferViewCreateInfo*> pCreateInfo, const VkAllocationCallbacks* pAllocator, gsl::not_null<VkBufferView*> pView);

private:
	Buffer* buffer;
	VkFormat format;
	uint64_t offset;
	uint64_t range;
};
