#include "PipelineCompiler.h"

#include "CompiledModuleBuilder.h"

#include <PipelineData.h>

#include <SPIRVDecorate.h>
#include <SPIRVInstruction.h>
#include <SPIRVType.h>
#include <utility>

class PipelineVertexCompiledModuleBuilder final : public CompiledModuleBuilder
{
public:
	PipelineVertexCompiledModuleBuilder(CPJit* jit, const SPIRV::SPIRVModule* vertexShader, const std::vector<const std::vector<VkDescriptorSetLayoutBinding>*>& layoutBindings, std::function<void*(const std::string&)> getFunction) :
		CompiledModuleBuilder{jit, std::move(getFunction)},
		vertexShader{vertexShader},
		layoutBindings{layoutBindings}
	{
	}

protected:
	void MainCompilation() override
	{
		// TODO: Prefer globals over inttoptr of a constant address - this gives you dereferencability information. In MCJIT, use getSymbolAddress to provide actual address.
		std::vector<LLVMTypeRef> assemblerOutputMembers
		{
			LLVMInt32TypeInContext(context),
			LLVMInt32TypeInContext(context),
		};
		const auto assemblerOutputType = StructType(assemblerOutputMembers, "_AssemblerOutput", true);

		std::vector<LLVMTypeRef> builtinInputMembers
		{
			LLVMInt32TypeInContext(context),
			LLVMInt32TypeInContext(context),
		};
		const auto builtinInputType = StructType(builtinInputMembers, "_BuiltinInput", true);

		std::vector<LLVMTypeRef> builtinOutputMembers
		{
			LLVMVectorType(LLVMFloatTypeInContext(context), 4),
			LLVMFloatTypeInContext(context),
			LLVMArrayType(LLVMFloatTypeInContext(context), 1),
		};
		const auto builtinOutputType = StructType(builtinOutputMembers, "_BuiltinOutput", true);

		std::vector<LLVMTypeRef> outputMembers
		{
			builtinOutputType,
		};
		const auto outputType = LLVMPointerType(StructType(outputMembers, "_Output", true), 0);
		const auto outputVariable = GlobalVariable(LLVMPointerType(LLVMInt8TypeInContext(context), 0), LLVMExternalLinkage, "@outputStorage");

		std::vector<LLVMTypeRef> parameters
		{
			LLVMPointerType(assemblerOutputType, 0),
			LLVMInt32TypeInContext(context),
		};
		const auto functionType = LLVMFunctionType(LLVMVoidType(), parameters.data(), static_cast<uint32_t>(parameters.size()), false);
		const auto function = LLVMAddFunction(module, "@VertexProcessing", functionType);
		LLVMSetLinkage(function, LLVMExternalLinkage);

		const auto basicBlock = LLVMAppendBasicBlockInContext(context, function, "");
		LLVMPositionBuilderAtEnd(builder, basicBlock);

		const auto stride = CalculateOutputStride(vertexShader);

		const auto rawId = CreateLoad(CreateGEP(LLVMGetParam(function, 0), 0, 0));
		const auto vertexId = CreateLoad(CreateGEP(LLVMGetParam(function, 0), 0, 1));
		const auto instanceId = LLVMGetParam(function, 1);
		
		// Get shader variables
		const auto shaderBuiltinInput = LLVMBuildIntToPtr(builder, ConstU64(reinterpret_cast<uint64_t>(getFunction("!VertexBuiltinInput"))), LLVMPointerType(builtinInputType, 0), "");
		const auto shaderBuiltinOutput = LLVMBuildIntToPtr(builder, ConstU64(reinterpret_cast<uint64_t>(getFunction("!VertexBuiltinOutput"))), LLVMPointerType(builtinOutputType, 0), "");
		
		// Set vertex shader builtin
		CreateStore(vertexId, CreateGEP(shaderBuiltinInput, 0, 0), false);
		CreateStore(instanceId, CreateGEP(shaderBuiltinInput, 0, 1), false);
		
		// Call the vertex shader
		const auto vertexShaderEntry = LLVMAddFunction(module, "!VertexShader", LLVMFunctionType(LLVMVoidType(), nullptr, 0, false));
		LLVMSetLinkage(vertexShaderEntry, LLVMExternalLinkage);
		LLVMBuildCall(builder, vertexShaderEntry, nullptr, 0, "");
		
		// Copy vertex builtin output to storage
		const auto builtinOutputPointer = CreateLoad(shaderBuiltinOutput);
		const auto outputStorage = LLVMBuildBitCast(builder, CreateGEP(CreateLoad(outputVariable), {LLVMBuildMul(builder, rawId, ConstU32(stride), "")}), outputType, "");
		CreateStore(builtinOutputPointer, CreateGEP(outputStorage, 0, 0));

		LLVMBuildRetVoid(builder);
	}
	
private:
	const SPIRV::SPIRVModule* vertexShader;
	const std::vector<const std::vector<VkDescriptorSetLayoutBinding>*>& layoutBindings;
	
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
					TODO_ERROR();
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
						TODO_ERROR();
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
			return GetVariableSize(type->getScalarType()) * type->getMatrixColumnCount() * type->getMatrixColumnType()->getVectorComponentCount();
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
};

CompiledModule* CompileVertexPipeline(CPJit* jit, const SPIRV::SPIRVModule* vertexShader, const std::vector<const std::vector<VkDescriptorSetLayoutBinding>*>& layoutBindings, std::function<void*(const std::string&)> getFunction)
{
	return PipelineVertexCompiledModuleBuilder(jit, vertexShader, layoutBindings, std::move(getFunction)).Compile();
}
