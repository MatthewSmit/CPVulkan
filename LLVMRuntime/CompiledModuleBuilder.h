#pragma once
#include <Base.h>

#include "Jit.h"

#include <llvm-c/Core.h>
#include <llvm-c/Types.h>

class CPJit;

class CompiledModuleBuilder
{
public:
	explicit CompiledModuleBuilder(CPJit* jit, std::function<void*(const std::string&)> getFunction = nullptr);
	virtual ~CompiledModuleBuilder();

	CompiledModule* Compile();

protected:
	LLVMTargetDataRef layout{};

	CPJit* jit;
	std::function<void*(const std::string&)> getFunction;
	LLVMContextRef context{};
	LLVMModuleRef module{};
	LLVMBuilderRef builder{};

	virtual void MainCompilation() = 0;

	// Generic Helpers

	bool HasName(LLVMValueRef value);

	std::string GetName(LLVMValueRef value);

	void SetName(LLVMValueRef value, const std::string& name);

	// Constant Helpers
	LLVMValueRef ConstF32(float value);
	LLVMValueRef ConstF64(double value);
	LLVMValueRef ConstI8(int8_t value);
	LLVMValueRef ConstI16(int16_t value);
	LLVMValueRef ConstI32(int32_t value);
	LLVMValueRef ConstI64(int64_t value);
	LLVMValueRef ConstU8(uint8_t value);
	LLVMValueRef ConstU16(uint16_t value);
	LLVMValueRef ConstU32(uint32_t value);
	LLVMValueRef ConstU64(uint64_t value);

	// Type Helpers
	template<typename T>
	LLVMTypeRef GetType();
	
	LLVMTypeRef StructType(std::vector<LLVMTypeRef>& members, const std::string& name, bool isPacked = false);

	// Builder helpers

	LLVMValueRef GlobalVariable(LLVMTypeRef type, LLVMLinkage linkage, const std::string& name);

	LLVMValueRef GlobalVariable(LLVMTypeRef type, bool isConstant, LLVMLinkage linkage, LLVMValueRef initialiser, const std::string& name);
	
	LLVMValueRef CreateLoad(LLVMValueRef pointer, bool isVolatile = false, const std::string& name = "");

	LLVMValueRef CreateStore(LLVMValueRef src, LLVMValueRef dst, bool isVolatile = false);

	LLVMValueRef CreateGEP(LLVMValueRef pointer, uint32_t index0);

	LLVMValueRef CreateGEP(LLVMValueRef pointer, uint32_t index0, uint32_t index1);
	
	LLVMValueRef CreateGEP(LLVMValueRef pointer, std::vector<LLVMValueRef> indices);
};
