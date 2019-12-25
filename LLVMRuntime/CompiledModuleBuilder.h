#pragma once
#include <Base.h>

#include "Jit.h"

#include <llvm-c/Core.h>
#include <llvm-c/Types.h>

namespace Intrinsics
{
	enum ID
	{
		not_intrinsic = 0,

		// Get the intrinsic enums generated from Intrinsics.td
#define GET_INTRINSIC_ENUM_VALUES
#include "llvm/IR/IntrinsicEnums.inc"
#undef GET_INTRINSIC_ENUM_VALUES
	};
}

class CPJit;

class CompiledModuleBuilder
{
public:
	CPJit* jit{};
	LLVMContextRef context{};
	LLVMModuleRef module{};
	LLVMBuilderRef builder{};

	void Initialise(CPJit* jit, LLVMContextRef context, LLVMModuleRef module);

	LLVMValueRef CompileMainFunction();
	virtual LLVMValueRef CompileMainFunctionImpl() = 0;

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

	LLVMTypeRef ScalarType(LLVMTypeRef type);
	
	LLVMTypeRef StructType(std::vector<LLVMTypeRef>& members, const std::string& name, bool isPacked = false);

	// Builder helpers

	template<int NumberValues>
	LLVMValueRef CreateIntrinsic(Intrinsics::ID intrinsic, std::array<LLVMValueRef, NumberValues> values)
	{
		std::array<LLVMTypeRef, NumberValues> parameterTypes;
		for (auto i = 0u; i < NumberValues; i++)
		{
			parameterTypes[i] = LLVMTypeOf(values[i]);
		}

		const auto declaration = LLVMGetIntrinsicDeclaration(module, intrinsic, parameterTypes.data(), parameterTypes.size());
		return LLVMBuildCall(builder, declaration, values.data(), static_cast<uint32_t>(values.size()), "");
	}

	LLVMValueRef GlobalVariable(LLVMTypeRef type, LLVMLinkage linkage, const std::string& name);

	LLVMValueRef GlobalVariable(LLVMTypeRef type, bool isConstant, LLVMLinkage linkage, LLVMValueRef initialiser, const std::string& name);

	LLVMValueRef CreateRetVoid();

	LLVMValueRef CreateRet(LLVMValueRef value);

	LLVMValueRef CreateAggregateRet(LLVMValueRef* returnValues, uint32_t numberValues);

	LLVMValueRef CreateBr(LLVMBasicBlockRef destination);

	LLVMValueRef CreateCondBr(LLVMValueRef conditional, LLVMBasicBlockRef thenBlock, LLVMBasicBlockRef elseBlock);

	LLVMValueRef CreateSwitch(LLVMValueRef value, LLVMBasicBlockRef elseBlock, uint32_t numberCases);
	
	LLVMValueRef CreateIndirectBr(LLVMValueRef address, uint32_t numberDestinations);

	LLVMValueRef CreateInvoke(LLVMValueRef function, LLVMValueRef* arguments, uint32_t numberArguments, LLVMBasicBlockRef thenBlock, LLVMBasicBlockRef catchBlock, const std::string& name = "");

	LLVMValueRef CreateUnreachable();

	LLVMValueRef CreateResume(LLVMValueRef exception);
	
	LLVMValueRef CreateLandingPad(LLVMTypeRef type, LLVMValueRef PersFn, uint32_t numClauses, const std::string& name = "");
	
	LLVMValueRef CreateCleanupRet(LLVMValueRef catchPad, LLVMBasicBlockRef block);

	LLVMValueRef CreateCatchRet(LLVMValueRef catchPad, LLVMBasicBlockRef block);

	LLVMValueRef CreateCatchPad(LLVMValueRef parentPad, LLVMValueRef* args, uint32_t numArgs, const std::string& name = "");

	LLVMValueRef CreateCleanupPad(LLVMValueRef parentPad, LLVMValueRef* args, uint32_t numArgs, const std::string& name = "");

	LLVMValueRef CreateCatchSwitch(LLVMValueRef parentPad, LLVMBasicBlockRef unwindBlock, uint32_t numHandlers, const std::string& name = "");

	LLVMValueRef CreateAdd(LLVMValueRef lhs, LLVMValueRef rhs, const std::string& name = "");
	
	LLVMValueRef CreateNSWAdd(LLVMValueRef lhs, LLVMValueRef rhs, const std::string& name = "");
	
	LLVMValueRef CreateNUWAdd(LLVMValueRef lhs, LLVMValueRef rhs, const std::string& name = "");
	
	LLVMValueRef CreateFAdd(LLVMValueRef lhs, LLVMValueRef rhs, const std::string& name = "");
	
	LLVMValueRef CreateSub(LLVMValueRef lhs, LLVMValueRef rhs, const std::string& name = "");
	
	LLVMValueRef CreateNSWSub(LLVMValueRef lhs, LLVMValueRef rhs, const std::string& name = "");
	
	LLVMValueRef CreateNUWSub(LLVMValueRef lhs, LLVMValueRef rhs, const std::string& name = "");
	
	LLVMValueRef CreateFSub(LLVMValueRef lhs, LLVMValueRef rhs, const std::string& name = "");
	
	LLVMValueRef CreateMul(LLVMValueRef lhs, LLVMValueRef rhs, const std::string& name = "");
	
	LLVMValueRef CreateNSWMul(LLVMValueRef lhs, LLVMValueRef rhs, const std::string& name = "");
	
	LLVMValueRef CreateNUWMul(LLVMValueRef lhs, LLVMValueRef rhs, const std::string& name = "");
	
	LLVMValueRef CreateFMul(LLVMValueRef lhs, LLVMValueRef rhs, const std::string& name = "");
	
	LLVMValueRef CreateUDiv(LLVMValueRef lhs, LLVMValueRef rhs, const std::string& name = "");
	
	LLVMValueRef CreateExactUDiv(LLVMValueRef lhs, LLVMValueRef rhs, const std::string& name = "");
	
	LLVMValueRef CreateSDiv(LLVMValueRef lhs, LLVMValueRef rhs, const std::string& name = "");
	
	LLVMValueRef CreateExactSDiv(LLVMValueRef lhs, LLVMValueRef rhs, const std::string& name = "");
	
	LLVMValueRef CreateFDiv(LLVMValueRef lhs, LLVMValueRef rhs, const std::string& name = "");
	
	LLVMValueRef CreateURem(LLVMValueRef lhs, LLVMValueRef rhs, const std::string& name = "");
	
	LLVMValueRef CreateSRem(LLVMValueRef lhs, LLVMValueRef rhs, const std::string& name = "");
	
	LLVMValueRef CreateFRem(LLVMValueRef lhs, LLVMValueRef rhs, const std::string& name = "");
	
	LLVMValueRef CreateShl(LLVMValueRef lhs, LLVMValueRef rhs, const std::string& name = "");
	
	LLVMValueRef CreateLShr(LLVMValueRef lhs, LLVMValueRef rhs, const std::string& name = "");
	
	LLVMValueRef CreateAShr(LLVMValueRef lhs, LLVMValueRef rhs, const std::string& name = "");
	
	LLVMValueRef CreateAnd(LLVMValueRef lhs, LLVMValueRef rhs, const std::string& name = "");
	
	LLVMValueRef CreateOr(LLVMValueRef lhs, LLVMValueRef rhs, const std::string& name = "");
	
	LLVMValueRef CreateXor(LLVMValueRef lhs, LLVMValueRef rhs, const std::string& name = "");
	
	LLVMValueRef CreateBinOp(LLVMOpcode opcode, LLVMValueRef lhs, LLVMValueRef rhs, const std::string& name = "");
	
	LLVMValueRef CreateNeg(LLVMValueRef value, const std::string& name = "");
	
	LLVMValueRef CreateNSWNeg(LLVMValueRef value, const std::string& name = "");
	
	LLVMValueRef CreateNUWNeg(LLVMValueRef value, const std::string& name = "");
	
	LLVMValueRef CreateFNeg(LLVMValueRef value, const std::string& name = "");
	
	LLVMValueRef CreateNot(LLVMValueRef value, const std::string& name = "");

	LLVMValueRef CreateMalloc(LLVMTypeRef type, const std::string& name = "");
	
	LLVMValueRef CreateArrayMalloc(LLVMTypeRef type, LLVMValueRef value, const std::string& name = "");

	LLVMValueRef CreateMemSet(LLVMValueRef pointer, LLVMValueRef value, LLVMValueRef length, uint32_t alignment);

	LLVMValueRef CreateMemCpy(LLVMValueRef destination, uint32_t destinationAlignment, LLVMValueRef source, uint32_t sourceAlignment, LLVMValueRef size, bool isVolatile = false);

	LLVMValueRef CreateMemMove(LLVMValueRef destination, uint32_t destinationAlignment, LLVMValueRef source, uint32_t sourceAlignment, LLVMValueRef size, bool isVolatile = false);

	LLVMValueRef CreateAlloca(LLVMTypeRef type, const std::string& name = "");
	
	LLVMValueRef CreateAlloca(LLVMTypeRef type, LLVMValueRef value, const std::string& name = "");
	
	LLVMValueRef CreateFree(LLVMValueRef pointer);

	LLVMValueRef CreateLoad(LLVMValueRef pointer, bool isVolatile = false, const std::string& name = "");
	
	LLVMValueRef CreateLoad(LLVMTypeRef type, LLVMValueRef pointer, bool isVolatile = false, const std::string& name = "");
	
	LLVMValueRef CreateStore(LLVMValueRef value, LLVMValueRef pointer, bool isVolatile = false);

	LLVMValueRef CreateGEP(LLVMValueRef pointer, uint32_t index0);

	LLVMValueRef CreateGEP(LLVMValueRef pointer, uint32_t index0, uint32_t index1);

	LLVMValueRef CreateGEP(LLVMValueRef pointer, const std::vector<uint32_t>& indices);
	
	LLVMValueRef CreateGEP(LLVMValueRef pointer, std::vector<LLVMValueRef> indices);

	LLVMValueRef CreateInBoundsGEP(LLVMValueRef pointer, uint32_t index0);

	LLVMValueRef CreateInBoundsGEP(LLVMValueRef pointer, uint32_t index0, uint32_t index1);

	LLVMValueRef CreateInBoundsGEP(LLVMValueRef pointer, std::vector<LLVMValueRef> indices);

	LLVMValueRef CreateGlobalString(const std::string& str, const std::string& name = "");
	
	LLVMValueRef CreateGlobalStringPtr(const std::string& str, const std::string& name = "");

	LLVMValueRef CreateTrunc(LLVMValueRef value, LLVMTypeRef destinationType, const std::string& name = "");

	LLVMValueRef CreateZExt(LLVMValueRef value, LLVMTypeRef destinationType, const std::string& name = "");

	LLVMValueRef CreateSExt(LLVMValueRef value, LLVMTypeRef destinationType, const std::string& name = "");

	LLVMValueRef CreateFPToUI(LLVMValueRef value, LLVMTypeRef destinationType, const std::string& name = "");

	LLVMValueRef CreateFPToSI(LLVMValueRef value, LLVMTypeRef destinationType, const std::string& name = "");

	LLVMValueRef CreateUIToFP(LLVMValueRef value, LLVMTypeRef destinationType, const std::string& name = "");

	LLVMValueRef CreateSIToFP(LLVMValueRef value, LLVMTypeRef destinationType, const std::string& name = "");

	LLVMValueRef CreateFPTrunc(LLVMValueRef value, LLVMTypeRef destinationType, const std::string& name = "");

	LLVMValueRef CreateFPExt(LLVMValueRef value, LLVMTypeRef destinationType, const std::string& name = "");

	LLVMValueRef CreateFPExtOrTrunc(LLVMValueRef value, LLVMTypeRef destinationType, const std::string& name = "");

	LLVMValueRef CreatePtrToInt(LLVMValueRef value, LLVMTypeRef destinationType, const std::string& name = "");

	LLVMValueRef CreateIntToPtr(LLVMValueRef value, LLVMTypeRef destinationType, const std::string& name = "");

	LLVMValueRef CreateBitCast(LLVMValueRef value, LLVMTypeRef destinationType, const std::string& name = "");

	LLVMValueRef CreateAddrSpaceCast(LLVMValueRef value, LLVMTypeRef destinationType, const std::string& name = "");

	LLVMValueRef CreateZExtOrTrunc(LLVMValueRef value, LLVMTypeRef destinationType, const std::string& name = "");
	
	LLVMValueRef CreateZExtOrBitCast(LLVMValueRef value, LLVMTypeRef destinationType, const std::string& name = "");

	LLVMValueRef CreateSExtOrTrunc(LLVMValueRef value, LLVMTypeRef destinationType, const std::string& name = "");

	LLVMValueRef CreateSExtOrBitCast(LLVMValueRef value, LLVMTypeRef destinationType, const std::string& name = "");

	LLVMValueRef CreateTruncOrBitCast(LLVMValueRef value, LLVMTypeRef destinationType, const std::string& name = "");

	LLVMValueRef CreateCast(LLVMOpcode opcode, LLVMValueRef value, LLVMTypeRef destinationType, const std::string& name = "");

	LLVMValueRef CreatePointerCast(LLVMValueRef value, LLVMTypeRef destinationType, const std::string& name = "");

	LLVMValueRef CreateIntCast(LLVMValueRef value, LLVMTypeRef destinationType, bool isSigned, const std::string& name = "");

	LLVMValueRef CreateFPCast(LLVMValueRef value, LLVMTypeRef destinationType, const std::string& name = "");

	LLVMValueRef CreateICmp(LLVMIntPredicate opcode, LLVMValueRef lhs, LLVMValueRef rhs, const std::string& name = "");

	LLVMValueRef CreateICmpEQ(LLVMValueRef lhs, LLVMValueRef rhs, const std::string& name = "")
	{
		return CreateICmp(LLVMIntEQ, lhs, rhs, name);
	}

	LLVMValueRef CreateICmpNE(LLVMValueRef lhs, LLVMValueRef rhs, const std::string& name = "")
	{
		return CreateICmp(LLVMIntNE, lhs, rhs, name);
	}

	LLVMValueRef CreateICmpUGT(LLVMValueRef lhs, LLVMValueRef rhs, const std::string& name = "")
	{
		return CreateICmp(LLVMIntUGT, lhs, rhs, name);
	}

	LLVMValueRef CreateICmpUGE(LLVMValueRef lhs, LLVMValueRef rhs, const std::string& name = "")
	{
		return CreateICmp(LLVMIntUGE, lhs, rhs, name);
	}

	LLVMValueRef CreateICmpULT(LLVMValueRef lhs, LLVMValueRef rhs, const std::string& name = "")
	{
		return CreateICmp(LLVMIntULT, lhs, rhs, name);
	}

	LLVMValueRef CreateICmpULE(LLVMValueRef lhs, LLVMValueRef rhs, const std::string& name = "")
	{
		return CreateICmp(LLVMIntULE, lhs, rhs, name);
	}

	LLVMValueRef CreateICmpSGT(LLVMValueRef lhs, LLVMValueRef rhs, const std::string& name = "")
	{
		return CreateICmp(LLVMIntSGT, lhs, rhs, name);
	}

	LLVMValueRef CreateICmpSGE(LLVMValueRef lhs, LLVMValueRef rhs, const std::string& name = "")
	{
		return CreateICmp(LLVMIntSGE, lhs, rhs, name);
	}

	LLVMValueRef CreateICmpSLT(LLVMValueRef lhs, LLVMValueRef rhs, const std::string& name = "")
	{
		return CreateICmp(LLVMIntSLT, lhs, rhs, name);
	}

	LLVMValueRef CreateICmpSLE(LLVMValueRef lhs, LLVMValueRef rhs, const std::string& name = "")
	{
		return CreateICmp(LLVMIntSLE, lhs, rhs, name);
	}
	
	LLVMValueRef CreateFCmp(LLVMRealPredicate opcode, LLVMValueRef lhs, LLVMValueRef rhs, const std::string& name = "");

	LLVMValueRef CreateFCmpOEQ(LLVMValueRef lhs, LLVMValueRef rhs, const std::string& name = "")
	{
		return CreateFCmp(LLVMRealOEQ, lhs, rhs, name);
	}
	
	LLVMValueRef CreateFCmpOGT(LLVMValueRef lhs, LLVMValueRef rhs, const std::string& name = "")
	{
		return CreateFCmp(LLVMRealOGT, lhs, rhs, name);
	}

	LLVMValueRef CreateFCmpOGE(LLVMValueRef lhs, LLVMValueRef rhs, const std::string& name = "")
	{
		return CreateFCmp(LLVMRealOGE, lhs, rhs, name);
	}

	LLVMValueRef CreateFCmpOLT(LLVMValueRef lhs, LLVMValueRef rhs, const std::string& name = "")
	{
		return CreateFCmp(LLVMRealOLT, lhs, rhs, name);
	}

	LLVMValueRef CreateFCmpOLE(LLVMValueRef lhs, LLVMValueRef rhs, const std::string& name = "")
	{
		return CreateFCmp(LLVMRealOLE, lhs, rhs, name);
	}

	LLVMValueRef CreateFCmpONE(LLVMValueRef lhs, LLVMValueRef rhs, const std::string& name = "")
	{
		return CreateFCmp(LLVMRealONE, lhs, rhs, name);
	}

	LLVMValueRef CreateFCmpORD(LLVMValueRef lhs, LLVMValueRef rhs, const std::string& name = "")
	{
		return CreateFCmp(LLVMRealORD, lhs, rhs, name);
	}

	LLVMValueRef CreateFCmpUNO(LLVMValueRef lhs, LLVMValueRef rhs, const std::string& name = "")
	{
		return CreateFCmp(LLVMRealUNO, lhs, rhs, name);
	}

	LLVMValueRef CreateFCmpUEQ(LLVMValueRef lhs, LLVMValueRef rhs, const std::string& name = "")
	{
		return CreateFCmp(LLVMRealUEQ, lhs, rhs, name);
	}

	LLVMValueRef CreateFCmpUGT(LLVMValueRef lhs, LLVMValueRef rhs, const std::string& name = "")
	{
		return CreateFCmp(LLVMRealUGT, lhs, rhs, name);
	}

	LLVMValueRef CreateFCmpUGE(LLVMValueRef lhs, LLVMValueRef rhs, const std::string& name = "")
	{
		return CreateFCmp(LLVMRealUGE, lhs, rhs, name);
	}

	LLVMValueRef CreateFCmpULT(LLVMValueRef lhs, LLVMValueRef rhs, const std::string& name = "")
	{
		return CreateFCmp(LLVMRealULT, lhs, rhs, name);
	}

	LLVMValueRef CreateFCmpULE(LLVMValueRef lhs, LLVMValueRef rhs, const std::string& name = "")
	{
		return CreateFCmp(LLVMRealULE, lhs, rhs, name);
	}

	LLVMValueRef CreateFCmpUNE(LLVMValueRef lhs, LLVMValueRef rhs, const std::string& name = "")
	{
		return CreateFCmp(LLVMRealUNE, lhs, rhs, name);
	}
	
	LLVMValueRef CreatePhi(LLVMTypeRef type, const std::string& name = "");
	
	LLVMValueRef CreateCall(LLVMValueRef function, LLVMValueRef* arguments, uint32_t numberArguments, const std::string& name = "");
	
	LLVMValueRef CreateCall(LLVMValueRef function, std::vector<LLVMValueRef> arguments, const std::string& name = "");
	
	LLVMValueRef CreateSelect(LLVMValueRef conditional, LLVMValueRef thenValue, LLVMValueRef elseValue, const std::string& name = "");
	
	LLVMValueRef CreateVAArg(LLVMValueRef list, LLVMTypeRef type, const std::string& name = "");
	
	LLVMValueRef CreateExtractElement(LLVMValueRef vector, LLVMValueRef Index, const std::string& name = "");
	
	LLVMValueRef CreateInsertElement(LLVMValueRef vector, LLVMValueRef element, LLVMValueRef index, const std::string& name = "");
	
	LLVMValueRef CreateShuffleVector(LLVMValueRef value1, LLVMValueRef value2, LLVMValueRef mask, const std::string& name = "");
	
	LLVMValueRef CreateExtractValue(LLVMValueRef aggregate, uint32_t index, const std::string& name = "");

	LLVMValueRef CreateInsertValue(LLVMValueRef aggregate, LLVMValueRef element, uint32_t index, const std::string& name = "");

	LLVMValueRef CreateIsNull(LLVMValueRef value, const std::string& name = "");
	
	LLVMValueRef CreateIsNotNull(LLVMValueRef value, const std::string& name = "");
	
	LLVMValueRef CreatePtrDiff(LLVMValueRef lhs, LLVMValueRef rhs, const std::string& name = "");
	
	LLVMValueRef CreateFence(LLVMAtomicOrdering ordering, bool singleThread, const std::string& name = "");
	
	LLVMValueRef CreateAtomicRMW(LLVMAtomicRMWBinOp opcode, LLVMValueRef pointer, LLVMValueRef value, LLVMAtomicOrdering ordering, bool singleThread = false);

	LLVMValueRef CreateAtomicCmpXchg(LLVMValueRef pointer, LLVMValueRef cmp, LLVMValueRef New, LLVMAtomicOrdering successOrdering, LLVMAtomicOrdering failureOrdering, bool singleThread = false);

	LLVMValueRef CreateVectorSplat(uint32_t vectorSize, LLVMValueRef value, const std::string& name = "");
};
