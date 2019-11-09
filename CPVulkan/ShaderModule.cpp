#include "ShaderModule.h"

#include "Device.h"

// #include "SpirvParser.h"
// #include "../SPIRVToLLVM/Converter.h"
#include <SPIRVModule.h>

#include <cassert>

struct membuf : std::streambuf
{
	membuf(char const* base, size_t size)
	{
		const auto p(const_cast<char*>(base));
		this->setg(p, p, p + size);
	}
};

struct imemstream final : virtual membuf, std::istream
{
	imemstream(char const* base, size_t size)
		: membuf(base, size), std::istream(static_cast<std::streambuf*>(this))
	{
	}
};

ShaderModule::~ShaderModule()
{
	// TODO: Delete once pipeline takes ownership
	// delete module;
}

VkResult ShaderModule::Create(const VkShaderModuleCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkShaderModule* pShaderModule)
{
	assert(pCreateInfo->sType == VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO);
	assert(pCreateInfo->codeSize % 4 == 0);

	auto shaderModule = Allocate<ShaderModule>(pAllocator, VK_SYSTEM_ALLOCATION_SCOPE_DEVICE);
	if (!shaderModule)
	{
		return VK_ERROR_OUT_OF_HOST_MEMORY;
	}

	auto next = static_cast<const VkBaseInStructure*>(pCreateInfo->pNext);
	while (next)
	{
		const auto type = next->sType;
		switch (type)
		{
		case VK_STRUCTURE_TYPE_SHADER_MODULE_VALIDATION_CACHE_CREATE_INFO_EXT:
			FATAL_ERROR();
		}
		next = next->pNext;
	}

	if (pCreateInfo->flags)
	{
		FATAL_ERROR();
	}

	imemstream stream{reinterpret_cast<const char*>(pCreateInfo->pCode), pCreateInfo->codeSize};

	SPIRV::TranslatorOptions options{};
	shaderModule->module = SPIRV::SPIRVModule::createSPIRVModule(options);
	stream >> *shaderModule->module;
	if (!shaderModule->module->isModuleValid())
	{
		// BM->getError(ErrMsg);
		// return nullptr;
		FATAL_ERROR();
	}

	WrapVulkan(shaderModule, pShaderModule);
	return VK_SUCCESS;
}

VkResult Device::CreateShaderModule(const VkShaderModuleCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkShaderModule* pShaderModule)
{
	return ShaderModule::Create(pCreateInfo, pAllocator, pShaderModule);
}

void Device::DestroyShaderModule(VkShaderModule shaderModule, const VkAllocationCallbacks* pAllocator)
{
	if (shaderModule)
	{
		Free(UnwrapVulkan<ShaderModule>(shaderModule), pAllocator);
	}
}
