#pragma once
#include "Base.h"

class Buffer final
{
public:
	Buffer() = default;
	Buffer(const Buffer&) = delete;
	Buffer(Buffer&&) = delete;
	~Buffer() = default;

	Buffer& operator=(const Buffer&) = delete;
	Buffer&& operator=(const Buffer&&) = delete;

	void OnDelete(const VkAllocationCallbacks*)
	{
	}

	VkResult BindMemory(DeviceMemory* memory, uint64_t memoryOffset);

	void GetMemoryRequirements(VkMemoryRequirements* pMemoryRequirements) const;
	void GetMemoryRequirements(VkMemoryRequirements2* pMemoryRequirements) const;

	static VkResult Create(gsl::not_null<const VkBufferCreateInfo*> pCreateInfo, const VkAllocationCallbacks* pAllocator, VkBuffer* pBuffer);

	[[nodiscard]] gsl::span<uint8_t> getData() const
	{
		return data;
	}

	[[nodiscard]] gsl::span<uint8_t> getData(uint64_t offset, uint64_t size) const
	{
		return data.subspan(offset, size);
	}
	
	[[nodiscard]] uint8_t* getDataPtr(uint64_t offset, uint64_t size) const
	{
		return getData(offset, size).data();
	}
	
	[[nodiscard]] uint64_t getSize() const { return size; }

private:
	VkBufferCreateFlags flags{};
	VkBufferUsageFlags usage{};

	gsl::span<uint8_t> data{};
	uint64_t size{};
};
