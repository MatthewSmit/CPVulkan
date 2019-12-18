#pragma once
#include <Base.h>

class CompiledModule;
class CPJit;

namespace SPIRV
{
	class SPIRVFunction;
	class SPIRVModule;
	class SPIRVVariable;
}

CP_DLL_EXPORT std::string MangleName(const SPIRV::SPIRVVariable* variable);
CP_DLL_EXPORT std::string MangleName(const SPIRV::SPIRVFunction* function);