#pragma once
#include <Base.h>

#include "Jit.h"

using LLVMBuilderRef = struct LLVMOpaqueBuilder*;
using LLVMContextRef = struct LLVMOpaqueContext*;
using LLVMModuleRef = struct LLVMOpaqueModule*;
using LLVMOrcModuleHandle = uint64_t;

class CPJit;

class CompiledModule final
{
public:
	explicit CompiledModule(CPJit* jit, LLVMContextRef context, LLVMModuleRef module, std::function<void*(const std::string&)> getFunction);
	CP_DLL_EXPORT ~CompiledModule();

	uint64_t ResolveSymbol(const char* name);

	CP_DLL_EXPORT void* getPointer(const std::string& name) const;
	CP_DLL_EXPORT void* getOptionalPointer(const std::string& name) const;
	CP_DLL_EXPORT FunctionPointer getFunctionPointer(const std::string& name) const;
	 
private:
	CPJit* jit;
	LLVMContextRef context;
	std::function<void*(const std::string&)> getFunction;
	LLVMOrcModuleHandle orcModule;
};