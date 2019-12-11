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
			TODO_ERROR();
		}
		
		if (targetArrayElement >= bindingValues[targetBinding].count)
		{
			TODO_ERROR();
		}

		if (descriptorWrite.descriptorType != bindingTypes[targetBinding])
		{
			TODO_ERROR();
		}

		auto& value = bindingValues[targetBinding].values[targetArrayElement];

		switch (descriptorWrite.descriptorType)
		{
		case VK_DESCRIPTOR_TYPE_SAMPLER:
			value.Image.Type = ImageDescriptorType::None;
			value.Image.Data.Image = nullptr;
			value.Image.ImageSampler = UnwrapVulkan<Sampler>(descriptorWrite.pImageInfo->sampler);
			break;
						
		case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
			value.Image.Type = ImageDescriptorType::Image;
			value.Image.Data.Image = UnwrapVulkan<ImageView>(descriptorWrite.pImageInfo->imageView);
			value.Image.ImageSampler = UnwrapVulkan<Sampler>(descriptorWrite.pImageInfo->sampler);
			break;
						
		case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
		case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
			value.Image.Type = ImageDescriptorType::Image;
			value.Image.Data.Image = UnwrapVulkan<ImageView>(descriptorWrite.pImageInfo->imageView);
			value.Image.ImageSampler = nullptr;
			break;
			
		case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
		case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
			value.Image.Type = ImageDescriptorType::Buffer;
			value.Image.Data.Buffer = UnwrapVulkan<BufferView>(*descriptorWrite.pTexelBufferView);
			value.Image.ImageSampler = nullptr;
			break;
			
		case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
		case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
		case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
		case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
			value.Buffer = *descriptorWrite.pBufferInfo;
			if (value.Buffer.range == VK_WHOLE_SIZE)
			{
				value.Buffer.range = UnwrapVulkan<Buffer>(value.Buffer.buffer)->getSize() - value.Buffer.offset;
			}
			break;
			
		case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
			TODO_ERROR();
			
		case VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK_EXT:
			TODO_ERROR();
			
		case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV:
			TODO_ERROR();
			
		default:
			FATAL_ERROR();
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
				TODO_ERROR();

			case VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_INLINE_UNIFORM_BLOCK_EXT:
				TODO_ERROR();

			default:
				break;
			}
			next = static_cast<const VkBaseInStructure*>(next)->pNext;
		}

		UnwrapVulkan<DescriptorSet>(descriptorWrite.dstSet)->Update(descriptorWrite);
	}

	for (auto i = 0u; i < descriptorCopyCount; i++)
	{
		TODO_ERROR();
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
			TODO_ERROR();
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
			TODO_ERROR();

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
			TODO_ERROR();
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
