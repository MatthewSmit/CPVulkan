#include "PipelineCompiler.h"

#include "CompiledModule.h"
#include "CompiledModuleBuilder.h"
#include "Compilers.h"
#include "ImageCompiler.h"
#include "SPIRVBaseCompiler.h"
#include "SPIRVCompiler.h"

#include <Formats.h>
#include <PipelineData.h>

#include <SPIRVDecorate.h>
#include <SPIRVInstruction.h>
#include <SPIRVType.h>

#include <llvm-c/Target.h>

#include <utility>

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
	
	LLVMValueRef ConvertValue(const SPIRV::SPIRVValue* spirvValue, LLVMValueRef currentFunction) override
	{
		switch (spirvValue->getOpCode())
		{
		case OpConstant:
			{
				const auto spirvConstant = static_cast<const SPIRV::SPIRVConstant*>(spirvValue);
				const auto spirvType = spirvConstant->getType();
				const auto llvmType = ConvertType(spirvType);
				switch (spirvType->getOpCode())
				{
				case OpTypeInt:
					return LLVMConstInt(llvmType, spirvConstant->getInt64Value(), static_cast<SPIRV::SPIRVTypeInt*>(spirvType)->isSigned());

				case OpTypeFloat:
					{
						switch (spirvType->getFloatBitWidth())
						{
						case 16:
							TODO_ERROR();

						case 32:
							return LLVMConstReal(llvmType, spirvConstant->getFloatValue());

						case 64:
							return LLVMConstReal(llvmType, spirvConstant->getDoubleValue());

						default:
							TODO_ERROR();
						}
					}

				default:
					TODO_ERROR();
				}
			}

		case OpConstantTrue:
		case OpConstantFalse:
			return LLVMConstInt(LLVMInt1TypeInContext(context), spirvValue->getOpCode() == OpConstantTrue, false);

		case OpConstantNull:
			return LLVMConstNull(ConvertType(spirvValue->getType()));

		case OpConstantComposite:
			{
				const auto constantComposite = static_cast<const SPIRV::SPIRVConstantComposite*>(spirvValue);
				std::vector<LLVMValueRef> constants;
				for (auto& element : constantComposite->getElements())
				{
					constants.push_back(ConvertValue(element, currentFunction));
				}
				switch (constantComposite->getType()->getOpCode())
				{
				case OpTypeVector:
					return LLVMConstVector(constants.data(), static_cast<uint32_t>(constants.size()));

				case OpTypeArray:
					{
						const auto arrayType = ConvertType(constantComposite->getType()->getArrayElementType());
						if (arrayStrideMultiplier.find(arrayType) != arrayStrideMultiplier.end())
						{
							// TODO: Support stride
							TODO_ERROR();
						}
						return LLVMConstArray(arrayType, constants.data(), static_cast<uint32_t>(constants.size()));
					}

				case OpTypeMatrix:
				case OpTypeStruct:
					return LLVMConstNamedStruct(ConvertType(constantComposite->getType()), constants.data(), static_cast<uint32_t>(constants.size()));

				default:
					TODO_ERROR();
				}
			}

		case OpConstantSampler:
			{
				//auto BCS = static_cast<SPIRVConstantSampler*>(BV);
				//return mapValue(BV, oclTransConstantSampler(BCS, BB));
				TODO_ERROR();
			}

		case OpConstantPipeStorage:
			{
				//auto BCPS = static_cast<SPIRVConstantPipeStorage*>(BV);
				//return mapValue(BV, oclTransConstantPipeStorage(BCPS));
				TODO_ERROR();
			}

		default:
			TODO_ERROR();
		}
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
		const auto llvmType = ConvertType(spirvType);
		const auto& attribute = FindAttribute(location);
		const auto& binding = FindBinding(attribute.binding);

		// offset = static_cast<uint64_t>(binding.stride) * (vertex or instance) + attribute.offset;
		const auto bufferIndex = CreateZExt(binding.inputRate == VK_VERTEX_INPUT_RATE_VERTEX ? vertexId : instanceId, LLVMInt64TypeInContext(context));
		auto bufferOffset = CreateMul(ConstU64(binding.stride), bufferIndex);
		bufferOffset = CreateAdd(bufferOffset, ConstU64(attribute.offset));

		const auto shaderFormat = GetVariableFormat(spirvType);

		auto bufferData = CreateLoad(CreateGEP(CreateLoad(pipelineState), { 0, 0, binding.binding }));
		bufferData = CreateGEP(bufferData, { bufferOffset });

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
				const auto shaderAddress = CreateIntToPtr(ConstU64(reinterpret_cast<uint64_t>(llvmVertexShader->getPointer(MangleName(variable)))), ConvertType(variable->getType()));
				
				if (spirvType->isTypeArray())
				{
					const auto llvmType = ConvertType(spirvType);
					const auto elementSize = GetVariableSize(spirvType->getArrayElementType());
					const auto multiplier = elementSize > 16 ? 2 : 1;
					for (auto j = 0u; j < LLVMGetArrayLength(llvmType); j++)
					{
						const auto columnAddress = CreateGEP(shaderAddress, 0, j);
						EmitCopyInput(location + j * multiplier, vertexId, instanceId, columnAddress, spirvType->getArrayElementType());
					}
				}
				else if (spirvType->isTypeMatrix())
				{
					const auto elementSize = GetVariableSize(spirvType->getMatrixColumnType());
					const auto multiplier = elementSize > 16 ? 2 : 1;
					for (auto j = 0u; j < spirvType->getMatrixColumnCount(); j++)
					{
						const auto columnAddress = CreateGEP(shaderAddress, 0, j);
						EmitCopyInput(location + j * multiplier, vertexId, instanceId, columnAddress, spirvType->getMatrixColumnType());
					}
				}
				else
				{
					EmitCopyInput(location, vertexId, instanceId, shaderAddress, spirvType);
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
		const auto outputStorage = CreateBitCast(CreateGEP(CreateLoad(outputVariable), {CreateMul(rawId, ConstU32(static_cast<uint32_t>(outputStride)))}), outputType);
		CreateStore(shaderBuiltinOutput, CreateGEP(outputStorage, 0, 0));

		// Copy custom output to storage
		for (auto i = 0u, j = 0u; i < spirvVertexShader->getNumVariables(); i++)
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
				CreateStore(shaderValue, CreateGEP(outputStorage, 0, j + 1));
				j++;
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
