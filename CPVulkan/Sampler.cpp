#include "Sampler.h"

#include "Device.h"

#include <cassert>

VkResult Sampler::Create(const VkSamplerCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSampler* pSampler)
{
	assert(pCreateInfo->sType == VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO);

	auto sampler = Allocate<Sampler>(pAllocator, VK_SYSTEM_ALLOCATION_SCOPE_DEVICE);

	auto next = pCreateInfo->pNext;
	while (next)
	{
		auto type = *static_cast<const VkStructureType*>(next);
		switch (type)
		{
		default:
			FATAL_ERROR();
		}
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
	sampler->maxAnisotropy = pCreateInfo->maxAnisotropy;
	sampler->compareEnable = pCreateInfo->compareEnable;
	sampler->compareOp = pCreateInfo->compareOp;
	sampler->minLod = pCreateInfo->minLod;
	sampler->maxLod = pCreateInfo->maxLod;
	sampler->borderColor = pCreateInfo->borderColor;
	sampler->unnormalizedCoordinates = pCreateInfo->unnormalizedCoordinates;
	
	*pSampler = reinterpret_cast<VkSampler>(sampler);
	return VK_SUCCESS;
}

VkResult Device::CreateSampler(const VkSamplerCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSampler* pSampler)
{
	return Sampler::Create(pCreateInfo, pAllocator, pSampler);
}

void Device::DestroySampler(VkSampler sampler, const VkAllocationCallbacks* pAllocator)
{
	Free(reinterpret_cast<Sampler*>(sampler), pAllocator);
}
