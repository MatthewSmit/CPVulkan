#include "DescriptorSet.h"

#include "DescriptorSetLayout.h"
#include "Device.h"

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

	for (auto& binding : bindings)
	{
		if (std::get<0>(binding) == descriptorWrite.descriptorType && std::get<1>(binding) == descriptorWrite.dstBinding)
		{
			Bindings newBinding{};
			switch (descriptorWrite.descriptorType)
			{
			case VK_DESCRIPTOR_TYPE_SAMPLER:
			case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
			case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
			case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
				newBinding.ImageInfo = *descriptorWrite.pImageInfo;
				break;
				
			case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
			case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
				newBinding.TexelBufferView = *descriptorWrite.pTexelBufferView;
				break;
				
			case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
			case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER: 
			case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC: 
			case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC: 
				newBinding.BufferInfo = *descriptorWrite.pBufferInfo;
				break;
				
			case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
				FATAL_ERROR();
				
			case VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK_EXT:
				FATAL_ERROR();
				
			case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV:
				FATAL_ERROR();
				
			default:
				FATAL_ERROR();
			}
			binding = std::make_tuple(descriptorWrite.descriptorType, descriptorWrite.dstBinding, newBinding);
			return;
		}
	}
	
	FATAL_ERROR();
}

void Device::UpdateDescriptorSets(uint32_t descriptorWriteCount, const VkWriteDescriptorSet* pDescriptorWrites, uint32_t descriptorCopyCount, const VkCopyDescriptorSet* pDescriptorCopies)
{
	for (auto i = 0u; i < descriptorWriteCount; i++)
	{
		const auto& descriptorWrite = pDescriptorWrites[i];
		assert(descriptorWrite.sType == VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET);

		auto next = descriptorWrite.pNext;
		while (next)
		{
			const auto type = static_cast<const VkBaseInStructure*>(next)->sType;
			switch (type)
			{
			case VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_NV:
				FATAL_ERROR();

			case VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_INLINE_UNIFORM_BLOCK_EXT:
				FATAL_ERROR();

			default:
				break;
			}
			next = static_cast<const VkBaseInStructure*>(next)->pNext;
		}

		UnwrapVulkan<DescriptorSet>(descriptorWrite.dstSet)->Update(descriptorWrite);
	}

	for (auto i = 0u; i < descriptorCopyCount; i++)
	{
		FATAL_ERROR();
	}
}

VkResult DescriptorSet::Create(VkDescriptorPool descriptorPool, VkDescriptorSetLayout pSetLayout, VkDescriptorSet* pDescriptorSet)
{
	auto descriptorSet = Allocate<DescriptorSet>(nullptr, VK_SYSTEM_ALLOCATION_SCOPE_DEVICE);
	if (!descriptorSet)
	{
		return VK_ERROR_OUT_OF_HOST_MEMORY;
	}

	const auto descriptorSetLayout = UnwrapVulkan<DescriptorSetLayout>(pSetLayout);
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

	WrapVulkan(descriptorSet, pDescriptorSet);
	return VK_SUCCESS;
}

VkResult Device::AllocateDescriptorSets(const VkDescriptorSetAllocateInfo* pAllocateInfo, VkDescriptorSet* pDescriptorSets)
{
	assert(pAllocateInfo->sType == VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO);

	auto next = pAllocateInfo->pNext;
	while (next)
	{
		const auto type = static_cast<const VkBaseInStructure*>(next)->sType;
		switch (type)
		{
		case VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO_EXT:
			FATAL_ERROR();

		default:
			break;
		}
		next = static_cast<const VkBaseInStructure*>(next)->pNext;
	}

	for (auto i = 0u; i < pAllocateInfo->descriptorSetCount; i++)
	{
		const auto result = DescriptorSet::Create(pAllocateInfo->descriptorPool, pAllocateInfo->pSetLayouts[i], &pDescriptorSets[i]);
		if (result != VK_SUCCESS)
		{
			FATAL_ERROR();
		}
	}

	return VK_SUCCESS;
}

VkResult Device::FreeDescriptorSets(VkDescriptorPool descriptorPool, uint32_t descriptorSetCount, const VkDescriptorSet* pDescriptorSets)
{
	for (auto i = 0u; i < descriptorSetCount; i++)
	{
		if (pDescriptorSets[i])
		{
			Free(UnwrapVulkan<DescriptorSet>(pDescriptorSets[i]), nullptr);
		}
	}

	return VK_SUCCESS;
}
