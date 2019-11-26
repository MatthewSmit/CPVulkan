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
	
	void Update(const VkWriteDescriptorSet& descriptorWrite);

	static VkResult Create(DescriptorPool* descriptorPool, VkDescriptorSetLayout pSetLayout, VkDescriptorSet* pDescriptorSet);

	[[nodiscard]] uint32_t getNumberBindings() const { return numberBindings; }
	
	void getBinding(uint32_t binding, VkDescriptorType& descriptorType, Descriptor*& value) const
	{
		assert(binding < numberBindings);
		descriptorType = bindingTypes[binding];
		value = &bindingValues[binding];
	}

private:
	DescriptorSetLayout* layout{};

	uint32_t numberBindings{};
	std::unique_ptr<VkDescriptorType[]> bindingTypes{};
	std::unique_ptr<Descriptor[]> bindingValues{};
};