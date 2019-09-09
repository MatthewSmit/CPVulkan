#pragma once
#include "Base.h"

class Sampler final : public VulkanBase
{
public:
	~Sampler() override = default;
	
	static VkResult Create(const VkSamplerCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSampler* pSampler);

private:
	VkSamplerCreateFlags flags{};
	VkFilter magFilter{};
	VkFilter minFilter{};
	VkSamplerMipmapMode mipmapMode{};
	VkSamplerAddressMode addressModeU{};
	VkSamplerAddressMode addressModeV{};
	VkSamplerAddressMode addressModeW{};
	float mipLodBias{};
	VkBool32 anisotropyEnable{};
	float maxAnisotropy{};
	VkBool32 compareEnable{};
	VkCompareOp compareOp{};
	float minLod{};
	float maxLod{};
	VkBorderColor borderColor{};
	VkBool32 unnormalizedCoordinates{};
};