#include "DescriptorSet.h"

#include "Buffer.h"
#include "DescriptorPool.h"
#include "DescriptorSetLayout.h"
#include "Device.h"

void DescriptorSet::Update(const VkWriteDescriptorSet& descriptorWrite)
{
	auto targetBinding = descriptorWrite.dstBinding;
	auto targetArrayElement = descriptorWrite.dstArrayElement;
	for (auto i = 0u; i < descriptorWrite.descriptorCount; i++)
	{
		if (targetBinding >= numberBindings)
		{
			FATAL_ERROR();
		}
		
		if (targetArrayElement >= bindingValues[targetBinding].count)
		{
			FATAL_ERROR();
		}

		if (descriptorWrite.descriptorType != bindingTypes[targetBinding])
		{
			FATAL_ERROR();
		}

		auto& value = bindingValues[targetBinding].values[targetArrayElement];

		switch (descriptorWrite.descriptorType)
		{
		case VK_DESCRIPTOR_TYPE_SAMPLER:
			value.ImageDescriptor.Type = ImageDescriptorType::None;
			value.ImageDescriptor.Data.Image = nullptr;
			value.ImageDescriptor.Sampler = UnwrapVulkan<Sampler>(descriptorWrite.pImageInfo->sampler);
			break;
						
		case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
			value.ImageDescriptor.Type = ImageDescriptorType::Image;
			value.ImageDescriptor.Data.Image = UnwrapVulkan<ImageView>(descriptorWrite.pImageInfo->imageView);
			value.ImageDescriptor.Sampler = UnwrapVulkan<Sampler>(descriptorWrite.pImageInfo->sampler);
			break;
						
		case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
			value.ImageDescriptor.Type = ImageDescriptorType::Image;
			value.ImageDescriptor.Data.Image = UnwrapVulkan<ImageView>(descriptorWrite.pImageInfo->imageView);
			value.ImageDescriptor.Sampler = nullptr;
			break;
			
		case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE: FATAL_ERROR();
			
		case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
		case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
			value.ImageDescriptor.Type = ImageDescriptorType::Buffer;
			value.ImageDescriptor.Data.Buffer = UnwrapVulkan<BufferView>(*descriptorWrite.pTexelBufferView);
			value.ImageDescriptor.Sampler = nullptr;
			break;
			
		case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
		case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
		case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
		case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
			value.BufferInfo = *descriptorWrite.pBufferInfo;
			if (value.BufferInfo.range == VK_WHOLE_SIZE)
			{
				value.BufferInfo.range = UnwrapVulkan<Buffer>(value.BufferInfo.buffer)->getSize() - value.BufferInfo.offset;
			}
			break;
			
		case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT: FATAL_ERROR();
		case VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK_EXT: FATAL_ERROR();
		case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV: FATAL_ERROR();
		default: FATAL_ERROR();
		}

		targetArrayElement += 1;
	}
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

VkResult DescriptorSet::Create(DescriptorPool* descriptorPool, VkDescriptorSetLayout pSetLayout, VkDescriptorSet* pDescriptorSet)
{
	auto descriptorSet = descriptorPool->createDescriptorPool();
	if (!descriptorSet)
	{
		return VK_ERROR_OUT_OF_HOST_MEMORY;
	}

	const auto descriptorSetLayout = UnwrapVulkan<DescriptorSetLayout>(pSetLayout);
	descriptorSet->layout = descriptorSetLayout;

	for (const auto& binding : descriptorSetLayout->getBindings())
	{
		descriptorSet->numberBindings = std::max(descriptorSet->numberBindings, binding.binding + 1);
	}

	descriptorSet->bindingTypes = std::unique_ptr<VkDescriptorType[]>(new VkDescriptorType[descriptorSet->numberBindings]);
	descriptorSet->bindingValues = std::unique_ptr<Descriptor[]>(new Descriptor[descriptorSet->numberBindings]);
	for (auto i = 0u; i < descriptorSet->numberBindings; i++)
	{
		descriptorSet->bindingValues[i].count = 0;
	}

	for (const auto& binding : descriptorSetLayout->getBindings())
	{
		assert(descriptorSet->bindingValues[binding.binding].count == 0);
		descriptorSet->bindingTypes[binding.binding] = binding.descriptorType;
		descriptorSet->bindingValues[binding.binding].count = binding.descriptorCount;
		descriptorSet->bindingValues[binding.binding].values = std::unique_ptr<DescriptorValue[]>(new DescriptorValue[binding.descriptorCount]);
		for (auto i = 0u; i < binding.descriptorCount; i++)
		{
			descriptorSet->bindingValues[binding.binding].values[i] = {};
		}

		// TODO: Stage flags

		if (binding.pImmutableSamplers)
		{
			FATAL_ERROR();
		}
	}

	WrapVulkan(descriptorSet, pDescriptorSet);
	return VK_SUCCESS;
}

VkResult Device::AllocateDescriptorSets(const VkDescriptorSetAllocateInfo* pAllocateInfo, VkDescriptorSet* pDescriptorSets)
{
	assert(pAllocateInfo->sType == VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO);

	auto next = static_cast<const VkBaseInStructure*>(pAllocateInfo->pNext);
	while (next)
	{
		const auto type = next->sType;
		switch (type)
		{
		case VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO_EXT:
			FATAL_ERROR();

		default:
			break;
		}
		next = next->pNext;
	}

	for (auto i = 0u; i < pAllocateInfo->descriptorSetCount; i++)
	{
		const auto result = DescriptorSet::Create(UnwrapVulkan<DescriptorPool>(pAllocateInfo->descriptorPool), pAllocateInfo->pSetLayouts[i], &pDescriptorSets[i]);
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
			UnwrapVulkan<DescriptorPool>(descriptorPool)->freeDescriptorPool(UnwrapVulkan<DescriptorSet>(pDescriptorSets[i]));
		}
	}

	return VK_SUCCESS;
}
