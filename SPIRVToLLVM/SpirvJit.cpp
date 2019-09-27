#include "Converter.h"

#include "SpirvFunctions.h"

#include <llvm/ExecutionEngine/Orc/LLJIT.h>

#include <llvm/ExecutionEngine/Orc/CompileUtils.h>
#include <llvm/ExecutionEngine/Orc/ExecutionUtils.h>
#include <llvm/ExecutionEngine/Orc/IRCompileLayer.h>
#include <llvm/ExecutionEngine/Orc/JITTargetMachineBuilder.h>
#include <llvm/ExecutionEngine/Orc/RTDyldObjectLinkingLayer.h>
#include <llvm/ExecutionEngine/SectionMemoryManager.h>

#if !defined(_MSC_VER)
#define __debugbreak() __asm__("int3")

inline void strcpy_s(char* destination, const char* source)
{
	strcpy(destination, source);
}
#endif
#define FATAL_ERROR() if (1) { __debugbreak(); abort(); } else (void)0

static std::unordered_map<std::string, void*> functions{}; // TODO: Attach to SpirvJit

class SpirvCompiledModule
{
public:
	SpirvCompiledModule(llvm::Module* module, llvm::orc::ThreadSafeContext context, llvm::orc::JITDylib& dylib);
	~SpirvCompiledModule();

	llvm::Module* module;
	llvm::orc::ThreadSafeContext context;
	llvm::orc::JITDylib& dylib;
};

static llvm::orc::SymbolNameSet GetSpirvFunctions(llvm::orc::JITDylib& parent, const llvm::orc::SymbolNameSet& names)
{
	llvm::orc::SymbolNameSet addedSymbols;
	llvm::orc::SymbolMap newSymbols;
	for (const auto& name : names)
	{
		if ((*name).empty())
		{
			continue;
		}
		
		std::string tmp((*name).data(), (*name).size());
		auto functionPtr = functions.find(tmp);
		void* function;
		if (functionPtr != functions.end())
		{
			function = functionPtr->second;
		}
		else
		{
			functionPtr = getSpirvFunctions().find(tmp);
			if (functionPtr != getSpirvFunctions().end())
			{
				function = functionPtr->second;
			}
			else
			{
				continue;
			}
		}
		
		addedSymbols.insert(name);
		newSymbols[name] = llvm::JITEvaluatedSymbol(static_cast<llvm::JITTargetAddress>(reinterpret_cast<uintptr_t>(function)), llvm::JITSymbolFlags::Exported | llvm::JITSymbolFlags::Callable);
	}

	// Add any new symbols to parent. Since the generator is only called for symbols
	// that are not already defined, this will never trigger a duplicate
	// definition error, so we can wrap this call in a 'cantFail'.
	if (!newSymbols.empty())
	{
		cantFail(parent.define(absoluteSymbols(std::move(newSymbols))));
	}
		
	return addedSymbols;
}

class SpirvJit::Impl
{
public:
	Impl(llvm::orc::JITTargetMachineBuilder targetMachineBuilder, const llvm::DataLayout& dataLayout)
		: objectLayer(executionSession, []() { return std::make_unique<llvm::SectionMemoryManager>(); }),
		  compileLayer(executionSession, objectLayer, llvm::orc::ConcurrentIRCompiler(std::move(targetMachineBuilder))),
		  dataLayout(dataLayout),
		  mangle(executionSession, this->dataLayout)
	{
		// TODO: Only setOverrideObjectFlagsWithResponsibilityFlags on windows
		objectLayer.setOverrideObjectFlagsWithResponsibilityFlags(true);
		objectLayer.setAutoClaimResponsibilityForObjectSymbols(true);
	}

	SpirvCompiledModule* CompileModule(const SPIRV::SPIRVModule* spirvModule, spv::ExecutionModel executionModel)
	{
		auto context = llvm::orc::ThreadSafeContext(std::make_unique<llvm::LLVMContext>());
		auto module = llvm::orc::ThreadSafeModule(ConvertSpirv(context.getContext(), spirvModule, executionModel), context);
		const auto moduleValue = module.getModule();
		auto& dylib = executionSession.createJITDylib("shader");
		dylib.setGenerator(GetSpirvFunctions);
		
		if (module.getModule()->getDataLayout().isDefault())
		{
			module.getModule()->setDataLayout(dataLayout);
		}

		cantFail(compileLayer.add(dylib, std::move(module), executionSession.allocateVModule()));
		return new SpirvCompiledModule(moduleValue, std::move(context), dylib);
	}

	llvm::Expected<llvm::JITEvaluatedSymbol> lookup(SpirvCompiledModule* module, llvm::StringRef name)
	{
		return executionSession.lookup(&module->dylib, mangle(name.str()));
	}

	void* getPointer(SpirvCompiledModule* module, const std::string& name)
	{
		return reinterpret_cast<void*>(cantFail(lookup(module, name)).getAddress());
	}

private:
	llvm::orc::ExecutionSession executionSession;
	llvm::orc::RTDyldObjectLinkingLayer objectLayer;
	llvm::orc::IRCompileLayer compileLayer;
	
	llvm::DataLayout dataLayout;
	llvm::orc::MangleAndInterner mangle;
};

SpirvJit::SpirvJit()
{
	LLVMInitializeNativeTarget();
	LLVMInitializeNativeAsmPrinter();
	LLVMInitializeNativeAsmParser();
	
	auto targetMachineBuilder = llvm::orc::JITTargetMachineBuilder::detectHost();

	if (!targetMachineBuilder)
	{
		FATAL_ERROR();
	}

	auto dataLayout = targetMachineBuilder->getDefaultDataLayoutForTarget();
	if (!dataLayout)
	{
		FATAL_ERROR();
	}

	impl = new Impl(std::move(*targetMachineBuilder), std::move(*dataLayout));
}

SpirvJit::~SpirvJit()
{
	delete impl;
}

SpirvCompiledModule* SpirvJit::CompileModule(const SPIRV::SPIRVModule* spirvModule, spv::ExecutionModel executionModel)
{
	return impl->CompileModule(spirvModule, executionModel);
}

void* SpirvJit::getPointer(SpirvCompiledModule* module, const std::string& name)
{
	return impl->getPointer(module, name);
}

SpirvJit::FunctionPointer SpirvJit::getFunctionPointer(SpirvCompiledModule* module, const std::string& name)
{
	return reinterpret_cast<FunctionPointer>(getPointer(module, name));
}

SpirvCompiledModule::SpirvCompiledModule(llvm::Module* module, llvm::orc::ThreadSafeContext context, llvm::orc::JITDylib& dylib) :
	module{module},
	context{std::move(context)},
	dylib{dylib}
{
}

SpirvCompiledModule::~SpirvCompiledModule() = default;

void __declspec(dllexport) AddSpirvFunction(const std::string& name, void* pointer)
{
	functions.insert(std::make_pair(name, pointer));
}