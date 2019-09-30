#pragma once
#include "Base.h"

class Sampler final
{
public:
	Sampler() = default;
	Sampler(const Sampler&) = delete;
	Sampler(Sampler&&) = delete;
	~Sampler() = default;

	Sampler& operator=(const Sampler&) = delete;
	Sampler&& operator=(const Sampler&&) = delete;

	void OnDelete(const VkAllocationCallbacks*)
	{
	}
	
	static VkResult Create(const VkSamplerCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSampler* pSampler);
	
	[[nodiscard]] VkSamplerCreateFlags getFlags() const { return flags; };
	[[nodiscard]] VkFilter getMagFilter() const { return magFilter; };
	[[nodiscard]] VkFilter getMinFilter() const { return minFilter; };
	[[nodiscard]] VkSamplerMipmapMode getMipmapMode() const { return mipmapMode; };
	[[nodiscard]] VkSamplerAddressMode getAddressModeU() const { return addressModeU; };
	[[nodiscard]] VkSamplerAddressMode getAddressModeV() const { return addressModeV; };
	[[nodiscard]] VkSamplerAddressMode getAddressModeW() const { return addressModeW; };
	[[nodiscard]] float getMipLodBias() const { return mipLodBias; };
	[[nodiscard]] bool getAnisotropyEnable() const { return anisotropyEnable; };
	[[nodiscard]] float getMaxAnisotropy() const { return maxAnisotropy; };
	[[nodiscard]] bool getCompareEnable() const { return compareEnable; };
	[[nodiscard]] VkCompareOp getCompareOp() const { return compareOp; };
	[[nodiscard]] float getMinLod() const { return minLod; };
	[[nodiscard]] float getMaxLod() const { return maxLod; };
	[[nodiscard]] VkBorderColor getBorderColor() const { return borderColor; };
	[[nodiscard]] bool getUnnormalisedCoordinates() const { return unnormalisedCoordinates; };

private:
	VkSamplerCreateFlags flags{};
	VkFilter magFilter{};
	VkFilter minFilter{};
	VkSamplerMipmapMode mipmapMode{};
	VkSamplerAddressMode addressModeU{};
	VkSamplerAddressMode addressModeV{};
	VkSamplerAddressMode addressModeW{};
	float mipLodBias{};
	bool anisotropyEnable{};
	float maxAnisotropy{};
	bool compareEnable{};
	VkCompareOp compareOp{};
	float minLod{};
	float maxLod{};
	VkBorderColor borderColor{};
	bool unnormalisedCoordinates{};
};