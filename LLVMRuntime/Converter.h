#pragma once
#include "spirv.hpp"

#include "Helper.h"

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
	class SPIRVVariable;
}

class SpirvCompiledModule;

using FunctionPointer = void (*)();

class STL_DLL_EXPORT SpirvJit
{
public:
	SpirvJit();
	~SpirvJit();

	SpirvCompiledModule* CompileModule(const SPIRV::SPIRVModule* spirvModule, spv::ExecutionModel executionModel);
	SpirvCompiledModule* CompileModule(std::unique_ptr<llvm::LLVMContext> context, std::unique_ptr<llvm::Module> module);

	void AddFunction(const std::string& name, FunctionPointer pointer);
	void SetUserData(void* userData);
	
	void* getPointer(const SpirvCompiledModule* module, const std::string& name);
	FunctionPointer getFunctionPointer(const SpirvCompiledModule* module, const std::string& name);

private:
	class Impl;
	Impl* impl;
};

std::unique_ptr<llvm::Module> ConvertSpirv(llvm::LLVMContext* context, const SPIRV::SPIRVModule* spirvModule, spv::ExecutionModel executionModel);

std::string STL_DLL_EXPORT MangleName(const SPIRV::SPIRVVariable* variable);