#pragma once
#include "Base.h"

class Buffer
{
public:
	Buffer() = default;
	Buffer(const Buffer&) = delete;
	Buffer(Buffer&&) = delete;
	virtual ~Buffer() = default;

	Buffer& operator=(const Buffer&) = delete;
	Buffer&& operator=(const Buffer&&) = delete;

	virtual VkResult BindMemory(VkDeviceMemory memory, uint64_t memoryOffset);

	virtual void GetMemoryRequirements(VkMemoryRequirements* pMemoryRequirements) const noexcept;
	virtual void GetMemoryRequirements(VkMemoryRequirements2* pMemoryRequirements) const noexcept;

	static VkResult Create(gsl::not_null<const VkBufferCreateInfo*> pCreateInfo, const VkAllocationCallbacks* pAllocator, VkBuffer* pBuffer);

	[[nodiscard]] virtual uint8_t* getData(uint64_t offset, uint64_t size) const noexcept { return data.data() + offset; }

protected:
	VkBufferCreateFlags flags{};
	VkBufferUsageFlags usage{};

	gsl::span<uint8_t> data{};
	uint64_t size{};
};

class SparseBuffer final : public Buffer
{
public:
	SparseBuffer() = default;
	SparseBuffer(const SparseBuffer&) = delete;
	SparseBuffer(SparseBuffer&&) = delete;
	~SparseBuffer() override = default;

	SparseBuffer& operator=(const SparseBuffer&) = delete;
	SparseBuffer&& operator=(const SparseBuffer&&) = delete;

	VkResult BindMemory(VkDeviceMemory memory, uint64_t memoryOffset) override;
	void SparseBindMemory(uint32_t bindCount, const VkSparseMemoryBind* pBinds);

	void GetMemoryRequirements(VkMemoryRequirements* pMemoryRequirements) const noexcept override;

	[[nodiscard]] uint8_t* getData(uint64_t offset, uint64_t size) const noexcept override;

	void setPages(uint64_t pages)
	{
		pointers.resize(pages);
	}

private:
	std::vector<void*> pointers{};
};
