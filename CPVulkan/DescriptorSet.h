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
	DescriptorSet() = default;
	DescriptorSet(const DescriptorSet&) = delete;
	DescriptorSet(DescriptorSet&&) = delete;
	~DescriptorSet() = default;

	DescriptorSet& operator=(const DescriptorSet&) = delete;
	DescriptorSet&& operator=(const DescriptorSet&&) = delete;
	
	void Update(const VkWriteDescriptorSet& descriptorWrite);

	static VkResult Create(VkDescriptorPool descriptorPool, VkDescriptorSetLayout pSetLayout, VkDescriptorSet* pDescriptorSet);

	[[nodiscard]] const std::vector<std::tuple<VkDescriptorType, uint32_t, Bindings>>& getBindings() const { return bindings; }

private:
	DescriptorSetLayout* layout{};

	std::vector<std::tuple<VkDescriptorType, uint32_t, Bindings>> bindings{};
};