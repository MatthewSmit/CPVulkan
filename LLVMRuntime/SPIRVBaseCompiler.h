#pragma once
#include "CompiledModuleBuilder.h"

namespace SPIRV
{
	class SPIRVType;
	class SPIRVTypeImage;
}

class SPIRVBaseCompiledModuleBuilder : public CompiledModuleBuilder
{
public:
	explicit SPIRVBaseCompiledModuleBuilder(CPJit* jit, std::function<void*(const std::string&)> getFunction = nullptr) :
		CompiledModuleBuilder{jit, getFunction}
	{
	}
	
	~SPIRVBaseCompiledModuleBuilder() override = default;

protected:
	std::unordered_map<uint32_t, LLVMTypeRef> typeMapping{};
	std::unordered_map<LLVMTypeRef, uint32_t> arrayStrideMultiplier{};
	std::unordered_map<LLVMTypeRef, std::vector<uint32_t>> structIndexMapping{};
	
	LLVMTypeRef ConvertType(const SPIRV::SPIRVType* spirvType, bool isClassMember = false);

	LLVMTypeRef CreateOpaqueImageType(const std::string& name);
	
	static std::string GetTypeName(const char* baseName, const std::string& postfixes);
	
	static std::string GetTypeName(const SPIRV::SPIRVType* type);

	static std::string GetImageTypeName(const SPIRV::SPIRVTypeImage* imageType);
};
