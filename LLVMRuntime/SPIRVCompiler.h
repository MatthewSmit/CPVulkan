#pragma once
#include <Base.h>

#include "spirv.hpp"

namespace llvm
{
	class LLVMContext;
	class Module;
}

namespace SPIRV
{
	class SPIRVModule;
	class SPIRVVariable;
}

std::unique_ptr<llvm::Module> ConvertSpirv(llvm::LLVMContext* context, const SPIRV::SPIRVModule* spirvModule, spv::ExecutionModel executionModel, const VkSpecializationInfo* specializationInfo);

std::string CP_DLL_EXPORT MangleName(const SPIRV::SPIRVVariable* variable);