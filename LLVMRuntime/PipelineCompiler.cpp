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

static VkFormat GetVariableFormat(SPIRV::SPIRVType* type)
{
	if (type->isTypeVector())
	{
		if (type->getVectorComponentType()->isTypeFloat(16))
		{
			switch (type->getVectorComponentCount())
			{
			case 2:
				return VK_FORMAT_R16G16_SFLOAT;
			case 3:
				return VK_FORMAT_R16G16B16_SFLOAT;
			case 4:
				return VK_FORMAT_R16G16B16A16_SFLOAT;
			}
		}

		if (type->getVectorComponentType()->isTypeFloat(32))
		{
			switch (type->getVectorComponentCount())
			{
			case 2:
				return VK_FORMAT_R32G32_SFLOAT;
			case 3:
				return VK_FORMAT_R32G32B32_SFLOAT;
			case 4:
				return VK_FORMAT_R32G32B32A32_SFLOAT;
			}
		}

		if (type->getVectorComponentType()->isTypeFloat(64))
		{
			switch (type->getVectorComponentCount())
			{
			case 2:
				return VK_FORMAT_R64G64_SFLOAT;
			case 3:
				return VK_FORMAT_R64G64B64_SFLOAT;
			case 4:
				return VK_FORMAT_R64G64B64A64_SFLOAT;
			}
		}

		if (type->getVectorComponentType()->isTypeInt(8) && static_cast<SPIRV::SPIRVTypeInt*>(type->getVectorComponentType())->isSigned())
		{
			switch (type->getVectorComponentCount())
			{
			case 2:
				return VK_FORMAT_R8G8_SINT;
			case 3:
				return VK_FORMAT_R8G8B8_SINT;
			case 4:
				return VK_FORMAT_R8G8B8A8_SINT;
			}
		}

		if (type->getVectorComponentType()->isTypeInt(16) && static_cast<SPIRV::SPIRVTypeInt*>(type->getVectorComponentType())->isSigned())
		{
			switch (type->getVectorComponentCount())
			{
			case 2:
				return VK_FORMAT_R16G16_SINT;
			case 3:
				return VK_FORMAT_R16G16B16_SINT;
			case 4:
				return VK_FORMAT_R16G16B16A16_SINT;
			}
		}

		if (type->getVectorComponentType()->isTypeInt(32) && static_cast<SPIRV::SPIRVTypeInt*>(type->getVectorComponentType())->isSigned())
		{
			switch (type->getVectorComponentCount())
			{
			case 2:
				return VK_FORMAT_R32G32_SINT;
			case 3:
				return VK_FORMAT_R32G32B32_SINT;
			case 4:
				return VK_FORMAT_R32G32B32A32_SINT;
			}
		}

		if (type->getVectorComponentType()->isTypeInt(64) && static_cast<SPIRV::SPIRVTypeInt*>(type->getVectorComponentType())->isSigned())
		{
			switch (type->getVectorComponentCount())
			{
			case 2:
				return VK_FORMAT_R64G64_SINT;
			case 3:
				return VK_FORMAT_R64G64B64_SINT;
			case 4:
				return VK_FORMAT_R64G64B64A64_SINT;
			}
		}

		if (type->getVectorComponentType()->isTypeInt(8) && !static_cast<SPIRV::SPIRVTypeInt*>(type->getVectorComponentType())->isSigned())
		{
			switch (type->getVectorComponentCount())
			{
			case 2:
				return VK_FORMAT_R8G8_UINT;
			case 3:
				return VK_FORMAT_R8G8B8_UINT;
			case 4:
				return VK_FORMAT_R8G8B8A8_UINT;
			}
		}

		if (type->getVectorComponentType()->isTypeInt(16) && !static_cast<SPIRV::SPIRVTypeInt*>(type->getVectorComponentType())->isSigned())
		{
			switch (type->getVectorComponentCount())
			{
			case 2:
				return VK_FORMAT_R16G16_UINT;
			case 3:
				return VK_FORMAT_R16G16B16_UINT;
			case 4:
				return VK_FORMAT_R16G16B16A16_UINT;
			}
		}

		if (type->getVectorComponentType()->isTypeInt(32) && !static_cast<SPIRV::SPIRVTypeInt*>(type->getVectorComponentType())->isSigned())
		{
			switch (type->getVectorComponentCount())
			{
			case 2:
				return VK_FORMAT_R32G32_UINT;
			case 3:
				return VK_FORMAT_R32G32B32_UINT;
			case 4:
				return VK_FORMAT_R32G32B32A32_UINT;
			}
		}

		if (type->getVectorComponentType()->isTypeInt(64) && !static_cast<SPIRV::SPIRVTypeInt*>(type->getVectorComponentType())->isSigned())
		{
			switch (type->getVectorComponentCount())
			{
			case 2:
				return VK_FORMAT_R64G64_UINT;
			case 3:
				return VK_FORMAT_R64G64B64_UINT;
			case 4:
				return VK_FORMAT_R64G64B64A64_UINT;
			}
		}
	}

	if (type->isTypeFloat(16))
	{
		return VK_FORMAT_R16_SFLOAT;
	}

	if (type->isTypeFloat(32))
	{
		return VK_FORMAT_R32_SFLOAT;
	}

	if (type->isTypeFloat(64))
	{
		return VK_FORMAT_R64_SFLOAT;
	}

	if (type->isTypeInt(8) && static_cast<SPIRV::SPIRVTypeInt*>(type)->isSigned())
	{
		return VK_FORMAT_R8_SINT;
	}

	if (type->isTypeInt(16) && static_cast<SPIRV::SPIRVTypeInt*>(type)->isSigned())
	{
		return VK_FORMAT_R16_SINT;
	}

	if (type->isTypeInt(32) && static_cast<SPIRV::SPIRVTypeInt*>(type)->isSigned())
	{
		return VK_FORMAT_R32_SINT;
	}

	if (type->isTypeInt(64) && static_cast<SPIRV::SPIRVTypeInt*>(type)->isSigned())
	{
		return VK_FORMAT_R64_SINT;
	}

	if (type->isTypeInt(8) && !static_cast<SPIRV::SPIRVTypeInt*>(type)->isSigned())
	{
		return VK_FORMAT_R8_UINT;
	}

	if (type->isTypeInt(16) && !static_cast<SPIRV::SPIRVTypeInt*>(type)->isSigned())
	{
		return VK_FORMAT_R16_UINT;
	}

	if (type->isTypeInt(32) && !static_cast<SPIRV::SPIRVTypeInt*>(type)->isSigned())
	{
		return VK_FORMAT_R32_UINT;
	}

	if (type->isTypeInt(64) && !static_cast<SPIRV::SPIRVTypeInt*>(type)->isSigned())
	{
		return VK_FORMAT_R64_UINT;
	}

	FATAL_ERROR();
}

class PipelineVertexCompiledModuleBuilder final : public SPIRVBaseCompiledModuleBuilder
{
public:
	PipelineVertexCompiledModuleBuilder(CPJit* jit, const SPIRV::SPIRVModule* spirvVertexShader, CompiledModule* llvmVertexShader,
	                                    const std::vector<const std::vector<VkDescriptorSetLayoutBinding>*>& layoutBindings, 
	                                    const std::vector<VkVertexInputBindingDescription>& vertexBindingDescriptions,
	                                    const std::vector<VkVertexInputAttributeDescription>& vertexAttributeDescriptions,
	                                    std::function<void*(const std::string&)> getFunction) :
		SPIRVBaseCompiledModuleBuilder{jit, std::move(getFunction)},
		spirvVertexShader{spirvVertexShader},
		llvmVertexShader{llvmVertexShader},
		layoutBindings{layoutBindings},
		vertexBindingDescriptions{vertexBindingDescriptions},
		vertexAttributeDescriptions{vertexAttributeDescriptions}
	{
	}

protected:
	void MainCompilation() override
	{
		// TODO: Prefer globals over inttoptr of a constant address - this gives you dereferencability information. In MCJIT, use getSymbolAddress to provide actual address.

		std::vector<LLVMTypeRef> pipelineStateMembers
		{
			// uint8_t* vertexBinding[MAX_VERTEX_INPUT_BINDINGS];
			LLVMArrayType(LLVMPointerType(LLVMInt8TypeInContext(context), 0), MAX_VERTEX_INPUT_BINDINGS),
		};
		const auto pipelineStateType = StructType(pipelineStateMembers, "_PipelineState", true);
		pipelineState = GlobalVariable(LLVMPointerType(pipelineStateType, 0), LLVMExternalLinkage, "@pipelineState");
		
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
			LLVMInt32TypeInContext(context),
		};
		const auto functionType = LLVMFunctionType(LLVMVoidType(), parameters.data(), static_cast<uint32_t>(parameters.size()), false);
		const auto function = LLVMAddFunction(module, "@VertexProcessing", functionType);
		LLVMSetLinkage(function, LLVMExternalLinkage);

		const auto basicBlock = LLVMAppendBasicBlockInContext(context, function, "");
		LLVMPositionBuilderAtEnd(builder, basicBlock);

		const auto numberVertices = LLVMGetParam(function, 1);
		const auto instanceId = LLVMGetParam(function, 2);
		
		// Get shader variables
		const auto shaderBuiltinInputAddress = CreateIntToPtr(ConstU64(reinterpret_cast<uint64_t>(llvmVertexShader->getPointer("_builtinInput"))), LLVMPointerType(builtinInputType, 0));
		const auto shaderBuiltinOutputAddress = CreateIntToPtr(ConstU64(reinterpret_cast<uint64_t>(llvmVertexShader->getPointer("_builtinOutput"))), LLVMPointerType(builtinOutputType, 0));

		CreateFor(function, ConstU32(0), numberVertices, ConstU32(1), [&](LLVMValueRef i, LLVMBasicBlockRef, LLVMBasicBlockRef)
		{
			const auto rawId = CreateLoad(CreateGEP(LLVMGetParam(function, 0), std::vector<LLVMValueRef>{i, ConstU32(0)}));
			const auto vertexId = CreateLoad(CreateGEP(LLVMGetParam(function, 0), std::vector<LLVMValueRef>{i, ConstU32(1)}));
			CompileProcessVertex(rawId, vertexId, instanceId,
			                     shaderBuiltinInputAddress, shaderBuiltinOutputAddress,
			                     outputVariable, outputType);
		});

		LLVMBuildRetVoid(builder);
	}
	
private:
	const SPIRV::SPIRVModule* spirvVertexShader;
	CompiledModule* llvmVertexShader;
	const std::vector<const std::vector<VkDescriptorSetLayoutBinding>*>& layoutBindings;
	const std::vector<VkVertexInputBindingDescription>& vertexBindingDescriptions;
	const std::vector<VkVertexInputAttributeDescription>& vertexAttributeDescriptions;

	LLVMValueRef pipelineState{};

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

	const VkVertexInputAttributeDescription& FindAttribute(uint32_t location)
	{
		for (const auto& attribute : vertexAttributeDescriptions)
		{
			if (attribute.location == location)
			{
				return attribute;
			}
		}
		FATAL_ERROR();
	}
	
	const VkVertexInputBindingDescription& FindBinding(uint32_t binding)
	{
		for (const auto& bindingDescription : vertexBindingDescriptions)
		{
			if (bindingDescription.binding == binding)
			{
				return bindingDescription;
			}
		}
		FATAL_ERROR();
	}

	void CompileProcessVertex(LLVMValueRef rawId, LLVMValueRef vertexId, LLVMValueRef instanceId,
	                          LLVMValueRef shaderBuiltinInputAddress, LLVMValueRef shaderBuiltinOutputAddress, 
	                          LLVMValueRef outputVariable, LLVMTypeRef outputType)
	{
		const auto outputStride = LLVMSizeOfTypeInBits(jit->getDataLayout(), LLVMGetElementType(outputType)) / 8;
		
		// Set vertex shader builtin
		CreateStore(vertexId, CreateGEP(shaderBuiltinInputAddress, 0, 0), false);
		CreateStore(instanceId, CreateGEP(shaderBuiltinInputAddress, 0, 1), false);

		// Copy custom input to shader
		for (auto i = 0u; i < spirvVertexShader->getNumVariables(); i++)
		{
			const auto variable = spirvVertexShader->getVariable(i);
			if (variable->getStorageClass() == StorageClassInput)
			{
				auto locations = variable->getDecorate(DecorationLocation);
				if (locations.empty())
				{
					continue;
				}

				const auto location = *locations.begin();

				const auto spirvType = variable->getType()->getPointerElementType();
				const auto llvmType = ConvertType(spirvType);

				const auto shaderAddress = CreateIntToPtr(ConstU64(reinterpret_cast<uint64_t>(llvmVertexShader->getPointer(MangleName(variable)))), ConvertType(variable->getType()));
				
				if (spirvType->isTypeArray() || spirvType->isTypeMatrix())
				{
					//const auto format = GetVariableFormat(input.type->getMatrixColumnType());
					//const auto vectorStride = GetVariableSize(input.type->getMatrixColumnType());
					//for (auto i = 0u; i < input.type->getMatrixColumnCount(); i++)
					//{
					//	const auto& attribute = FindAttribute(input.location + i, vertexInputState);
					//	const auto& binding = FindBinding(attribute.binding, vertexInputState);
					//	uint64_t offset;
					//	if (binding.inputRate == VK_VERTEX_INPUT_RATE_VERTEX)
					//	{
					//		offset = vertexBindingOffset[binding.binding] + static_cast<uint64_t>(binding.stride)* vertex + attribute.offset;
					//	}
					//	else
					//	{
					//		offset = vertexBindingOffset[binding.binding] + static_cast<uint64_t>(binding.stride)* instance + attribute.offset;
					//	}
					//	const auto size = GetFormatInformation(attribute.format).TotalSize;
					//	const auto pointer = static_cast<uint8_t*>(input.pointer) + i * vectorStride;
					//	if (format != attribute.format)
					//	{
					//		CopyFormatConversion(deviceState, pointer, vertexBinding[binding.binding]->getDataPtr(offset, size), format, attribute.format);
					//	}
					//	else
					//	{
					//		memcpy(pointer, vertexBinding[binding.binding]->getDataPtr(offset, size), size);
					//	}
					//}
					TODO_ERROR();
				}
				else
				{
					const auto& attribute = FindAttribute(location);
					const auto& binding = FindBinding(attribute.binding);

					// offset = static_cast<uint64_t>(binding.stride) * (vertex or instance) + attribute.offset;
					const auto index = CreateZExt(binding.inputRate == VK_VERTEX_INPUT_RATE_VERTEX ? vertexId : instanceId, LLVMInt64TypeInContext(context));
					auto offset = CreateMul(ConstU64(binding.stride), index);
					offset = CreateAdd(offset, ConstU64(attribute.offset));

					// const auto size = LLVMSizeOfTypeInBits(jit->getDataLayout(), llvmType) / 8;
					const auto inputFormat = GetVariableFormat(spirvType);

					auto bufferData = CreateLoad(CreateGEP(CreateLoad(pipelineState), {0, 0, binding.binding}));
					bufferData = CreateBitCast(CreateGEP(bufferData, {offset}), LLVMPointerType(llvmType, 0));
					bufferData = CreateLoad(bufferData);
					
					if (inputFormat != attribute.format)
					{
						// CopyFormatConversion(deviceState, input.pointer, vertexBinding[binding.binding]->getDataPtr(offset, size), input.format, attribute.format);
						TODO_ERROR();
					}
					else
					{
						CreateStore(bufferData, shaderAddress);
					}
				}
			}
		}

		// Call the vertex shader
		const auto vertexShaderEntry = LLVMAddFunction(module, "!VertexShader", LLVMFunctionType(LLVMVoidType(), nullptr, 0, false));
		LLVMSetLinkage(vertexShaderEntry, LLVMExternalLinkage);
		LLVMBuildCall(builder, vertexShaderEntry, nullptr, 0, "");

		// TODO: Modify vertex shader to use pointers directly instead of copying

		// Copy vertex builtin output to storage
		const auto shaderBuiltinOutput = CreateLoad(shaderBuiltinOutputAddress);
		const auto outputStorage = CreateBitCast(CreateGEP(CreateLoad(outputVariable), {LLVMBuildMul(builder, rawId, ConstU32(outputStride), "")}), outputType);
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
	}

	void CreateFor(LLVMValueRef currentFunction, LLVMValueRef initialiser, LLVMValueRef comparison, LLVMValueRef increment, 
	               std::function<void(LLVMValueRef i, LLVMBasicBlockRef continueBlock, LLVMBasicBlockRef breakBlock)> forFunction)
	{
		const auto comparisonBlock = LLVMAppendBasicBlock(currentFunction, "for-comparison");
		const auto bodyBlock = LLVMAppendBasicBlock(currentFunction, "for-body");
		const auto incrementBlock = LLVMAppendBasicBlock(currentFunction, "for-increment");
		const auto endBlock = LLVMAppendBasicBlock(currentFunction, "for-end");

		// auto i = initialiser
		const auto index = CreateAlloca(LLVMTypeOf(initialiser), "for-index");
		CreateStore(initialiser, index);
		CreateBr(comparisonBlock);

		// break if i >= comparison
		LLVMPositionBuilderAtEnd(builder, comparisonBlock);
		const auto reachedEnd = CreateICmpUGE(CreateLoad(index), comparison);
		CreateCondBr(reachedEnd, endBlock, bodyBlock);

		// main body
		LLVMPositionBuilderAtEnd(builder, bodyBlock);
		forFunction(CreateLoad(index), incrementBlock, endBlock);
		CreateBr(incrementBlock);

		// index += increment
		LLVMPositionBuilderAtEnd(builder, incrementBlock);
		CreateStore(CreateAdd(CreateLoad(index), increment), index);
		CreateBr(comparisonBlock);

		LLVMPositionBuilderAtEnd(builder, endBlock);
	}
};

CompiledModule* CompileVertexPipeline(CPJit* jit, const SPIRV::SPIRVModule* spirvVertexShader, CompiledModule* llvmVertexShader,
                                      const std::vector<const std::vector<VkDescriptorSetLayoutBinding>*>& layoutBindings,
                                      const std::vector<VkVertexInputBindingDescription>& vertexBindingDescriptions,
                                      const std::vector<VkVertexInputAttributeDescription>& vertexAttributeDescriptions,
                                      std::function<void*(const std::string&)> getFunction)
{
	return PipelineVertexCompiledModuleBuilder(jit, spirvVertexShader, llvmVertexShader, 
	                                           layoutBindings, 
	                                           vertexBindingDescriptions,
	                                           vertexAttributeDescriptions,
	                                           std::move(getFunction)).Compile();
}
