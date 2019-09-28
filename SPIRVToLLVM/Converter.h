#pragma once
#include "spirv.hpp"

#if defined(_MSC_VER)
#define DLL_EXPORT __declspec(dllexport)
#else
#define DLL_EXPORT
#endif

#include <memory>
#include <string>

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

class DLL_EXPORT SpirvJit
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

void DLL_EXPORT AddSpirvFunction(const std::string& name, FunctionPointer pointer);

std::unique_ptr<llvm::Module> ConvertSpirv(llvm::LLVMContext* context, const SPIRV::SPIRVModule* spirvModule, spv::ExecutionModel executionModel);