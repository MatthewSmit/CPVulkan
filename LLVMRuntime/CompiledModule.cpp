#include "CompiledModule.h"
#include "CompiledModuleBuilder.h"

#include "Jit.h"
#include "SpirvFunctions.h"

#include <Half.h>

#include <llvm-c/Core.h>
#include <llvm-c/OrcBindings.h>
#include <llvm-c/Support.h>

#include <iostream>
#include <utility>

CP_DLL_EXPORT std::string DumpModule(LLVMModuleRef module)
{
	const auto pointer = LLVMPrintModuleToString(module);
	const std::string str = pointer;
	LLVMDisposeMessage(pointer);
	return str;
}

CP_DLL_EXPORT std::string DumpType(LLVMTypeRef type)
{
	const auto pointer = LLVMPrintTypeToString(type);
	const std::string str = pointer;
	LLVMDisposeMessage(pointer);
	return str;
}

CP_DLL_EXPORT std::string DumpValue(LLVMValueRef value)
{
	const auto pointer = LLVMPrintValueToString(value);
	const auto str = pointer + std::string{"; "} + DumpType(LLVMTypeOf(value));
	LLVMDisposeMessage(pointer);
	return str;
}

static uint64_t SymbolResolverStub(const char* name, void* lookupContext)
{
	return static_cast<CompiledModule*>(lookupContext)->ResolveSymbol(name);
}

CompiledModule::CompiledModule(CPJit* jit, LLVMContextRef context, LLVMModuleRef module, std::function<void*(const std::string&)> getFunction) :
	jit{jit},
	context{context},
	getFunction{std::move(getFunction)}
{
	jit->RunOnCompileThread([&]()
	{
		const auto error = LLVMOrcAddEagerlyCompiledIR(jit->getOrc(), &orcModule, module, SymbolResolverStub, this);
		if (error)
		{
			const auto errorMessage = LLVMGetErrorMessage(error);
			TODO_ERROR();
		}
	});
}

CompiledModule::~CompiledModule()
{
	jit->RunOnCompileThread([&]()
	{
		const auto error = LLVMOrcRemoveModule(jit->getOrc(), orcModule);
		if (error)
		{
			const auto errorMessage = LLVMGetErrorMessage(error);
			TODO_ERROR();
		}
	});
	LLVMContextDispose(context);
}

uint64_t CompiledModule::ResolveSymbol(const char* name)
{
	// 	const auto hasGlobalPrefix = (dataLayout.getGlobalPrefix() != '\0');
	// 	
	// 	llvm::orc::SymbolNameSet addedSymbols;
	// 	llvm::orc::SymbolMap newSymbols;
	// 	for (const auto& name : names)
	// 	{
	// 		if ((*name).empty())
	// 		{
	// 			continue;
	// 		}
	//
	// 		std::string tmp((*name).data(), (*name).size());
	//
	FunctionPointer function = nullptr;

	if (getFunction)
	{
		function = reinterpret_cast<FunctionPointer>(getFunction(name));
	}

	if (!function)
	{
		function = jit->getFunction(name);
	}

	if (!function)
	{
		const auto functionPtr = getSpirvFunctions().find(name);
		if (functionPtr != getSpirvFunctions().end())
		{
			function = functionPtr->second;
		}
	}

	if (!function)
	{
		//if (hasGlobalPrefix)
		//{
		//	if ((*name).front() != dataLayout.getGlobalPrefix())
		//	{
		//		continue;
		//	}
		//	tmp = std::string((*name).data() + (hasGlobalPrefix ? 1 : 0), (*name).size());
		//}

		function = reinterpret_cast<FunctionPointer>(LLVMSearchForAddressOfSymbol(name));
	}

	return reinterpret_cast<uint64_t>(function);
}

void* CompiledModule::getPointer(const std::string& name) const
{
	const auto symbol = getOptionalPointer(name);
	if (!symbol)
	{
		FATAL_ERROR();
	}
	return symbol;
}

void* CompiledModule::getOptionalPointer(const std::string& name) const
{
	LLVMOrcTargetAddress symbolAddress;
	
	jit->RunOnCompileThread([&]()
	{
		const auto error = LLVMOrcGetSymbolAddressIn(jit->getOrc(), &symbolAddress, orcModule, name.c_str());
		if (error)
		{
			const auto errorMessage = LLVMGetErrorMessage(error);
			TODO_ERROR();
		}
	});

	return reinterpret_cast<void*>(symbolAddress);
}

FunctionPointer CompiledModule::getFunctionPointer(const std::string& name) const
{
	return reinterpret_cast<FunctionPointer>(getPointer(name));
}

CompiledModuleBuilder::CompiledModuleBuilder(CPJit* jit, std::function<void*(const std::string&)> getFunction) :
	jit{jit},
	getFunction{std::move(getFunction)}
{
	this->layout = jit->getDataLayout();
}

CompiledModuleBuilder::~CompiledModuleBuilder() = default;

CompiledModule* CompiledModuleBuilder::Compile()
{
	CompiledModule* compiledModule;
	this->jit->RunOnCompileThread([&]()
	{
		context = LLVMContextCreate();
		module = LLVMModuleCreateWithNameInContext("", context);
		builder = LLVMCreateBuilderInContext(context);

		MainCompilation();

		LLVMDisposeBuilder(builder);

#ifdef NDEBUG
		LLVMRunPassManager(this->jit->getPassManager(), module);
#endif

		// TODO: Soft fail?
		// LLVMVerifyModule(module, LLVMAbortProcessAction, nullpointer);

		std::cout << DumpModule(module) << std::endl;

		compiledModule = new CompiledModule(jit, context, module, getFunction);

		const auto userData = compiledModule->getOptionalPointer("@userData");
		if (userData)
		{
			*reinterpret_cast<void**>(userData) = jit->getUserData();
		}
	});

	return compiledModule;
}

bool CompiledModuleBuilder::HasName(LLVMValueRef value)
{
	size_t length;
	LLVMGetValueName2(value, &length);
	return length > 0;
}

std::string CompiledModuleBuilder::GetName(LLVMValueRef value)
{
	size_t length;
	const auto pointer = LLVMGetValueName2(value, &length);
	return std::string{pointer, length};
}

void CompiledModuleBuilder::SetName(LLVMValueRef value, const std::string& name)
{
	LLVMSetValueName2(value, name.c_str(), name.size());
}

LLVMValueRef CompiledModuleBuilder::ConstF32(float value)
{
	return LLVMConstReal(LLVMFloatTypeInContext(context), value);
}

LLVMValueRef CompiledModuleBuilder::ConstF64(double value)
{
	return LLVMConstReal(LLVMDoubleTypeInContext(context), value);
}

LLVMValueRef CompiledModuleBuilder::ConstI8(int8_t value)
{
	return LLVMConstInt(LLVMInt8TypeInContext(context), value, true);
}

LLVMValueRef CompiledModuleBuilder::ConstI16(int16_t value)
{
	return LLVMConstInt(LLVMInt16TypeInContext(context), value, true);
}

LLVMValueRef CompiledModuleBuilder::ConstI32(int32_t value)
{
	return LLVMConstInt(LLVMInt32TypeInContext(context), value, true);
}

LLVMValueRef CompiledModuleBuilder::ConstI64(int64_t value)
{
	return LLVMConstInt(LLVMInt64TypeInContext(context), value, true);
}

LLVMValueRef CompiledModuleBuilder::ConstU8(uint8_t value)
{
	return LLVMConstInt(LLVMInt8TypeInContext(context), value, false);
}

LLVMValueRef CompiledModuleBuilder::ConstU16(uint16_t value)
{
	return LLVMConstInt(LLVMInt16TypeInContext(context), value, false);
}

LLVMValueRef CompiledModuleBuilder::ConstU32(uint32_t value)
{
	return LLVMConstInt(LLVMInt32TypeInContext(context), value, false);
}

LLVMValueRef CompiledModuleBuilder::ConstU64(uint64_t value)
{
	return LLVMConstInt(LLVMInt64TypeInContext(context), value, false);
}

template<>
LLVMTypeRef CompiledModuleBuilder::GetType<half>()
{
	return LLVMHalfTypeInContext(context);
}

template<>
LLVMTypeRef CompiledModuleBuilder::GetType<float>()
{
	return LLVMFloatTypeInContext(context);
}

template<>
LLVMTypeRef CompiledModuleBuilder::GetType<double>()
{
	return LLVMDoubleTypeInContext(context);
}

template<>
LLVMTypeRef CompiledModuleBuilder::GetType<int8_t>()
{
	return LLVMInt8TypeInContext(context);
}

template<>
LLVMTypeRef CompiledModuleBuilder::GetType<int16_t>()
{
	return LLVMInt16TypeInContext(context);
}

template<>
LLVMTypeRef CompiledModuleBuilder::GetType<int32_t>()
{
	return LLVMInt32TypeInContext(context);
}

template<>
LLVMTypeRef CompiledModuleBuilder::GetType<int64_t>()
{
	return LLVMInt64TypeInContext(context);
}

template<>
LLVMTypeRef CompiledModuleBuilder::GetType<uint8_t>()
{
	return LLVMInt8TypeInContext(context);
}

template<>
LLVMTypeRef CompiledModuleBuilder::GetType<uint16_t>()
{
	return LLVMInt16TypeInContext(context);
}

template<>
LLVMTypeRef CompiledModuleBuilder::GetType<uint32_t>()
{
	return LLVMInt32TypeInContext(context);
}

template<>
LLVMTypeRef CompiledModuleBuilder::GetType<uint64_t>()
{
	return LLVMInt64TypeInContext(context);
}

LLVMTypeRef CompiledModuleBuilder::ScalarType(LLVMTypeRef type)
{
	switch (LLVMGetTypeKind(type))
	{
	case LLVMVoidTypeKind:
	case LLVMHalfTypeKind:
	case LLVMFloatTypeKind:
	case LLVMDoubleTypeKind:
	case LLVMX86_FP80TypeKind:
	case LLVMFP128TypeKind:
	case LLVMPPC_FP128TypeKind:
	case LLVMIntegerTypeKind:
	case LLVMX86_MMXTypeKind:
		return type;
		
	case LLVMArrayTypeKind:
	case LLVMPointerTypeKind:
	case LLVMVectorTypeKind:
		return LLVMGetElementType(type);
		
	default:
		FATAL_ERROR();
	}
}


LLVMTypeRef CompiledModuleBuilder::StructType(std::vector<LLVMTypeRef>& members, const std::string& name, bool isPacked)
{
	const auto type = LLVMStructCreateNamed(context, name.c_str());
	LLVMStructSetBody(type, members.data(), static_cast<uint32_t>(members.size()), isPacked);
	return type;
}

LLVMValueRef CompiledModuleBuilder::GlobalVariable(LLVMTypeRef type, LLVMLinkage linkage, const std::string& name)
{
	const auto global = LLVMAddGlobal(module, type, name.c_str());
	LLVMSetLinkage(global, linkage);
	LLVMSetInitializer(global, LLVMConstNull(type));
	LLVMSetAlignment(global, ALIGNMENT);
	return global;
}

LLVMValueRef CompiledModuleBuilder::GlobalVariable(LLVMTypeRef type, bool isConstant, LLVMLinkage linkage, LLVMValueRef initialiser, const std::string& name)
{
	const auto global = LLVMAddGlobal(module, type, name.c_str());
	LLVMSetLinkage(global, linkage);
	LLVMSetInitializer(global, initialiser);
	LLVMSetAlignment(global, ALIGNMENT);
	LLVMSetGlobalConstant(global, isConstant);
	return global;
}

LLVMValueRef CompiledModuleBuilder::CreateRetVoid()
{
	return LLVMBuildRetVoid(builder);
}

LLVMValueRef CompiledModuleBuilder::CreateRet(LLVMValueRef value)
{
	return LLVMBuildRet(builder, value);
}

LLVMValueRef CompiledModuleBuilder::CreateAggregateRet(LLVMValueRef* returnValues, uint32_t numberValues)
{
	return LLVMBuildAggregateRet(builder, returnValues, numberValues);
}

LLVMValueRef CompiledModuleBuilder::CreateBr(LLVMBasicBlockRef destination)
{
	return LLVMBuildBr(builder, destination);
}

LLVMValueRef CompiledModuleBuilder::CreateCondBr(LLVMValueRef conditional, LLVMBasicBlockRef thenBlock, LLVMBasicBlockRef elseBlock)
{
	return LLVMBuildCondBr(builder, conditional, thenBlock, elseBlock);
}

LLVMValueRef CompiledModuleBuilder::CreateSwitch(LLVMValueRef value, LLVMBasicBlockRef elseBlock, uint32_t numberCases)
{
	return LLVMBuildSwitch(builder, value, elseBlock, numberCases);
}

LLVMValueRef CompiledModuleBuilder::CreateIndirectBr(LLVMValueRef address, uint32_t numberDestinations)
{
	return LLVMBuildIndirectBr(builder, address, numberDestinations);
}

LLVMValueRef CompiledModuleBuilder::CreateInvoke(LLVMValueRef function, LLVMValueRef* arguments, uint32_t numberArguments, LLVMBasicBlockRef thenBlock, LLVMBasicBlockRef catchBlock, const std::string& name)
{
	return LLVMBuildInvoke(builder, function, arguments, numberArguments, thenBlock, catchBlock, name.c_str());
}

LLVMValueRef CompiledModuleBuilder::CreateUnreachable()
{
	return LLVMBuildUnreachable(builder);
}

LLVMValueRef CompiledModuleBuilder::CreateResume(LLVMValueRef exception)
{
	return LLVMBuildResume(builder, exception);
}

LLVMValueRef CompiledModuleBuilder::CreateLandingPad(LLVMTypeRef type, LLVMValueRef PersFn, uint32_t numClauses, const std::string& name)
{
	return LLVMBuildLandingPad(builder, type, PersFn, numClauses, name.c_str());
}

LLVMValueRef CompiledModuleBuilder::CreateCleanupRet(LLVMValueRef catchPad, LLVMBasicBlockRef block)
{
	return LLVMBuildCleanupRet(builder, catchPad, block);
}

LLVMValueRef CompiledModuleBuilder::CreateCatchRet(LLVMValueRef catchPad, LLVMBasicBlockRef block)
{
	return LLVMBuildCatchRet(builder, catchPad, block);
}

LLVMValueRef CompiledModuleBuilder::CreateCatchPad(LLVMValueRef parentPad, LLVMValueRef* args, uint32_t numArgs, const std::string& name)
{
	return LLVMBuildCatchPad(builder, parentPad, args, numArgs, name.c_str());
}

LLVMValueRef CompiledModuleBuilder::CreateCleanupPad(LLVMValueRef parentPad, LLVMValueRef* args, uint32_t numArgs, const std::string& name)
{
	return LLVMBuildCleanupPad(builder, parentPad, args, numArgs, name.c_str());
}

LLVMValueRef CompiledModuleBuilder::CreateCatchSwitch(LLVMValueRef parentPad, LLVMBasicBlockRef unwindBlock, uint32_t numHandlers, const std::string& name)
{
	return LLVMBuildCatchSwitch(builder, parentPad, unwindBlock, numHandlers, name.c_str());
}

LLVMValueRef CompiledModuleBuilder::CreateAdd(LLVMValueRef lhs, LLVMValueRef rhs, const std::string& name)
{
	return LLVMBuildAdd(builder, lhs, rhs, name.c_str());
}

LLVMValueRef CompiledModuleBuilder::CreateNSWAdd(LLVMValueRef lhs, LLVMValueRef rhs, const std::string& name)
{
	return LLVMBuildNSWAdd(builder, lhs, rhs, name.c_str());
}

LLVMValueRef CompiledModuleBuilder::CreateNUWAdd(LLVMValueRef lhs, LLVMValueRef rhs, const std::string& name)
{
	return LLVMBuildNUWAdd(builder, lhs, rhs, name.c_str());
}

LLVMValueRef CompiledModuleBuilder::CreateFAdd(LLVMValueRef lhs, LLVMValueRef rhs, const std::string& name)
{
	return LLVMBuildFAdd(builder, lhs, rhs, name.c_str());
}

LLVMValueRef CompiledModuleBuilder::CreateSub(LLVMValueRef lhs, LLVMValueRef rhs, const std::string& name)
{
	return LLVMBuildSub(builder, lhs, rhs, name.c_str());
}

LLVMValueRef CompiledModuleBuilder::CreateNSWSub(LLVMValueRef lhs, LLVMValueRef rhs, const std::string& name)
{
	return LLVMBuildNSWSub(builder, lhs, rhs, name.c_str());
}

LLVMValueRef CompiledModuleBuilder::CreateNUWSub(LLVMValueRef lhs, LLVMValueRef rhs, const std::string& name)
{
	return LLVMBuildNUWSub(builder, lhs, rhs, name.c_str());
}

LLVMValueRef CompiledModuleBuilder::CreateFSub(LLVMValueRef lhs, LLVMValueRef rhs, const std::string& name)
{
	return LLVMBuildFSub(builder, lhs, rhs, name.c_str());
}

LLVMValueRef CompiledModuleBuilder::CreateMul(LLVMValueRef lhs, LLVMValueRef rhs, const std::string& name)
{
	return LLVMBuildMul(builder, lhs, rhs, name.c_str());
}

LLVMValueRef CompiledModuleBuilder::CreateNSWMul(LLVMValueRef lhs, LLVMValueRef rhs, const std::string& name)
{
	return LLVMBuildNSWMul(builder, lhs, rhs, name.c_str());
}

LLVMValueRef CompiledModuleBuilder::CreateNUWMul(LLVMValueRef lhs, LLVMValueRef rhs, const std::string& name)
{
	return LLVMBuildNUWMul(builder, lhs, rhs, name.c_str());
}

LLVMValueRef CompiledModuleBuilder::CreateFMul(LLVMValueRef lhs, LLVMValueRef rhs, const std::string& name)
{
	return LLVMBuildFMul(builder, lhs, rhs, name.c_str());
}

LLVMValueRef CompiledModuleBuilder::CreateUDiv(LLVMValueRef lhs, LLVMValueRef rhs, const std::string& name)
{
	return LLVMBuildUDiv(builder, lhs, rhs, name.c_str());
}

LLVMValueRef CompiledModuleBuilder::CreateExactUDiv(LLVMValueRef lhs, LLVMValueRef rhs, const std::string& name)
{
	return LLVMBuildExactUDiv(builder, lhs, rhs, name.c_str());
}

LLVMValueRef CompiledModuleBuilder::CreateSDiv(LLVMValueRef lhs, LLVMValueRef rhs, const std::string& name)
{
	return LLVMBuildSDiv(builder, lhs, rhs, name.c_str());
}

LLVMValueRef CompiledModuleBuilder::CreateExactSDiv(LLVMValueRef lhs, LLVMValueRef rhs, const std::string& name)
{
	return LLVMBuildExactSDiv(builder, lhs, rhs, name.c_str());
}

LLVMValueRef CompiledModuleBuilder::CreateFDiv(LLVMValueRef lhs, LLVMValueRef rhs, const std::string& name)
{
	return LLVMBuildFDiv(builder, lhs, rhs, name.c_str());
}

LLVMValueRef CompiledModuleBuilder::CreateURem(LLVMValueRef lhs, LLVMValueRef rhs, const std::string& name)
{
	return LLVMBuildURem(builder, lhs, rhs, name.c_str());
}

LLVMValueRef CompiledModuleBuilder::CreateSRem(LLVMValueRef lhs, LLVMValueRef rhs, const std::string& name)
{
	return LLVMBuildSRem(builder, lhs, rhs, name.c_str());
}

LLVMValueRef CompiledModuleBuilder::CreateFRem(LLVMValueRef lhs, LLVMValueRef rhs, const std::string& name)
{
	return LLVMBuildFRem(builder, lhs, rhs, name.c_str());
}

LLVMValueRef CompiledModuleBuilder::CreateShl(LLVMValueRef lhs, LLVMValueRef rhs, const std::string& name)
{
	return LLVMBuildShl(builder, lhs, rhs, name.c_str());
}

LLVMValueRef CompiledModuleBuilder::CreateLShr(LLVMValueRef lhs, LLVMValueRef rhs, const std::string& name)
{
	return LLVMBuildLShr(builder, lhs, rhs, name.c_str());
}

LLVMValueRef CompiledModuleBuilder::CreateAShr(LLVMValueRef lhs, LLVMValueRef rhs, const std::string& name)
{
	return LLVMBuildAShr(builder, lhs, rhs, name.c_str());
}

LLVMValueRef CompiledModuleBuilder::CreateAnd(LLVMValueRef lhs, LLVMValueRef rhs, const std::string& name)
{
	return LLVMBuildAnd(builder, lhs, rhs, name.c_str());
}

LLVMValueRef CompiledModuleBuilder::CreateOr(LLVMValueRef lhs, LLVMValueRef rhs, const std::string& name)
{
	return LLVMBuildOr(builder, lhs, rhs, name.c_str());
}

LLVMValueRef CompiledModuleBuilder::CreateXor(LLVMValueRef lhs, LLVMValueRef rhs, const std::string& name)
{
	return LLVMBuildXor(builder, lhs, rhs, name.c_str());
}

LLVMValueRef CompiledModuleBuilder::CreateBinOp(LLVMOpcode opcode, LLVMValueRef lhs, LLVMValueRef rhs, const std::string& name)
{
	return LLVMBuildBinOp(builder, opcode, lhs, rhs, name.c_str());
}

LLVMValueRef CompiledModuleBuilder::CreateNeg(LLVMValueRef value, const std::string& name)
{
	return LLVMBuildNeg(builder, value, name.c_str());
}

LLVMValueRef CompiledModuleBuilder::CreateNSWNeg(LLVMValueRef value, const std::string& name)
{
	return LLVMBuildNSWNeg(builder, value, name.c_str());
}

LLVMValueRef CompiledModuleBuilder::CreateNUWNeg(LLVMValueRef value, const std::string& name)
{
	return LLVMBuildNUWNeg(builder, value, name.c_str());
}

LLVMValueRef CompiledModuleBuilder::CreateFNeg(LLVMValueRef value, const std::string& name)
{
	return LLVMBuildFNeg(builder, value, name.c_str());
}

LLVMValueRef CompiledModuleBuilder::CreateNot(LLVMValueRef value, const std::string& name)
{
	return LLVMBuildNot(builder, value, name.c_str());
}

LLVMValueRef CompiledModuleBuilder::CreateMalloc(LLVMTypeRef type, const std::string& name)
{
	return LLVMBuildMalloc(builder, type, name.c_str());
}

LLVMValueRef CompiledModuleBuilder::CreateArrayMalloc(LLVMTypeRef type, LLVMValueRef value, const std::string& name)
{
	return LLVMBuildArrayMalloc(builder, type, value, name.c_str());
}

LLVMValueRef CompiledModuleBuilder::CreateMemSet(LLVMValueRef pointer, LLVMValueRef value, LLVMValueRef length, uint32_t alignment)
{
	return LLVMBuildMemSet(builder, pointer, value, length, alignment);
}

LLVMValueRef CompiledModuleBuilder::CreateMemCpy(LLVMValueRef destination, uint32_t destinationAlignment, LLVMValueRef source, uint32_t sourceAlignment, LLVMValueRef size, bool isVolatile)
{
	const auto value = LLVMBuildMemCpy(builder, destination, destinationAlignment, source, sourceAlignment, size);
	LLVMSetVolatile(value, isVolatile);
	return value;
}

LLVMValueRef CompiledModuleBuilder::CreateMemMove(LLVMValueRef destination, uint32_t destinationAlignment, LLVMValueRef source, uint32_t sourceAlignment, LLVMValueRef size, bool isVolatile)
{
	const auto value = LLVMBuildMemMove(builder, destination, destinationAlignment, source, sourceAlignment, size);
	LLVMSetVolatile(value, isVolatile);
	return value;
}

LLVMValueRef CompiledModuleBuilder::CreateAlloca(LLVMTypeRef type, const std::string& name)
{
	return LLVMBuildAlloca(builder, type, name.c_str());
}

LLVMValueRef CompiledModuleBuilder::CreateAlloca(LLVMTypeRef type, LLVMValueRef value, const std::string& name)
{
	return LLVMBuildArrayAlloca(builder, type, value, name.c_str());
}

LLVMValueRef CompiledModuleBuilder::CreateFree(LLVMValueRef pointer)
{
	return LLVMBuildFree(builder, pointer);
}

LLVMValueRef CompiledModuleBuilder::CreateLoad(LLVMValueRef pointer, bool isVolatile, const std::string& name)
{
	const auto value = LLVMBuildLoad(builder, pointer, name.c_str());
	LLVMSetVolatile(value, isVolatile);
	LLVMSetAlignment(value, ALIGNMENT);
	return value;
}

LLVMValueRef CompiledModuleBuilder::CreateLoad(LLVMTypeRef type, LLVMValueRef pointer, bool isVolatile, const std::string& name)
{
	const auto value = LLVMBuildLoad2(builder, type, pointer, name.c_str());
	LLVMSetVolatile(value, isVolatile);
	LLVMSetAlignment(value, ALIGNMENT);
	return value;
}

LLVMValueRef CompiledModuleBuilder::CreateStore(LLVMValueRef value, LLVMValueRef pointer, bool isVolatile)
{
	value = LLVMBuildStore(builder, value, pointer);
	LLVMSetVolatile(value, isVolatile);
	LLVMSetAlignment(value, ALIGNMENT);
	return value;
}

LLVMValueRef CompiledModuleBuilder::CreateGEP(LLVMValueRef pointer, uint32_t index0)
{
	std::vector<LLVMValueRef> indices
	{
		LLVMConstInt(LLVMInt32TypeInContext(context), index0, false),
	};
	return LLVMBuildGEP(builder, pointer, indices.data(), static_cast<uint32_t>(indices.size()), "");
}

LLVMValueRef CompiledModuleBuilder::CreateGEP(LLVMValueRef pointer, uint32_t index0, uint32_t index1)
{
	std::vector<LLVMValueRef> indices
	{
		LLVMConstInt(LLVMInt32TypeInContext(context), index0, false),
		LLVMConstInt(LLVMInt32TypeInContext(context), index1, false),
	};
	return LLVMBuildGEP(builder, pointer, indices.data(), static_cast<uint32_t>(indices.size()), "");
}

LLVMValueRef CompiledModuleBuilder::CreateGEP(LLVMValueRef pointer, std::vector<LLVMValueRef> indices)
{
	return LLVMBuildGEP(builder, pointer, indices.data(), static_cast<uint32_t>(indices.size()), "");
}

LLVMValueRef CompiledModuleBuilder::CreateInBoundsGEP(LLVMValueRef pointer, uint32_t index0)
{
	const auto value = CreateGEP(pointer, index0);
	// TODO: LLVMSetIsInBounds(value, true);
	return value;
}

LLVMValueRef CompiledModuleBuilder::CreateInBoundsGEP(LLVMValueRef pointer, uint32_t index0, uint32_t index1)
{
	const auto value = CreateGEP(pointer, index0, index1);
	// TODO: LLVMSetIsInBounds(value, true);
	return value;
}

LLVMValueRef CompiledModuleBuilder::CreateInBoundsGEP(LLVMValueRef pointer, std::vector<LLVMValueRef> indices)
{
	const auto value = CreateGEP(pointer, std::move(indices));
	// TODO: LLVMSetIsInBounds(value, true);
	return value;
}

LLVMValueRef CompiledModuleBuilder::CreateGlobalString(const std::string& str, const std::string& name)
{
	return LLVMBuildGlobalString(builder, str.c_str(), name.c_str());
}

LLVMValueRef CompiledModuleBuilder::CreateGlobalStringPtr(const std::string& str, const std::string& name)
{
	return LLVMBuildGlobalStringPtr(builder, str.c_str(), name.c_str());
}

LLVMValueRef CompiledModuleBuilder::CreateTrunc(LLVMValueRef value, LLVMTypeRef destinationType, const std::string& name)
{
	return LLVMBuildTrunc(builder, value, destinationType, name.c_str());
}

LLVMValueRef CompiledModuleBuilder::CreateZExt(LLVMValueRef value, LLVMTypeRef destinationType, const std::string& name)
{
	return LLVMBuildZExt(builder, value, destinationType, name.c_str());
}

LLVMValueRef CompiledModuleBuilder::CreateSExt(LLVMValueRef value, LLVMTypeRef destinationType, const std::string& name)
{
	return LLVMBuildSExt(builder, value, destinationType, name.c_str());
}

LLVMValueRef CompiledModuleBuilder::CreateFPToUI(LLVMValueRef value, LLVMTypeRef destinationType, const std::string& name)
{
	return LLVMBuildFPToUI(builder, value, destinationType, name.c_str());
}

LLVMValueRef CompiledModuleBuilder::CreateFPToSI(LLVMValueRef value, LLVMTypeRef destinationType, const std::string& name)
{
	return LLVMBuildFPToSI(builder, value, destinationType, name.c_str());
}

LLVMValueRef CompiledModuleBuilder::CreateUIToFP(LLVMValueRef value, LLVMTypeRef destinationType, const std::string& name)
{
	return LLVMBuildUIToFP(builder, value, destinationType, name.c_str());
}

LLVMValueRef CompiledModuleBuilder::CreateSIToFP(LLVMValueRef value, LLVMTypeRef destinationType, const std::string& name)
{
	return LLVMBuildSIToFP(builder, value, destinationType, name.c_str());
}

LLVMValueRef CompiledModuleBuilder::CreateFPTrunc(LLVMValueRef value, LLVMTypeRef destinationType, const std::string& name)
{
	return LLVMBuildFPTrunc(builder, value, destinationType, name.c_str());
}

LLVMValueRef CompiledModuleBuilder::CreateFPExt(LLVMValueRef value, LLVMTypeRef destinationType, const std::string& name)
{
	return LLVMBuildFPExt(builder, value, destinationType, name.c_str());
}

LLVMValueRef CompiledModuleBuilder::CreateFPExtOrTrunc(LLVMValueRef value, LLVMTypeRef destinationType, const std::string& name)
{
	const auto inputBits = LLVMSizeOfTypeInBits(jit->getDataLayout(), LLVMTypeOf(value));
	const auto outputBits = LLVMSizeOfTypeInBits(jit->getDataLayout(), destinationType);

	if (inputBits < outputBits)
	{
		return CreateFPExt(value, destinationType, name);
	}

	if (inputBits > outputBits)
	{
		return CreateFPTrunc(value, destinationType, name);
	}

	return value;
}

LLVMValueRef CompiledModuleBuilder::CreatePtrToInt(LLVMValueRef value, LLVMTypeRef destinationType, const std::string& name)
{
	return LLVMBuildPtrToInt(builder, value, destinationType, name.c_str());
}

LLVMValueRef CompiledModuleBuilder::CreateIntToPtr(LLVMValueRef value, LLVMTypeRef destinationType, const std::string& name)
{
	return LLVMBuildIntToPtr(builder, value, destinationType, name.c_str());
}

LLVMValueRef CompiledModuleBuilder::CreateBitCast(LLVMValueRef value, LLVMTypeRef destinationType, const std::string& name)
{
	return LLVMBuildBitCast(builder, value, destinationType, name.c_str());
}

LLVMValueRef CompiledModuleBuilder::CreateAddrSpaceCast(LLVMValueRef value, LLVMTypeRef destinationType, const std::string& name)
{
	return LLVMBuildAddrSpaceCast(builder, value, destinationType, name.c_str());
}

LLVMValueRef CompiledModuleBuilder::CreateZExtOrTrunc(LLVMValueRef value, LLVMTypeRef destinationType, const std::string& name)
{
	const auto inputBits = LLVMSizeOfTypeInBits(jit->getDataLayout(), LLVMTypeOf(value));
	const auto outputBits = LLVMSizeOfTypeInBits(jit->getDataLayout(), destinationType);

	if (inputBits < outputBits)
	{
		return CreateZExt(value, destinationType, name);
	}

	if (inputBits > outputBits)
	{
		return CreateTrunc(value, destinationType, name);
	}

	return value;
}

LLVMValueRef CompiledModuleBuilder::CreateZExtOrBitCast(LLVMValueRef value, LLVMTypeRef destinationType, const std::string& name)
{
	return LLVMBuildZExtOrBitCast(builder, value, destinationType, name.c_str());
}

LLVMValueRef CompiledModuleBuilder::CreateSExtOrTrunc(LLVMValueRef value, LLVMTypeRef destinationType, const std::string& name)
{
	const auto inputBits = LLVMSizeOfTypeInBits(jit->getDataLayout(), LLVMTypeOf(value));
	const auto outputBits = LLVMSizeOfTypeInBits(jit->getDataLayout(), destinationType);

	if (inputBits < outputBits)
	{
		return CreateSExt(value, destinationType, name);
	}

	if (inputBits > outputBits)
	{
		return CreateTrunc(value, destinationType, name);
	}

	return value;
}

LLVMValueRef CompiledModuleBuilder::CreateSExtOrBitCast(LLVMValueRef value, LLVMTypeRef destinationType, const std::string& name)
{
	return LLVMBuildSExtOrBitCast(builder, value, destinationType, name.c_str());
}

LLVMValueRef CompiledModuleBuilder::CreateTruncOrBitCast(LLVMValueRef value, LLVMTypeRef destinationType, const std::string& name)
{
	return LLVMBuildTruncOrBitCast(builder, value, destinationType, name.c_str());
}

LLVMValueRef CompiledModuleBuilder::CreateCast(LLVMOpcode opcode, LLVMValueRef value, LLVMTypeRef destinationType, const std::string& name)
{
	return LLVMBuildCast(builder, opcode, value, destinationType, name.c_str());
}

LLVMValueRef CompiledModuleBuilder::CreatePointerCast(LLVMValueRef value, LLVMTypeRef destinationType, const std::string& name)
{
	return LLVMBuildPointerCast(builder, value, destinationType, name.c_str());
}

LLVMValueRef CompiledModuleBuilder::CreateIntCast(LLVMValueRef value, LLVMTypeRef destinationType, bool isSigned, const std::string& name)
{
	return LLVMBuildIntCast2(builder, value, destinationType, isSigned, name.c_str());
}

LLVMValueRef CompiledModuleBuilder::CreateFPCast(LLVMValueRef value, LLVMTypeRef destinationType, const std::string& name)
{
	return LLVMBuildFPCast(builder, value, destinationType, name.c_str());
}

LLVMValueRef CompiledModuleBuilder::CreateICmp(LLVMIntPredicate opcode, LLVMValueRef lhs, LLVMValueRef rhs, const std::string& name)
{
	return LLVMBuildICmp(builder, opcode, lhs, rhs, name.c_str());
}

LLVMValueRef CompiledModuleBuilder::CreateFCmp(LLVMRealPredicate opcode, LLVMValueRef lhs, LLVMValueRef rhs, const std::string& name)
{
	return LLVMBuildFCmp(builder, opcode, lhs, rhs, name.c_str());
}

LLVMValueRef CompiledModuleBuilder::CreatePhi(LLVMTypeRef type, const std::string& name)
{
	return LLVMBuildPhi(builder, type, name.c_str());
}

LLVMValueRef CompiledModuleBuilder::CreateCall(LLVMValueRef function, LLVMValueRef* arguments, uint32_t numberArguments, const std::string& name)
{
	return LLVMBuildCall(builder, function, arguments, numberArguments, name.c_str());
}

LLVMValueRef CompiledModuleBuilder::CreateCall(LLVMValueRef function, std::vector<LLVMValueRef> arguments, const std::string& name)
{
	return LLVMBuildCall(builder, function, arguments.data(), static_cast<uint32_t>(arguments.size()), name.c_str());
}

LLVMValueRef CompiledModuleBuilder::CreateSelect(LLVMValueRef conditional, LLVMValueRef thenValue, LLVMValueRef elseValue, const std::string& name)
{
	return LLVMBuildSelect(builder, conditional, thenValue, elseValue, name.c_str());
}

LLVMValueRef CompiledModuleBuilder::CreateVAArg(LLVMValueRef list, LLVMTypeRef type, const std::string& name)
{
	return LLVMBuildVAArg(builder, list, type, name.c_str());
}

LLVMValueRef CompiledModuleBuilder::CreateExtractElement(LLVMValueRef vector, LLVMValueRef Index, const std::string& name)
{
	return LLVMBuildExtractElement(builder, vector, Index, name.c_str());
}

LLVMValueRef CompiledModuleBuilder::CreateInsertElement(LLVMValueRef vector, LLVMValueRef element, LLVMValueRef index, const std::string& name)
{
	return LLVMBuildInsertElement(builder, vector, element, index, name.c_str());
}

LLVMValueRef CompiledModuleBuilder::CreateShuffleVector(LLVMValueRef value1, LLVMValueRef value2, LLVMValueRef mask, const std::string& name)
{
	return LLVMBuildShuffleVector(builder, value1, value2, mask, name.c_str());
}

LLVMValueRef CompiledModuleBuilder::CreateExtractValue(LLVMValueRef aggregate, uint32_t index, const std::string& name)
{
	return LLVMBuildExtractValue(builder, aggregate, index, name.c_str());
}

LLVMValueRef CompiledModuleBuilder::CreateInsertValue(LLVMValueRef aggregate, LLVMValueRef element, uint32_t index, const std::string& name)
{
	return LLVMBuildInsertValue(builder, aggregate, element, index, name.c_str());
}

LLVMValueRef CompiledModuleBuilder::CreateIsNull(LLVMValueRef value, const std::string& name)
{
	return LLVMBuildIsNull(builder, value, name.c_str());
}

LLVMValueRef CompiledModuleBuilder::CreateIsNotNull(LLVMValueRef value, const std::string& name)
{
	return LLVMBuildIsNotNull(builder, value, name.c_str());
}

LLVMValueRef CompiledModuleBuilder::CreatePtrDiff(LLVMValueRef lhs, LLVMValueRef rhs, const std::string& name)
{
	return LLVMBuildPtrDiff(builder, lhs, rhs, name.c_str());
}

LLVMValueRef CompiledModuleBuilder::CreateFence(LLVMAtomicOrdering ordering, bool singleThread, const std::string& name)
{
	return LLVMBuildFence(builder, ordering, singleThread, name.c_str());
}

LLVMValueRef CompiledModuleBuilder::CreateAtomicRMW(LLVMAtomicRMWBinOp opcode, LLVMValueRef pointer, LLVMValueRef value, LLVMAtomicOrdering ordering, bool singleThread)
{
	return LLVMBuildAtomicRMW(builder, opcode, pointer, value, ordering, singleThread);
}

LLVMValueRef CompiledModuleBuilder::CreateAtomicCmpXchg(LLVMValueRef pointer, LLVMValueRef cmp, LLVMValueRef New, LLVMAtomicOrdering successOrdering, LLVMAtomicOrdering failureOrdering, bool singleThread)
{
	return LLVMBuildAtomicCmpXchg(builder, pointer, cmp, New, successOrdering, failureOrdering, singleThread);
}

LLVMValueRef CompiledModuleBuilder::CreateVectorSplat(uint32_t vectorSize, LLVMValueRef value, const std::string& name)
{
	assert(vectorSize <= 4);
	
	const auto undefVector = LLVMGetUndef(LLVMVectorType(LLVMTypeOf(value), vectorSize));
	const auto tmpVector = CreateInsertElement(undefVector, value, ConstI32(0), name.empty() ? "" : name + ".splatinsert");
	LLVMValueRef zeroValues[]
	{
		ConstI32(0),
		ConstI32(0),
		ConstI32(0),
		ConstI32(0),
	};
	const auto zeros = LLVMConstVector(zeroValues, vectorSize);
	return CreateShuffleVector(tmpVector, undefVector, zeros, name.empty() ? "" : name + ".splat");
}