#pragma once
#include "Base.h"

#include <vector>

class DescriptorSetLayout;

enum class ImageDescriptorType
{
	None = 0x0000,
	Buffer = 0x0001,
	Image = 0x0002,
};

struct ImageDescriptor
{
	ImageDescriptorType Type;

	union
	{
		BufferView* Buffer;
		ImageView* Image;
	} Data;

	Sampler* Sampler;
};

union Bindings
{
	ImageDescriptor ImageDescriptor;
	VkDescriptorBufferInfo BufferInfo;
};

class DescriptorSet final
{
public:
	DescriptorSet() = default;
	DescriptorSet(const DescriptorSet&) = delete;
	DescriptorSet(DescriptorSet&&) = delete;
	~DescriptorSet() = default;

	DescriptorSet& operator=(const DescriptorSet&) = delete;
	DescriptorSet&& operator=(const DescriptorSet&&) = delete;

	void OnDelete(const VkAllocationCallbacks*)
	{
	}
	
	void Update(const VkWriteDescriptorSet& descriptorWrite);

	static VkResult Create(DescriptorPool* descriptorPool, VkDescriptorSetLayout pSetLayout, VkDescriptorSet* pDescriptorSet);

	[[nodiscard]] const std::vector<std::tuple<VkDescriptorType, uint32_t, Bindings>>& getBindings() const { return bindings; }

private:
	DescriptorSetLayout* layout{};

	std::vector<std::tuple<VkDescriptorType, uint32_t, Bindings>> bindings{};
};