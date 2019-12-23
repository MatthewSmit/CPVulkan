#pragma once
#include <Base.h>

#include "Jit.h"
#include "../CPVulkan/PipelineCache.h"

using LLVMBuilderRef = struct LLVMOpaqueBuilder*;
using LLVMContextRef = struct LLVMOpaqueContext*;
using LLVMModuleRef = struct LLVMOpaqueModule*;
using LLVMOrcModuleHandle = uint64_t;

class CPJit;

class CompiledModule final
{
public:
	CompiledModule(CPJit* jit, LLVMContextRef context, LLVMModuleRef module, std::function<void*(const std::string&)> getFunction);
	CP_DLL_EXPORT ~CompiledModule();

	CP_DLL_EXPORT std::vector<uint8_t> ExportBitcode();

	uint64_t ResolveSymbol(const char* name);

	CP_DLL_EXPORT static CompiledModule* CreateFromBitcode(CPJit* jit, const std::function<void*(const std::basic_string<char>&)>& getFunction, const std::vector<uint8_t>& data);

	CP_DLL_EXPORT void* getPointer(const std::string& name) const;
	CP_DLL_EXPORT void* getOptionalPointer(const std::string& name) const;
	CP_DLL_EXPORT FunctionPointer getFunctionPointer(const std::string& name) const;

private:
	CPJit* jit;
	LLVMContextRef context;
	LLVMModuleRef module;
	std::function<void*(const std::string&)> getFunction;
	LLVMOrcModuleHandle orcModule;
};