#include "Sampler.h"

#include "Device.h"

#include <cassert>

VkResult Sampler::Create(const VkSamplerCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSampler* pSampler)
{
	assert(pCreateInfo->sType == VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO);

	auto sampler = Allocate<Sampler>(pAllocator, VK_SYSTEM_ALLOCATION_SCOPE_DEVICE);
	if (!sampler)
	{
		return VK_ERROR_OUT_OF_HOST_MEMORY;
	}

	auto next = pCreateInfo->pNext;
	while (next)
	{
		const auto type = static_cast<const VkBaseInStructure*>(next)->sType;
		switch (type)
		{
		case VK_STRUCTURE_TYPE_SAMPLER_REDUCTION_MODE_CREATE_INFO_EXT:
			FATAL_ERROR();
			
		case VK_STRUCTURE_TYPE_SAMPLER_YCBCR_CONVERSION_CREATE_INFO:
			FATAL_ERROR();

		default:
			break;
		}
		next = static_cast<const VkBaseInStructure*>(next)->pNext;
	}
	
	sampler->flags = pCreateInfo->flags;
	sampler->magFilter = pCreateInfo->magFilter;
	sampler->minFilter = pCreateInfo->minFilter;
	sampler->mipmapMode = pCreateInfo->mipmapMode;
	sampler->addressModeU = pCreateInfo->addressModeU;
	sampler->addressModeV = pCreateInfo->addressModeV;
	sampler->addressModeW = pCreateInfo->addressModeW;
	sampler->mipLodBias = pCreateInfo->mipLodBias;
	sampler->anisotropyEnable = pCreateInfo->anisotropyEnable;
	sampler->maxAnisotropy = std::min(pCreateInfo->maxAnisotropy, MAX_SAMPLER_ANISOTROPY);
	sampler->compareEnable = pCreateInfo->compareEnable;
	sampler->compareOp = pCreateInfo->compareOp;
	sampler->minLod = pCreateInfo->minLod;
	sampler->maxLod = pCreateInfo->maxLod;
	sampler->borderColour = pCreateInfo->borderColor;
	sampler->unnormalisedCoordinates = pCreateInfo->unnormalizedCoordinates;

	WrapVulkan(sampler, pSampler);
	return VK_SUCCESS;
}

VkResult Device::CreateSampler(const VkSamplerCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSampler* pSampler)
{
	return Sampler::Create(pCreateInfo, pAllocator, pSampler);
}

void Device::DestroySampler(VkSampler sampler, const VkAllocationCallbacks* pAllocator)
{
	if (sampler)
	{
		Free(UnwrapVulkan<Sampler>(sampler), pAllocator);
	}
}
