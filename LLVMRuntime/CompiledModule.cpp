#include "CompiledModule.h"
#include "CompiledModuleBuilder.h"

#include "Jit.h"
#include "SpirvFunctions.h"

#include <Half.h>

#include <llvm-c/Analysis.h>
#include <llvm-c/Core.h>
#include <llvm-c/OrcBindings.h>
#include <llvm-c/Support.h>

#include <iostream>
#include <utility>

CP_DLL_EXPORT std::string DumpModule(LLVMModuleRef module)
{
	const auto ptr = LLVMPrintModuleToString(module);
	const std::string str = ptr;
	LLVMDisposeMessage(ptr);
	return str;
}

CP_DLL_EXPORT std::string DumpType(LLVMTypeRef type)
{
	const auto ptr = LLVMPrintTypeToString(type);
	const std::string str = ptr;
	LLVMDisposeMessage(ptr);
	return str;
}

CP_DLL_EXPORT std::string DumpValue(LLVMValueRef value)
{
	const auto ptr = LLVMPrintValueToString(value);
	const auto str = ptr + std::string{"; "} + DumpType(LLVMTypeOf(value));
	LLVMDisposeMessage(ptr);
	return str;
}

static uint64_t SymbolResolverStub(const char* name, void* lookupContext)
{
	return static_cast<CompiledModule*>(lookupContext)->ResolveSymbol(name);
}

CompiledModule::CompiledModule(CPJit* jit, LLVMContextRef context, LLVMModuleRef module, std::function<void*(const std::string&)> getFunction) :
	jit{jit},
	context{context},
	module{module},
	getFunction{getFunction}
{
	const auto error = LLVMOrcAddEagerlyCompiledIR(jit->getOrc(), &orcModule, module, SymbolResolverStub, this);
	if (error)
	{
		const auto errorMessage = LLVMOrcGetErrorMsg(jit->getOrc());
		FATAL_ERROR();
	}
}

CompiledModule::~CompiledModule()
{
	const auto error = LLVMOrcRemoveModule(jit->getOrc(), orcModule);
	if (error)
	{
		const auto errorMessage = LLVMOrcGetErrorMsg(jit->getOrc());
		FATAL_ERROR();
	}
	
	LLVMDisposeModule(module);
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
	const auto error = LLVMOrcGetSymbolAddressIn(jit->getOrc(), &symbolAddress, orcModule, name.c_str());

	if (error)
	{
		const auto errorMessage = LLVMOrcGetErrorMsg(jit->getOrc());
		FATAL_ERROR();
	}

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
	context = LLVMContextCreate();
	module = LLVMModuleCreateWithNameInContext("", context);
	builder = LLVMCreateBuilderInContext(context);

	MainCompilation();

	LLVMDisposeBuilder(builder);
	
	// TODO: Optimisations

	// TODO: Soft fail?
	// LLVMVerifyModule(module, LLVMAbortProcessAction, nullptr);

	std::cout << DumpModule(module) << std::endl;

	const auto compiledModule = new CompiledModule(jit, context, module, getFunction);

	const auto userData = compiledModule->getOptionalPointer("@userData");
	if (userData)
	{
		*reinterpret_cast<void**>(userData) = jit->getUserData();
	}

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
	const auto ptr = LLVMGetValueName2(value, &length);
	return std::string{ ptr, length };
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

LLVMValueRef CompiledModuleBuilder::CreateLoad(LLVMValueRef pointer, bool isVolatile, const std::string& name)
{
	const auto value = LLVMBuildLoad(builder, pointer, name.c_str());
	LLVMSetVolatile(value, isVolatile);
	LLVMSetAlignment(value, ALIGNMENT);
	return value;
}

LLVMValueRef CompiledModuleBuilder::CreateStore(LLVMValueRef src, LLVMValueRef dst, bool isVolatile)
{
	const auto value = LLVMBuildStore(builder, src, dst);
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
