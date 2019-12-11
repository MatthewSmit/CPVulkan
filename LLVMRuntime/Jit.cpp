#include "Jit.h"

#include "SPIRVCompiler.h"
#include "SpirvFunctions.h"

#include <llvm-c/Core.h>
#include <llvm-c/OrcBindings.h>
#include <llvm-c/Support.h>
#include <llvm-c/Target.h>
#include <llvm-c/TargetMachine.h>

#include <atomic>
#include <mutex>
#include <queue>
#include <thread>
#include <utility>

struct Task
{
	explicit Task(std::function<void()> action) :
		action{std::move(action)}
	{
	}
	
	std::function<void()> action;
	std::mutex mutex{};
	std::condition_variable event{};
	std::atomic_bool isDone{};
};

class CPJit::Impl
{
public:
	Impl()
	{
		this->compileThread = std::thread{
			[this]()
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

				targetMachine = LLVMCreateTargetMachine(target, targetTriple, hostCpu, hostCpuFeatures, LLVMCodeGenLevelAggressive, LLVMRelocDefault, LLVMCodeModelDefault);

				dataLayout = LLVMCreateTargetDataLayout(targetMachine);

				LLVMDisposeMessage(hostCpuFeatures);
				LLVMDisposeMessage(hostCpu);
				LLVMDisposeMessage(targetTriple);

				orcInstance = LLVMOrcCreateInstance(targetMachine);

				LLVMLoadLibraryPermanently(nullptr);
				
				this->ThreadUpdate();
			}
		};
	}

	~Impl()
	{
		this->Call([this]()
		{
			shouldExit = true;

			LLVMDisposeTargetData(dataLayout);
			LLVMOrcDisposeInstance(orcInstance);
		});
		this->compileThread.join();
	}
	
	void ThreadUpdate()
	{
		while (true)
		{
			Task* task = nullptr;
			{
				std::unique_lock<std::mutex> lock{compileMutex};
				while (compileQueue.empty())
				{
					conditionalEvent.wait(lock);
				}

				task = compileQueue.front();
				compileQueue.pop();
			}

			task->action();
			
			{
				std::unique_lock<std::mutex> lock{task->mutex};
				task->isDone = true;
				task->event.notify_one();
			}

			if (shouldExit)
			{
				break;
			}
		}
	}

	void Call(const std::function<void()>& action)
	{
		if (std::this_thread::get_id() == compileThread.get_id())
		{
			action();
		}
		else
		{
			Task task(action);

			{
				std::unique_lock<std::mutex> lock{compileMutex};
				compileQueue.push(&task);
				conditionalEvent.notify_one();
			}

			{
				std::unique_lock<std::mutex> lock{task.mutex};
				while (!task.isDone)
				{
					task.event.wait(lock);
				}
			}
		}
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

	std::thread compileThread;
	std::queue<Task*> compileQueue{};
	std::mutex compileMutex;
	std::condition_variable conditionalEvent{};
	std::atomic_bool shouldExit{};
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

void CPJit::RunOnCompileThread(const std::function<void()>& action)
{
	impl->Call(action);
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
