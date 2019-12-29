#include "PipelineCompiler.h"

#include "CompiledModule.h"
#include "CompiledModuleBuilder.h"
#include "Compilers.h"
#include "ImageCompiler.h"
#include "SPIRVCompiler.h"

#include <Formats.h>

#include <SPIRVDecorate.h>
#include <SPIRVInstruction.h>
#include <SPIRVType.h>

#include <llvm-c/Target.h>

static VkFormat GetVariableFormat(const SPIRV::SPIRVType* type)
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

	if (type->isTypeInt(8) && static_cast<const SPIRV::SPIRVTypeInt*>(type)->isSigned())
	{
		return VK_FORMAT_R8_SINT;
	}

	if (type->isTypeInt(16) && static_cast<const SPIRV::SPIRVTypeInt*>(type)->isSigned())
	{
		return VK_FORMAT_R16_SINT;
	}

	if (type->isTypeInt(32) && static_cast<const SPIRV::SPIRVTypeInt*>(type)->isSigned())
	{
		return VK_FORMAT_R32_SINT;
	}

	if (type->isTypeInt(64) && static_cast<const SPIRV::SPIRVTypeInt*>(type)->isSigned())
	{
		return VK_FORMAT_R64_SINT;
	}

	if (type->isTypeInt(8) && !static_cast<const SPIRV::SPIRVTypeInt*>(type)->isSigned())
	{
		return VK_FORMAT_R8_UINT;
	}

	if (type->isTypeInt(16) && !static_cast<const SPIRV::SPIRVTypeInt*>(type)->isSigned())
	{
		return VK_FORMAT_R16_UINT;
	}

	if (type->isTypeInt(32) && !static_cast<const SPIRV::SPIRVTypeInt*>(type)->isSigned())
	{
		return VK_FORMAT_R32_UINT;
	}

	if (type->isTypeInt(64) && !static_cast<const SPIRV::SPIRVTypeInt*>(type)->isSigned())
	{
		return VK_FORMAT_R64_UINT;
	}

	FATAL_ERROR();
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
		return GetVariableSize(type->getScalarType()) * type->getVectorComponentCount();
	}

	if (type->isTypeFloat() || type->isTypeInt())
	{
		return type->getBitWidth() / 8;
	}

	FATAL_ERROR();
}

class BasePipelineCompiledModuleBuilder : public CompiledModuleBuilder
{
public:
	BasePipelineCompiledModuleBuilder(const SPIRV::SPIRVModule* shader,
	                                  const SPIRV::SPIRVFunction* entryPoint,
	                                  const VkSpecializationInfo* specializationInfo,
	                                  const GraphicsPipelineStateStorage* state) :
		shader{shader},
		entryPoint{entryPoint},
		specializationInfo{specializationInfo},
		state{state}
	{
	}
	
	~BasePipelineCompiledModuleBuilder() override = default;

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

	void CreateIf(LLVMValueRef currentFunction, LLVMValueRef comparision, std::function<void(LLVMBasicBlockRef endBlock)> trueFunction, std::function<void(LLVMBasicBlockRef endBlock)> falseFunction)
	{
		assert(trueFunction || falseFunction);
		
		const auto trueBlock = trueFunction ? LLVMAppendBasicBlock(currentFunction, "if-true") : nullptr;
		const auto falseBlock = falseFunction ? LLVMCreateBasicBlockInContext(context, "if-false") : nullptr;
		const auto endBlock = LLVMCreateBasicBlockInContext(context, "if-end");

		// Perform conditional branch
		if (!trueBlock)
		{
			CreateCondBr(comparision, endBlock, falseBlock);
		}
		else if (!falseBlock)
		{
			CreateCondBr(comparision, trueBlock, endBlock);
		}
		else
		{
			CreateCondBr(comparision, trueBlock, falseBlock);
		}

		if (trueBlock)
		{
			// true body
			LLVMPositionBuilderAtEnd(builder, trueBlock);
			trueFunction(endBlock);
			CreateBr(endBlock);
		}

		if (falseBlock)
		{
			// false body
			LLVMAppendExistingBasicBlock(currentFunction, falseBlock);
			LLVMPositionBuilderAtEnd(builder, falseBlock);
			falseFunction(endBlock);
			CreateBr(endBlock);
		}

		LLVMAppendExistingBasicBlock(currentFunction, endBlock);
		LLVMPositionBuilderAtEnd(builder, endBlock);
	}

	LLVMValueRef CreatePhiIf(LLVMValueRef currentFunction, LLVMValueRef comparision, 
	                         const std::function<LLVMValueRef()>& trueFunction,
	                         const std::function<LLVMValueRef()>& falseFunction)
	{
		assert(trueFunction && falseFunction);
		
		auto trueBlock = LLVMAppendBasicBlock(currentFunction, "if-true");
		auto falseBlock = LLVMAppendBasicBlock(currentFunction, "if-false");
		const auto endBlock = LLVMAppendBasicBlock(currentFunction, "if-end");

		// Perform conditional branch
		CreateCondBr(comparision, trueBlock, falseBlock);
		
		// true body
		LLVMPositionBuilderAtEnd(builder, trueBlock);
		const auto trueResult = trueFunction();
		trueBlock = LLVMGetInsertBlock(builder);
		CreateBr(endBlock);

		// false body
		LLVMPositionBuilderAtEnd(builder, falseBlock);
		const auto falseResult = falseFunction();
		falseBlock = LLVMGetInsertBlock(builder);
		CreateBr(endBlock);

		LLVMPositionBuilderAtEnd(builder, endBlock);
		const auto result = CreatePhi(LLVMTypeOf(trueResult));
		LLVMValueRef incomingValues[2]
		{
			trueResult,
			falseResult
		};
		LLVMBasicBlockRef incomingBlocks[2]
		{
			trueBlock,
			falseBlock
		};
		LLVMAddIncoming(result, incomingValues, incomingBlocks, 2);
		return result;
	}

	template<int size>
	std::array<LLVMValueRef, size> CreatePhiIf(LLVMValueRef currentFunction, LLVMValueRef comparision, 
	                                           const std::function<std::array<LLVMValueRef, size>()>& trueFunction,
	                                           const std::function<std::array<LLVMValueRef, size>()>& falseFunction)
	{
		assert(trueFunction && falseFunction);
		
		auto trueBlock = LLVMAppendBasicBlock(currentFunction, "if-true");
		auto falseBlock = LLVMAppendBasicBlock(currentFunction, "if-false");
		const auto endBlock = LLVMAppendBasicBlock(currentFunction, "if-end");

		// Perform conditional branch
		CreateCondBr(comparision, trueBlock, falseBlock);
		
		// true body
		LLVMPositionBuilderAtEnd(builder, trueBlock);
		const auto trueResult = trueFunction();
		trueBlock = LLVMGetInsertBlock(builder);
		CreateBr(endBlock);

		// false body
		LLVMPositionBuilderAtEnd(builder, falseBlock);
		const auto falseResult = falseFunction();
		falseBlock = LLVMGetInsertBlock(builder);
		CreateBr(endBlock);

		LLVMPositionBuilderAtEnd(builder, endBlock);
		std::array<LLVMValueRef, size> results;
		for (auto i = 0; i < size; i++)
		{
			results[i] = CreatePhi(LLVMTypeOf(trueResult[i]));
			LLVMValueRef incomingValues[2]
			{
				trueResult[i],
				falseResult[i]
			};
			LLVMBasicBlockRef incomingBlocks[2]
			{
				trueBlock,
				falseBlock
			};
			LLVMAddIncoming(results[i], incomingValues, incomingBlocks, 2);
		}
		return results;
	}

protected:
	const SPIRV::SPIRVModule* shader;
	const SPIRV::SPIRVFunction* entryPoint;
	const VkSpecializationInfo* specializationInfo;
	const GraphicsPipelineStateStorage* state;

	std::unique_ptr<SPIRVCompiledModuleBuilder> shaderModuleBuilder{};
	LLVMValueRef shaderEntryPoint{};
	LLVMValueRef pipelineState{};
	
	void CompileShader(ExecutionModel executionModel)
	{
		//TODO: share structs
		shaderModuleBuilder = std::make_unique<SPIRVCompiledModuleBuilder>(shader, executionModel, entryPoint, specializationInfo);
		shaderModuleBuilder->Initialise(jit, context, module);
		shaderEntryPoint = shaderModuleBuilder->CompileMainFunction();
	}

	void CreatePipelineState()
	{
		std::vector<LLVMTypeRef> pipelineStateMembers
		{
			// uint8_t* vertexBindingPtr[MAX_VERTEX_INPUT_BINDINGS];
			LLVMArrayType(LLVMPointerType(LLVMInt8TypeInContext(context), 0), MAX_VERTEX_INPUT_BINDINGS),
			// ImageView* imageAttachment[MAX_FRAGMENT_OUTPUT_ATTACHMENTS];
			LLVMArrayType(LLVMPointerType(LLVMInt8TypeInContext(context), 0), MAX_FRAGMENT_OUTPUT_ATTACHMENTS),
			// ImageView* depthStencilAttachment;
			LLVMPointerType(LLVMInt8TypeInContext(context), 0),
		};
		const auto pipelineStateType = StructType(pipelineStateMembers, "_PipelineState", true);
		pipelineState = GlobalVariable(LLVMPointerType(pipelineStateType, 0), LLVMExternalLinkage, "@pipelineState");
	}
};

class PipelineVertexCompiledModuleBuilder final : public BasePipelineCompiledModuleBuilder
{
public:
	PipelineVertexCompiledModuleBuilder(const SPIRV::SPIRVModule* vertexShader,
	                                    const SPIRV::SPIRVFunction* entryPoint,
	                                    const VkSpecializationInfo* specializationInfo,
	                                    const GraphicsPipelineStateStorage* state) :
		BasePipelineCompiledModuleBuilder{vertexShader, entryPoint, specializationInfo, state}
	{
	}

	~PipelineVertexCompiledModuleBuilder() override = default;

protected:
	LLVMValueRef CompileMainFunctionImpl() override
	{
		CompileShader(ExecutionModelVertex);
		CreatePipelineState();
		
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
		
		std::array<LLVMTypeRef, 3> parameters
		{
			LLVMPointerType(assemblerOutputType, 0),
			LLVMInt32TypeInContext(context),
			LLVMInt32TypeInContext(context),
		};
		const auto functionType = LLVMFunctionType(LLVMVoidType(), parameters.data(), static_cast<uint32_t>(parameters.size()), false);
		const auto function = LLVMAddFunction(module, "@main", functionType);
		LLVMSetLinkage(function, LLVMExternalLinkage);

		const auto basicBlock = LLVMAppendBasicBlockInContext(context, function, "");
		LLVMPositionBuilderAtEnd(builder, basicBlock);

		const auto numberVertices = LLVMGetParam(function, 1);
		const auto instanceId = LLVMGetParam(function, 2);
		
		// Get shader variables
		const auto shaderBuiltinInputAddress = CreateBitCast(shaderModuleBuilder->getBuiltinInput(), LLVMPointerType(builtinInputType, 0));
		const auto shaderBuiltinOutputAddress = CreateBitCast(shaderModuleBuilder->getBuiltinOutput(), LLVMPointerType(builtinOutputType, 0));

		CreateFor(function, ConstU32(0), numberVertices, ConstU32(1), [&](LLVMValueRef i, LLVMBasicBlockRef, LLVMBasicBlockRef)
		{
			const auto rawId = CreateLoad(CreateGEP(LLVMGetParam(function, 0), std::vector<LLVMValueRef>{i, ConstU32(0)}));
			const auto vertexId = CreateLoad(CreateGEP(LLVMGetParam(function, 0), std::vector<LLVMValueRef>{i, ConstU32(1)}));
			CompileProcessVertex(rawId, vertexId, instanceId,
			                     shaderBuiltinInputAddress, shaderBuiltinOutputAddress,
			                     outputVariable, outputType);
		});

		LLVMBuildRetVoid(builder);

		return function;
	}
	
private:
	void AddOutputData(std::vector<LLVMTypeRef>& outputMembers)
	{
		for (auto i = 0u; i < shader->getNumVariables(); i++)
		{
			const auto variable = shader->getVariable(i);
			if (variable->getStorageClass() == StorageClassOutput)
			{
				auto locations = variable->getDecorate(DecorationLocation);
				if (locations.empty())
				{
					continue;
				}

				outputMembers.push_back(shaderModuleBuilder->ConvertType(variable->getType()->getPointerElementType()));
			}
		}
	}

	const VkVertexInputAttributeDescription& FindAttribute(uint32_t location)
	{
		for (const auto& attribute : state->getVertexInputState().VertexAttributeDescriptions)
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
		for (const auto& bindingDescription : state->getVertexInputState().VertexBindingDescriptions)
		{
			if (bindingDescription.binding == binding)
			{
				return bindingDescription;
			}
		}
		FATAL_ERROR();
	}

	LLVMTypeRef GetTypeFromFormat(VkFormat format)
	{
		switch (format)
		{
		case VK_FORMAT_R8_UINT:
		case VK_FORMAT_R8_SINT:
			return LLVMInt8TypeInContext(context);
		case VK_FORMAT_R8G8_UINT:
		case VK_FORMAT_R8G8_SINT:
			return LLVMVectorType(LLVMInt8TypeInContext(context), 2);
		case VK_FORMAT_R8G8B8_UINT:
		case VK_FORMAT_R8G8B8_SINT:
			return LLVMVectorType(LLVMInt8TypeInContext(context), 3);
		case VK_FORMAT_R8G8B8A8_UINT:
		case VK_FORMAT_R8G8B8A8_SINT:
			return LLVMVectorType(LLVMInt8TypeInContext(context), 4);

		case VK_FORMAT_R16_UINT:
		case VK_FORMAT_R16_SINT:
			return LLVMInt16TypeInContext(context);
		case VK_FORMAT_R16_SFLOAT:
			return LLVMHalfTypeInContext(context);
		case VK_FORMAT_R16G16_UINT:
		case VK_FORMAT_R16G16_SINT:
			return LLVMVectorType(LLVMInt16TypeInContext(context), 2);
		case VK_FORMAT_R16G16_SFLOAT:
			return LLVMVectorType(LLVMHalfTypeInContext(context), 2);
		case VK_FORMAT_R16G16B16_UINT:
		case VK_FORMAT_R16G16B16_SINT:
			return LLVMVectorType(LLVMInt16TypeInContext(context), 3);
		case VK_FORMAT_R16G16B16_SFLOAT:
			return LLVMVectorType(LLVMHalfTypeInContext(context), 3);
		case VK_FORMAT_R16G16B16A16_UINT:
		case VK_FORMAT_R16G16B16A16_SINT:
			return LLVMVectorType(LLVMInt16TypeInContext(context), 4);
		case VK_FORMAT_R16G16B16A16_SFLOAT:
			return LLVMVectorType(LLVMHalfTypeInContext(context), 4);
			
		case VK_FORMAT_R32_UINT:
		case VK_FORMAT_R32_SINT:
			return LLVMInt32TypeInContext(context);
		case VK_FORMAT_R32_SFLOAT:
			return LLVMFloatTypeInContext(context);
		case VK_FORMAT_R32G32_UINT:
		case VK_FORMAT_R32G32_SINT:
			return LLVMVectorType(LLVMInt32TypeInContext(context), 2);
		case VK_FORMAT_R32G32_SFLOAT:
			return LLVMVectorType(LLVMFloatTypeInContext(context), 2);
		case VK_FORMAT_R32G32B32_UINT:
		case VK_FORMAT_R32G32B32_SINT:
			return LLVMVectorType(LLVMInt32TypeInContext(context), 3);
		case VK_FORMAT_R32G32B32_SFLOAT:
			return LLVMVectorType(LLVMFloatTypeInContext(context), 3);
		case VK_FORMAT_R32G32B32A32_UINT:
		case VK_FORMAT_R32G32B32A32_SINT:
			return LLVMVectorType(LLVMInt32TypeInContext(context), 4);
		case VK_FORMAT_R32G32B32A32_SFLOAT:
			return LLVMVectorType(LLVMFloatTypeInContext(context), 4);
			
		case VK_FORMAT_R64_UINT:
		case VK_FORMAT_R64_SINT:
			return LLVMInt64TypeInContext(context);
		case VK_FORMAT_R64_SFLOAT:
			return LLVMDoubleTypeInContext(context);
		case VK_FORMAT_R64G64_UINT:
		case VK_FORMAT_R64G64_SINT:
			return LLVMVectorType(LLVMInt64TypeInContext(context), 2);
		case VK_FORMAT_R64G64_SFLOAT:
			return LLVMVectorType(LLVMDoubleTypeInContext(context), 2);
		case VK_FORMAT_R64G64B64_UINT:
		case VK_FORMAT_R64G64B64_SINT:
			return LLVMVectorType(LLVMInt64TypeInContext(context), 3);
		case VK_FORMAT_R64G64B64_SFLOAT:
			return LLVMVectorType(LLVMDoubleTypeInContext(context), 3);
		case VK_FORMAT_R64G64B64A64_UINT:
		case VK_FORMAT_R64G64B64A64_SINT:
			return LLVMVectorType(LLVMInt64TypeInContext(context), 4);
		case VK_FORMAT_R64G64B64A64_SFLOAT:
			return LLVMVectorType(LLVMDoubleTypeInContext(context), 4);
			 
		default:
			return nullptr;
		}
	}

	LLVMValueRef EmitVectorConversion(LLVMValueRef value, LLVMTypeRef inputType, LLVMTypeRef targetType, bool isTargetSigned)
	{
		if (LLVMGetTypeKind(targetType) != LLVMVectorTypeKind)
		{
			FATAL_ERROR();
		}

		if (LLVMGetVectorSize(targetType) != LLVMGetVectorSize(inputType))
		{
			FATAL_ERROR();
		}

		switch (LLVMGetTypeKind(LLVMGetElementType(inputType)))
		{
		case LLVMHalfTypeKind:
		case LLVMFloatTypeKind:
		case LLVMDoubleTypeKind:
			switch (LLVMGetTypeKind(LLVMGetElementType(targetType)))
			{
			case LLVMHalfTypeKind:
			case LLVMFloatTypeKind:
			case LLVMDoubleTypeKind:
				return CreateFPExtOrTrunc(value, targetType);

			case LLVMIntegerTypeKind:
				TODO_ERROR();

			default:
				FATAL_ERROR();
			}

		case LLVMIntegerTypeKind:
			switch (LLVMGetTypeKind(LLVMGetElementType(targetType)))
			{
			case LLVMHalfTypeKind:
			case LLVMFloatTypeKind:
			case LLVMDoubleTypeKind:
				TODO_ERROR();

			case LLVMIntegerTypeKind:
				if (isTargetSigned)
				{
					return CreateSExtOrTrunc(value, targetType);
				}
				return CreateZExtOrTrunc(value, targetType);

			default:
				FATAL_ERROR();
			}

		default:
			FATAL_ERROR();
		}
	}

	LLVMValueRef EmitConversion(LLVMValueRef value, LLVMTypeRef inputType, LLVMTypeRef targetType, bool isTargetSigned)
	{
		switch (LLVMGetTypeKind(inputType))
		{
		case LLVMHalfTypeKind:
		case LLVMFloatTypeKind:
		case LLVMDoubleTypeKind:
			switch (LLVMGetTypeKind(targetType))
			{
			case LLVMHalfTypeKind:
			case LLVMFloatTypeKind:
			case LLVMDoubleTypeKind:
				return CreateFPExtOrTrunc(value, targetType);

			case LLVMIntegerTypeKind:
				TODO_ERROR();

			case LLVMVectorTypeKind:
				TODO_ERROR();

			default:
				FATAL_ERROR();
			}
			
		case LLVMIntegerTypeKind:
			switch (LLVMGetTypeKind(targetType))
			{
			case LLVMHalfTypeKind:
			case LLVMFloatTypeKind:
			case LLVMDoubleTypeKind:
				TODO_ERROR();

			case LLVMIntegerTypeKind:
				if (isTargetSigned)
				{
					return CreateSExtOrTrunc(value, targetType);
				}
				return CreateZExtOrTrunc(value, targetType);

			case LLVMVectorTypeKind:
				TODO_ERROR();

			default:
				FATAL_ERROR();
			}
			
		case LLVMVectorTypeKind:
			return EmitVectorConversion(value, inputType, targetType, isTargetSigned);
			
		default:
			FATAL_ERROR();
		}
	}

	void EmitCopyInput(uint32_t location, LLVMValueRef vertexId, LLVMValueRef instanceId, LLVMValueRef shaderAddress, const SPIRV::SPIRVType* spirvType)
	{
		const auto llvmType = shaderModuleBuilder->ConvertType(spirvType);
		const auto& attribute = FindAttribute(location);
		const auto& binding = FindBinding(attribute.binding);

		// offset = static_cast<uint64_t>(binding.stride) * (vertex or instance) + attribute.offset;
		const auto bufferIndex = CreateZExt(binding.inputRate == VK_VERTEX_INPUT_RATE_VERTEX ? vertexId : instanceId, LLVMInt64TypeInContext(context));
		auto bufferOffset = CreateMul(ConstU64(binding.stride), bufferIndex);
		bufferOffset = CreateAdd(bufferOffset, ConstU64(attribute.offset));

		const auto shaderFormat = GetVariableFormat(spirvType);

		auto bufferData = CreateLoad(CreateGEP(CreateLoad(pipelineState), {0, 0, binding.binding}));
		bufferData = CreateGEP(bufferData, {bufferOffset});

		if (shaderFormat == attribute.format)
		{
			// Handle straight copy when formats are identical
			bufferData = CreateBitCast(bufferData, LLVMPointerType(llvmType, 0));
			bufferData = CreateLoad(bufferData);
		}
		else
		{
			const auto attributeType = GetTypeFromFormat(attribute.format);
			if (attributeType != nullptr)
			{
				// Handle simple conversion when formats map to llvm type
				bufferData = CreateBitCast(bufferData, LLVMPointerType(attributeType, 0));
				bufferData = CreateLoad(bufferData);
				bufferData = EmitConversion(bufferData, attributeType, llvmType, GetFormatInformation(shaderFormat).Base == BaseType::SInt);
			}
			else
			{
				// Handle complicated loading, such as packed formats
				const auto shaderType = LLVMGetElementType(LLVMTypeOf(shaderAddress));
				LLVMTypeRef vectorType;
				if (LLVMGetTypeKind(shaderType) == LLVMVectorTypeKind)
				{
					if (LLVMGetVectorSize(shaderType) == 4)
					{
						vectorType = shaderType;
					}
					else
					{
						vectorType = LLVMVectorType(LLVMGetElementType(shaderType), 4);
					}
				}
				else
				{
					vectorType = LLVMVectorType(shaderType, 4);
				}

				bufferData = EmitGetPixel(this, bufferData, vectorType, &GetFormatInformation(attribute.format));

				if (LLVMGetTypeKind(shaderType) == LLVMVectorTypeKind)
				{
					if (vectorType != shaderType)
					{
						auto vector = LLVMGetUndef(shaderType);
						for (auto j = 0u; j < LLVMGetVectorSize(shaderType); j++)
						{
							const auto vectorIndex = ConstI32(j);
							vector = CreateInsertElement(vector, CreateExtractElement(bufferData, vectorIndex), vectorIndex);
						}
						bufferData = vector;
					}
				}
				else
				{
					bufferData = CreateExtractElement(bufferData, ConstI32(0));
				}
			}
		}
		CreateStore(bufferData, shaderAddress);
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
		for (auto i = 0u; i < shader->getNumVariables(); i++)
		{
			const auto variable = shader->getVariable(i);
			if (variable->getStorageClass() == StorageClassInput)
			{
				auto locations = variable->getDecorate(DecorationLocation);
				if (locations.empty())
				{
					continue;
				}

				const auto location = *locations.begin();
				const auto spirvType = variable->getType()->getPointerElementType();
				const auto shaderVariable = shaderModuleBuilder->ConvertValue(variable, nullptr);
				
				if (spirvType->isTypeArray())
				{
					const auto llvmType = shaderModuleBuilder->ConvertType(spirvType);
					const auto elementSize = GetVariableSize(spirvType->getArrayElementType());
					const auto multiplier = elementSize > 16 ? 2 : 1;
					for (auto j = 0u; j < LLVMGetArrayLength(llvmType); j++)
					{
						const auto columnAddress = CreateGEP(shaderVariable, 0, j);
						EmitCopyInput(location + j * multiplier, vertexId, instanceId, columnAddress, spirvType->getArrayElementType());
					}
				}
				else if (spirvType->isTypeMatrix())
				{
					const auto elementSize = GetVariableSize(spirvType->getMatrixColumnType());
					const auto multiplier = elementSize > 16 ? 2 : 1;
					for (auto j = 0u; j < spirvType->getMatrixColumnCount(); j++)
					{
						const auto columnAddress = CreateGEP(shaderVariable, 0, j);
						EmitCopyInput(location + j * multiplier, vertexId, instanceId, columnAddress, spirvType->getMatrixColumnType());
					}
				}
				else
				{
					EmitCopyInput(location, vertexId, instanceId, shaderVariable, spirvType);
				}
			}
		}

		// Call the vertex shader
		LLVMBuildCall(builder, shaderEntryPoint, nullptr, 0, "");

		// TODO: Modify vertex shader to use pointers directly instead of copying

		// Copy vertex builtin output to storage
		const auto shaderBuiltinOutput = CreateLoad(shaderBuiltinOutputAddress);
		const auto outputStorage = CreateBitCast(CreateGEP(CreateLoad(outputVariable), {CreateMul(rawId, ConstU32(static_cast<uint32_t>(outputStride)))}), outputType);
		CreateStore(shaderBuiltinOutput, CreateGEP(outputStorage, 0, 0));

		// Copy custom output to storage
		for (auto i = 0u, j = 0u; i < shader->getNumVariables(); i++)
		{
			const auto variable = shader->getVariable(i);
			if (variable->getStorageClass() == StorageClassOutput)
			{
				// TODO: use location?
				auto locations = variable->getDecorate(DecorationLocation);
				if (locations.empty())
				{
					continue;
				}

				const auto shaderVariable = shaderModuleBuilder->ConvertValue(variable, nullptr);
				const auto shaderValue = CreateLoad(shaderVariable);
				CreateStore(shaderValue, CreateGEP(outputStorage, 0, j + 1));
				j++;
			}
		}
	}
};

class PipelineFragmentCompiledModuleBuilder final : public BasePipelineCompiledModuleBuilder
{
public:
	PipelineFragmentCompiledModuleBuilder(const SPIRV::SPIRVModule* fragmentShader,
	                                      const SPIRV::SPIRVFunction* entryPoint,
	                                      const VkSpecializationInfo* specializationInfo,
	                                      const GraphicsPipelineStateStorage* state) :
		BasePipelineCompiledModuleBuilder{fragmentShader, entryPoint, specializationInfo, state}
	{
	}

	~PipelineFragmentCompiledModuleBuilder() override = default;

protected:
	LLVMValueRef mainFunction{};
	LLVMValueRef depth{};
	LLVMValueRef x{};
	LLVMValueRef y{};
	LLVMValueRef depthResult{};
	LLVMValueRef stencilResult{};

	LLVMValueRef currentDepth{};
	
	LLVMValueRef CompileMainFunctionImpl() override
	{
		CompileShader(ExecutionModelFragment);
		CreatePipelineState();

		std::array<LLVMTypeRef, 3> parameters
		{
			LLVMFloatTypeInContext(context),
			LLVMInt32TypeInContext(context),
			LLVMInt32TypeInContext(context),
		};
		const auto functionType = LLVMFunctionType(LLVMVoidTypeInContext(context), parameters.data(), static_cast<uint32_t>(parameters.size()), false);
		mainFunction = LLVMAddFunction(module, "@main", functionType);
		LLVMSetLinkage(mainFunction, LLVMExternalLinkage);

		depth = LLVMGetParam(mainFunction, 0);
		x = LLVMGetParam(mainFunction, 1);
		y = LLVMGetParam(mainFunction, 2);

		const auto basicBlock = LLVMAppendBasicBlockInContext(context, mainFunction, "");
		LLVMPositionBuilderAtEnd(builder, basicBlock);

		// TODO: 27.2. Discard Rectangles Test
		// TODO: 27.3. Scissor Test
		// TODO: 27.4. Exclusive Scissor Test
		// TODO: 27.5. Sample Mask

		// TODO: If early per-fragment operations are enabled by the fragment shader, these operations are also performed:
		// TODO: 27.11. Depth Bounds Test
		// TODO: 27.12. Stencil Test
		// TODO: 27.13. Depth Test
		// TODO: 27.14. Representative Fragment Test
		// TODO: 27.15. Sample Counting

		// Call the shader
		const auto shaderResult = CreateCall(shaderEntryPoint, {});

		CreateIf(mainFunction, shaderResult, nullptr, [&](LLVMBasicBlockRef endFragmentBlock)
		{
			// TODO: 27.8. Mixed attachment samples
			// TODO: 27.9. Multisample Coverage
			// TODO: 27.10. Depth and Stencil Operations

			CompileDepthBoundsTest(endFragmentBlock);
			CompileStencilTest();
			CompileDepthTest();
			CompileDepthStencilWrite();
			// TODO: 27.14. Representative Fragment Test
			// TODO: 27.15. Sample Counting
			// TODO: 27.16. Fragment Coverage To Color
			// TODO: 27.17. Coverage Reduction
			CompileWriteFragment();
		});

		CreateRetVoid();

		return mainFunction;
	}

	void CompileDepthBoundsTest(LLVMBasicBlockRef endFragmentBlock)
	{
		// 27.11. Depth Bounds Test
		if (state->getDepthStencilState().DepthBoundsTestEnable && state->getSubpass().depthStencilAttachment.attachment != VK_ATTACHMENT_UNUSED)
		{
			const auto startBlock = LLVMAppendBasicBlockInContext(context, mainFunction, "start-depth-bounds-test");

			CreateBr(startBlock);
			LLVMPositionBuilderAtEnd(builder, startBlock);
			
			const auto depthStencilAttachment = CreateLoad(CreateGEP(CreateLoad(pipelineState), {0, 2}));
			const auto hasDepth = CreateICmpNE(depthStencilAttachment, LLVMConstNull(LLVMTypeOf(depthStencilAttachment)));
			CreateIf(mainFunction, hasDepth, [&](LLVMBasicBlockRef endBlock)
			{
				const auto currentDepth = GetCurrentDepth(depthStencilAttachment);

				LLVMValueRef minDepthBounds;
				LLVMValueRef maxDepthBounds;
				
				if (state->getDynamicState().DynamicDepthBounds)
				{
					//if (currentDepth < deviceState->graphicsPipelineState.dynamicState.minDepthBounds ||
					//	currentDepth > deviceState->graphicsPipelineState.dynamicState.maxDepthBounds)
					//{
					//	return;
					//}
					TODO_ERROR();
				}
				else
				{
					minDepthBounds = ConstF32(state->getDepthStencilState().MinDepthBounds);
					maxDepthBounds = ConstF32(state->getDepthStencilState().MaxDepthBounds);
				}

				const auto isDepthLess = CreateFCmpULT(currentDepth, minDepthBounds, "depth-test-less");
				const auto isDepthGreater = CreateFCmpUGT(currentDepth, maxDepthBounds, "depth-test-greater");
				const auto isDepthOutOfBounds = CreateOr(isDepthLess, isDepthGreater, "depth-test-out-of-bounds");
				CreateCondBr(isDepthOutOfBounds, endFragmentBlock, endBlock);
			}, nullptr);
		}
	}

	void CompileStencilTest()
	{
		// 27.12. Stencil Test
		if (state->getDepthStencilState().StencilTestEnable)
		{
			// auto stencilResult = true;
			// auto& stencilOpState = front
			// 	                       ? deviceState->graphicsPipelineState.pipeline->getDepthStencilState().Front
			// 	                       : deviceState->graphicsPipelineState.pipeline->getDepthStencilState().Back;
			//
			// uint8_t stencilReference = stencilOpState.reference;
			// uint8_t currentStencil = 0;
			// if (deviceState->graphicsPipelineState.pipeline->getDynamicState().DynamicStencilReference)
			// {
			// 	TODO_ERROR();
			// }
			//
			// if (shaderModule->getStencilExport())
			// {
			// 	stencilReference = builtinOutput->stencilReference;
			// }
			//
			// if (deviceState->graphicsPipelineState.pipeline->getDepthStencilState().StencilTestEnable)
			// {
			// 	auto compareMask = stencilOpState.compareMask;
			// 	if (deviceState->graphicsPipelineState.pipeline->getDynamicState().DynamicStencilCompareMask)
			// 	{
			// 		TODO_ERROR();
			// 	}
			//
			// 	currentStencil = GetStencilPixel(deviceState, stencilImage.first.format, stencilImage.second->getImage(), 
			// 	                                 x, y, 0, 
			// 	                                 stencilImage.second->getSubresourceRange().baseMipLevel, stencilImage.second->getSubresourceRange().baseArrayLayer); 
			// 	stencilResult = CompareTest(stencilReference & compareMask, currentStencil & compareMask, stencilOpState.compareOp);
			// }
			TODO_ERROR();
		}
		else
		{
			stencilResult = ConstBool(true);
		}
	}

	void CompileDepthTest()
	{
		// 27.13. Depth Test
		if (state->getDepthStencilState().DepthTestEnable && state->getSubpass().depthStencilAttachment.attachment != VK_ATTACHMENT_UNUSED)
		{
			const auto depthStencilAttachment = CreateLoad(CreateGEP(CreateLoad(pipelineState), {0, 2}));
			const auto hasDepth = CreateICmpNE(depthStencilAttachment, LLVMConstNull(LLVMTypeOf(depthStencilAttachment)));
			depthResult = CreatePhiIf(mainFunction, hasDepth,
			                          [&]()
			                          {
				                          if (state->getRasterizationState().DepthClampEnable)
				                          {
					                          // depth = std::clamp(depth, viewport.minDepth, viewport.maxDepth);
					                          TODO_ERROR();
				                          }

				                          return CompileFCompareTest(depth, GetCurrentDepth(depthStencilAttachment), state->getDepthStencilState().DepthCompareOp);
			                          },
			                          [&]()
			                          {
				                          return ConstBool(true);
			                          });
		}
		else
		{
			depthResult = ConstBool(true);
		}
	}

	void CompileDepthStencilWrite()
	{
		if (state->getDepthStencilState().StencilTestEnable)
		{
			// if (deviceState->graphicsPipelineState.pipeline->getDepthStencilState().StencilTestEnable && stencilImage.second)
			// {
			// 	const auto stencilOperation = !stencilResult ? stencilOpState.failOp : (!depthResult ? stencilOpState.depthFailOp : stencilOpState.passOp);
			// 	uint8_t writeValue;
			// 	switch (stencilOperation)
			// 	{
			// 	case VK_STENCIL_OP_KEEP:
			// 		writeValue = currentStencil;
			// 		break;
			//
			// 	case VK_STENCIL_OP_ZERO:
			// 		writeValue = 0;
			// 		break;
			//
			// 	case VK_STENCIL_OP_REPLACE:
			// 		writeValue = stencilReference;
			// 		break;
			//
			// 	case VK_STENCIL_OP_INCREMENT_AND_CLAMP:
			// 		writeValue = currentStencil < 0xFF ? currentStencil + 1 : 0xFF;
			// 		break;
			//
			// 	case VK_STENCIL_OP_DECREMENT_AND_CLAMP:
			// 		writeValue = currentStencil > 0 ? currentStencil - 1 : 0;
			// 		break;
			//
			// 	case VK_STENCIL_OP_INVERT:
			// 		writeValue = ~currentStencil;
			// 		break;
			//
			// 	case VK_STENCIL_OP_INCREMENT_AND_WRAP:
			// 		writeValue = currentStencil + 1;
			// 		break;
			//
			// 	case VK_STENCIL_OP_DECREMENT_AND_WRAP:
			// 		writeValue = currentStencil - 1;
			// 		break;
			//
			// 	default:
			// 		FATAL_ERROR();
			// 	}
			//
			// 	auto writeMask = stencilOpState.writeMask;
			// 	if (deviceState->graphicsPipelineState.pipeline->getDynamicState().DynamicStencilWriteMask)
			// 	{
			// 		TODO_ERROR();
			// 	}
			//
			// 	writeValue = (writeValue & writeMask) | (currentStencil & ~writeMask);
			//
			// 	if (depthWrite)
			// 	{
			// 		const VkClearDepthStencilValue input
			// 		{
			// 			depth,
			// 			writeValue,
			// 		};
			//
			// 		// TODO: No clamp if float format?
			// 		SetPixel(deviceState, stencilImage.first.format, stencilImage.second->getImage(),
			// 		         x, y, 0,
			// 		         stencilImage.second->getSubresourceRange().baseMipLevel, stencilImage.second->getSubresourceRange().baseArrayLayer,
			// 		         input);
			// 	}
			// 	else
			// 	{
			// 		const VkClearDepthStencilValue input
			// 		{
			// 			currentDepth,
			// 			writeValue,
			// 		};
			//
			// 		// TODO: No clamp if float format?
			// 		SetPixel(deviceState, stencilImage.first.format, stencilImage.second->getImage(),
			// 		         x, y, 0,
			// 		         stencilImage.second->getSubresourceRange().baseMipLevel, stencilImage.second->getSubresourceRange().baseArrayLayer,
			// 		         input);
			// 	}
			// }
			// else if (depthWrite)
			// {
			// 	const VkClearDepthStencilValue input
			// 	{
			// 		depth,
			// 		stencilImage.second == nullptr
			// 			? 0u
			// 			: GetStencilPixel(deviceState, depthImage.first.format, depthImage.second->getImage(), 
			// 			                  x, y, 0, 
			// 			                  depthImage.second->getSubresourceRange().baseMipLevel, depthImage.second->getSubresourceRange().baseArrayLayer),
			// 	};
			//
			// 	// TODO: No clamp if float format?
			// 	SetPixel(deviceState, depthImage.first.format, depthImage.second->getImage(),
			// 	         x, y, 0,
			// 	         depthImage.second->getSubresourceRange().baseMipLevel, depthImage.second->getSubresourceRange().baseArrayLayer,
			// 	         input);
			// }
			//
			TODO_ERROR();
		}
		
		if (state->getDepthStencilState().DepthTestEnable && state->getDepthStencilState().DepthWriteEnable && state->getSubpass().depthStencilAttachment.attachment != VK_ATTACHMENT_UNUSED)
		{
			const auto depthStencilAttachment = CreateLoad(CreateGEP(CreateLoad(pipelineState), {0, 2}));
			const auto hasDepth = CreateICmpNE(depthStencilAttachment, LLVMConstNull(LLVMTypeOf(depthStencilAttachment)));
			const auto depthWrite = CreateAnd(depthResult, hasDepth);
			CreateIf(mainFunction, depthWrite, 
			         [&](LLVMBasicBlockRef)
			         {
				         // TODO: No clamp if float format?
				         auto setDepthPixel = LLVMGetNamedFunction(module, "@setDepthPixel");
				         if (!setDepthPixel)
				         {
					         std::array<LLVMTypeRef, 5> parameters
					         {
						         LLVMPointerType(LLVMInt8TypeInContext(context), 0),
						         LLVMPointerType(LLVMInt8TypeInContext(context), 0),
						         LLVMInt32TypeInContext(context),
						         LLVMInt32TypeInContext(context),
						         LLVMFloatTypeInContext(context),
					         };
					         const auto functionType = LLVMFunctionType(LLVMVoidTypeInContext(context), parameters.data(), static_cast<uint32_t>(parameters.size()), false);
					         setDepthPixel = LLVMAddFunction(module, "@setDepthPixel", functionType);
				         }

				         std::array<LLVMValueRef, 5> arguments
				         {
					         CreateLoad(shaderModuleBuilder->getUserData()),
					         depthStencilAttachment,
					         x,
					         y,
					         depth,
				         };
				         CreateCall(setDepthPixel, arguments.data(), static_cast<uint32_t>(arguments.size()));
			         }, nullptr);
		}
	}

	void CompileWriteFragment()
	{
		const auto write = CreateAnd(stencilResult, depthResult);
		CreateIf(mainFunction, write, 
		         [&](LLVMBasicBlockRef)
		         {
			         for (auto i = 0u; i < state->getSubpass().colourAttachments.size(); i++)
			         {
				         const auto& attachment = state->getSubpass().colourAttachments[i];
				         if (attachment.attachment != VK_ATTACHMENT_UNUSED)
				         {
					         CompileWriteFragmentAttachment(i, state->getAttachments()[attachment.attachment]);
				         }
			         }
		         }, nullptr);
	}

	LLVMValueRef GetCurrentDepth(LLVMValueRef depthImage)
	{
		if (currentDepth)
		{
			return currentDepth;
		}

		auto getDepthPixel = LLVMGetNamedFunction(module, "@getDepthPixel");
		if (!getDepthPixel)
		{
			std::array<LLVMTypeRef, 4> parameters
			{
				LLVMPointerType(LLVMInt8TypeInContext(context), 0),
				LLVMPointerType(LLVMInt8TypeInContext(context), 0),
				LLVMInt32TypeInContext(context),
				LLVMInt32TypeInContext(context),
			};
			const auto functionType = LLVMFunctionType(LLVMFloatTypeInContext(context), parameters.data(), static_cast<uint32_t>(parameters.size()), false);
			getDepthPixel = LLVMAddFunction(module, "@getDepthPixel", functionType);
		}

		std::array<LLVMValueRef, 4> arguments
		{
			CreateLoad(shaderModuleBuilder->getUserData()),
			depthImage,
			x,
			y,
		};
		return CreateCall(getDepthPixel, arguments.data(), static_cast<uint32_t>(arguments.size()));
	}

	LLVMValueRef CompileFClamp(LLVMValueRef value, LLVMValueRef min, LLVMValueRef max)
	{
		const auto isMin = CreateFCmpOLT(value, min);
		const auto isMax = CreateFCmpOGT(value, max);
		value = CreateSelect(isMin, min, value);
		value = CreateSelect(isMax, max, value);
		return value;
	}

	LLVMValueRef CompileFCompareTest(LLVMValueRef reference, LLVMValueRef value, const VkCompareOp compare)
	{
		switch (compare)
		{
		case VK_COMPARE_OP_NEVER: return ConstBool(false);
		case VK_COMPARE_OP_LESS: return CreateFCmpULT(reference, value);
		case VK_COMPARE_OP_EQUAL: return CreateFCmpUEQ(reference, value);
		case VK_COMPARE_OP_LESS_OR_EQUAL: return CreateFCmpULE(reference, value);
		case VK_COMPARE_OP_GREATER: return CreateFCmpUGT(reference, value);
		case VK_COMPARE_OP_NOT_EQUAL: return CreateFCmpUNE(reference, value);
		case VK_COMPARE_OP_GREATER_OR_EQUAL: return CreateFCmpUGE(reference, value);
		case VK_COMPARE_OP_ALWAYS: return ConstBool(true);

		default:
			FATAL_ERROR();
		}
	}

	LLVMValueRef CompileICompareTest(LLVMValueRef reference, LLVMValueRef value, const VkCompareOp compare)
	{
		switch (compare)
		{
		case VK_COMPARE_OP_NEVER: return ConstBool(false);
		case VK_COMPARE_OP_LESS: return CreateICmpULE(reference, value);
		case VK_COMPARE_OP_EQUAL: return CreateICmpEQ(reference, value);
		case VK_COMPARE_OP_LESS_OR_EQUAL: return CreateICmpULE(reference, value);
		case VK_COMPARE_OP_GREATER: return CreateICmpUGT(reference, value);
		case VK_COMPARE_OP_NOT_EQUAL: return CreateICmpNE(reference, value);
		case VK_COMPARE_OP_GREATER_OR_EQUAL: return CreateICmpUGE(reference, value);
		case VK_COMPARE_OP_ALWAYS: return ConstBool(true);

		default:
			FATAL_ERROR();
		}
	}

	void CompileWriteFragmentAttachment(uint32_t index, const AttachmentDescription& attachmentDescription)
	{
		const auto attachment = CreateLoad(CreateGEP(CreateLoad(pipelineState), {0, 1, index}));
		const auto hasImage = CreateICmpNE(attachment, LLVMConstNull(LLVMTypeOf(attachment)));
		CreateIf(mainFunction, hasImage, [&](LLVMBasicBlockRef)
		{
			const auto data = FindFragmentOutput(index);

			const auto& formatInformation = GetFormatInformation(attachmentDescription.format);

			uint32_t bits = 0;
			assert(formatInformation.Type == FormatType::Normal);
			if (formatInformation.Normal.RedOffset != INVALID_OFFSET &&
				formatInformation.Normal.GreenOffset != INVALID_OFFSET &&
				formatInformation.Normal.BlueOffset != INVALID_OFFSET &&
				formatInformation.Normal.AlphaOffset != INVALID_OFFSET)
			{
				bits = 4;
			}
			else if (formatInformation.Normal.RedOffset != INVALID_OFFSET &&
				formatInformation.Normal.GreenOffset != INVALID_OFFSET &&
				formatInformation.Normal.BlueOffset != INVALID_OFFSET &&
				formatInformation.Normal.AlphaOffset == INVALID_OFFSET)
			{
				bits = 3;
			}
			else if (formatInformation.Normal.RedOffset != INVALID_OFFSET &&
				formatInformation.Normal.GreenOffset != INVALID_OFFSET &&
				formatInformation.Normal.BlueOffset == INVALID_OFFSET &&
				formatInformation.Normal.AlphaOffset == INVALID_OFFSET)
			{
				bits = 2;
			}
			else if (formatInformation.Normal.RedOffset != INVALID_OFFSET &&
				formatInformation.Normal.GreenOffset == INVALID_OFFSET &&
				formatInformation.Normal.BlueOffset == INVALID_OFFSET &&
				formatInformation.Normal.AlphaOffset == INVALID_OFFSET)
			{
				bits = 1;
			}

			switch (formatInformation.Base)
			{
			case BaseType::UNorm:
			case BaseType::SNorm:
			case BaseType::UScaled:
			case BaseType::SScaled:
			case BaseType::UFloat:
			case BaseType::SFloat:
			case BaseType::SRGB:
				switch (bits)
				{
				case 1:
					CompileWriteFragmentBlend(index, attachmentDescription, data, LLVMFloatTypeInContext(context), "@setPixelF32");
					break;
					
				case 2:
					CompileWriteFragmentBlend(index, attachmentDescription, data, LLVMVectorType(LLVMFloatTypeInContext(context), 2), "@setPixelF32[2]");
					break;
					
				case 3:
					CompileWriteFragmentBlend(index, attachmentDescription, data, LLVMVectorType(LLVMFloatTypeInContext(context), 3), "@setPixelF32[3]");
					break;
					
				case 4:
					CompileWriteFragmentBlend(index, attachmentDescription, data, LLVMVectorType(LLVMFloatTypeInContext(context), 4), "@setPixelF32[4]");
					break;
					
				default:
					FATAL_ERROR();
				}
				break;
				          
			case BaseType::UInt:
				switch (bits)
				{
				case 1:
					CompileWriteFragmentBlend(index, attachmentDescription, data, LLVMInt32TypeInContext(context), "@setPixelU32");
					break;

				case 2:
					CompileWriteFragmentBlend(index, attachmentDescription, data, LLVMVectorType(LLVMInt32TypeInContext(context), 2), "@setPixelU32[2]");
					break;

				case 3:
					CompileWriteFragmentBlend(index, attachmentDescription, data, LLVMVectorType(LLVMInt32TypeInContext(context), 3), "@setPixelU32[3]");
					break;

				case 4:
					CompileWriteFragmentBlend(index, attachmentDescription, data, LLVMVectorType(LLVMInt32TypeInContext(context), 4), "@setPixelU32[4]");
					break;

				default:
					FATAL_ERROR();
				}
				break;
			         	
			case BaseType::SInt:
				switch (bits)
				{
				case 1:
					CompileWriteFragmentBlend(index, attachmentDescription, data, LLVMInt32TypeInContext(context), "@setPixelI32");
					break;

				case 2:
					CompileWriteFragmentBlend(index, attachmentDescription, data, LLVMVectorType(LLVMInt32TypeInContext(context), 2), "@setPixelI32[2]");
					break;

				case 3:
					CompileWriteFragmentBlend(index, attachmentDescription, data, LLVMVectorType(LLVMInt32TypeInContext(context), 3), "@setPixelI32[3]");
					break;

				case 4:
					CompileWriteFragmentBlend(index, attachmentDescription, data, LLVMVectorType(LLVMInt32TypeInContext(context), 4), "@setPixelI32[4]");
					break;

				default:
					FATAL_ERROR();
				}
				break;
						 	
			default:
				FATAL_ERROR();
			}
		}, nullptr);
	}

	LLVMValueRef FindFragmentOutput(uint32_t index)
	{
		for (auto i = 0u; i < shader->getNumVariables(); i++)
		{
			const auto& variable = shader->getVariable(i);
			if (variable->getStorageClass() == StorageClassOutput)
			{
				auto locations = variable->getDecorate(DecorationLocation);
				if (locations.empty())
				{
					continue;
				}
				
				const auto location = *locations.begin();
				if (location == index)
				{
					return shaderModuleBuilder->ConvertValue(variable, nullptr);
				}
			}
		}

		FATAL_ERROR();
	}

	void CompileWriteFragmentBlend(uint32_t index, const AttachmentDescription& attachmentDescription, LLVMValueRef colour, LLVMTypeRef colourType, const char* pixelFunction)
	{
		// 28.1. Blending
		const auto& blend = state->getColourBlendState().Attachments[index];
		if (blend.blendEnable)
		{
			TODO_ERROR();
			// 		const auto destination = ImageFetch<glm::fvec4, glm::ivec2>(deviceState, images[j].first.format, images[j].second, glm::ivec2(x, y));
			// 		if (blend.blendEnable)
			// 		{
			// 			auto constant = *reinterpret_cast<const glm::fvec4*>(deviceState->graphicsPipelineState.pipeline->getColourBlendState().BlendConstants);
			// 			colour = ApplyBlend(colour, destination, constant, blend);
			// 		}
			// 
		}
		
		// 28.2. Logical Operations
		if (state->getColourBlendState().LogicOpEnable)
		{
			TODO_ERROR();
		}
		
		// 28.3. Color Write Mask
		if (blend.colorWriteMask != (VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT))
		{
			// colour.r = (blend.colorWriteMask & VK_COLOR_COMPONENT_R_BIT) ? colour.r : destination.r;
			// colour.g = (blend.colorWriteMask & VK_COLOR_COMPONENT_G_BIT) ? colour.g : destination.g;
			// colour.b = (blend.colorWriteMask & VK_COLOR_COMPONENT_B_BIT) ? colour.b : destination.b;
			// colour.a = (blend.colorWriteMask & VK_COLOR_COMPONENT_A_BIT) ? colour.a : destination.a;
			TODO_ERROR();
		}

		// TODO: support non float types
		auto setPixel = LLVMGetNamedFunction(module, pixelFunction);
		if (!setPixel)
		{
			std::array<LLVMTypeRef, 5> parameters
			{
				LLVMPointerType(LLVMInt8TypeInContext(context), 0),
				LLVMPointerType(LLVMInt8TypeInContext(context), 0),
				LLVMInt32TypeInContext(context),
				LLVMInt32TypeInContext(context),
				LLVMPointerType(colourType, 0),
			};
			const auto functionType = LLVMFunctionType(LLVMVoidTypeInContext(context), parameters.data(), static_cast<uint32_t>(parameters.size()), false);
			setPixel = LLVMAddFunction(module, pixelFunction, functionType);
		}
		
		const auto attachment = CreateLoad(CreateGEP(CreateLoad(pipelineState), {0, 1, index}));
		std::array<LLVMValueRef, 5> arguments
		{
			CreateLoad(shaderModuleBuilder->getUserData()),
			attachment,
			x,
			y,
			colour,
		};
		CreateCall(setPixel, arguments.data(), static_cast<uint32_t>(arguments.size()));
	}
};

CompiledModule* CompileVertexPipeline(CPJit* jit,
                                      const GraphicsPipelineStateStorage* state,
                                      const SPIRV::SPIRVModule* vertexShader,
                                      const SPIRV::SPIRVFunction* entryPoint,
                                      const VkSpecializationInfo* specializationInfo)
{
	PipelineVertexCompiledModuleBuilder builder
	{
		vertexShader,
		entryPoint,
		specializationInfo,
		state
	};
	return Compile(&builder, jit);
}


CompiledModule* CompileFragmentPipeline(CPJit* jit,
                                        const GraphicsPipelineStateStorage* state,
                                        const SPIRV::SPIRVModule* fragmentShader,
                                        const SPIRV::SPIRVFunction* entryPoint,
                                        const VkSpecializationInfo* specializationInfo)
{
	PipelineFragmentCompiledModuleBuilder builder
	{
		fragmentShader,
		entryPoint,
		specializationInfo,
		state
	};
	return Compile(&builder, jit);
}