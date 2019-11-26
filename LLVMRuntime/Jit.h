#pragma once
#include <Base.h>

#include <spirv.hpp>

namespace llvm
{
	class LLVMContext;
	class Module;
}

namespace SPIRV
{
	class SPIRVModule;
}

class CompiledModule;

using FunctionPointer = void (*)();

class CP_DLL_EXPORT CPJit
{
public:
	CPJit();
	~CPJit();

	CompiledModule* CompileModule(const SPIRV::SPIRVModule* spirvModule, spv::ExecutionModel executionModel, const VkSpecializationInfo* specializationInfo);
	CompiledModule* CompileModule(std::unique_ptr<llvm::LLVMContext> context, std::unique_ptr<llvm::Module> module);
	void FreeModule(CompiledModule* compiledModule);

	void AddFunction(const std::string& name, FunctionPointer pointer);
	void SetUserData(void* userData);

	void* getPointer(const CompiledModule* module, const std::string& name);
	FunctionPointer getFunctionPointer(const CompiledModule* module, const std::string& name);

private:
	class Impl;
	Impl* impl;
};