#include "DescriptorSet.h"

#include "Buffer.h"
#include "DescriptorPool.h"
#include "DescriptorSetLayout.h"
#include "Device.h"

VkResult DescriptorSet::Initialise(DescriptorSetLayout* descriptorSetLayout)
{
	layout = descriptorSetLayout;

	auto numberBindings = 0u;
	for (const auto& binding : descriptorSetLayout->getBindings())
	{
		numberBindings = std::max(numberBindings, binding.binding + 1);
	}

	bindingTypes = std::vector<VkDescriptorType>(numberBindings);
	bindingValues = std::vector<Descriptor>(numberBindings);
	for (auto i = 0u; i < numberBindings; i++)
	{
		bindingValues[i].count = 0;
	}

	for (const auto& binding : descriptorSetLayout->getBindings())
	{
		assert(bindingValues[binding.binding].count == 0);
		bindingTypes[binding.binding] = binding.descriptorType;
		bindingValues[binding.binding].immutable = binding.pImmutableSamplers != nullptr;
		bindingValues[binding.binding].count = binding.descriptorCount;
		bindingValues[binding.binding].values = std::unique_ptr<DescriptorValue[]>(new DescriptorValue[binding.descriptorCount]);

		for (auto i = 0u; i < binding.descriptorCount; i++)
		{
			bindingValues[binding.binding].values[i] = {};
		}
		
		if (binding.pImmutableSamplers)
		{
			assert(binding.descriptorType == VK_DESCRIPTOR_TYPE_SAMPLER || binding.descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
			for (auto i = 0u; i < binding.descriptorCount; i++)
			{
				bindingValues[binding.binding].values[i].Image.Type = ImageDescriptorType::None;
				bindingValues[binding.binding].values[i].Image.Data.Image = nullptr;
				bindingValues[binding.binding].values[i].Image.ImageSampler = binding.pImmutableSamplers[i]
					                                                              ? UnwrapVulkan<Sampler>(binding.pImmutableSamplers[i])
					                                                              : nullptr;
			}
		}
	}

	return VK_SUCCESS;
}

void DescriptorSet::Update(const VkWriteDescriptorSet& descriptorWrite)
{
	const auto targetBinding = descriptorWrite.dstBinding;
	auto targetArrayElement = descriptorWrite.dstArrayElement;
	for (auto i = 0u; i < descriptorWrite.descriptorCount; i++)
	{
		if (targetBinding >= getNumberBindings())
		{
			FATAL_ERROR();
		}
		
		if (targetArrayElement >= bindingValues[targetBinding].count)
		{
			TODO_ERROR();
		}

		if (descriptorWrite.descriptorType != bindingTypes[targetBinding])
		{
			FATAL_ERROR();
		}

		auto& value = bindingValues[targetBinding].values[targetArrayElement];

		switch (descriptorWrite.descriptorType)
		{
		case VK_DESCRIPTOR_TYPE_SAMPLER:
			if (!bindingValues[targetBinding].immutable)
			{
				value.Image.Type = ImageDescriptorType::None;
				value.Image.Data.Image = nullptr;
				value.Image.ImageSampler = UnwrapVulkan<Sampler>(descriptorWrite.pImageInfo[i].sampler);
			}
			break;
						
		case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
		case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
			value.Image.Type = ImageDescriptorType::Image;
			value.Image.Data.Image = UnwrapVulkan<ImageView>(descriptorWrite.pImageInfo[i].imageView);
			if (!bindingValues[targetBinding].immutable)
			{
				value.Image.ImageSampler = UnwrapVulkan<Sampler>(descriptorWrite.pImageInfo[i].sampler);
			}
			else
			{
				assert(value.Image.ImageSampler);
			}
			break;
						
		case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
		case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
			value.Image.Type = ImageDescriptorType::Image;
			value.Image.Data.Image = UnwrapVulkan<ImageView>(descriptorWrite.pImageInfo[i].imageView);
			value.Image.ImageSampler = nullptr;
			break;
			
		case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
		case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
			value.Image.Type = ImageDescriptorType::Buffer;
			value.Image.Data.Buffer = UnwrapVulkan<BufferView>(descriptorWrite.pTexelBufferView[i]);
			value.Image.ImageSampler = nullptr;
			break;
			
		case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
		case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
		case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
		case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
			value.Buffer = descriptorWrite.pBufferInfo[i];
			if (value.Buffer.range == VK_WHOLE_SIZE)
			{
				// TODO: might be buggy with dynamic offsets
				value.Buffer.range = UnwrapVulkan<Buffer>(value.Buffer.buffer)->getSize() - value.Buffer.offset;
			}
			break;
			
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

void DescriptorSet::CopyFrom(const VkCopyDescriptorSet& descriptorCopy)
{
	const auto source = UnwrapVulkan<DescriptorSet>(descriptorCopy.srcSet);

	const auto srcBinding = descriptorCopy.srcBinding;
	auto srcArrayElement = descriptorCopy.srcArrayElement;
	const auto dstBinding = descriptorCopy.dstBinding;
	auto dstArrayElement = descriptorCopy.dstArrayElement;

	for (auto i = 0u; i < descriptorCopy.descriptorCount; i++)
	{
		if (srcBinding >= source->getNumberBindings())
		{
			FATAL_ERROR();
		}

		if (dstBinding >= getNumberBindings())
		{
			FATAL_ERROR();
		}

		if (srcArrayElement >= source->bindingValues[srcBinding].count)
		{
			TODO_ERROR();
		}

		if (dstArrayElement >= bindingValues[dstBinding].count)
		{
			TODO_ERROR();
		}

		if (bindingTypes[dstBinding] != source->bindingTypes[srcBinding])
		{
			FATAL_ERROR();
		}

		if (bindingValues[dstBinding].immutable)
		{
			bindingValues[dstBinding].values[dstArrayElement].Image.Type = source->bindingValues[srcBinding].values[srcArrayElement].Image.Type;
			bindingValues[dstBinding].values[dstArrayElement].Image.Data = source->bindingValues[srcBinding].values[srcArrayElement].Image.Data;
		}
		else
		{
			bindingValues[dstBinding].values[dstArrayElement] = source->bindingValues[srcBinding].values[srcArrayElement];
		}

		srcArrayElement += 1;
		dstArrayElement += 1;
	}
}

void Device::UpdateDescriptorSets(uint32_t descriptorWriteCount, const VkWriteDescriptorSet* pDescriptorWrites, uint32_t descriptorCopyCount, const VkCopyDescriptorSet* pDescriptorCopies)
{
	for (auto i = 0u; i < descriptorWriteCount; i++)
	{
		const auto& descriptorWrite = pDescriptorWrites[i];
		assert(descriptorWrite.sType == VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET);

		auto next = static_cast<const VkBaseInStructure*>(descriptorWrite.pNext);
		while (next)
		{
			const auto type = next->sType;
			switch (type)
			{
			case VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_NV:
				TODO_ERROR();

			case VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_INLINE_UNIFORM_BLOCK_EXT:
				TODO_ERROR();

			default:
				break;
			}
			next = next->pNext;
		}

		UnwrapVulkan<DescriptorSet>(descriptorWrite.dstSet)->Update(descriptorWrite);
	}

	for (auto i = 0u; i < descriptorCopyCount; i++)
	{
		const auto& descriptorCopy = pDescriptorCopies[i];
		assert(descriptorCopy.sType == VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET);

		UnwrapVulkan<DescriptorSet>(descriptorCopy.dstSet)->CopyFrom(descriptorCopy);
	}
}

VkResult DescriptorSet::Create(DescriptorPool* descriptorPool, VkDescriptorSetLayout pSetLayout, VkDescriptorSet* pDescriptorSet)
{
	auto descriptorSet = descriptorPool->CreateDescriptorSet();
	if (!descriptorSet)
	{
		return VK_ERROR_OUT_OF_HOST_MEMORY;
	}

	const auto result = descriptorSet->Initialise(UnwrapVulkan<DescriptorSetLayout>(pSetLayout));
	WrapVulkan(descriptorSet, pDescriptorSet);
	return result;
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
			for (auto j = 0u; j < i; j++)
			{
				if (pDescriptorSets[j])
				{
					UnwrapVulkan<DescriptorPool>(pAllocateInfo->descriptorPool)->FreeDescriptorSet(UnwrapVulkan<DescriptorSet>(pDescriptorSets[j]));
				}
			}

			for (auto j = 0u; j < pAllocateInfo->descriptorSetCount; j++)
			{
				pDescriptorSets[j] = VK_NULL_HANDLE;
			}
			
			return result;
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
			UnwrapVulkan<DescriptorPool>(descriptorPool)->FreeDescriptorSet(UnwrapVulkan<DescriptorSet>(pDescriptorSets[i]));
		}
	}

	return VK_SUCCESS;
}