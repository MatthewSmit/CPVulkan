#pragma once
#include "spirv.hpp"

#include "Helper.h"

#include <memory>
#include <string>

using FunctionPointer = void (*)();

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

class DLL_EXPORT SpirvJit
{
public:
	using FunctionPointer = void (*)();
	
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