#pragma once
#include "Base.h"

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

	Sampler* ImageSampler;
};

union DescriptorValue
{
	ImageDescriptor Image;
	VkDescriptorBufferInfo Buffer;
};

struct Descriptor
{
	bool immutable;
	uint32_t count;
	std::unique_ptr<DescriptorValue[]> values;
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

	VkResult Initialise(DescriptorSetLayout* descriptorSetLayout);
	
	void Update(const VkWriteDescriptorSet& descriptorWrite);
	void CopyFrom(const VkCopyDescriptorSet& descriptorCopy);

	static VkResult Create(DescriptorPool* descriptorPool, VkDescriptorSetLayout pSetLayout, VkDescriptorSet* pDescriptorSet);

	[[nodiscard]] uint32_t getNumberBindings() const { return static_cast<uint32_t>(bindingTypes.size()); }
	
	void getBinding(uint32_t binding, VkDescriptorType& descriptorType, const Descriptor*& value) const
	{
		assert(binding < bindingTypes.size());
		descriptorType = bindingTypes[binding];
		value = &bindingValues[binding];
	}

private:
	DescriptorSetLayout* layout{};

	std::vector<VkDescriptorType> bindingTypes{};
	std::vector<Descriptor> bindingValues{};
};