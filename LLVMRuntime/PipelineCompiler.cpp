#include "PipelineCompiler.h"

#include <llvm/ExecutionEngine/Orc/CompileUtils.h>
#include <llvm/ExecutionEngine/Orc/LLJIT.h>
#include <llvm/ExecutionEngine/SectionMemoryManager.h>
#include <llvm/IR/IRBuilder.h>

#include <iostream>
#include <utility>
#include <vector>

struct State
{
	llvm::DataLayout& layout;
	llvm::LLVMContext& context;
	llvm::IRBuilder<>& builder;
	llvm::Module* module;
};

static void Dump(llvm::Module* llvmModule)
{
	std::string dump;
	llvm::raw_string_ostream DumpStream(dump);
	llvmModule->print(DumpStream, nullptr);
	std::cout << dump << std::endl;
}

CompiledModule* CompileVertexPipeline(CPJit* jit, const std::vector<const std::vector<VkDescriptorSetLayoutBinding>*>& layoutBindings, std::function<void*(const std::string&)> getFunction)
{
	LLVMInitializeNativeTarget();
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
	
	auto context = std::make_unique<llvm::LLVMContext>();
	llvm::IRBuilder<> builder(*context);
	auto module = std::make_unique<llvm::Module>("", *context);

	State state
	{
		dataLayout.get(),
		*context,
		builder,
		module.get(),
	};

	const auto assemblerOutputType = llvm::PointerType::get(llvm::StructType::create(state.context, {
		                                                                                 llvm::Type::getInt32Ty(state.context),
		                                                                                 llvm::Type::getInt32Ty(state.context),
	                                                                                 }, "", true), 0);

	const auto builtinInputType = llvm::PointerType::get(llvm::StructType::create(state.context, {
		                                                                              llvm::Type::getInt32Ty(state.context),
		                                                                              llvm::Type::getInt32Ty(state.context),
	                                                                              }, "_BuiltinInput", true), 0);
	auto builtinInputVariable = new llvm::GlobalVariable(*state.module, builtinInputType, false, llvm::GlobalVariable::ExternalLinkage, llvm::Constant::getNullValue(builtinInputType), "@builtinInput", nullptr, llvm::GlobalValue::NotThreadLocal, 0);
	builtinInputVariable->setAlignment(ALIGNMENT);

	const auto builtinOutputType = llvm::PointerType::get(llvm::StructType::create(state.context, {
		                                                                               llvm::VectorType::get(llvm::Type::getFloatTy(state.context), 4),
		                                                                               llvm::Type::getFloatTy(state.context),
		                                                                               llvm::ArrayType::get(llvm::Type::getFloatTy(state.context), 1),
	                                                                               }, "_BuiltinOutput", true), 0);
	auto builtinOutputVariable = new llvm::GlobalVariable(*state.module, builtinOutputType, false, llvm::GlobalVariable::ExternalLinkage, llvm::Constant::getNullValue(builtinOutputType), "@builtinOutput", nullptr, llvm::GlobalValue::NotThreadLocal, 0);
	builtinOutputVariable->setAlignment(ALIGNMENT);

	const auto functionType = llvm::FunctionType::get(builder.getVoidTy(), {
		                                                  assemblerOutputType
	                                                  }, false);
	const auto function = llvm::Function::Create(static_cast<llvm::FunctionType*>(functionType),
	                                             llvm::GlobalVariable::ExternalLinkage,
	                                             "@VertexProcessing",
	                                             module.get());

	const auto basicBlock = llvm::BasicBlock::Create(*context, "", function);
	builder.SetInsertPoint(basicBlock);

	const auto rawId = state.builder.CreateLoad(state.builder.CreateConstGEP2_32(nullptr, &*function->arg_begin(), 0, 0));
	const auto vertexId = state.builder.CreateLoad(state.builder.CreateConstGEP2_32(nullptr, &*function->arg_begin(), 0, 1));

	// Call the vertex shader
	const auto vertexShader = llvm::Function::Create(llvm::FunctionType::get(builder.getVoidTy(), {}, false), llvm::GlobalValue::ExternalLinkage, "!VertexShader", module.get());
	builder.CreateCall(vertexShader);

	// Copy vertex builtin output to storage
	const auto builtinOutputPointer = builder.CreateAlignedLoad(builder.CreateIntToPtr(state.builder.getInt64(reinterpret_cast<uint64_t>(getFunction("!VertexBuiltinOutput"))), builtinOutputType), ALIGNMENT);
	const auto builtinOutputStorage = state.builder.CreateInBoundsGEP(state.builder.CreateAlignedLoad(builtinOutputVariable, ALIGNMENT), {rawId});
	builder.CreateAlignedStore(builtinOutputPointer, builtinOutputStorage, ALIGNMENT);

	builder.CreateRetVoid();

	// TODO: Optimise

	Dump(module.get());

	return jit->CompileModule(std::move(context), std::move(module), std::move(getFunction));
}