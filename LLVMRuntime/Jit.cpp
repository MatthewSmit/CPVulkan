#include "Jit.h"

#include "SPIRVCompiler.h"
#include "SpirvFunctions.h"

#include <llvm-c/Core.h>
#include <llvm-c/OrcBindings.h>
#include <llvm-c/Support.h>
#include <llvm-c/Target.h>
#include <llvm-c/TargetMachine.h>

class CPJit::Impl
{
public:
	Impl()
	{
		LLVMInitializeNativeTarget();
		LLVMInitializeNativeAsmPrinter();
		LLVMInitializeNativeAsmParser();
		
		const auto targetTriple = LLVMGetDefaultTargetTriple();
		const auto hostCpu = LLVMGetHostCPUName();
		const auto hostCpuFeatures = LLVMGetHostCPUFeatures();
		
		LLVMTargetRef target;
		if (LLVMGetTargetFromTriple(targetTriple, &target, nullptr) != 0)
		{
			TODO_ERROR();
		}

		// TODO: CodeGenLevel?
		targetMachine = LLVMCreateTargetMachine(target, targetTriple, hostCpu, hostCpuFeatures, LLVMCodeGenLevelNone, LLVMRelocDefault, LLVMCodeModelDefault);

		dataLayout = LLVMCreateTargetDataLayout(targetMachine);
		
		LLVMDisposeMessage(hostCpuFeatures);
		LLVMDisposeMessage(hostCpu);
		LLVMDisposeMessage(targetTriple);

		orcInstance = LLVMOrcCreateInstance(targetMachine);

		LLVMLoadLibraryPermanently(nullptr);
	}

	~Impl()
	{
		LLVMDisposeTargetMachine(targetMachine);
		LLVMDisposeTargetData(dataLayout);
		LLVMOrcDisposeInstance(orcInstance);
	}

	void AddFunction(const std::string& name, FunctionPointer pointer)
	{
		functions.insert(std::make_pair(name, pointer));
	}

	[[nodiscard]] FunctionPointer getFunction(const std::string& name)
	{
		const auto function = functions.find(name);
		if (function == functions.end())
		{
			return nullptr;
		}
		return function->second;
	}

	[[nodiscard]] void* getUserData() const
	{
		return userData;
	}

	[[nodiscard]] LLVMTargetDataRef getDataLayout() const
	{
		return dataLayout;
	}

	[[nodiscard]] LLVMOrcJITStackRef getOrc() const
	{
		return orcInstance;
	}
	
	void setUserData(void* userData)
	{
		this->userData = userData;
	}

private:
	LLVMTargetMachineRef targetMachine;
	LLVMTargetDataRef dataLayout;
	LLVMOrcJITStackRef orcInstance;
	
	std::unordered_map<std::string, FunctionPointer> functions{};
	void* userData{};
};

CPJit::CPJit()
{
	impl = new Impl();
}

CPJit::~CPJit()
{
	delete impl;
}

void CPJit::AddFunction(const std::string& name, FunctionPointer pointer)
{
	impl->AddFunction(name, pointer);
}

FunctionPointer CPJit::getFunction(const std::string& name)
{
	return impl->getFunction(name);
}

void* CPJit::getUserData() const
{
	return impl->getUserData();
}

LLVMTargetDataRef CPJit::getDataLayout() const
{
	return impl->getDataLayout();
}

LLVMOrcJITStackRef CPJit::getOrc() const
{
	return impl->getOrc();
}

void CPJit::setUserData(void* userData)
{
	impl->setUserData(userData);
}