#include "PipelineCompiler.h"

#include <PipelineData.h>
#include <SPIRVInstruction.h>
#include <SPIRVModule.h>

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

static uint32_t GetVariableSize(SPIRV::SPIRVType* type)
{
	if (type->isTypeArray())
	{
		const auto size = GetVariableSize(type->getArrayElementType());
		if (type->hasDecorate(DecorationArrayStride))
		{
			const auto stride = *type->getDecorate(DecorationArrayStride).begin();
			if (stride != size)
			{
				FATAL_ERROR();
			}
		}

		return size * type->getArrayLength();
	}

	if (type->isTypeStruct())
	{
		auto size = 0u;
		for (auto i = 0u; i < type->getStructMemberCount(); i++)
		{
			const auto decorate = type->getMemberDecorate(i, DecorationOffset);
			if (decorate && decorate->getLiteral(0) != size)
			{
				const auto offset = decorate->getLiteral(0);
				if (offset < size)
				{
					FATAL_ERROR();
				}
				else
				{
					size = offset;
				}
			}
			size += GetVariableSize(type->getStructMemberType(i));
		}
		return size;
	}

	if (type->isTypeMatrix())
	{
		// TODO: Matrix stride & so on
		return GetVariableSize(type->getMatrixComponentType()) * type->getMatrixColumnCount() * type->getMatrixRowCount();
	}

	if (type->isTypeVector())
	{
		return GetVariableSize(type->getVectorComponentType()) * type->getVectorComponentCount();
	}

	if (type->isTypeFloat() || type->isTypeInt())
	{
		return type->getBitWidth() / 8;
	}

	FATAL_ERROR();
}

static uint32_t CalculateOutputStride(const SPIRV::SPIRVModule* spirvModule)
{
	uint32_t stride = sizeof(VertexBuiltinOutput);

	for (auto i = 0u; i < spirvModule->getNumVariables(); i++)
	{
		const auto variable = spirvModule->getVariable(i);
		if (variable->getStorageClass() == StorageClassOutput)
		{
			auto locations = variable->getDecorate(DecorationLocation);
			if (locations.empty())
			{
				continue;
			}
			
			if (variable->getType()->getPointerElementType()->isTypeArray())
			{
				const auto size = variable->getType()->getPointerElementType()->getArrayLength();
				const auto type = variable->getType()->getPointerElementType()->getArrayElementType();
				const auto variableSize = GetVariableSize(type);
				for (auto j = 0U; j < size; j++)
				{
					stride += variableSize;
				}
			}
			else
			{
				const auto type = variable->getType()->getPointerElementType();
				stride += GetVariableSize(type);
			}
		}
	}
	
	return stride;
}

CompiledModule* CompileVertexPipeline(CPJit* jit, const SPIRV::SPIRVModule* vertexShader, const std::vector<const std::vector<VkDescriptorSetLayoutBinding>*>& layoutBindings, std::function<void*(const std::string&)> getFunction)
{
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

	const auto assemblerOutputType = llvm::StructType::create(state.context, {
		                                                          llvm::Type::getInt32Ty(state.context),
		                                                          llvm::Type::getInt32Ty(state.context),
	                                                          }, "", true);

	const auto builtinInputType = llvm::StructType::create(state.context, {
		                                                       llvm::Type::getInt32Ty(state.context),
		                                                       llvm::Type::getInt32Ty(state.context),
	                                                       }, "_BuiltinInput", true);

	const auto builtinOutputType = llvm::StructType::create(state.context, {
		                                                        llvm::VectorType::get(llvm::Type::getFloatTy(state.context), 4),
		                                                        llvm::Type::getFloatTy(state.context),
		                                                        llvm::ArrayType::get(llvm::Type::getFloatTy(state.context), 1),
	                                                        }, "_BuiltinOutput", true);

	const auto outputType = llvm::PointerType::get(llvm::StructType::create(state.context, {
		                                                                        builtinOutputType,
	                                                                        }, "_Output", true), 0);
	
	auto outputVariable = new llvm::GlobalVariable(*state.module, builder.getInt8PtrTy(), false, llvm::GlobalVariable::ExternalLinkage, llvm::Constant::getNullValue(builder.getInt8PtrTy()), "@outputStorage", nullptr, llvm::GlobalValue::NotThreadLocal, 0);
	outputVariable->setAlignment(ALIGNMENT);

	const auto functionType = llvm::FunctionType::get(builder.getVoidTy(), {
		                                                  llvm::PointerType::get(assemblerOutputType, 0),
		                                                  state.builder.getInt32Ty(),
	                                                  }, false);
	const auto function = llvm::Function::Create(static_cast<llvm::FunctionType*>(functionType),
	                                             llvm::GlobalVariable::ExternalLinkage,
	                                             "@VertexProcessing",
	                                             module.get());

	const auto basicBlock = llvm::BasicBlock::Create(*context, "", function);
	builder.SetInsertPoint(basicBlock);

	const auto stride = CalculateOutputStride(vertexShader);

	const auto rawId = state.builder.CreateLoad(state.builder.CreateConstGEP2_32(nullptr, &*function->arg_begin(), 0, 0));
	const auto vertexId = state.builder.CreateLoad(state.builder.CreateConstGEP2_32(nullptr, &*function->arg_begin(), 0, 1));
	const auto instanceId = &*(function->arg_begin() + 1);

	// Get shader variables
	const auto shaderBuiltinInput = builder.CreateIntToPtr(state.builder.getInt64(reinterpret_cast<uint64_t>(getFunction("!VertexBuiltinInput"))), llvm::PointerType::get(builtinInputType, 0));
	const auto shaderBuiltinOutput = builder.CreateIntToPtr(state.builder.getInt64(reinterpret_cast<uint64_t>(getFunction("!VertexBuiltinOutput"))), llvm::PointerType::get(builtinOutputType, 0));

	// Set vertex shader builtin
	state.builder.CreateAlignedStore(vertexId, state.builder.CreateConstGEP2_32(nullptr, shaderBuiltinInput, 0, 0), ALIGNMENT);
	state.builder.CreateAlignedStore(instanceId, state.builder.CreateConstGEP2_32(nullptr, shaderBuiltinInput, 0, 1), ALIGNMENT);

	// Call the vertex shader
	const auto vertexShaderEntry = llvm::Function::Create(llvm::FunctionType::get(builder.getVoidTy(), {}, false), llvm::GlobalValue::ExternalLinkage, "!VertexShader", module.get());
	builder.CreateCall(vertexShaderEntry);

	// Copy vertex builtin output to storage
	const auto builtinOutputPointer = builder.CreateAlignedLoad(shaderBuiltinOutput, ALIGNMENT);
	const auto outputStorage = state.builder.CreateBitCast(state.builder.CreateGEP(state.builder.CreateAlignedLoad(outputVariable, ALIGNMENT), {state.builder.CreateMul(rawId, state.builder.getInt32(stride))}), outputType);
	builder.CreateAlignedStore(builtinOutputPointer, state.builder.CreateConstGEP2_32(nullptr, outputStorage, 0, 0), ALIGNMENT);

	builder.CreateRetVoid();

	// TODO: Optimise

	Dump(module.get());

	return jit->CompileModule(std::move(context), std::move(module), std::move(getFunction));
}
