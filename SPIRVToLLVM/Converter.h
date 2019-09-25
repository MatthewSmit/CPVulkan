#pragma once
#include "spirv.hpp"

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

class __declspec(dllexport) SpirvJit
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

void __declspec(dllexport) AddSpirvFunction(const std::string& name, void* pointer);

std::unique_ptr<llvm::Module> ConvertSpirv(llvm::LLVMContext* context, const SPIRV::SPIRVModule* spirvModule, spv::ExecutionModel executionModel);