#pragma once
#include <Base.h>

constexpr auto ALIGNMENT = 8;

class CompiledModule;

using FunctionPointer = void (*)();
using LLVMOrcJITStackRef = struct LLVMOrcOpaqueJITStack*;
using LLVMTargetDataRef = struct LLVMOpaqueTargetData*;

class CP_DLL_EXPORT CPJit
{
public:
	CPJit();
	~CPJit();

	void AddFunction(const std::string& name, FunctionPointer pointer);

	[[nodiscard]] FunctionPointer getFunction(const std::string& name);
	[[nodiscard]] void* getUserData() const;
	[[nodiscard]] LLVMTargetDataRef getDataLayout() const;
	[[nodiscard]] LLVMOrcJITStackRef getOrc() const;

	void setUserData(void* userData);

private:
	class Impl;
	Impl* impl;
};