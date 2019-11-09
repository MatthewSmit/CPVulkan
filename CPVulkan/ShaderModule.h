#pragma once
#include "Base.h"

namespace SPIRV
{
	class SPIRVModule;
}

class ShaderModule final
{
public:
	ShaderModule() = default;
	ShaderModule(const ShaderModule&) = delete;
	ShaderModule(ShaderModule&&) = delete;
	~ShaderModule();

	ShaderModule& operator=(const ShaderModule&) = delete;
	ShaderModule&& operator=(const ShaderModule&&) = delete;

	void OnDelete(const VkAllocationCallbacks*)
	{
	}

	static VkResult Create(const VkShaderModuleCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkShaderModule* pShaderModule);

	[[nodiscard]] const SPIRV::SPIRVModule* getModule() const { return module; }

private:
	SPIRV::SPIRVModule* module;
};
