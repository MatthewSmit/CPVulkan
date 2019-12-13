#include "PipelineCompiler.h"

#include "CompiledModule.h"
#include "CompiledModuleBuilder.h"
#include "SPIRVBaseCompiler.h"
#include "SPIRVCompiler.h"

#include <PipelineData.h>

#include <SPIRVDecorate.h>
#include <SPIRVInstruction.h>
#include <SPIRVType.h>

#include <llvm-c/Target.h>

#include <utility>

class PipelineVertexCompiledModuleBuilder final : public SPIRVBaseCompiledModuleBuilder
{
public:
	PipelineVertexCompiledModuleBuilder(CPJit* jit, const SPIRV::SPIRVModule* spirvVertexShader, CompiledModule* llvmVertexShader, const std::vector<const std::vector<VkDescriptorSetLayoutBinding>*>& layoutBindings, std::function<void*(const std::string&)> getFunction) :
		SPIRVBaseCompiledModuleBuilder{jit, std::move(getFunction)},
		spirvVertexShader{spirvVertexShader},
		llvmVertexShader{llvmVertexShader},
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

		AddOutputData(outputMembers);
		
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

		const auto stride = LLVMSizeOfTypeInBits(jit->getDataLayout(), LLVMGetElementType(outputType)) / 8;

		const auto rawId = CreateLoad(CreateGEP(LLVMGetParam(function, 0), 0, 0));
		const auto vertexId = CreateLoad(CreateGEP(LLVMGetParam(function, 0), 0, 1));
		const auto instanceId = LLVMGetParam(function, 1);
		
		// Get shader variables
		const auto shaderBuiltinInputAddress = CreateIntToPtr(ConstU64(reinterpret_cast<uint64_t>(getFunction("!VertexBuiltinInput"))), LLVMPointerType(builtinInputType, 0));
		const auto shaderBuiltinOutputAddress = CreateIntToPtr(ConstU64(reinterpret_cast<uint64_t>(getFunction("!VertexBuiltinOutput"))), LLVMPointerType(builtinOutputType, 0));
		
		// Set vertex shader builtin
		CreateStore(vertexId, CreateGEP(shaderBuiltinInputAddress, 0, 0), false);
		CreateStore(instanceId, CreateGEP(shaderBuiltinInputAddress, 0, 1), false);
		
		// Call the vertex shader
		const auto vertexShaderEntry = LLVMAddFunction(module, "!VertexShader", LLVMFunctionType(LLVMVoidType(), nullptr, 0, false));
		LLVMSetLinkage(vertexShaderEntry, LLVMExternalLinkage);
		LLVMBuildCall(builder, vertexShaderEntry, nullptr, 0, "");

		// TODO: Modify vertex shader to use pointer directly
		
		// Copy vertex builtin output to storage
		const auto shaderBuiltinOutput = CreateLoad(shaderBuiltinOutputAddress);
		const auto outputStorage = CreateBitCast(CreateGEP(CreateLoad(outputVariable), {LLVMBuildMul(builder, rawId, ConstU32(stride), "")}), outputType);
		CreateStore(shaderBuiltinOutput, CreateGEP(outputStorage, 0, 0));

		// Copy custom output to storage
		for (auto i = 0u; i < spirvVertexShader->getNumVariables(); i++)
		{
			const auto variable = spirvVertexShader->getVariable(i);
			if (variable->getStorageClass() == StorageClassOutput)
			{
				auto locations = variable->getDecorate(DecorationLocation);
				if (locations.empty())
				{
					continue;
				}

				const auto shaderAddress = CreateIntToPtr(ConstU64(reinterpret_cast<uint64_t>(llvmVertexShader->getPointer(MangleName(variable)))), ConvertType(variable->getType()));
				const auto shaderValue = CreateLoad(shaderAddress);
				CreateStore(shaderValue, CreateGEP(outputStorage, 0, i + 1));
			}
		}

		LLVMBuildRetVoid(builder);
	}
	
private:
	const SPIRV::SPIRVModule* spirvVertexShader;
	CompiledModule* llvmVertexShader;
	const std::vector<const std::vector<VkDescriptorSetLayoutBinding>*>& layoutBindings;

	void AddOutputData(std::vector<LLVMTypeRef>& outputMembers)
	{
		for (auto i = 0u; i < spirvVertexShader->getNumVariables(); i++)
		{
			const auto variable = spirvVertexShader->getVariable(i);
			if (variable->getStorageClass() == StorageClassOutput)
			{
				auto locations = variable->getDecorate(DecorationLocation);
				if (locations.empty())
				{
					continue;
				}

				outputMembers.push_back(ConvertType(variable->getType()->getPointerElementType()));
			}
		}
	}
};

CompiledModule* CompileVertexPipeline(CPJit* jit, const SPIRV::SPIRVModule* spirvVertexShader, CompiledModule* llvmVertexShader, const std::vector<const std::vector<VkDescriptorSetLayoutBinding>*>& layoutBindings, std::function<void*(const std::string&)> getFunction)
{
	return PipelineVertexCompiledModuleBuilder(jit, spirvVertexShader, llvmVertexShader, layoutBindings, std::move(getFunction)).Compile();
}
