#include "Jit.h"

#include "SPIRVCompiler.h"
#include "SpirvFunctions.h"

#include <llvm/ExecutionEngine/Orc/CompileUtils.h>
#include <llvm/ExecutionEngine/Orc/ExecutionUtils.h>
#include <llvm/ExecutionEngine/Orc/IRCompileLayer.h>
#include <llvm/ExecutionEngine/Orc/JITTargetMachineBuilder.h>
#include <llvm/ExecutionEngine/Orc/LLJIT.h>
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

class CompiledModule
{
public:
	CompiledModule(llvm::Module* module, llvm::orc::ThreadSafeContext context, llvm::orc::JITDylib& dylib);
	~CompiledModule();

	llvm::Module* module;
	llvm::orc::ThreadSafeContext context;
	llvm::orc::JITDylib& dylib;
};

class CPJit::Impl
{
public:
	Impl(llvm::orc::JITTargetMachineBuilder&& targetMachineBuilder, llvm::DataLayout&& dataLayout)
		: objectLayer(executionSession, []() { return std::make_unique<llvm::SectionMemoryManager>(); }),
		  compileLayer(executionSession, objectLayer, llvm::orc::ConcurrentIRCompiler(std::move(targetMachineBuilder))),
		  dataLayout(dataLayout),
		  mangle(executionSession, this->dataLayout)
	{
		// TODO: Only setOverrideObjectFlagsWithResponsibilityFlags on windows
		objectLayer.setOverrideObjectFlagsWithResponsibilityFlags(true);
		objectLayer.setAutoClaimResponsibilityForObjectSymbols(true);

		std::string errorMessage;
		library = llvm::sys::DynamicLibrary::getPermanentLibrary(nullptr, &errorMessage);
		if (!library.isValid())
		{
			FATAL_ERROR();
		}
	}

	CompiledModule* CompileModule(const SPIRV::SPIRVModule* spirvModule, spv::ExecutionModel executionModel)
	{
		auto context = std::make_unique<llvm::LLVMContext>();
		auto module = ConvertSpirv(context.get(), spirvModule, executionModel);
		return CompileModule(std::move(context), std::move(module));
	}

	CompiledModule* CompileModule(std::unique_ptr<llvm::LLVMContext> context, std::unique_ptr<llvm::Module> module)
	{
		const auto safeContext = llvm::orc::ThreadSafeContext(std::move(context));
		auto safeModule = llvm::orc::ThreadSafeModule(std::move(module), safeContext);
		const auto moduleValue = safeModule.getModule();
		auto& dylib = executionSession.createJITDylib("shader");
		dylib.setGenerator([this](llvm::orc::JITDylib& parent, const llvm::orc::SymbolNameSet& names)
		{
			return getFunctions(parent, names);
		});
		auto xxx = llvm::orc::DynamicLibrarySearchGenerator::Load(nullptr, dataLayout);
		
		if (safeModule.getModule()->getDataLayout().isDefault())
		{
			safeModule.getModule()->setDataLayout(dataLayout);
		}

		cantFail(compileLayer.add(dylib, std::move(safeModule), executionSession.allocateVModule()));
		const auto compiledModule = new CompiledModule(moduleValue, safeContext, dylib);
		auto userDataPtr = Lookup(compiledModule, "@userData");
		if (userDataPtr)
		{
			*reinterpret_cast<void**>(userDataPtr->getAddress()) = userData;
		}
		return compiledModule;
	}

	void AddFunction(const std::string& name, FunctionPointer pointer)
	{
		functions.insert(std::make_pair(name, pointer));
	}
	
	void SetUserData(void* userData)
	{
		this->userData = userData;
	}

	llvm::Expected<llvm::JITEvaluatedSymbol> Lookup(const CompiledModule* module, llvm::StringRef name)
	{
		return executionSession.lookup(&module->dylib, mangle(name.str()));
	}

	void* getPointer(const CompiledModule* module, const std::string& name)
	{
		return reinterpret_cast<void*>(cantFail(Lookup(module, name)).getAddress());
	}

private:
	llvm::orc::ExecutionSession executionSession;
	llvm::orc::RTDyldObjectLinkingLayer objectLayer;
	llvm::orc::IRCompileLayer compileLayer;
	
	llvm::DataLayout dataLayout;
	llvm::orc::MangleAndInterner mangle;

	std::unordered_map<std::string, FunctionPointer> functions{};
	llvm::sys::DynamicLibrary library;
	void* userData;

	llvm::orc::SymbolNameSet getFunctions(llvm::orc::JITDylib& parent, const llvm::orc::SymbolNameSet& names)
	{
		const auto hasGlobalPrefix = (dataLayout.getGlobalPrefix() != '\0');
		
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
			FunctionPointer function;
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
					if (hasGlobalPrefix)
					{
						if ((*name).front() != dataLayout.getGlobalPrefix())
						{
							continue;
						}
						tmp = std::string((*name).data() + (hasGlobalPrefix ? 1 : 0), (*name).size());
					}
					if (auto address = library.getAddressOfSymbol(tmp.c_str()))
					{
						function = reinterpret_cast<FunctionPointer>(address);
					}
					else
					{
						continue;
					}
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
};

CPJit::CPJit()
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

CPJit::~CPJit()
{
	delete impl;
}

CompiledModule* CPJit::CompileModule(const SPIRV::SPIRVModule* spirvModule, spv::ExecutionModel executionModel)
{
	return impl->CompileModule(spirvModule, executionModel);
}

CompiledModule* CPJit::CompileModule(std::unique_ptr<llvm::LLVMContext> context, std::unique_ptr<llvm::Module> module)
{
	return impl->CompileModule(std::move(context), std::move(module));
}

void CPJit::AddFunction(const std::string& name, FunctionPointer pointer)
{
	impl->AddFunction(name, pointer);
}

void CPJit::SetUserData(void* userData)
{
	impl->SetUserData(userData);
}

void* CPJit::getPointer(const CompiledModule* module, const std::string& name)
{
	return impl->getPointer(module, name);
}

FunctionPointer CPJit::getFunctionPointer(const CompiledModule* module, const std::string& name)
{
	return reinterpret_cast<FunctionPointer>(getPointer(module, name));
}

CompiledModule::CompiledModule(llvm::Module* module, llvm::orc::ThreadSafeContext context, llvm::orc::JITDylib& dylib) :
	module{module},
	context{std::move(context)},
	dylib{dylib}
{
}

CompiledModule::~CompiledModule() = default;