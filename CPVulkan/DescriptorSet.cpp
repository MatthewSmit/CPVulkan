#include "DescriptorSet.h"

#include "DescriptorSetLayout.h"

void DescriptorSet::Update(const VkWriteDescriptorSet& descriptorWrite)
{
	if (descriptorWrite.dstArrayElement)
	{
		FATAL_ERROR();
	}

	if (descriptorWrite.descriptorCount != 1)
	{
		FATAL_ERROR();
	}

	for (auto i = 0u; i < bindings.size(); i++)
	{
		const auto& binding = bindings[i];
		if (std::get<0>(binding) == descriptorWrite.descriptorType && std::get<1>(binding) == descriptorWrite.dstBinding)
		{
			Bindings newBinding{};
			switch (descriptorWrite.descriptorType)
			{
			case VK_DESCRIPTOR_TYPE_SAMPLER:
				FATAL_ERROR();
			case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
				FATAL_ERROR();
			case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE: 
				FATAL_ERROR();
			case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE: 
				FATAL_ERROR();
			case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER: 
				FATAL_ERROR();
			case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER: 
				FATAL_ERROR();
			case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
				newBinding.BufferInfo = *descriptorWrite.pBufferInfo;
				bindings[i] = std::make_tuple(descriptorWrite.descriptorType, descriptorWrite.dstBinding, newBinding);
				break;
			case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER: 
				FATAL_ERROR();
			case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC: 
				FATAL_ERROR();
			case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC: 
				FATAL_ERROR();
			case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
				FATAL_ERROR();
			case VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK_EXT:
				FATAL_ERROR();
			case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV:
				FATAL_ERROR();
			default:
				FATAL_ERROR();
			}
			return;
		}
	}
	
	FATAL_ERROR();
}

VkResult DescriptorSet::Create(VkDescriptorPool descriptorPool, VkDescriptorSetLayout pSetLayout, VkDescriptorSet* pDescriptorSet)
{
	auto descriptorSet = Allocate<DescriptorSet>(nullptr);

	const auto descriptorSetLayout = reinterpret_cast<DescriptorSetLayout*>(pSetLayout);
	descriptorSet->layout = descriptorSetLayout;

	for (const auto& binding : descriptorSetLayout->getBindings())
	{
		if (binding.descriptorCount != 1)
		{
			FATAL_ERROR();
		}

		// TODO: Stage flags

		if (binding.pImmutableSamplers)
		{
			FATAL_ERROR();
		}
		
		descriptorSet->bindings.emplace_back(std::make_tuple(binding.descriptorType, binding.binding, Bindings{}));
	}

	*pDescriptorSet = reinterpret_cast<VkDescriptorSet>(descriptorSet);
	return VK_SUCCESS;
}
