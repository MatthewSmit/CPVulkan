#include "SPIRVBaseCompiler.h"

#include <SPIRVDecorate.h>
#include <SPIRVType.h>

#include <llvm-c/OrcBindings.h>

LLVMTypeRef SPIRVBaseCompiledModuleBuilder::ConvertType(const SPIRV::SPIRVType* spirvType, bool isClassMember)
{
	const auto cachedType = typeMapping.find(spirvType->getId());
	if (cachedType != typeMapping.end())
	{
		return cachedType->second;
	}
		
	LLVMTypeRef llvmType{};
		
	switch (spirvType->getOpCode())
	{
	case OpTypeVoid:
		llvmType = LLVMVoidTypeInContext(context);
		break;
			
	case OpTypeBool:
		llvmType = LLVMInt1TypeInContext(context);
		break;
			
	case OpTypeInt:
		llvmType = LLVMIntTypeInContext(context, spirvType->getIntegerBitWidth());
		break;
			
	case OpTypeFloat:
		switch (spirvType->getFloatBitWidth())
		{
		case 16:
			llvmType = LLVMHalfTypeInContext(context);
			break;
			
		case 32:
			llvmType = LLVMFloatTypeInContext(context);
			break;
			
		case 64:
			llvmType = LLVMDoubleTypeInContext(context);
			break;
			
		default:
			TODO_ERROR();
		}
			
		break;
			
	case OpTypeArray:
		{
			const auto elementType = ConvertType(spirvType->getArrayElementType());
			auto multiplier = 1u;
			if (spirvType->hasDecorate(DecorationArrayStride))
			{
				const auto stride = *spirvType->getDecorate(DecorationArrayStride).begin();
				const auto originalStride = LLVMSizeOfTypeInBits(layout, elementType) / 8;
				if (stride != originalStride)
				{
					multiplier = stride / originalStride;
					assert((stride % originalStride) == 0);
				}
			}
			llvmType = LLVMArrayType(elementType, spirvType->getArrayLength() * multiplier);
			if (multiplier > 1)
			{
				arrayStrideMultiplier[llvmType] = multiplier;
			}
			break;
		}
			
	case OpTypeRuntimeArray:
		{
			const auto runtimeArray = static_cast<const SPIRV::SPIRVTypeRuntimeArray*>(spirvType);
			const auto elementType = ConvertType(runtimeArray->getElementType());
			auto multiplier = 1u;
			if (runtimeArray->hasDecorate(DecorationArrayStride))
			{
				const auto stride = *spirvType->getDecorate(DecorationArrayStride).begin();
				const auto originalStride = LLVMSizeOfTypeInBits(layout, elementType) / 8;
				if (stride != originalStride)
				{
					multiplier = stride / originalStride;
					assert((stride % originalStride) == 0);
				}
			}
			llvmType = LLVMArrayType(elementType, 0);
			if (multiplier > 1)
			{
				arrayStrideMultiplier[llvmType] = multiplier;
			}
			break;
		}
			
	case OpTypePointer:
		llvmType = LLVMPointerType(ConvertType(spirvType->getPointerElementType(), isClassMember), 0);
		break;
			
	case OpTypeVector:
		llvmType = LLVMVectorType(ConvertType(spirvType->getVectorComponentType()), spirvType->getVectorComponentCount());
		break;
			
	case OpTypeOpaque:
		llvmType = LLVMStructCreateNamed(context, spirvType->getName().c_str());
		break;
			
	case OpTypeFunction:
		{
			const auto functionType = static_cast<const SPIRV::SPIRVTypeFunction*>(spirvType);
			const auto returnType = ConvertType(functionType->getReturnType());
			std::vector<LLVMTypeRef> parameters;
			for (size_t i = 0; i != functionType->getNumParameters(); ++i)
			{
				parameters.push_back(ConvertType(functionType->getParameterType(i)));
			}
			llvmType = LLVMFunctionType(returnType, parameters.data(), static_cast<uint32_t>(parameters.size()), false);
			break;
		}
			
	case OpTypeImage:
		{
			const auto image = static_cast<const SPIRV::SPIRVTypeImage*>(spirvType);
			llvmType = CreateOpaqueImageType(GetImageTypeName(image));
			break;
		}
			
	case OpTypeSampler:
		{
			llvmType = CreateOpaqueImageType("Sampler");
			break;
		}
			
	case OpTypeSampledImage:
		{
			const auto sampledImage = static_cast<const SPIRV::SPIRVTypeSampledImage*>(spirvType);
			llvmType = CreateOpaqueImageType(GetTypeName(sampledImage));
			break;
		}
			
	case OpTypeStruct:
		{
			// TODO: Handle case when on x32 platform and struct contains pointers.
			// When using the PhysicalStorageBuffer64EXT addressing model, pointers with a class of PhysicalStorageBuffer should be treated as 64 bits wide.
			const auto strct = static_cast<const SPIRV::SPIRVTypeStruct*>(spirvType);
			const auto& name = strct->getName();
			llvmType = LLVMStructCreateNamed(context, name.c_str());
			auto& indexMapping = structIndexMapping[llvmType];
			std::vector<LLVMTypeRef> types{};
			uint64_t currentOffset = 0;
			// TODO: If no offset decoration and struct is not packed, insert own packing
			for (auto i = 0u; i < strct->getMemberCount(); ++i)
			{
				const auto decorate = strct->getMemberDecorate(i, DecorationOffset);
				if (decorate && decorate->getLiteral(0) != currentOffset)
				{
					const auto offset = decorate->getLiteral(0);
					if (offset < currentOffset)
					{
						TODO_ERROR();
					}
					else
					{
						// TODO: Use array of uint8 instead?
						auto difference = offset - currentOffset;
						while (difference >= 8)
						{
							types.push_back(LLVMInt64TypeInContext(context));
							difference -= 8;
						}
						while (difference >= 4)
						{
							types.push_back(LLVMInt32TypeInContext(context));
							difference -= 4;
						}
						while (difference >= 2)
						{
							types.push_back(LLVMInt16TypeInContext(context));
							difference -= 2;
						}
						while (difference >= 1)
						{
							types.push_back(LLVMInt8TypeInContext(context));
							difference -= 1;
						}
					}
					currentOffset = offset;
				}
					
				const auto llvmMemberType = ConvertType(strct->getMemberType(i), true);
				indexMapping.push_back(static_cast<uint32_t>(types.size()));
				types.push_back(llvmMemberType);

				if (strct->getMemberType(i)->getOpCode() == OpTypeRuntimeArray)
				{
					assert(i + 1 == strct->getMemberCount());
				}
				else
				{
					currentOffset += LLVMSizeOfTypeInBits(layout, llvmMemberType) / 8;
				}
			}
			LLVMStructSetBody(llvmType, types.data(), static_cast<uint32_t>(types.size()), true);
			break;
		}
			
	case OpTypeMatrix:
		{
			if (spirvType->hasDecorate(DecorationMatrixStride))
			{
				TODO_ERROR();
			}

			if (spirvType->hasDecorate(DecorationRowMajor))
			{
				TODO_ERROR();
			}

			auto name = std::string{"@Matrix.Col"};
			name += std::to_string(spirvType->getMatrixColumnCount());
			name += 'x';
			name += std::to_string(spirvType->getMatrixColumnType()->getVectorComponentCount());

			std::vector<LLVMTypeRef> types{};
			for (auto i = 0u; i < spirvType->getMatrixColumnCount(); i++)
			{
				types.push_back(ConvertType(spirvType->getMatrixColumnType(), true));
			}
			llvmType = StructType(types, name);
			break;
		}
			
	case OpTypePipe:
		{
			//   auto PT = static_cast<SPIRVTypePipe *>(T);
			//   return mapType(T, getOrCreateOpaquePtrType(
			//                         M,
			//                         transOCLPipeTypeName(PT, IsClassMember,
			//                                              PT->getAccessQualifier()),
			//                         getOCLOpaqueTypeAddrSpace(T->getOpCode())));
			TODO_ERROR();
		}
			
	case OpTypePipeStorage:
		{
			//   auto PST = static_cast<SPIRVTypePipeStorage *>(T);
			//   return mapType(
			//       T, getOrCreateOpaquePtrType(M, transOCLPipeStorageTypeName(PST),
			//                                   getOCLOpaqueTypeAddrSpace(T->getOpCode())));
			TODO_ERROR();
		}
		
	default:
		TODO_ERROR();
	}
		
	typeMapping[spirvType->getId()] = llvmType;
		
	return llvmType;
}

LLVMTypeRef SPIRVBaseCompiledModuleBuilder::CreateOpaqueImageType(const std::string& name)
{
	auto opaqueType = LLVMGetTypeByName(module, name.c_str());
	if (!opaqueType)
	{
		std::vector<LLVMTypeRef> members
		{
			LLVMInt32TypeInContext(context),
			LLVMPointerType(LLVMInt8TypeInContext(context), 0),
			LLVMPointerType(LLVMInt8TypeInContext(context), 0),
		};
		opaqueType = StructType(members, name);
	}

	return LLVMPointerType(opaqueType, 0);
}

std::string SPIRVBaseCompiledModuleBuilder::GetTypeName(const char* baseName, const std::string& postfixes)
{
	if (postfixes.empty())
	{
		return baseName;
	}

	return std::string{baseName} + "_" + postfixes;
}

std::string SPIRVBaseCompiledModuleBuilder::GetTypeName(const SPIRV::SPIRVType* type)
{
	switch (type->getOpCode())
	{
	case OpTypeVoid: TODO_ERROR();
	case OpTypeBool: TODO_ERROR();

	case OpTypeInt:
		return (static_cast<const SPIRV::SPIRVTypeInt*>(type)->isSigned() ? "I" : "U") + std::to_string(type->getIntegerBitWidth());

	case OpTypeFloat:
		return "F" + std::to_string(type->getFloatBitWidth());

	case OpTypeVector:
		return GetTypeName(type->getVectorComponentType()) + "[" + std::to_string(type->getVectorComponentCount()) + "]";

	case OpTypeMatrix:
		// TODO: Use array instead?
		if (type->hasDecorate(DecorationMatrixStride))
		{
			TODO_ERROR();
		}

		if (type->hasDecorate(DecorationRowMajor))
		{
			TODO_ERROR();
		}

		return GetTypeName(type->getScalarType()) + "[" + std::to_string(type->getMatrixColumnCount()) + "," + std::to_string(type->getMatrixColumnType()->getVectorComponentCount()) + ",col]";

	case OpTypeImage:
		return GetImageTypeName(static_cast<const SPIRV::SPIRVTypeImage*>(type));

	case OpTypeSampler:
		TODO_ERROR();

	case OpTypeSampledImage:
		return "Sampled" + GetImageTypeName(static_cast<const SPIRV::SPIRVTypeSampledImage*>(type)->getImageType());

	case OpTypeArray: TODO_ERROR();
	case OpTypeRuntimeArray: TODO_ERROR();
	case OpTypeStruct: TODO_ERROR();
	case OpTypeOpaque: TODO_ERROR();
	case OpTypePointer: TODO_ERROR();
	case OpTypeFunction: TODO_ERROR();
	case OpTypeEvent: TODO_ERROR();
	case OpTypeDeviceEvent: TODO_ERROR();
	case OpTypeReserveId: TODO_ERROR();
	case OpTypeQueue: TODO_ERROR();
	case OpTypePipe: TODO_ERROR();
	case OpTypeForwardPointer: TODO_ERROR();
	case OpTypePipeStorage: TODO_ERROR();
	case OpTypeNamedBarrier: TODO_ERROR();
	case OpTypeAccelerationStructureNV: TODO_ERROR();
	case OpTypeCooperativeMatrixNV: TODO_ERROR();
	case OpTypeVmeImageINTEL: TODO_ERROR();
	case OpTypeAvcImePayloadINTEL: TODO_ERROR();
	case OpTypeAvcRefPayloadINTEL: TODO_ERROR();
	case OpTypeAvcSicPayloadINTEL: TODO_ERROR();
	case OpTypeAvcMcePayloadINTEL: TODO_ERROR();
	case OpTypeAvcMceResultINTEL: TODO_ERROR();
	case OpTypeAvcImeResultINTEL: TODO_ERROR();
	case OpTypeAvcImeResultSingleReferenceStreamoutINTEL: TODO_ERROR();
	case OpTypeAvcImeResultDualReferenceStreamoutINTEL: TODO_ERROR();
	case OpTypeAvcImeSingleReferenceStreaminINTEL: TODO_ERROR();
	case OpTypeAvcImeDualReferenceStreaminINTEL: TODO_ERROR();
	case OpTypeAvcRefResultINTEL: TODO_ERROR();
	case OpTypeAvcSicResultINTEL: TODO_ERROR();

	default:
		TODO_ERROR();
	}
}

std::string SPIRVBaseCompiledModuleBuilder::GetImageTypeName(const SPIRV::SPIRVTypeImage* imageType)
{
	const auto& descriptor = imageType->getDescriptor();

	auto sampledType = "Image[" + GetTypeName(imageType->getSampledType());

	switch (descriptor.Dim)
	{
	case Dim1D:
		sampledType += ",1D";
		break;

	case Dim2D:
		sampledType += ",2D";
		break;

	case Dim3D:
		sampledType += ",3D";
		break;

	case DimCube:
		sampledType += ",cube";
		break;

	case DimRect:
		sampledType += ",rect";
		break;

	case DimBuffer:
		sampledType += ",buffer";
		break;

	case DimSubpassData:
		sampledType += ",subpass";
		break;

	default:
		TODO_ERROR();
	}

	if (descriptor.Arrayed)
	{
		sampledType += ",array";
	}

	if (descriptor.MS)
	{
		sampledType += ",ms";
	}

	sampledType += "]";

	return sampledType;
}
