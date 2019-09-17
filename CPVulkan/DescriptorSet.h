#pragma once
#include "Base.h"

class DescriptorSetLayout;

union Bindings
{
	VkDescriptorImageInfo ImageInfo;
	VkDescriptorBufferInfo BufferInfo;
	VkBufferView TexelBufferView;
};

class DescriptorSet final
{
public:
	~DescriptorSet() = default;
	
	void Update(const VkWriteDescriptorSet& descriptorWrite);

	static VkResult Create(VkDescriptorPool descriptorPool, VkDescriptorSetLayout pSetLayout, VkDescriptorSet* pDescriptorSet);

	[[nodiscard]] const std::vector<std::tuple<VkDescriptorType, uint32_t, Bindings>>& getBindings() const { return bindings; }

private:
	DescriptorSetLayout* layout{};

	std::vector<std::tuple<VkDescriptorType, uint32_t, Bindings>> bindings{};
	
	//VK_DESCRIPTOR_TYPE_SAMPLER = 0,
	//VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER = 1,
	//VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE = 2,
	//VK_DESCRIPTOR_TYPE_STORAGE_IMAGE = 3,
	//VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER = 4,
	//VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER = 5,
	//VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER = 6,
	//VK_DESCRIPTOR_TYPE_STORAGE_BUFFER = 7,
	//VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC = 8,
	//VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC = 9,
	//VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT = 10,
	//VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK_EXT = 1000138000,
	//VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV = 1000165000,
};