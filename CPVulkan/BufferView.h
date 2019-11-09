#pragma once
#include "Base.h"

class Buffer;

class BufferView final
{
public:
	BufferView() = default;
	BufferView(const BufferView&) = delete;
	BufferView(BufferView&&) = delete;
	~BufferView() = default;

	BufferView& operator=(const BufferView&) = delete;
	BufferView&& operator=(const BufferView&&) = delete;

	void OnDelete(const VkAllocationCallbacks*)
	{
	}
	
	static VkResult Create(gsl::not_null<const VkBufferViewCreateInfo*> pCreateInfo, const VkAllocationCallbacks* pAllocator, VkBufferView* pView);

	[[nodiscard]] Buffer* getBuffer() const { return buffer; }
	[[nodiscard]] VkFormat getFormat() const { return format; }
	[[nodiscard]] uint64_t getOffset() const { return offset; }
	[[nodiscard]] uint64_t getRange() const { return range; }

private:
	Buffer* buffer;
	VkFormat format;
	uint64_t offset;
	uint64_t range;
};
