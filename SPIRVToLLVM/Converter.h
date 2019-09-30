#pragma once
#include "spirv.hpp"

#include <memory>
#include <string>

#if defined(_MSC_VER) && defined(SPIRVToLLVM_EXPORTS)
#define STL_DLL_EXPORT __declspec(dllexport)
#else
#define STL_DLL_EXPORT
#endif

namespace llvm
{
	class LLVMContext;
	class Module;
}

namespace SPIRV
{
	class SPIRVModule;
}

class SpirvCompiledModule;

using FunctionPointer = void (*)();

class STL_DLL_EXPORT SpirvJit
{
public:
	SpirvJit();
	~SpirvJit();

	SpirvCompiledModule* CompileModule(const SPIRV::SPIRVModule* spirvModule, spv::ExecutionModel executionModel);
	
	void* getPointer(SpirvCompiledModule* module, const std::string& name);
	FunctionPointer getFunctionPointer(SpirvCompiledModule* module, const std::string& name);

private:
	class Impl;
	Impl* impl;
};

void STL_DLL_EXPORT AddSpirvFunction(const std::string& name, FunctionPointer pointer);

std::unique_ptr<llvm::Module> ConvertSpirv(llvm::LLVMContext* context, const SPIRV::SPIRVModule* spirvModule, spv::ExecutionModel executionModel);