#include "DescriptorSetLayout.h"

#include "Device.h"
#include "Util.h"

#include <cassert>

DescriptorSetLayout::~DescriptorSetLayout()
{
	for (const auto& binding : bindings)
	{
		if (binding.pImmutableSamplers)
		{
			delete[] binding.pImmutableSamplers;
		}
	}
}

VkResult DescriptorSetLayout::Create(const VkDescriptorSetLayoutCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDescriptorSetLayout* pSetLayout)
{
	assert(pCreateInfo->sType == VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO);

	auto descriptorSetLayout = Allocate<DescriptorSetLayout>(pAllocator, VK_SYSTEM_ALLOCATION_SCOPE_DEVICE);
	if (!descriptorSetLayout)
	{
		return VK_ERROR_OUT_OF_HOST_MEMORY;
	}

	auto next = static_cast<const VkBaseInStructure*>(pCreateInfo->pNext);
	while (next)
	{
		const auto type = next->sType;
		switch (type)
		{
		case VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT:
			{
				// Not likely we need these flags currently, so we just ignore them
				const auto bindingLayoutFlagsInfo = reinterpret_cast<const VkDescriptorSetLayoutBindingFlagsCreateInfoEXT*>(next);
				break;
			}
		}
		next = next->pNext;
	}

	descriptorSetLayout->bindings = ArrayToVector(pCreateInfo->bindingCount, pCreateInfo->pBindings);
	for (auto& binding : descriptorSetLayout->bindings)
	{
		if ((binding.descriptorType == VK_DESCRIPTOR_TYPE_SAMPLER || binding.descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) && binding.pImmutableSamplers)
		{
			const auto newSamplers = new VkSampler[binding.descriptorCount];
			memcpy(newSamplers, binding.pImmutableSamplers, sizeof(VkSampler) * binding.descriptorCount);
			binding.pImmutableSamplers = newSamplers;
		}
		else
		{
			binding.pImmutableSamplers = nullptr;
		}
	}

	WrapVulkan(descriptorSetLayout, pSetLayout);
	return VK_SUCCESS;
}

VkResult Device::CreateDescriptorSetLayout(const VkDescriptorSetLayoutCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDescriptorSetLayout* pSetLayout)
{
	return DescriptorSetLayout::Create(pCreateInfo, pAllocator, pSetLayout);
}

void Device::DestroyDescriptorSetLayout(VkDescriptorSetLayout descriptorSetLayout, const VkAllocationCallbacks* pAllocator)
{
	if (descriptorSetLayout)
	{
		Free(UnwrapVulkan<DescriptorSetLayout>(descriptorSetLayout), pAllocator);
	}
}

void Device::GetDescriptorSetLayoutSupport(const VkDescriptorSetLayoutCreateInfo* pCreateInfo, VkDescriptorSetLayoutSupport* pSupport)
{
	// TODO

	auto next = static_cast<const VkBaseInStructure*>(pCreateInfo->pNext);
	while (next)
	{
		const auto type = next->sType;
		switch (type)
		{
		case VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT:
			TODO_ERROR();
		}
		next = next->pNext;
	}
	
	pSupport->supported = true;
}