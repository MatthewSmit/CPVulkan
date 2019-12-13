#pragma once
#include <Base.h>

#include <spirv.hpp>

class CompiledModule;
class CPJit;

namespace SPIRV
{
	class SPIRVFunction;
	class SPIRVModule;
	class SPIRVVariable;
}

CP_DLL_EXPORT CompiledModule* CompileSPIRVModule(CPJit* jit, const SPIRV::SPIRVModule* spirvModule, spv::ExecutionModel executionModel, const SPIRV::SPIRVFunction* entryPoint, const VkSpecializationInfo* specializationInfo);

CP_DLL_EXPORT std::string MangleName(const SPIRV::SPIRVVariable* variable);
CP_DLL_EXPORT std::string MangleName(const SPIRV::SPIRVFunction* function);