#pragma once
#include "CompiledModuleBuilder.h"

namespace SPIRV
{
	class SPIRVType;
	class SPIRVTypeImage;
	class SPIRVValue;
}

class SPIRVBaseCompiledModuleBuilder : public CompiledModuleBuilder
{
protected:
	std::unordered_map<uint32_t, LLVMTypeRef> typeMapping{};
	std::unordered_map<LLVMTypeRef, uint32_t> arrayStrideMultiplier{};
	std::unordered_map<LLVMTypeRef, std::vector<uint32_t>> structIndexMapping{};

	template<typename T>
	T GetConstant(LLVMValueRef value);
	
	LLVMTypeRef ConvertType(const SPIRV::SPIRVType* spirvType, bool isClassMember = false);

	virtual LLVMValueRef ConvertValue(const SPIRV::SPIRVValue* spirvValue, LLVMValueRef currentFunction) = 0;

	LLVMTypeRef CreateOpaqueImageType(const std::string& name);
	
	static std::string GetTypeName(const char* baseName, const std::string& postfixes);
	
	static std::string GetTypeName(const SPIRV::SPIRVType* type);

	static std::string GetImageTypeName(const SPIRV::SPIRVTypeImage* imageType);
};
