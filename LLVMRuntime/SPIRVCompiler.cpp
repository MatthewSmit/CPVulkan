#include "SPIRVCompiler.h"

#include "CompiledModuleBuilder.h"
#include "Compilers.h"
#include "Jit.h"

#include <Base.h>
#include <Half.h>
#include <SPIRVInstruction.h>
#include <SPIRVModule.h>

#include <llvm-c/Core.h>
#include <llvm-c/DebugInfo.h>
#include <llvm-c/Target.h>

#define EMIT_DEBUG 0

static std::unordered_map<Op, LLVMAtomicRMWBinOp> instructionLookupAtomic
{
	{OpAtomicExchange, LLVMAtomicRMWBinOpXchg},
	{OpAtomicIAdd, LLVMAtomicRMWBinOpAdd},
	{OpAtomicISub, LLVMAtomicRMWBinOpSub},
	{OpAtomicSMin, LLVMAtomicRMWBinOpMin},
	{OpAtomicUMin, LLVMAtomicRMWBinOpUMin},
	{OpAtomicSMax, LLVMAtomicRMWBinOpMax},
	{OpAtomicUMax, LLVMAtomicRMWBinOpUMax},
	{OpAtomicAnd, LLVMAtomicRMWBinOpAnd},
	{OpAtomicOr, LLVMAtomicRMWBinOpOr},
	{OpAtomicXor, LLVMAtomicRMWBinOpXor},
};

template<>
int32_t SPIRVCompiledModuleBuilder::GetConstant(LLVMValueRef value)
{
	if (!LLVMIsAConstantInt(value))
	{
		TODO_ERROR();
	}

	return static_cast<int32_t>(LLVMConstIntGetSExtValue(value));
}

LLVMTypeRef SPIRVCompiledModuleBuilder::ConvertType(const SPIRV::SPIRVType* spirvType, bool isClassMember)
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
				const auto originalStride = LLVMSizeOfTypeInBits(jit->getDataLayout(), elementType) / 8;
				if (stride != originalStride)
				{
					multiplier = stride / originalStride;
					assert((stride % originalStride) == 0);
				}
			}
			llvmType = LLVMArrayType(elementType, GetConstant<int32_t>(ConvertValue(static_cast<const SPIRV::SPIRVTypeArray*>(spirvType)->getLength(), nullptr)) * multiplier);
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
				const auto originalStride = LLVMSizeOfTypeInBits(jit->getDataLayout(), elementType) / 8;
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
			for (auto i = 0u; i != functionType->getNumParameters(); ++i)
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
					currentOffset += LLVMSizeOfTypeInBits(jit->getDataLayout(), llvmMemberType) / 8;
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

LLVMTypeRef SPIRVCompiledModuleBuilder::CreateOpaqueImageType(const std::string& name)
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

std::string SPIRVCompiledModuleBuilder::GetTypeName(const char* baseName, const std::string& postfixes)
{
	if (postfixes.empty())
	{
		return baseName;
	}

	return std::string{baseName} + "_" + postfixes;
}

std::string SPIRVCompiledModuleBuilder::GetTypeName(const SPIRV::SPIRVType* type)
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

std::string SPIRVCompiledModuleBuilder::GetImageTypeName(const SPIRV::SPIRVTypeImage* imageType)
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

template<typename T>
T SPIRVCompiledModuleBuilder::GetSpecOverride(const SPIRV::SPIRVValue* spirvValue)
{
	const auto id = *spirvValue->getDecorate(DecorationSpecId).begin();
	for (auto i = 0u; i < specializationInfo->mapEntryCount; i++)
	{
		const auto& entry = specializationInfo->pMapEntries[i];
		if (entry.constantID == id)
		{
			if (entry.size != sizeof(T))
			{
				TODO_ERROR();
			}

			return *reinterpret_cast<const T*>(static_cast<const uint8_t*>(specializationInfo->pData) + entry.offset);
		}
	}

	TODO_ERROR();
}

template<typename LoopInstructionType>
void SPIRVCompiledModuleBuilder::SetLLVMLoopMetadata(const LoopInstructionType* loopMerge, LLVMValueRef branchInstruction)
{
	if (!loopMerge)
	{
		return;
	}

	assert(branchInstruction && LLVMIsABranchInst(branchInstruction));

	auto self = LLVMMDNodeInContext2(context, nullptr, 0);
	// LLVMSetOperand(self, 0, self);
	const auto loopControl = static_cast<LoopControlMask>(loopMerge->getLoopControl());
	if (loopControl == LoopControlMaskNone)
	{
		LLVMSetMetadata(branchInstruction, LLVMGetMDKindIDInContext(context, "llvm.loop", sizeof("llvm.loop") - 1), LLVMMetadataAsValue(context, self));
		return;
	}

	// unsigned NumParam = 0;
	// std::vector<llvm::Metadata *> Metadata;
	// std::vector<SPIRVWord> LoopControlParameters = LM->getLoopControlParameters();
	// Metadata.push_back(llvm::MDNode::get(*Context, Self));

	// // To correctly decode loop control parameters, order of checks for loop
	// // control masks must match with the order given in the spec (see 3.23),
	// // i.e. check smaller-numbered bits first.
	// // Unroll and UnrollCount loop controls can't be applied simultaneously with
	// // DontUnroll loop control.
	// if (LC & LoopControlUnrollMask)
	// 	Metadata.push_back(getMetadataFromName("llvm.loop.unroll.enable"));
	// else if (LC & LoopControlDontUnrollMask)
	// 	Metadata.push_back(getMetadataFromName("llvm.loop.unroll.disable"));
	// if (LC & LoopControlDependencyInfiniteMask)
	// 	Metadata.push_back(getMetadataFromName("llvm.loop.ivdep.enable"));
	// if (LC & LoopControlDependencyLengthMask) {
	// 	if (!LoopControlParameters.empty()) {
	// 		Metadata.push_back(llvm::MDNode::get(
	// 			*Context,
	// 			getMetadataFromNameAndParameter("llvm.loop.ivdep.safelen",
	// 			                                LoopControlParameters[NumParam])));
	// 		++NumParam;
	// 		assert(NumParam <= LoopControlParameters.size() &&
	// 			"Missing loop control parameter!");
	// 	}
	// }
	// // Placeholder for LoopControls added in SPIR-V 1.4 spec (see 3.23)
	// if (LC & LoopControlMinIterationsMask) {
	// 	++NumParam;
	// 	assert(NumParam <= LoopControlParameters.size() &&
	// 		"Missing loop control parameter!");
	// }
	// if (LC & LoopControlMaxIterationsMask) {
	// 	++NumParam;
	// 	assert(NumParam <= LoopControlParameters.size() &&
	// 		"Missing loop control parameter!");
	// }
	// if (LC & LoopControlIterationMultipleMask) {
	// 	++NumParam;
	// 	assert(NumParam <= LoopControlParameters.size() &&
	// 		"Missing loop control parameter!");
	// }
	// if (LC & LoopControlPeelCountMask) {
	// 	++NumParam;
	// 	assert(NumParam <= LoopControlParameters.size() &&
	// 		"Missing loop control parameter!");
	// }
	// if (LC & LoopControlPartialCountMask && !(LC & LoopControlDontUnrollMask)) {
	// 	// If unroll factor is set as '1' - disable loop unrolling
	// 	if (1 == LoopControlParameters[NumParam])
	// 		Metadata.push_back(getMetadataFromName("llvm.loop.unroll.disable"));
	// 	else
	// 		Metadata.push_back(llvm::MDNode::get(
	// 			*Context,
	// 			getMetadataFromNameAndParameter("llvm.loop.unroll.count",
	// 			                                LoopControlParameters[NumParam])));
	// 	++NumParam;
	// 	assert(NumParam <= LoopControlParameters.size() &&
	// 		"Missing loop control parameter!");
	// }
	// if (LC & LoopControlExtendedControlsMask) {
	// 	while (NumParam < LoopControlParameters.size()) {
	// 		switch (LoopControlParameters[NumParam]) {
	// 		case InitiationIntervalINTEL: {
	// 			// To generate a correct integer part of metadata we skip a parameter
	// 			// that encodes name of the metadata and take the next one
	// 			Metadata.push_back(llvm::MDNode::get(
	// 				*Context,
	// 				getMetadataFromNameAndParameter(
	// 					"llvm.loop.ii.count", LoopControlParameters[++NumParam])));
	// 			break;
	// 		}
	// 		case MaxConcurrencyINTEL: {
	// 			Metadata.push_back(llvm::MDNode::get(
	// 				*Context, getMetadataFromNameAndParameter(
	// 					"llvm.loop.max_concurrency.count",
	// 					LoopControlParameters[++NumParam])));
	// 			break;
	// 		}
	// 		default:
	// 			break;
	// 		}
	// 		++NumParam;
	// 	}
	// }
	// llvm::MDNode *Node = llvm::MDNode::get(*Context, Metadata);

	// // Set the first operand to refer itself
	// Node->replaceOperandWith(0, Node);
	// BI->setMetadata("llvm.loop", Node);
	TODO_ERROR();
}

SPIRVCompiledModuleBuilder::SPIRVCompiledModuleBuilder(const SPIRV::SPIRVModule* spirvModule, spv::ExecutionModel executionModel,
                                                       const SPIRV::SPIRVFunction* entryPoint, const VkSpecializationInfo* specializationInfo):
	spirvModule{spirvModule},
	executionModel{executionModel},
	entryPoint{entryPoint},
	specializationInfo{specializationInfo}
{
	if (spirvModule->getAddressingModel() != AddressingModelLogical && spirvModule->getAddressingModel() != AddressingModelPhysicalStorageBuffer64)
	{
		TODO_ERROR();
	}
}

void SPIRVCompiledModuleBuilder::AddDebugInformation(const SPIRV::SPIRVEntry* spirvEntry)
{
	if (spirvEntry->hasLine())
	{
		const auto& line = spirvEntry->getLine();
		TODO_ERROR();
	}
}

LLVMValueRef SPIRVCompiledModuleBuilder::CompileMainFunctionImpl()
{
	AddBuiltin();

#if EMIT_DEBUG
		diBuilder = LLVMCreateDIBuilder(module);
#endif

	userData = GlobalVariable(LLVMPointerType(LLVMInt8TypeInContext(context), 0), LLVMExternalLinkage, "@userData");

	// TODO: Only compile variables linked to entry point
	for (auto i = 0u; i < spirvModule->getNumVariables(); i++)
	{
		ConvertValue(spirvModule->getVariable(i), nullptr);
	}

	// TODO: Only compile functions linked to entry point
	for (auto i = 0u; i < spirvModule->getNumFunctions(); i++)
	{
		const auto spirvFunction = spirvModule->getFunction(i);
		const auto function = ConvertFunction(spirvFunction);
		for (auto j = 0u; j < spirvModule->getFunction(i)->getNumBasicBlock(); j++)
		{
			const auto spirvBasicBlock = spirvModule->getFunction(i)->getBasicBlock(j);
			for (auto k = 0u; k < spirvBasicBlock->getNumInst(); k++)
			{
				const auto instruction = spirvBasicBlock->getInst(k);
				if (instruction->getOpCode() == OpPhi)
				{
					auto phi = static_cast<SPIRV::SPIRVPhi*>(instruction);
					auto llvmPhi = ConvertValue(phi, function);
					phi->foreachPair([&](SPIRV::SPIRVValue* incomingValue, SPIRV::SPIRVBasicBlock* incomingBasicBlock, size_t)
					{
						auto phiBasicBlock = GetBasicBlock(incomingBasicBlock);
						auto translated = ConvertValue(incomingValue, function);
						LLVMAddIncoming(llvmPhi, &translated, &phiBasicBlock, 1);
					});
				}
			}
		}
	}

	// Handle Finding Internal Settings, such as WorkgroupSize
	if (executionModel == ExecutionModelGLCompute || executionModel == ExecutionModelKernel)
	{
		for (const auto& entry : spirvModule->getEntries())
		{
			for (const auto& decorate : entry.second->getDecorates())
			{
				if (decorate.first == DecorationBuiltIn)
				{
					const auto builtin = static_cast<BuiltIn>(decorate.second->getLiteral(0));
					if (builtin == BuiltInWorkgroupSize)
					{
						const auto llvmValue = ConvertValue(spirvModule->getValue(entry.first), nullptr);
						const auto llvmType = LLVMTypeOf(llvmValue);
						if (LLVMGetTypeKind(llvmType) != LLVMVectorTypeKind || LLVMGetVectorSize(llvmType) != 3 || LLVMGetTypeKind(LLVMGetElementType(llvmType))
							!= LLVMIntegerTypeKind)
						{
							TODO_ERROR();
						}

						GlobalVariable(llvmType, true, LLVMExternalLinkage, llvmValue, "@WorkgroupSize");
					}
				}
			}
		}
	}

#if EMIT_DEBUG
		LLVMDIBuilderFinalize(diBuilder);
		LLVMDisposeDIBuilder(diBuilder);
#endif

	return ConvertFunction(entryPoint);
}

LLVMLinkage SPIRVCompiledModuleBuilder::ConvertLinkage(const SPIRV::SPIRVValue* value)
{
	switch (value->getLinkageType())
	{
	case LinkageTypeExport:
		// if (V->getOpCode() == OpVariable) {
		// 	if (static_cast<const SPIRVVariable*>(V)->getInitializer() == 0)
		// 		// Tentative definition
		// 		return GlobalValue::CommonLinkage;
		// }
		// return GlobalValue::ExternalLinkage;
		TODO_ERROR();

	case LinkageTypeImport:
		// // Function declaration
		// if (V->getOpCode() == OpFunction) {
		// 	if (static_cast<const SPIRVFunction*>(V)->getNumBasicBlock() == 0)
		// 		return GlobalValue::ExternalLinkage;
		// }
		// // Variable declaration
		// if (V->getOpCode() == OpVariable) {
		// 	if (static_cast<const SPIRVVariable*>(V)->getInitializer() == 0)
		// 		return GlobalValue::ExternalLinkage;
		// }
		// // Definition
		// return GlobalValue::AvailableExternallyLinkage;
		TODO_ERROR();

	case LinkageTypeInternal:
		return LLVMExternalLinkage;

	default:
		TODO_ERROR();
	}
}

bool SPIRVCompiledModuleBuilder::IsOpaqueType(SPIRV::SPIRVType* spirvType)
{
	switch (spirvType->getOpCode())
	{
	case OpTypeVoid:
		TODO_ERROR();

	case OpTypeBool:
	case OpTypeInt:
	case OpTypeFloat:
	case OpTypeVector:
	case OpTypeMatrix:
		return false;

	case OpTypeImage:
	case OpTypeSampler:
	case OpTypeSampledImage:
		return true;

	case OpTypeArray:
	case OpTypeRuntimeArray:
		return IsOpaqueType(spirvType->getArrayElementType());

	case OpTypeStruct:
		for (auto i = 0u; i < spirvType->getStructMemberCount(); i++)
		{
			if (IsOpaqueType(spirvType->getStructMemberType(i)))
			{
				TODO_ERROR();
			}
		}
		return false;

	case OpTypePointer:
		return spirvType->getModule()->getAddressingModel() == AddressingModelLogical;

	case OpTypeOpaque:
	case OpTypeFunction:
	case OpTypePipe:
	case OpTypePipeStorage:
		TODO_ERROR();

	default:
		TODO_ERROR();
	}
}

bool SPIRVCompiledModuleBuilder::HasSpecOverride(const SPIRV::SPIRVValue* spirvValue)
{
	if (specializationInfo == nullptr || !spirvValue->hasDecorate(DecorationSpecId))
	{
		return false;
	}

	const auto id = *spirvValue->getDecorate(DecorationSpecId).begin();
	for (auto i = 0u; i < specializationInfo->mapEntryCount; i++)
	{
		const auto& entry = specializationInfo->pMapEntries[i];
		if (entry.constantID == id)
		{
			return true;
		}
	}
	return false;
}

LLVMValueRef SPIRVCompiledModuleBuilder::HandleSpecConstantOperation(const SPIRV::SPIRVSpecConstantOp* spirvValue)
{
	auto opwords = spirvValue->getOpWords();
	const auto operation = static_cast<Op>(opwords[0]);
	opwords.erase(opwords.begin(), opwords.begin() + 1);

	switch (operation)
	{
	case OpVectorShuffle:
		{
			const auto v1 = ConvertValue(spirvValue->getModule()->getValue(opwords[0]), nullptr);
			const auto v2 = ConvertValue(spirvValue->getModule()->getValue(opwords[1]), nullptr);
			std::vector<LLVMValueRef> components{};
			for (auto i = 2u; i < opwords.size(); i++)
			{
				const auto index = opwords[i];
				if (index == 0xFFFFFFFF)
				{
					components.push_back(LLVMGetUndef(LLVMInt32TypeInContext(context)));
				}
				else
				{
					components.push_back(ConstU32(index));
				}
			}
			return LLVMConstShuffleVector(v1, v2, LLVMConstVector(components.data(), static_cast<uint32_t>(components.size())));
		}

	case OpCompositeExtract:
		{
			auto composite = ConvertValue(spirvValue->getModule()->getValue(opwords[0]), nullptr);
			for (auto i = 1u; i < opwords.size(); i++)
			{
				const auto index = ConstU32(opwords[i]);
				switch (LLVMGetTypeKind(LLVMTypeOf(composite)))
				{
				case LLVMStructTypeKind:
					TODO_ERROR();

				case LLVMArrayTypeKind:
					TODO_ERROR();

				case LLVMVectorTypeKind:
					assert(i + 1 == opwords.size());
					composite = LLVMConstExtractElement(composite, index);
					break;

				default:
					TODO_ERROR();
				}
			}
			return composite;
		}

	case OpCompositeInsert:
		{
			const auto value = ConvertValue(spirvValue->getModule()->getValue(opwords[0]), nullptr);
			auto composite = ConvertValue(spirvValue->getModule()->getValue(opwords[1]), nullptr);
			for (auto i = 2u; i < opwords.size(); i++)
			{
				const auto index = ConstU32(opwords[i]);
				switch (LLVMGetTypeKind(LLVMTypeOf(composite)))
				{
				case LLVMStructTypeKind:
					TODO_ERROR();

				case LLVMArrayTypeKind:
					TODO_ERROR();

				case LLVMVectorTypeKind:
					assert(i + 1 == opwords.size());
					composite = LLVMConstInsertElement(composite, value, index);
					break;

				default:
					TODO_ERROR();
				}
			}
			return composite;
		}

	case OpSelect:
		{
			assert(opwords.size() == 3);
			const auto c = ConvertValue(spirvValue->getModule()->getValue(opwords[0]), nullptr);
			const auto t = ConvertValue(spirvValue->getModule()->getValue(opwords[1]), nullptr);
			const auto f = ConvertValue(spirvValue->getModule()->getValue(opwords[2]), nullptr);
			return LLVMConstSelect(c, t, f);
		}

	case OpAccessChain:
		TODO_ERROR();

	case OpInBoundsAccessChain:
		TODO_ERROR();

	case OpPtrAccessChain:
		TODO_ERROR();

	case OpInBoundsPtrAccessChain:
		TODO_ERROR();

	default:
		{
			const auto instruction = SPIRV::SPIRVInstTemplateBase::create(operation, spirvValue->getType(), spirvValue->getId(), opwords, nullptr,
			                                                              spirvValue->getModule());
			currentBlock = nullptr;
			const auto result = ConvertInstruction(instruction, nullptr);
			if (!LLVMIsConstant(result))
			{
				TODO_ERROR();
			}
			return result;
		}
	}
}

LLVMValueRef SPIRVCompiledModuleBuilder::ConvertValueNoDecoration(const SPIRV::SPIRVValue* spirvValue, LLVMValueRef currentFunction)
{
	switch (spirvValue->getOpCode())
	{
	case OpSpecConstant:
		if (HasSpecOverride(spirvValue))
		{
			const auto spirvConstant = static_cast<const SPIRV::SPIRVConstant*>(spirvValue);
			const auto spirvType = spirvConstant->getType();
			const auto llvmType = ConvertType(spirvType);
			switch (spirvType->getOpCode())
			{
			case OpTypeInt:
				switch (spirvType->getIntegerBitWidth())
				{
				case 8:
					return LLVMConstInt(llvmType, GetSpecOverride<uint8_t>(spirvValue), static_cast<SPIRV::SPIRVTypeInt*>(spirvType)->isSigned());

				case 16:
					return LLVMConstInt(llvmType, GetSpecOverride<uint16_t>(spirvValue), static_cast<SPIRV::SPIRVTypeInt*>(spirvType)->isSigned());

				case 32:
					return LLVMConstInt(llvmType, GetSpecOverride<uint32_t>(spirvValue), static_cast<SPIRV::SPIRVTypeInt*>(spirvType)->isSigned());

				case 64:
					return LLVMConstInt(llvmType, GetSpecOverride<uint64_t>(spirvValue), static_cast<SPIRV::SPIRVTypeInt*>(spirvType)->isSigned());

				default:
					TODO_ERROR();
				}

			case OpTypeFloat:
				switch (spirvType->getFloatBitWidth())
				{
				case 16:
					return LLVMConstReal(llvmType, GetSpecOverride<half>(spirvValue).toFloat());

				case 32:
					return LLVMConstReal(llvmType, GetSpecOverride<float>(spirvValue));

				case 64:
					return LLVMConstReal(llvmType, GetSpecOverride<double>(spirvValue));

				default:
					TODO_ERROR();
				}

			default:
				TODO_ERROR();
			}
		}

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

	case OpSpecConstantTrue:
	case OpSpecConstantFalse:
		if (HasSpecOverride(spirvValue))
		{
			return GetSpecOverride<VkBool32>(spirvValue)
				       ? LLVMConstInt(LLVMInt1TypeInContext(context), 1, false)
				       : LLVMConstInt(LLVMInt1TypeInContext(context), 0, false);
		}

	case OpConstantTrue:
	case OpConstantFalse:
		return LLVMConstInt(LLVMInt1TypeInContext(context), 
		                    spirvValue->getOpCode() == OpConstantTrue || spirvValue->getOpCode() == OpSpecConstantTrue,
		                    false);

	case OpConstantNull:
		return LLVMConstNull(ConvertType(spirvValue->getType()));

	case OpSpecConstantComposite:
		if (HasSpecOverride(spirvValue))
		{
			TODO_ERROR();
		}

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

	case OpSpecConstantOp:
		return HandleSpecConstantOperation(static_cast<const SPIRV::SPIRVSpecConstantOp*>(spirvValue));

	case OpUndef:
		return LLVMGetUndef(ConvertType(spirvValue->getType()));

	case OpVariable:
		{
			const auto variable = static_cast<const SPIRV::SPIRVVariable*>(spirvValue);
			const auto isPointer = (variable->getStorageClass() == StorageClassUniform || variable->getStorageClass() == StorageClassUniformConstant || variable
					->getStorageClass() == StorageClassStorageBuffer) &&
				!IsOpaqueType(variable->getType()->getPointerElementType());
			const auto llvmType = isPointer
				                      ? ConvertType(variable->getType())
				                      : ConvertType(variable->getType()->getPointerElementType());
			const auto linkage = ConvertLinkage(variable);
			const auto name = MangleName(variable);
			LLVMValueRef initialiser = nullptr;
			const auto spirvInitialiser = variable->getInitializer();
			if (spirvInitialiser)
			{
				initialiser = ConvertValue(spirvInitialiser, currentFunction);
			}
			else if (variable->getStorageClass() == StorageClassFunction)
			{
				return CreateAlloca(llvmType);
			}
			else
			{
				initialiser = LLVMConstNull(llvmType);
			}

			if (isPointer)
			{
				variablePointers.insert(variable->getId());
			}

			const auto llvmVariable = GlobalVariable(llvmType,
			                                         variable->isConstant(),
			                                         linkage,
			                                         initialiser,
			                                         name);
			LLVMSetUnnamedAddress(llvmVariable, variable->isConstant() &&
			                                    LLVMGetTypeKind(llvmType) == LLVMArrayTypeKind &&
			                                    LLVMGetTypeKind(LLVMGetElementType(llvmType)) == LLVMIntegerTypeKind &&
			                                    LLVMGetIntTypeWidth(LLVMGetElementType(llvmType)) == 8
				                                    ? LLVMGlobalUnnamedAddr
				                                    : LLVMNoUnnamedAddr);
			return llvmVariable;
		}

	case OpFunctionParameter:
		{
			const auto functionParameter = static_cast<const SPIRV::SPIRVFunctionParameter*>(spirvValue);
			assert(currentFunction);
			return LLVMGetParam(currentFunction, functionParameter->getArgNo());
		}

	case OpFunction:
		// return mapValue(BV, transFunction(static_cast<SPIRVFunction*>(BV)));
		TODO_ERROR();

	case OpLabel:
		// return mapValue(BV, BasicBlock::Create(*Context, BV->getName(), F));
		TODO_ERROR();

	default:
		TODO_ERROR();
	}
}

void SPIRVCompiledModuleBuilder::ConvertDecoration(LLVMValueRef llvmValue, const SPIRV::SPIRVValue* spirvValue)
{
	if (LLVMIsAAllocaInst(llvmValue) || LLVMIsAGlobalVariable(llvmValue))
	{
		uint32_t alignment;
		if (spirvValue->hasAlignment(&alignment))
		{
			LLVMSetAlignment(llvmValue, alignment);
		}
	}
}

LLVMValueRef SPIRVCompiledModuleBuilder::ConvertBuiltin(BuiltIn builtin)
{
	for (const auto mapping : builtinInputMapping)
	{
		if (mapping.first == builtin)
		{
			return CreateInBoundsGEP(builtinInputVariable, 0, mapping.second);
		}
	}

	for (const auto mapping : builtinOutputMapping)
	{
		if (mapping.first == builtin)
		{
			return CreateInBoundsGEP(builtinOutputVariable, 0, mapping.second);
		}
	}

	TODO_ERROR();
}

LLVMValueRef SPIRVCompiledModuleBuilder::ConvertValue(const SPIRV::SPIRVValue* spirvValue, LLVMValueRef currentFunction)
{
	const auto cachedType = valueMapping.find(spirvValue->getId());
	if (cachedType != valueMapping.end())
	{
		if (spirvValue->hasId() && variablePointers.find(spirvValue->getId()) != variablePointers.end())
		{
			if (const auto variable = LLVMIsAGlobalVariable(cachedType->second))
			{
				return CreateLoad(variable);
			}
		}
		return cachedType->second;
	}

	if (spirvValue->isVariable() && spirvValue->getType()->isTypePointer())
	{
		if (spirvValue->hasDecorate(DecorationBuiltIn))
		{
			const auto builtin = static_cast<BuiltIn>(*spirvValue->getDecorate(DecorationBuiltIn).begin());
			if (currentBlock == nullptr)
			{
				return nullptr;
			}
			return ConvertBuiltin(builtin);
		}

		if (spirvValue->getType()->getPointerElementType()->hasMemberDecorate(DecorationBuiltIn))
		{
			if (static_cast<const SPIRV::SPIRVVariable*>(spirvValue)->getStorageClass() == StorageClassInput)
			{
				return valueMapping[spirvValue->getId()] = builtinInputVariable;
			}
			if (static_cast<const SPIRV::SPIRVVariable*>(spirvValue)->getStorageClass() == StorageClassOutput)
			{
				return valueMapping[spirvValue->getId()] = builtinOutputVariable;
			}
			TODO_ERROR();
		}
	}

	const auto llvmValue = ConvertValueNoDecoration(spirvValue, currentFunction);
	if (!spirvValue->getName().empty() && !HasName(llvmValue))
	{
		SetName(llvmValue, spirvValue->getName());
	}

	ConvertDecoration(llvmValue, spirvValue);

	valueMapping[spirvValue->getId()] = llvmValue;
	return llvmValue;
}

std::vector<LLVMValueRef> SPIRVCompiledModuleBuilder::ConvertValue(std::vector<SPIRV::SPIRVValue*> spirvValues, LLVMValueRef currentFunction)
{
	std::vector<LLVMValueRef> results{spirvValues.size(), nullptr};
	for (size_t i = 0; i < spirvValues.size(); i++)
	{
		results[i] = ConvertValue(spirvValues[i], currentFunction);
	}
	return results;
}

bool SPIRVCompiledModuleBuilder::NeedsPointer(const SPIRV::SPIRVType* type)
{
	return type->isTypeMatrix() || type->isTypeVector() || type->isTypeStruct();
}

LLVMValueRef SPIRVCompiledModuleBuilder::GetInbuiltFunction(const std::string& functionNamePrefix, SPIRV::SPIRVType* returnType,
                                                            const std::vector<std::pair<const char*, const SPIRV::SPIRVType*>>& arguments, bool hasUserData)
{
	auto functionName = functionNamePrefix + "." + GetTypeName(returnType);
	for (const auto& argument : arguments)
	{
		functionName += '.';
		if (argument.first != nullptr)
		{
			functionName += argument.first;
			functionName += '.';
		}
		functionName += GetTypeName(argument.second);
	}

	const auto cachedFunction = functionCache.find(functionName);
	if (cachedFunction != functionCache.end())
	{
		return cachedFunction->second;
	}

	const auto returnNeedsPointer = NeedsPointer(returnType);
	const auto llvmReturnType = returnNeedsPointer ? LLVMVoidTypeInContext(context) : ConvertType(returnType);

	std::vector<LLVMTypeRef> params{arguments.size() + hasUserData + returnNeedsPointer};
	auto i = 0u;

	if (hasUserData)
	{
		params[i++] = LLVMPointerType(LLVMInt8TypeInContext(context), 0);
	}

	if (returnNeedsPointer)
	{
		params[i++] = LLVMPointerType(ConvertType(returnType), 0);
	}

	for (const auto& argument : arguments)
	{
		auto type = ConvertType(argument.second);
		if (NeedsPointer(argument.second))
		{
			type = LLVMPointerType(type, 0);
		}
		params[i++] = type;
	}

	const auto functionType = LLVMFunctionType(llvmReturnType, params.data(), static_cast<uint32_t>(params.size()), false);
	const auto function = LLVMAddFunction(module, functionName.c_str(), functionType);
	LLVMSetLinkage(function, LLVMExternalLinkage);
	functionCache[functionName] = function;
	return function;
}

LLVMValueRef SPIRVCompiledModuleBuilder::CallInbuiltFunction(LLVMValueRef function, SPIRV::SPIRVType* returnType,
                                                             const std::vector<std::pair<const SPIRV::SPIRVType*, LLVMValueRef>>& arguments, bool hasUserData)
{
	const auto returnNeedsPointer = NeedsPointer(returnType);
	std::vector<LLVMValueRef> params{arguments.size() + hasUserData + returnNeedsPointer};
	auto i = 0u;

	if (hasUserData)
	{
		params[i++] = CreateLoad(userData);
	}

	if (returnNeedsPointer)
	{
		params[i++] = LLVMBuildAlloca(builder, ConvertType(returnType), "");
	}

	for (const auto& argument : arguments)
	{
		if (NeedsPointer(argument.first))
		{
			params[i] = LLVMBuildAlloca(builder, ConvertType(argument.first), "");
			LLVMBuildStore(builder, argument.second, params[i]);
		}
		else
		{
			params[i] = argument.second;
		}

		i += 1;
	}

	auto result = LLVMBuildCall(builder, function, params.data(), static_cast<uint32_t>(params.size()), "");
	if (returnNeedsPointer)
	{
		result = LLVMBuildLoad(builder, params[hasUserData], "");
	}
	return result;
}

LLVMValueRef SPIRVCompiledModuleBuilder::CallInbuiltFunction(SPIRV::SPIRVMatrixTimesScalar* matrixTimesScalar, LLVMValueRef currentFunction)
{
	const auto matrixType = matrixTimesScalar->getMatrix()->getType();
	const auto scalarType = matrixTimesScalar->getScalar()->getType();

	const auto function = GetInbuiltFunction("@Matrix.Mult", matrixTimesScalar->getType(), {
		                                         {nullptr, matrixType},
		                                         {nullptr, scalarType},
	                                         });

	const auto matrixValue = ConvertValue(matrixTimesScalar->getMatrix(), currentFunction);
	const auto scalarValue = ConvertValue(matrixTimesScalar->getScalar(), currentFunction);

	return CallInbuiltFunction(function, matrixTimesScalar->getType(), {
		                           {matrixType, matrixValue},
		                           {scalarType, scalarValue},
	                           });
}

LLVMValueRef SPIRVCompiledModuleBuilder::CallInbuiltFunction(SPIRV::SPIRVVectorTimesMatrix* vectorTimesMatrix, LLVMValueRef currentFunction)
{
	const auto vectorType = vectorTimesMatrix->getVector()->getType();
	const auto matrixType = vectorTimesMatrix->getMatrix()->getType();

	const auto function = GetInbuiltFunction("@Matrix.Mult", vectorTimesMatrix->getType(), {
		                                         {nullptr, vectorType},
		                                         {nullptr, matrixType},
	                                         });

	const auto vectorValue = ConvertValue(vectorTimesMatrix->getVector(), currentFunction);
	const auto matrixValue = ConvertValue(vectorTimesMatrix->getMatrix(), currentFunction);

	return CallInbuiltFunction(function, vectorTimesMatrix->getType(), {
		                           {vectorType, vectorValue},
		                           {matrixType, matrixValue},
	                           });
}

LLVMValueRef SPIRVCompiledModuleBuilder::CallInbuiltFunction(SPIRV::SPIRVMatrixTimesVector* matrixTimesVector, LLVMValueRef currentFunction)
{
	const auto matrixType = matrixTimesVector->getMatrix()->getType();
	const auto vectorType = matrixTimesVector->getVector()->getType();

	const auto function = GetInbuiltFunction("@Matrix.Mult", matrixTimesVector->getType(), {
		                                         {nullptr, matrixType},
		                                         {nullptr, vectorType},
	                                         });

	const auto matrixValue = ConvertValue(matrixTimesVector->getMatrix(), currentFunction);
	const auto vectorValue = ConvertValue(matrixTimesVector->getVector(), currentFunction);

	return CallInbuiltFunction(function, matrixTimesVector->getType(), {
		                           {matrixType, matrixValue},
		                           {vectorType, vectorValue},
	                           });
}

LLVMValueRef SPIRVCompiledModuleBuilder::CallInbuiltFunction(SPIRV::SPIRVMatrixTimesMatrix* matrixTimesMatrix, LLVMValueRef currentFunction)
{
	const auto lhsType = matrixTimesMatrix->getMatrixLeft()->getType();
	const auto rhsType = matrixTimesMatrix->getMatrixRight()->getType();

	const auto function = GetInbuiltFunction("@Matrix.Mult", matrixTimesMatrix->getType(), {
		                                         {nullptr, lhsType},
		                                         {nullptr, rhsType},
	                                         });

	const auto lhsValue = ConvertValue(matrixTimesMatrix->getMatrixLeft(), currentFunction);
	const auto rhsValue = ConvertValue(matrixTimesMatrix->getMatrixRight(), currentFunction);

	return CallInbuiltFunction(function, matrixTimesMatrix->getType(), {
		                           {lhsType, lhsValue},
		                           {rhsType, rhsValue},
	                           });
}

LLVMValueRef SPIRVCompiledModuleBuilder::CallInbuiltFunction(SPIRV::SPIRVDot* dot, LLVMValueRef currentFunction)
{
	const auto argumentType = dot->getOperand(0)->getType();

	const auto function = GetInbuiltFunction("@Vector.Dot", dot->getType(), {
		                                         {nullptr, argumentType},
		                                         {nullptr, argumentType},
	                                         });

	const auto lhsValue = ConvertValue(dot->getOperand(0), currentFunction);
	const auto rhsValue = ConvertValue(dot->getOperand(1), currentFunction);

	return CallInbuiltFunction(function, dot->getType(), {
		                           {argumentType, lhsValue},
		                           {argumentType, rhsValue},
	                           });
}

LLVMValueRef SPIRVCompiledModuleBuilder::CallInbuiltFunction(SPIRV::SPIRVImageSampleImplicitLod* imageSampleImplicitLod, LLVMValueRef currentFunction)
{
	const auto spirvImageType = imageSampleImplicitLod->getOpValue(0)->getType();
	const auto coordinateType = imageSampleImplicitLod->getOpValue(1)->getType();

	if (imageSampleImplicitLod->getOpWords().size() > 2)
	{
		TODO_ERROR();
	}

	const auto function = GetInbuiltFunction("@Image.Sample.Implicit", imageSampleImplicitLod->getType(), {
		                                         {nullptr, spirvImageType},
		                                         {nullptr, coordinateType},
	                                         }, true);

	return CallInbuiltFunction(function, imageSampleImplicitLod->getType(), {
		                           {spirvImageType, ConvertValue(imageSampleImplicitLod->getOpValue(0), currentFunction)},
		                           {coordinateType, ConvertValue(imageSampleImplicitLod->getOpValue(1), currentFunction)},
	                           }, true);
}

LLVMValueRef SPIRVCompiledModuleBuilder::CallInbuiltFunction(SPIRV::SPIRVImageSampleExplicitLod* imageSampleExplicitLod, LLVMValueRef currentFunction)
{
	const auto spirvImageType = imageSampleExplicitLod->getOpValue(0)->getType();
	const auto coordinateType = imageSampleExplicitLod->getOpValue(1)->getType();
	const auto operand = imageSampleExplicitLod->getOpWord(2);

	if (operand == 2)
	{
		if (imageSampleExplicitLod->getOpWords().size() > 4)
		{
			TODO_ERROR();
		}

		auto float32 = SPIRV::SPIRVTypeFloat(32);

		const auto function = GetInbuiltFunction("@Image.Sample.Explicit", imageSampleExplicitLod->getType(), {
			                                         {nullptr, spirvImageType},
			                                         {nullptr, coordinateType},
			                                         {"Lod", &float32},
		                                         }, true);

		return CallInbuiltFunction(function, imageSampleExplicitLod->getType(), {
			                           {spirvImageType, ConvertValue(imageSampleExplicitLod->getOpValue(0), currentFunction)},
			                           {coordinateType, ConvertValue(imageSampleExplicitLod->getOpValue(1), currentFunction)},
			                           {&float32, ConvertValue(imageSampleExplicitLod->getOpValue(3), currentFunction)},
		                           }, true);
	}

	TODO_ERROR();
}

LLVMValueRef SPIRVCompiledModuleBuilder::CallInbuiltFunction(SPIRV::SPIRVImageFetch* imageFetch, LLVMValueRef currentFunction)
{
	const auto spirvImageType = imageFetch->getOpValue(0)->getType();
	const auto coordinateType = imageFetch->getOpValue(1)->getType();

	if (imageFetch->getOpWords().size() > 2)
	{
		TODO_ERROR();
	}

	const auto function = GetInbuiltFunction("@Image.Fetch", imageFetch->getType(), {
		                                         {nullptr, spirvImageType},
		                                         {nullptr, coordinateType},
	                                         }, true);

	return CallInbuiltFunction(function, imageFetch->getType(), {
		                           {spirvImageType, ConvertValue(imageFetch->getOpValue(0), currentFunction)},
		                           {coordinateType, ConvertValue(imageFetch->getOpValue(1), currentFunction)},
	                           }, true);
}

LLVMValueRef SPIRVCompiledModuleBuilder::CallInbuiltFunction(SPIRV::SPIRVImageRead* imageRead, LLVMValueRef currentFunction)
{
	const auto spirvImageType = imageRead->getOpValue(0)->getType();
	const auto coordinateType = imageRead->getOpValue(1)->getType();

	if (imageRead->getOpWords().size() > 2)
	{
		TODO_ERROR();
	}

	const auto function = GetInbuiltFunction("@Image.Read", imageRead->getType(), {
		                                         {nullptr, spirvImageType},
		                                         {nullptr, coordinateType},
	                                         }, true);

	return CallInbuiltFunction(function, imageRead->getType(), {
		                           {spirvImageType, ConvertValue(imageRead->getOpValue(0), currentFunction)},
		                           {coordinateType, ConvertValue(imageRead->getOpValue(1), currentFunction)},
	                           }, true);
}

std::vector<LLVMValueRef> SPIRVCompiledModuleBuilder::MapBuiltin(BuiltIn builtin, const std::vector<std::pair<BuiltIn, uint32_t>>& builtinMapping)
{
	for (const auto mapping : builtinMapping)
	{
		if (mapping.first == builtin)
		{
			return std::vector<LLVMValueRef>
			{
				LLVMConstInt(LLVMInt32TypeInContext(context), mapping.second, false)
			};
		}
	}

	TODO_ERROR();
}

std::vector<LLVMValueRef> SPIRVCompiledModuleBuilder::MapBuiltin(const std::vector<LLVMValueRef>& indices, SPIRV::SPIRVType* type,
                                                                 const std::vector<std::pair<BuiltIn, uint32_t>>& builtinMapping)
{
	const auto structType = type->getPointerElementType();
	if (!structType->isTypeStruct())
	{
		TODO_ERROR();
	}

	auto decorates = static_cast<SPIRV::SPIRVTypeStruct*>(structType)->getMemberDecorates();

	if (LLVMIsAConstantInt(indices[0]))
	{
		const auto index = LLVMConstIntGetZExtValue(indices[0]);
		for (const auto decorate : decorates)
		{
			if (decorate.first.first == index && decorate.first.second == DecorationBuiltIn)
			{
				auto newIndices = MapBuiltin(static_cast<BuiltIn>(decorate.second->getLiteral(0)), builtinMapping);
				for (auto i = 1u; i < indices.size(); i++)
				{
					newIndices.push_back(indices[i]);
				}
				return newIndices;
			}
		}
	}

	TODO_ERROR();
}

LLVMValueRef SPIRVCompiledModuleBuilder::ConvertOCLFromExtensionInstruction(const SPIRV::SPIRVExtInst* extensionInstruction, LLVMValueRef currentFunction)
{
	TODO_ERROR();
	// assert(BB && "Invalid BB");
	// std::string MangledName;
	// SPIRVWord EntryPoint = BC->getExtOp();
	// bool IsVarArg = false;
	// bool IsPrintf = false;
	// std::string UnmangledName;
	// auto BArgs = BC->getArguments();
	//
	// assert(BM->getBuiltinSet(BC->getExtSetId()) == SPIRVEIS_OpenCL &&
	//        "Not OpenCL extended instruction");
	// if (EntryPoint == OpenCLLIB::Printf)
	//   IsPrintf = true;
	// else {
	//   UnmangledName = OCLExtOpMap::map(static_cast<OCLExtOpKind>(EntryPoint));
	// }
	//
	// SPIRVDBG(spvdbgs() << "[transOCLBuiltinFromExtInst] OrigUnmangledName: "
	//                    << UnmangledName << '\n');
	// transOCLVectorLoadStore(UnmangledName, BArgs);
	//
	// std::vector<Type *> ArgTypes = transTypeVector(BC->getValueTypes(BArgs));
	//
	// if (IsPrintf) {
	//   MangledName = "printf";
	//   IsVarArg = true;
	//   ArgTypes.resize(1);
	// } else if (UnmangledName.find("read_image") == 0) {
	//   auto ModifiedArgTypes = ArgTypes;
	//   ModifiedArgTypes[1] = getOrCreateOpaquePtrType(M, "opencl.sampler_t");
	//   mangleOpenClBuiltin(UnmangledName, ModifiedArgTypes, MangledName);
	// } else {
	//   mangleOpenClBuiltin(UnmangledName, ArgTypes, MangledName);
	// }
	// SPIRVDBG(spvdbgs() << "[transOCLBuiltinFromExtInst] ModifiedUnmangledName: "
	//                    << UnmangledName << " MangledName: " << MangledName
	//                    << '\n');
	//
	// FunctionType *FT =
	//     FunctionType::get(transType(BC->getType()), ArgTypes, IsVarArg);
	// Function *F = M->getFunction(MangledName);
	// if (!F) {
	//   F = Function::Create(FT, GlobalValue::ExternalLinkage, MangledName, M);
	//   F->setCallingConv(CallingConv::SPIR_FUNC);
	//   if (isFuncNoUnwind())
	//     F->addFnAttr(Attribute::NoUnwind);
	// }
	// auto Args = transValue(BC->getValues(BArgs), F, BB);
	// SPIRVDBG(dbgs() << "[transOCLBuiltinFromExtInst] Function: " << *F
	//                 << ", Args: ";
	//          for (auto &I
	//               : Args) dbgs()
	//          << *I << ", ";
	//          dbgs() << '\n');
	// CallInst *Call = CallInst::Create(F, Args, BC->getName(), BB);
	// setCallingConv(Call);
	// addFnAttr(Call, Attribute::NoUnwind);
	// return transOCLBuiltinPostproc(BC, Call, BB, UnmangledName);
}

LLVMValueRef SPIRVCompiledModuleBuilder::ConvertOGLFromExtensionInstruction(const SPIRV::SPIRVExtInst* extensionInstruction, LLVMValueRef currentFunction)
{
	const auto entryPoint = static_cast<OpenGL::Entrypoints>(extensionInstruction->getExtOp());
	const auto functionName = "@" + SPIRV::OGLExtOpMap::map(entryPoint);

	std::vector<std::pair<const char*, const SPIRV::SPIRVType*>> argumentTypes{};
	for (const auto type : extensionInstruction->getArgumentValueTypes())
	{
		argumentTypes.emplace_back(std::make_pair(nullptr, type));
	}

	const auto function = GetInbuiltFunction(functionName, extensionInstruction->getType(), argumentTypes);

	std::vector<std::pair<const SPIRV::SPIRVType*, LLVMValueRef>> arguments{};
	for (auto i = 0u; i < extensionInstruction->getArguments().size(); i++)
	{
		arguments.emplace_back(std::make_pair(extensionInstruction->getArgumentValueTypes()[i],
		                                      ConvertValue(extensionInstruction->getArgumentValues()[i], currentFunction)));
	}
	return CallInbuiltFunction(function, extensionInstruction->getType(), arguments);
}

LLVMValueRef SPIRVCompiledModuleBuilder::ConvertDebugFromExtensionInstruction(const SPIRV::SPIRVExtInst* extensionInstruction, LLVMValueRef currentFunction)
{
	TODO_ERROR();
	// auto GetLocalVar = [&](SPIRVId Id) -> std::pair<DILocalVariable *, DebugLoc> {
	//   auto *LV = transDebugInst<DILocalVariable>(BM->get<SPIRVExtInst>(Id));
	//   DebugLoc DL = DebugLoc::get(LV->getLine(), 0, LV->getScope());
	//   return std::make_pair(LV, DL);
	// };
	// auto GetValue = [&](SPIRVId Id) -> Value * {
	//   auto *V = BM->get<SPIRVValue>(Id);
	//   return SPIRVReader->transValue(V, BB->getParent(), BB);
	// };
	// auto GetExpression = [&](SPIRVId Id) -> DIExpression * {
	//   return transDebugInst<DIExpression>(BM->get<SPIRVExtInst>(Id));
	// };
	// SPIRVWordVec Ops = DebugInst->getArguments();
	// switch (DebugInst->getExtOp()) {
	// case SPIRVDebug::Scope:
	// case SPIRVDebug::NoScope:
	//   return nullptr;
	// case SPIRVDebug::Declare: {
	//   using namespace SPIRVDebug::Operand::DebugDeclare;
	//   auto LocalVar = GetLocalVar(Ops[DebugLocalVarIdx]);
	//   if (getDbgInst<SPIRVDebug::DebugInfoNone>(Ops[VariableIdx])) {
	//     // If we don't have the variable(e.g. alloca might be promoted by mem2reg)
	//     // we should generate the following IR:
	//     // call void @llvm.dbg.declare(metadata !4, metadata !14, metadata !5)
	//     // !4 = !{}
	//     // DIBuilder::insertDeclare doesn't allow to pass nullptr for the Storage
	//     // parameter. To work around this limitation we create a dummy temp
	//     // alloca, use it to create llvm.dbg.declare, and then remove the alloca.
	//     auto *AI = new AllocaInst(Type::getInt8Ty(M->getContext()), 0, "tmp", BB);
	//     auto *DbgDeclare = Builder.insertDeclare(
	//         AI, LocalVar.first, GetExpression(Ops[ExpressionIdx]),
	//         LocalVar.second, BB);
	//     AI->eraseFromParent();
	//     return DbgDeclare;
	//   }
	//   return Builder.insertDeclare(GetValue(Ops[VariableIdx]), LocalVar.first,
	//                                GetExpression(Ops[ExpressionIdx]),
	//                                LocalVar.second, BB);
	// }
	// case SPIRVDebug::Value: {
	//   using namespace SPIRVDebug::Operand::DebugValue;
	//   auto LocalVar = GetLocalVar(Ops[DebugLocalVarIdx]);
	//   return Builder.insertDbgValueIntrinsic(
	//       GetValue(Ops[ValueIdx]), LocalVar.first,
	//       GetExpression(Ops[ExpressionIdx]), LocalVar.second, BB);
	// }
	// default:
	//   llvm_unreachable("Unknown debug intrinsic!");
	// }
}

// static llvm::Metadata* getMetadataFromName(State& state, const std::string& name)
// {
// 	return llvm::MDNode::get(state.context, llvm::MDString::get(state.context, name));
// }
//
// static std::vector<llvm::Metadata*> getMetadataFromNameAndParameter(State& state, const std::string& name, uint32_t parameter)
// {
// 	return
// 	{
// 		llvm::MDString::get(state.context, name),
// 		llvm::ConstantAsMetadata::get(llvm::ConstantInt::get(llvm::Type::getInt32Ty(state.context), parameter))
// 	};
// }

LLVMAtomicOrdering SPIRVCompiledModuleBuilder::ConvertSemantics(uint32_t semantics)
{
	if (semantics & 0x10)
	{
		return LLVMAtomicOrderingSequentiallyConsistent;
	}

	if (semantics & 0x08)
	{
		return LLVMAtomicOrderingAcquireRelease;
	}

	if (semantics & 0x04)
	{
		return LLVMAtomicOrderingRelease;
	}

	if (semantics & 0x02)
	{
		return LLVMAtomicOrderingAcquire;
	}

	return LLVMAtomicOrderingMonotonic;
}

bool SPIRVCompiledModuleBuilder::TranslateNonTemporalMetadata(LLVMValueRef instruction)
{
	auto metadata = LLVMValueAsMetadata(ConstI32(1));
	const auto node = LLVMMDNodeInContext2(context, &metadata, 1);
	const auto metadataId = LLVMGetMDKindIDInContext(context, "nontemporal", sizeof("nontemporal") - 1);
	LLVMSetMetadata(instruction, metadataId, LLVMMetadataAsValue(context, node));
	return true;
}

LLVMValueRef SPIRVCompiledModuleBuilder::GetConstantFloatOrVector(LLVMTypeRef type, double value)
{
	// const auto llvmValue = LLVMConstReal(type, value);
	// if (type->isVectorTy())
	// {
	// 	return llvm::ConstantVector::getSplat(type->getVectorNumElements(), llvmValue);
	// }
	// return llvmValue;
	TODO_ERROR();
}

void SPIRVCompiledModuleBuilder::MoveBuilder(LLVMBasicBlockRef basicBlock, SPIRV::SPIRVInstruction* instruction)
{
	currentBlock = basicBlock;
	LLVMPositionBuilderAtEnd(builder, basicBlock);
	AddDebugInformation(instruction);
}

LLVMValueRef SPIRVCompiledModuleBuilder::ConvertInstruction(SPIRV::SPIRVInstruction* instruction, LLVMValueRef currentFunction)
{
	switch (instruction->getOpCode())
	{
	case OpNop:
	case OpLoopMerge:
	case OpSelectionMerge:
	case OpLabel:
	case OpNoLine:
		return nullptr;

	case OpExtInst:
		{
			const auto extensionInstruction = reinterpret_cast<SPIRV::SPIRVExtInst*>(instruction);
			switch (extensionInstruction->getExtSetKind())
			{
			case SPIRV::SPIRVEIS_OpenCL:
				return ConvertOCLFromExtensionInstruction(extensionInstruction, currentFunction);

			case SPIRV::SPIRVEIS_OpenGL:
				return ConvertOGLFromExtensionInstruction(extensionInstruction, currentFunction);

			case SPIRV::SPIRVEIS_Debug:
				return ConvertDebugFromExtensionInstruction(extensionInstruction, currentFunction);

			default:
				TODO_ERROR();
			}
		}

	case OpFunctionCall:
		{
			const auto functionCall = reinterpret_cast<SPIRV::SPIRVFunctionCall*>(instruction);
			const auto llvmBasicBlock = currentBlock;
			const auto llvmFunctionType = ConvertFunction(functionCall->getFunction());
			MoveBuilder(llvmBasicBlock, instruction);
			return CreateCall(llvmFunctionType, ConvertValue(functionCall->getArgumentValues(), currentFunction));
		}

	case OpVariable:
		// TODO: Alloca?
		return ConvertValueNoDecoration(instruction, currentFunction);

		// case OpImageTexelPointer: break;

	case OpLoad:
		{
			const auto load = reinterpret_cast<SPIRV::SPIRVLoad*>(instruction);
			const auto llvmValue = CreateLoad(ConvertValue(load->getSrc(), currentFunction), load->SPIRVMemoryAccess::isVolatile(), load->getName());
			if (load->isNonTemporal())
			{
				TranslateNonTemporalMetadata(llvmValue);
			}
			return llvmValue;
		}

	case OpStore:
		{
			const auto store = reinterpret_cast<SPIRV::SPIRVStore*>(instruction);
			const auto llvmValue = CreateStore(ConvertValue(store->getSrc(), currentFunction),
			                                   ConvertValue(store->getDst(), currentFunction),
			                                   store->SPIRVMemoryAccess::isVolatile());
			if (store->isNonTemporal())
			{
				TranslateNonTemporalMetadata(llvmValue);
			}
			return llvmValue;
		}

	case OpCopyMemory:
		{
			const auto copyMemory = reinterpret_cast<SPIRV::SPIRVCopyMemory*>(instruction);
			const auto dst = ConvertValue(copyMemory->getTarget(), currentFunction);
			const auto src = ConvertValue(copyMemory->getSource(), currentFunction);
			const auto dstAlign = copyMemory->getTargetMemoryAccessMask() & MemoryAccessAlignedMask ? copyMemory->getTargetAlignment() : ALIGNMENT;
			const auto srcAlign = copyMemory->getSourceMemoryAccessMask() & MemoryAccessAlignedMask ? copyMemory->getSourceAlignment() : ALIGNMENT;
			const auto size = LLVMSizeOfTypeInBits(jit->getDataLayout(), LLVMGetElementType(LLVMTypeOf(dst))) / 8;
			return CreateMemCpy(dst, dstAlign, src, srcAlign, ConstU64(size),
			                    (copyMemory->getTargetMemoryAccessMask() & MemoryAccessVolatileMask) || (copyMemory->getSourceMemoryAccessMask() &
				                    MemoryAccessVolatileMask));
		}

		// case OpCopyMemorySized: break;

	case OpAccessChain:
	case OpInBoundsAccessChain:
	case OpPtrAccessChain:
	case OpInBoundsPtrAccessChain:
		{
			const auto accessChain = reinterpret_cast<SPIRV::SPIRVAccessChainBase*>(instruction);
			const auto base = ConvertValue(accessChain->getBase(), currentFunction);
			auto indices = ConvertValue(accessChain->getIndices(), currentFunction);

			if (LLVMTypeOf(base) == LLVMTypeOf(builtinInputVariable))
			{
				indices = MapBuiltin(indices, accessChain->getBase()->getType(), builtinInputMapping);
			}

			if (LLVMTypeOf(base) == LLVMTypeOf(builtinOutputVariable))
			{
				indices = MapBuiltin(indices, accessChain->getBase()->getType(), builtinOutputMapping);
			}

			if (!accessChain->hasPtrIndex())
			{
				indices.insert(indices.begin(), LLVMConstInt(LLVMInt32TypeInContext(context), 0, false));
			}

			// TODO: In Bounds

			auto currentType = LLVMTypeOf(base);
			auto llvmValue = base;
			assert(LLVMGetTypeKind(currentType) == LLVMPointerTypeKind);
			currentType = LLVMGetElementType(currentType);
			auto startIndex = 0u;
			for (auto k = 1u; k <= indices.size(); k++)
			{
				if (k == indices.size())
				{
					llvmValue = LLVMBuildGEP(builder, llvmValue, indices.data() + startIndex, k - startIndex, accessChain->getName().c_str());
					break;
				}

				switch (LLVMGetTypeKind(currentType))
				{
				case LLVMStructTypeKind:
					{
						if (LLVMIsConstant(indices[k]))
						{
							auto index = static_cast<uint32_t>(LLVMConstIntGetZExtValue(indices[k]));
							if (structIndexMapping.find(currentType) != structIndexMapping.end())
							{
								index = structIndexMapping.at(currentType).at(index);
								indices[k] = LLVMConstInt(LLVMInt32TypeInContext(context), index, false);
							}

							currentType = LLVMStructGetTypeAtIndex(currentType, index);
						}
						else
						{
							const auto structName = std::basic_string_view{LLVMGetStructName(currentType)};
							if (structName.rfind("@Matrix", 0) != 0)
							{
								TODO_ERROR();
							}

							std::vector<LLVMValueRef> range{};
							for (auto i = startIndex; i < k; i++)
							{
								range.push_back(indices[i]);
							}
							range.push_back(ConstI32(0));
							llvmValue = CreateGEP(llvmValue, range);
							currentType = LLVMStructGetTypeAtIndex(currentType, 0);
							startIndex = k;
						}
						break;
					}

				case LLVMArrayTypeKind:
					if (arrayStrideMultiplier.find(currentType) != arrayStrideMultiplier.end())
					{
						const auto multiplier = arrayStrideMultiplier[currentType];
						indices[k] = CreateMul(indices[k], LLVMConstInt(LLVMTypeOf(indices[k]), multiplier, false));
					}
					currentType = LLVMGetElementType(currentType);
					break;

				case LLVMVectorTypeKind:
					currentType = LLVMGetElementType(currentType);
					break;

				default: TODO_ERROR();
				}
			}

			return llvmValue;
		}

		// case OpArrayLength: break;
		// case OpGenericPtrMemSemantics: break;
		// case OpDecorate: break;
		// case OpMemberDecorate: break;
		// case OpDecorationGroup: break;
		// case OpGroupDecorate: break;
		// case OpGroupMemberDecorate: break;

	case OpVectorExtractDynamic:
		{
			auto vectorExtractDynamic = static_cast<SPIRV::SPIRVVectorExtractDynamic*>(instruction);
			return CreateExtractElement(ConvertValue(vectorExtractDynamic->getVector(), currentFunction),
			                            ConvertValue(vectorExtractDynamic->getIndex(), currentFunction));
		}

	case OpVectorInsertDynamic:
		{
			auto vectorInsertDynamic = static_cast<SPIRV::SPIRVVectorInsertDynamic*>(instruction);
			return CreateInsertElement(ConvertValue(vectorInsertDynamic->getVector(), currentFunction),
			                           ConvertValue(vectorInsertDynamic->getComponent(), currentFunction),
			                           ConvertValue(vectorInsertDynamic->getIndex(), currentFunction));
		}

	case OpVectorShuffle:
		{
			const auto vectorShuffle = reinterpret_cast<SPIRV::SPIRVVectorShuffle*>(instruction);

			const auto vector1 = ConvertValue(vectorShuffle->getVector1(), currentFunction);
			const auto vector2 = ConvertValue(vectorShuffle->getVector2(), currentFunction);

			assert(LLVMGetTypeKind(LLVMTypeOf(vector1)) == LLVMVectorTypeKind);
			assert(LLVMGetTypeKind(LLVMTypeOf(vector2)) == LLVMVectorTypeKind);
			assert(LLVMGetElementType(LLVMTypeOf(vector1)) == LLVMGetElementType(LLVMTypeOf(vector2)));

			if (LLVMGetVectorSize(LLVMTypeOf(vector1)) != LLVMGetVectorSize(LLVMTypeOf(vector2)))
			{
				auto v1Size = LLVMGetVectorSize(LLVMTypeOf(vector1));
				auto llvmValue = LLVMGetUndef(LLVMVectorType(LLVMGetElementType(LLVMTypeOf(vector1)), vectorShuffle->getComponents().size()));
				for (auto i = 0u; i < vectorShuffle->getComponents().size(); i++)
				{
					const auto componentIndex = vectorShuffle->getComponents()[i];
					if (componentIndex != 0xFFFFFFFF)
					{
						if (componentIndex < v1Size)
						{
							llvmValue = CreateInsertElement(llvmValue, CreateExtractElement(vector1, ConstI32(componentIndex)), ConstI32(i));
						}
						else
						{
							llvmValue = CreateInsertElement(llvmValue, CreateExtractElement(vector2, ConstI32(componentIndex - v1Size)), ConstI32(i));
						}
					}
				}
				return llvmValue;
			}

			std::vector<LLVMValueRef> components{};
			for (auto component : vectorShuffle->getComponents())
			{
				if (component == 0xFFFFFFFF)
				{
					components.push_back(LLVMGetUndef(LLVMInt32TypeInContext(context)));
				}
				else
				{
					components.push_back(ConstI32(component));
				}
			}

			return CreateShuffleVector(vector1, vector2, LLVMConstVector(components.data(), static_cast<uint32_t>(components.size())));
		}

	case OpCompositeConstruct:
		{
			const auto compositeConstruct = reinterpret_cast<SPIRV::SPIRVCompositeConstruct*>(instruction);

			LLVMValueRef llvmValue;
			switch (compositeConstruct->getType()->getOpCode())
			{
			case OpTypeVector:
				llvmValue = LLVMGetUndef(ConvertType(compositeConstruct->getType()));
				for (auto j = 0u, k = 0u; j < compositeConstruct->getConstituents().size(); j++)
				{
					const auto element = ConvertValue(compositeConstruct->getConstituents()[j], currentFunction);
					if (LLVMGetTypeKind(LLVMTypeOf(element)) == LLVMVectorTypeKind)
					{
						for (auto m = 0u; m < LLVMGetVectorSize(LLVMTypeOf(element)); m++)
						{
							const auto vectorElement = CreateExtractElement(element, ConstI32(m));
							llvmValue = CreateInsertElement(llvmValue, vectorElement, ConstI32(k++));
						}
					}
					else
					{
						llvmValue = CreateInsertElement(llvmValue, element, ConstI32(k++));
					}
				}
				break;

			case OpTypeMatrix:
				llvmValue = CreateAlloca(ConvertType(compositeConstruct->getType()));
				for (auto j = 0u; j < compositeConstruct->getConstituents().size(); j++)
				{
					auto destination = CreateGEP(llvmValue, 0, j);
					auto value = ConvertValue(compositeConstruct->getConstituents()[j], currentFunction);
					CreateStore(value, destination);
				}
				llvmValue = CreateLoad(llvmValue);
				break;

			case OpTypeArray:
				{
					const auto arrayType = ConvertType(compositeConstruct->getType());
					if (arrayStrideMultiplier.find(arrayType) != arrayStrideMultiplier.end())
					{
						// TODO: Support stride
						TODO_ERROR();
					}

					llvmValue = CreateAlloca(arrayType);
					for (auto j = 0u; j < compositeConstruct->getConstituents().size(); j++)
					{
						auto destination = CreateGEP(llvmValue, 0, j);
						auto value = ConvertValue(compositeConstruct->getConstituents()[j], currentFunction);
						CreateStore(value, destination);
					}
					llvmValue = CreateLoad(llvmValue);
					break;
				}

			case OpTypeStruct:
				{
					auto structType = ConvertType(compositeConstruct->getType());
					llvmValue = CreateAlloca(structType);
					for (auto j = 0u; j < compositeConstruct->getConstituents().size(); j++)
					{
						const auto index = structIndexMapping.find(structType) != structIndexMapping.end()
							                   ? structIndexMapping.at(structType).at(j)
							                   : j;

						auto destination = CreateGEP(llvmValue, 0, index);
						auto value = ConvertValue(compositeConstruct->getConstituents()[j], currentFunction);
						CreateStore(value, destination);
					}
					llvmValue = CreateLoad(llvmValue);
					break;
				}

			default:
				TODO_ERROR();
			}
			return llvmValue;
		}

	case OpCompositeExtract:
		{
			const auto compositeExtract = reinterpret_cast<SPIRV::SPIRVCompositeExtract*>(instruction);
			auto composite = ConvertValue(compositeExtract->getComposite(), currentFunction);
			if (compositeExtract->getComposite()->getType()->isTypeVector())
			{
				assert(compositeExtract->getIndices().size() == 1);
				return CreateExtractElement(composite, ConstU32(compositeExtract->getIndices()[0]));
			}

			auto currentType = LLVMTypeOf(composite);
			auto indices = compositeExtract->getIndices();
			for (auto k = 0u; k <= indices.size(); k++)
			{
				if (k == indices.size())
				{
					for (auto index : indices)
					{
						composite = CreateExtractValue(composite, index);
					}
					return composite;
				}

				switch (LLVMGetTypeKind(currentType))
				{
				case LLVMStructTypeKind:
					if (structIndexMapping.find(currentType) != structIndexMapping.end())
					{
						indices[k] = structIndexMapping.at(currentType).at(indices[k]);
					}
					currentType = LLVMStructGetTypeAtIndex(currentType, indices[k]);
					break;

				case LLVMArrayTypeKind:
					if (arrayStrideMultiplier.find(currentType) != arrayStrideMultiplier.end())
					{
						// TODO: Support stride
						TODO_ERROR();
					}
					currentType = LLVMGetElementType(currentType);
					break;

				case LLVMVectorTypeKind:
					{
						assert(k + 1 == indices.size());
						const auto vectorIndex = indices[k];
						indices.pop_back();
						// 	auto llvmValue = CreateExtractValue(composite, indices);
						// 	return CreateExtractElement(llvmValue, ConstU32(vectorIndex));
						TODO_ERROR();
					}
				default:
					TODO_ERROR();
				}
			}

			TODO_ERROR();
		}

	case OpCompositeInsert:
		{
			const auto compositeInsert = static_cast<SPIRV::SPIRVCompositeInsert*>(instruction);
			const auto composite = ConvertValue(compositeInsert->getComposite(), currentFunction);
			const auto value = ConvertValue(compositeInsert->getObject(), currentFunction);
			if (compositeInsert->getComposite()->getType()->isTypeVector())
			{
				assert(compositeInsert->getIndices().size() == 1);
				return CreateInsertElement(composite, value, ConstU32(compositeInsert->getIndices()[0]));
			}

			auto currentType = LLVMTypeOf(composite);
			auto indices = compositeInsert->getIndices();
			for (auto k = 0u; k <= indices.size(); k++)
			{
				// if (k == indices.size())
				// {
				// 	return CreateInsertValue(composite, value, indices);
				// }
				//
				// if (currentType->isStructTy())
				// {
				// 	if (structIndexMapping.find(currentType) != structIndexMapping.end())
				// 	{
				// 		indices[k] = structIndexMapping.at(currentType).at(indices[k]);
				// 	}
				// 	currentType = currentType->getStructElementType(indices[k]);
				// }
				// else if (currentType->isArrayTy())
				// {
				// 	if (arrayStrideMultiplier.find(currentType) != arrayStrideMultiplier.end())
				// 	{
				// 		// TODO: Support stride
				// 		TODO_ERROR();
				// 	}
				// 	currentType = currentType->getArrayElementType();
				// }
				// else if (currentType->isVectorTy())
				// {
				// 	assert(k + 1 == indices.size());
				// 	const auto vectorIndex = indices[k];
				// 	indices.pop_back();
				// 	auto llvmValue = CreateExtractValue(composite, indices);
				// 	llvmValue = CreateInsertElement(llvmValue, value, ConstU32(vectorIndex));
				// 	return CreateInsertValue(composite, llvmValue, indices);
				// }
				// else
				// {
				TODO_ERROR();
				// }
			}

			TODO_ERROR();
		}

	case OpCopyObject:
		{
			const auto op = static_cast<SPIRV::SPIRVCopyObject*>(instruction);
			const auto value = ConvertValue(op->getOperand(), currentFunction);
			const auto tmp = CreateAlloca(LLVMTypeOf(value));
			CreateStore(value, tmp);
			return CreateLoad(tmp);
		}

		// case OpTranspose: break;

	case OpSampledImage:
		{
			const auto sampledImage = reinterpret_cast<SPIRV::SPIRVSampledImage*>(instruction);

			// Get a storage struct, which is 3 pointers in size
			// TODO: Could have issues if called multiple times, as it would reuse the alloca
			auto storage = CreateAlloca(LLVMPointerType(LLVMInt8TypeInContext(context), 0), ConstI32(3));

			auto image = ConvertValue(sampledImage->getOpValue(0), currentFunction);
			auto sampler = ConvertValue(sampledImage->getOpValue(1), currentFunction);

			const auto functionName = "@Image.Combine";
			const auto cachedFunction = functionCache.find(functionName);
			LLVMValueRef function;
			if (cachedFunction != functionCache.end())
			{
				function = cachedFunction->second;
			}
			else
			{
				LLVMTypeRef parameterTypes[]
				{
					LLVMPointerType(LLVMInt8TypeInContext(context), 0),
					LLVMPointerType(LLVMInt8TypeInContext(context), 0),
					LLVMPointerType(LLVMPointerType(LLVMInt8TypeInContext(context), 0), 0),
				};

				const auto functionType = LLVMFunctionType(LLVMVoidTypeInContext(context), parameterTypes, 3, false);
				function = LLVMAddFunction(module, functionName, functionType);
				LLVMSetLinkage(function, LLVMExternalLinkage);
				functionCache[functionName] = function;
			}

			CreateCall(function, {
				           CreateBitCast(image, LLVMPointerType(LLVMInt8TypeInContext(context), 0)),
				           CreateBitCast(sampler, LLVMPointerType(LLVMInt8TypeInContext(context), 0)),
				           storage,
			           });

			storage = CreateBitCast(storage, ConvertType(sampledImage->getType()));
			return storage;
		}

	case OpImageSampleImplicitLod:
		{
			const auto imageSampleImplicitLod = reinterpret_cast<SPIRV::SPIRVImageSampleImplicitLod*>(instruction);
			return CallInbuiltFunction(imageSampleImplicitLod, currentFunction);
		}

	case OpImageSampleExplicitLod:
		{
			const auto imageSampleExplicitLod = reinterpret_cast<SPIRV::SPIRVImageSampleExplicitLod*>(instruction);
			return CallInbuiltFunction(imageSampleExplicitLod, currentFunction);
		}

		// case OpImageSampleDrefImplicitLod: break;
		// case OpImageSampleDrefExplicitLod: break;
		// case OpImageSampleProjImplicitLod: break;
		// case OpImageSampleProjExplicitLod: break;
		// case OpImageSampleProjDrefImplicitLod: break;
		// case OpImageSampleProjDrefExplicitLod: break;

	case OpImageFetch:
		{
			const auto imageFetch = reinterpret_cast<SPIRV::SPIRVImageFetch*>(instruction);
			return CallInbuiltFunction(imageFetch, currentFunction);
		}

		// case OpImageGather: break;
		// case OpImageDrefGather: break;

	case OpImageRead:
		{
			const auto imageRead = reinterpret_cast<SPIRV::SPIRVImageRead*>(instruction);
			return CallInbuiltFunction(imageRead, currentFunction);
		}

		// case OpImageWrite: break;

	case OpImage:
		{
			const auto image = reinterpret_cast<SPIRV::SPIRVImage*>(instruction);

			// Get a storage struct, which is 3 pointers in size
			// TODO: Could have issues if called multiple times, as it would reuse the alloca
			auto storage = CreateAlloca(LLVMPointerType(LLVMInt8TypeInContext(context), 0), ConstI32(3));

			auto sampledImage = ConvertValue(image->getOpValue(0), currentFunction);

			const auto functionName = "@Image.GetRaw";
			const auto cachedFunction = functionCache.find(functionName);
			LLVMValueRef function;
			if (cachedFunction != functionCache.end())
			{
				function = cachedFunction->second;
			}
			else
			{
				LLVMTypeRef parameterTypes[]
				{
					LLVMPointerType(LLVMInt8TypeInContext(context), 0),
					LLVMPointerType(LLVMPointerType(LLVMInt8TypeInContext(context), 0), 0),
				};

				const auto functionType = LLVMFunctionType(LLVMVoidTypeInContext(context), parameterTypes, 2, false);
				function = LLVMAddFunction(module, functionName, functionType);
				LLVMSetLinkage(function, LLVMExternalLinkage);
				functionCache[functionName] = function;
			}

			CreateCall(function, {
				           CreateBitCast(sampledImage, LLVMPointerType(LLVMInt8TypeInContext(context), 0)),
				           storage,
			           });
			storage = CreateBitCast(storage, ConvertType(image->getType()));
			return storage;
		}

		// case OpImageQueryFormat: break;
		// case OpImageQueryOrder: break;
		// case OpImageQuerySizeLod: break;

	case OpImageQuerySize:
		{
			const auto query = reinterpret_cast<const SPIRV::SPIRVImageInstBase*>(instruction);
			const auto spirvImageType = query->getOpValue(0)->getType();

			const auto function = GetInbuiltFunction("@Image.Query.Size", query->getType(), {
				                                         {nullptr, spirvImageType},
			                                         });

			return CallInbuiltFunction(function, query->getType(), {
				                           {spirvImageType, ConvertValue(query->getOpValue(0), currentFunction)},
			                           });
		}

		// case OpImageQueryLod: break;
		// case OpImageQueryLevels: break;
		// case OpImageQuerySamples: break;

	case OpConvertFToU:
		{
			const auto op = static_cast<SPIRV::SPIRVUnary*>(instruction);
			return CreateFPToUI(ConvertValue(op->getOperand(0), currentFunction),
			                    ConvertType(op->getType()));
		}

	case OpConvertFToS:
		{
			const auto op = static_cast<SPIRV::SPIRVUnary*>(instruction);
			return CreateFPToSI(ConvertValue(op->getOperand(0), currentFunction),
			                    ConvertType(op->getType()));
		}

	case OpConvertSToF:
		{
			const auto op = static_cast<SPIRV::SPIRVUnary*>(instruction);
			return CreateSIToFP(ConvertValue(op->getOperand(0), currentFunction),
			                    ConvertType(op->getType()));
		}

	case OpConvertUToF:
		{
			const auto op = static_cast<SPIRV::SPIRVUnary*>(instruction);
			return CreateUIToFP(ConvertValue(op->getOperand(0), currentFunction),
			                    ConvertType(op->getType()));
		}

	case OpUConvert:
		{
			const auto op = static_cast<SPIRV::SPIRVUnary*>(instruction);
			const auto value = ConvertValue(op->getOperand(0), currentFunction);
			const auto type = ConvertType(op->getType());
			return CreateZExtOrTrunc(value, type);
		}

	case OpSConvert:
		{
			const auto op = static_cast<SPIRV::SPIRVUnary*>(instruction);
			const auto value = ConvertValue(op->getOperand(0), currentFunction);
			const auto type = ConvertType(op->getType());
			return CreateSExtOrTrunc(value, type);
		}

	case OpFConvert:
		{
			const auto op = static_cast<SPIRV::SPIRVUnary*>(instruction);
			const auto value = ConvertValue(op->getOperand(0), currentFunction);
			const auto type = ConvertType(op->getType());
			return CreateFPExtOrTrunc(value, type);
		}

	case OpQuantizeToF16:
		{
			const auto op = static_cast<SPIRV::SPIRVUnary*>(instruction);
			auto value = ConvertValue(op->getOperand(0), currentFunction);
			const auto type = ConvertType(op->getType());
			value = CreateFPTrunc(value, LLVMGetTypeKind(type) == LLVMVectorTypeKind
				                             ? LLVMVectorType(LLVMHalfTypeInContext(context), LLVMGetVectorSize(type))
				                             : LLVMHalfTypeInContext(context));
			return CreateFPExt(value, type);
		}

		// case OpConvertPtrToU: break;
		// case OpSatConvertSToU: break;
		// case OpSatConvertUToS: break;
		// case OpConvertUToPtr: break;
		// case OpPtrCastToGeneric: break;
		// case OpGenericCastToPtr: break;
		// case OpGenericCastToPtrExplicit: break;

	case OpBitcast:
		{
			const auto op = static_cast<SPIRV::SPIRVUnary*>(instruction);
			return CreateBitCast(ConvertValue(op->getOperand(0), currentFunction), ConvertType(op->getType()));
		}

	case OpSNegate:
		{
			const auto op = static_cast<SPIRV::SPIRVUnary*>(instruction);
			return CreateNSWNeg(ConvertValue(op->getOperand(0), currentFunction));
		}

	case OpFNegate:
		{
			const auto op = static_cast<SPIRV::SPIRVUnary*>(instruction);
			return CreateFNeg(ConvertValue(op->getOperand(0), currentFunction));
		}

	case OpIAdd:
		{
			const auto op = static_cast<SPIRV::SPIRVBinary*>(instruction);
			return CreateAdd(ConvertValue(op->getOperand(0), currentFunction), ConvertValue(op->getOperand(1), currentFunction));
		}

	case OpFAdd:
		{
			const auto op = static_cast<SPIRV::SPIRVBinary*>(instruction);
			return CreateFAdd(ConvertValue(op->getOperand(0), currentFunction), ConvertValue(op->getOperand(1), currentFunction));
		}

	case OpISub:
		{
			const auto op = static_cast<SPIRV::SPIRVBinary*>(instruction);
			return CreateSub(ConvertValue(op->getOperand(0), currentFunction), ConvertValue(op->getOperand(1), currentFunction));
		}

	case OpFSub:
		{
			const auto op = static_cast<SPIRV::SPIRVBinary*>(instruction);
			return CreateFSub(ConvertValue(op->getOperand(0), currentFunction), ConvertValue(op->getOperand(1), currentFunction));
		}

	case OpIMul:
		{
			const auto op = static_cast<SPIRV::SPIRVBinary*>(instruction);
			return CreateMul(ConvertValue(op->getOperand(0), currentFunction), ConvertValue(op->getOperand(1), currentFunction));
		}

	case OpFMul:
		{
			const auto op = static_cast<SPIRV::SPIRVBinary*>(instruction);
			return CreateFMul(ConvertValue(op->getOperand(0), currentFunction), ConvertValue(op->getOperand(1), currentFunction));
		}

	case OpUDiv:
		{
			const auto op = static_cast<SPIRV::SPIRVBinary*>(instruction);
			return CreateUDiv(ConvertValue(op->getOperand(0), currentFunction), ConvertValue(op->getOperand(1), currentFunction));
		}

	case OpSDiv:
		{
			const auto op = static_cast<SPIRV::SPIRVBinary*>(instruction);
			return CreateSDiv(ConvertValue(op->getOperand(0), currentFunction), ConvertValue(op->getOperand(1), currentFunction));
		}

	case OpFDiv:
		{
			const auto op = static_cast<SPIRV::SPIRVBinary*>(instruction);
			return CreateFDiv(ConvertValue(op->getOperand(0), currentFunction), ConvertValue(op->getOperand(1), currentFunction));
		}

	case OpUMod:
		{
			const auto op = static_cast<SPIRV::SPIRVBinary*>(instruction);
			return CreateURem(ConvertValue(op->getOperand(0), currentFunction), ConvertValue(op->getOperand(1), currentFunction));
		}

	case OpSRem:
		{
			const auto op = static_cast<SPIRV::SPIRVBinary*>(instruction);
			return CreateSRem(ConvertValue(op->getOperand(0), currentFunction), ConvertValue(op->getOperand(1), currentFunction));
		}

	case OpSMod:
		{
			const auto op = static_cast<SPIRV::SPIRVBinary*>(instruction);
			auto left = ConvertValue(op->getOperand(0), currentFunction);
			auto right = ConvertValue(op->getOperand(1), currentFunction);
			auto llvmValue = CreateSDiv(left, right);
			llvmValue = CreateMul(right, llvmValue);
			return CreateSub(left, llvmValue);
		}

	case OpFRem:
		{
			const auto op = static_cast<SPIRV::SPIRVBinary*>(instruction);
			return CreateFRem(ConvertValue(op->getOperand(0), currentFunction), ConvertValue(op->getOperand(1), currentFunction));
		}

	case OpFMod:
		{
			const auto op = static_cast<SPIRV::SPIRVBinary*>(instruction);
			auto left = ConvertValue(op->getOperand(0), currentFunction);
			auto right = ConvertValue(op->getOperand(1), currentFunction);
			auto frem = CreateFRem(left, right);
			auto copySign = CreateIntrinsic<2>(Intrinsics::copysign, {frem, right});
			auto fadd = CreateFAdd(frem, right);
			auto cmp = CreateFCmpONE(frem, copySign);
			return CreateSelect(cmp, fadd, copySign);
		}

	case OpVectorTimesScalar:
		{
			const auto vectorTimesScalar = reinterpret_cast<SPIRV::SPIRVVectorTimesScalar*>(instruction);
			auto vector = ConvertValue(vectorTimesScalar->getVector(), currentFunction);
			auto scalar = ConvertValue(vectorTimesScalar->getScalar(), currentFunction);
			auto llvmValue = CreateVectorSplat(LLVMGetVectorSize(LLVMTypeOf(vector)), scalar);
			return CreateFMul(vector, llvmValue);
		}

	case OpMatrixTimesScalar:
		{
			const auto matrixTimesScalar = reinterpret_cast<SPIRV::SPIRVMatrixTimesScalar*>(instruction);
			return CallInbuiltFunction(matrixTimesScalar, currentFunction);
		}

	case OpVectorTimesMatrix:
		{
			const auto vectorTimesMatrix = reinterpret_cast<SPIRV::SPIRVVectorTimesMatrix*>(instruction);
			return CallInbuiltFunction(vectorTimesMatrix, currentFunction);
		}

	case OpMatrixTimesVector:
		{
			const auto matrixTimesVector = reinterpret_cast<SPIRV::SPIRVMatrixTimesVector*>(instruction);
			return CallInbuiltFunction(matrixTimesVector, currentFunction);
		}

	case OpMatrixTimesMatrix:
		{
			const auto matrixTimesMatrix = reinterpret_cast<SPIRV::SPIRVMatrixTimesMatrix*>(instruction);
			return CallInbuiltFunction(matrixTimesMatrix, currentFunction);
		}

		// case OpOuterProduct: break;

	case OpDot:
		{
			const auto dot = reinterpret_cast<SPIRV::SPIRVDot*>(instruction);
			return CallInbuiltFunction(dot, currentFunction);
		}

		// case OpIAddCarry: break;
		// case OpISubBorrow: break;
		// case OpUMulExtended: break;
		// case OpSMulExtended: break;

	case OpAny:
		{
			const auto op = reinterpret_cast<SPIRV::SPIRVUnary*>(instruction);
			// auto type = llvm::VectorType::get(state.builder.getInt32Ty(), op->getOperand(0)->getType()->getVectorComponentCount());
			// auto llvmValue = CreateZExt(ConvertValue(op->getOperand(0), currentFunction), type);
			// llvmValue = CreateUnaryIntrinsic(llvm::Intrinsic::experimental_vector_reduce_or, llvmValue);
			// return CreateICmpNE(llvmValue, state.builder.getInt32(0));
			const auto vector = ConvertValue(op->getOperand(0), currentFunction);
			auto llvmValue = CreateExtractElement(vector, ConstU32(0));
			for (auto i = 1u; i < LLVMGetVectorSize(LLVMTypeOf(vector)); i++)
			{
				auto tmp = CreateExtractElement(vector, ConstU32(i));
				llvmValue = CreateOr(llvmValue, tmp);
			}
			return llvmValue;
		}

	case OpAll:
		{
			const auto op = reinterpret_cast<SPIRV::SPIRVUnary*>(instruction);
			// auto type = llvm::VectorType::get(state.builder.getInt32Ty(), op->getOperand(0)->getType()->getVectorComponentCount());
			// auto llvmValue = CreateZExt(ConvertValue(op->getOperand(0), currentFunction), type);
			// llvmValue = CreateUnaryIntrinsic(llvm::Intrinsic::experimental_vector_reduce_and, llvmValue);
			// return CreateICmpNE(llvmValue, state.builder.getInt32(0));
			const auto vector = ConvertValue(op->getOperand(0), currentFunction);
			auto llvmValue = CreateExtractElement(vector, ConstU32(0));
			for (auto i = 1u; i < LLVMGetVectorSize(LLVMTypeOf(vector)); i++)
			{
				auto tmp = CreateExtractElement(vector, ConstU32(i));
				llvmValue = CreateAnd(llvmValue, tmp);
			}
			return llvmValue;
		}

	case OpIsNan:
		{
			const auto op = reinterpret_cast<SPIRV::SPIRVUnary*>(instruction);
			const auto value = ConvertValue(op->getOperand(0), currentFunction);
			return CreateFCmpUNO(value, GetConstantFloatOrVector(LLVMTypeOf(value), 0));
		}

	case OpIsInf:
		{
			TODO_ERROR();
		}

	case OpIsFinite:
		{
			TODO_ERROR();
		}

	case OpIsNormal:
		{
			TODO_ERROR();
		}

		// case OpSignBitSet: break;
		// case OpLessOrGreater: break;
		// case OpOrdered: break;
		// case OpUnordered: break;

	case OpLogicalOr:
		{
			const auto op = static_cast<SPIRV::SPIRVBinary*>(instruction);
			return CreateOr(ConvertValue(op->getOperand(0), currentFunction), ConvertValue(op->getOperand(1), currentFunction));
		}

	case OpLogicalAnd:
		{
			const auto op = static_cast<SPIRV::SPIRVBinary*>(instruction);
			return CreateAnd(ConvertValue(op->getOperand(0), currentFunction), ConvertValue(op->getOperand(1), currentFunction));
		}

	case OpLogicalNot:
		{
			const auto op = static_cast<SPIRV::SPIRVUnary*>(instruction);
			return CreateNot(ConvertValue(op->getOperand(0), currentFunction));
		}

	case OpSelect:
		{
			const auto op = static_cast<SPIRV::SPIRVSelect*>(instruction);
			return CreateSelect(ConvertValue(op->getCondition(), currentFunction),
			                    ConvertValue(op->getTrueValue(), currentFunction),
			                    ConvertValue(op->getFalseValue(), currentFunction));
		}

	case OpIEqual:
	case OpLogicalEqual:
		{
			const auto op = static_cast<SPIRV::SPIRVCompare*>(instruction);
			return CreateICmpEQ(ConvertValue(op->getOperand(0), currentFunction), ConvertValue(op->getOperand(1), currentFunction));
		}

	case OpINotEqual:
	case OpLogicalNotEqual:
		{
			const auto op = static_cast<SPIRV::SPIRVCompare*>(instruction);
			return CreateICmpNE(ConvertValue(op->getOperand(0), currentFunction), ConvertValue(op->getOperand(1), currentFunction));
		}

	case OpUGreaterThan:
		{
			const auto op = static_cast<SPIRV::SPIRVCompare*>(instruction);
			return CreateICmpUGT(ConvertValue(op->getOperand(0), currentFunction), ConvertValue(op->getOperand(1), currentFunction));
		}

	case OpSGreaterThan:
		{
			const auto op = static_cast<SPIRV::SPIRVCompare*>(instruction);
			return CreateICmpSGT(ConvertValue(op->getOperand(0), currentFunction), ConvertValue(op->getOperand(1), currentFunction));
		}

	case OpUGreaterThanEqual:
		{
			const auto op = static_cast<SPIRV::SPIRVCompare*>(instruction);
			return CreateICmpUGE(ConvertValue(op->getOperand(0), currentFunction), ConvertValue(op->getOperand(1), currentFunction));
		}

	case OpSGreaterThanEqual:
		{
			const auto op = static_cast<SPIRV::SPIRVCompare*>(instruction);
			return CreateICmpSGE(ConvertValue(op->getOperand(0), currentFunction), ConvertValue(op->getOperand(1), currentFunction));
		}

	case OpULessThan:
		{
			const auto op = static_cast<SPIRV::SPIRVCompare*>(instruction);
			return CreateICmpULT(ConvertValue(op->getOperand(0), currentFunction), ConvertValue(op->getOperand(1), currentFunction));
		}

	case OpSLessThan:
		{
			const auto op = static_cast<SPIRV::SPIRVCompare*>(instruction);
			return CreateICmpSLT(ConvertValue(op->getOperand(0), currentFunction), ConvertValue(op->getOperand(1), currentFunction));
		}

	case OpULessThanEqual:
		{
			const auto op = static_cast<SPIRV::SPIRVCompare*>(instruction);
			return CreateICmpULE(ConvertValue(op->getOperand(0), currentFunction), ConvertValue(op->getOperand(1), currentFunction));
		}

	case OpSLessThanEqual:
		{
			const auto op = static_cast<SPIRV::SPIRVCompare*>(instruction);
			return CreateICmpSLE(ConvertValue(op->getOperand(0), currentFunction), ConvertValue(op->getOperand(1), currentFunction));
		}

	case OpFOrdEqual:
		{
			const auto op = static_cast<SPIRV::SPIRVCompare*>(instruction);
			return CreateFCmpOEQ(ConvertValue(op->getOperand(0), currentFunction), ConvertValue(op->getOperand(1), currentFunction));
		}

	case OpFUnordEqual:
		{
			const auto op = static_cast<SPIRV::SPIRVCompare*>(instruction);
			return CreateFCmpUEQ(ConvertValue(op->getOperand(0), currentFunction), ConvertValue(op->getOperand(1), currentFunction));
		}

	case OpFOrdNotEqual:
		{
			const auto op = static_cast<SPIRV::SPIRVCompare*>(instruction);
			return CreateFCmpONE(ConvertValue(op->getOperand(0), currentFunction), ConvertValue(op->getOperand(1), currentFunction));
		}

	case OpFUnordNotEqual:
		{
			const auto op = static_cast<SPIRV::SPIRVCompare*>(instruction);
			return CreateFCmpUNE(ConvertValue(op->getOperand(0), currentFunction), ConvertValue(op->getOperand(1), currentFunction));
		}

	case OpFOrdLessThan:
		{
			const auto op = static_cast<SPIRV::SPIRVCompare*>(instruction);
			return CreateFCmpOLT(ConvertValue(op->getOperand(0), currentFunction), ConvertValue(op->getOperand(1), currentFunction));
		}

	case OpFUnordLessThan:
		{
			const auto op = static_cast<SPIRV::SPIRVCompare*>(instruction);
			return CreateFCmpULT(ConvertValue(op->getOperand(0), currentFunction), ConvertValue(op->getOperand(1), currentFunction));
		}

	case OpFOrdGreaterThan:
		{
			const auto op = static_cast<SPIRV::SPIRVCompare*>(instruction);
			return CreateFCmpOGT(ConvertValue(op->getOperand(0), currentFunction), ConvertValue(op->getOperand(1), currentFunction));
		}

	case OpFUnordGreaterThan:
		{
			const auto op = static_cast<SPIRV::SPIRVCompare*>(instruction);
			return CreateFCmpUGT(ConvertValue(op->getOperand(0), currentFunction), ConvertValue(op->getOperand(1), currentFunction));
		}

	case OpFOrdLessThanEqual:
		{
			const auto op = static_cast<SPIRV::SPIRVCompare*>(instruction);
			return CreateFCmpOLE(ConvertValue(op->getOperand(0), currentFunction), ConvertValue(op->getOperand(1), currentFunction));
		}

	case OpFUnordLessThanEqual:
		{
			const auto op = static_cast<SPIRV::SPIRVCompare*>(instruction);
			return CreateFCmpULE(ConvertValue(op->getOperand(0), currentFunction), ConvertValue(op->getOperand(1), currentFunction));
		}

	case OpFOrdGreaterThanEqual:
		{
			const auto op = static_cast<SPIRV::SPIRVCompare*>(instruction);
			return CreateFCmpOGE(ConvertValue(op->getOperand(0), currentFunction), ConvertValue(op->getOperand(1), currentFunction));
		}

	case OpFUnordGreaterThanEqual:
		{
			const auto op = static_cast<SPIRV::SPIRVCompare*>(instruction);
			return CreateFCmpUGE(ConvertValue(op->getOperand(0), currentFunction), ConvertValue(op->getOperand(1), currentFunction));
		}

	case OpShiftRightLogical:
		{
			const auto op = static_cast<SPIRV::SPIRVBinary*>(instruction);
			const auto base = ConvertValue(op->getOperand(0), currentFunction);
			const auto shift = ConvertValue(op->getOperand(1), currentFunction);
			if (LLVMTypeOf(base) == LLVMTypeOf(shift))
			{
				return CreateLShr(base, shift);
			}

			if (LLVMSizeOfTypeInBits(jit->getDataLayout(), LLVMTypeOf(base)) < LLVMSizeOfTypeInBits(jit->getDataLayout(), LLVMTypeOf(shift)))
			{
				auto llvmValue = CreateZExt(base, LLVMTypeOf(shift));
				llvmValue = CreateLShr(llvmValue, shift);
				return CreateTrunc(llvmValue, LLVMTypeOf(base));
			}

			auto isSigned = static_cast<SPIRV::SPIRVTypeInt*>(op->getOperand(1)->getType()->isTypeVector()
				                                                  ? op->getOperand(1)->getType()->getVectorComponentType()
				                                                  : op->getOperand(1)->getType())->isSigned();

			const auto llvmValue = isSigned
				                       ? CreateSExt(shift, LLVMTypeOf(base))
				                       : CreateZExt(shift, LLVMTypeOf(base));
			return CreateLShr(base, llvmValue);
		}

	case OpShiftRightArithmetic:
		{
			const auto op = static_cast<SPIRV::SPIRVBinary*>(instruction);
			const auto base = ConvertValue(op->getOperand(0), currentFunction);
			const auto shift = ConvertValue(op->getOperand(1), currentFunction);
			if (LLVMTypeOf(base) == LLVMTypeOf(shift))
			{
				return CreateAShr(base, shift);
			}

			if (LLVMSizeOfTypeInBits(jit->getDataLayout(), LLVMTypeOf(base)) < LLVMSizeOfTypeInBits(jit->getDataLayout(), LLVMTypeOf(shift)))
			{
				auto llvmValue = CreateSExt(base, LLVMTypeOf(shift));
				llvmValue = CreateAShr(llvmValue, shift);
				return CreateTrunc(llvmValue, LLVMTypeOf(base));
			}

			auto isSigned = static_cast<SPIRV::SPIRVTypeInt*>(op->getOperand(1)->getType()->isTypeVector()
				                                                  ? op->getOperand(1)->getType()->getVectorComponentType()
				                                                  : op->getOperand(1)->getType())->isSigned();
			auto llvmValue = isSigned
				                 ? CreateSExt(shift, LLVMTypeOf(base))
				                 : CreateZExt(shift, LLVMTypeOf(base));
			return CreateAShr(base, llvmValue);
		}

	case OpShiftLeftLogical:
		{
			const auto op = static_cast<SPIRV::SPIRVBinary*>(instruction);
			const auto base = ConvertValue(op->getOperand(0), currentFunction);
			const auto shift = ConvertValue(op->getOperand(1), currentFunction);
			if (LLVMTypeOf(base) == LLVMTypeOf(shift))
			{
				return CreateShl(base, shift);
			}

			if (LLVMSizeOfTypeInBits(jit->getDataLayout(), LLVMTypeOf(base)) < LLVMSizeOfTypeInBits(jit->getDataLayout(), LLVMTypeOf(shift)))
			{
				auto isSigned = static_cast<SPIRV::SPIRVTypeInt*>(op->getOperand(0)->getType()->isTypeVector()
					                                                  ? op->getOperand(0)->getType()->getVectorComponentType()
					                                                  : op->getOperand(0)->getType())->isSigned();
				auto llvmValue = isSigned
					                 ? CreateSExt(base, LLVMTypeOf(shift))
					                 : CreateZExt(base, LLVMTypeOf(shift));
				llvmValue = CreateShl(llvmValue, shift);
				return CreateTrunc(llvmValue, LLVMTypeOf(base));
			}

			auto isSigned = static_cast<SPIRV::SPIRVTypeInt*>(op->getOperand(1)->getType()->isTypeVector()
				                                                  ? op->getOperand(1)->getType()->getVectorComponentType()
				                                                  : op->getOperand(1)->getType())->isSigned();
			auto llvmValue = isSigned
				                 ? CreateSExt(shift, LLVMTypeOf(base))
				                 : CreateZExt(shift, LLVMTypeOf(base));
			return CreateShl(base, llvmValue);
		}

	case OpBitwiseOr:
		{
			const auto op = static_cast<SPIRV::SPIRVBinary*>(instruction);
			return CreateOr(ConvertValue(op->getOperand(0), currentFunction), ConvertValue(op->getOperand(1), currentFunction));
		}

	case OpBitwiseXor:
		{
			const auto op = static_cast<SPIRV::SPIRVBinary*>(instruction);
			return CreateXor(ConvertValue(op->getOperand(0), currentFunction), ConvertValue(op->getOperand(1), currentFunction));
		}

	case OpBitwiseAnd:
		{
			const auto op = static_cast<SPIRV::SPIRVBinary*>(instruction);
			return CreateAnd(ConvertValue(op->getOperand(0), currentFunction), ConvertValue(op->getOperand(1), currentFunction));
		}

	case OpNot:
		{
			const auto op = static_cast<SPIRV::SPIRVUnary*>(instruction);
			return CreateNot(ConvertValue(op->getOperand(0), currentFunction));
		}

	case OpBitFieldInsert:
		{
			const auto op = static_cast<SPIRV::SPIRVBitFieldInsert*>(instruction);
			const auto base = ConvertValue(op->getBase(), currentFunction);
			const auto insert = ConvertValue(op->getInsert(), currentFunction);
			auto offset = ConvertValue(op->getOffset(), currentFunction);
			auto count = ConvertValue(op->getCount(), currentFunction);
			LLVMValueRef mask;

			if (LLVMSizeOfTypeInBits(jit->getDataLayout(), LLVMTypeOf(offset)) < LLVMSizeOfTypeInBits(jit->getDataLayout(), LLVMTypeOf(base)))
			{
				offset = CreateZExt(offset, ScalarType(LLVMTypeOf(base)));
			}
			else if (LLVMSizeOfTypeInBits(jit->getDataLayout(), LLVMTypeOf(offset)) > LLVMSizeOfTypeInBits(jit->getDataLayout(), LLVMTypeOf(base)))
			{
				offset = CreateTrunc(offset, ScalarType(LLVMTypeOf(base)));
			}

			if (LLVMSizeOfTypeInBits(jit->getDataLayout(), LLVMTypeOf(count)) < LLVMSizeOfTypeInBits(jit->getDataLayout(), LLVMTypeOf(base)))
			{
				count = CreateZExt(count, ScalarType(LLVMTypeOf(base)));
			}
			else if (LLVMSizeOfTypeInBits(jit->getDataLayout(), LLVMTypeOf(count)) > LLVMSizeOfTypeInBits(jit->getDataLayout(), LLVMTypeOf(base)))
			{
				count = CreateTrunc(count, ScalarType(LLVMTypeOf(base)));
			}

			const auto negativeOne = LLVMConstInt(LLVMIntTypeInContext(context, LLVMSizeOfTypeInBits(jit->getDataLayout(), ScalarType(LLVMTypeOf(base)))), -1,
			                                      true);
			mask = CreateShl(negativeOne, count, "");
			mask = CreateXor(mask, negativeOne);
			mask = CreateShl(mask, offset);

			auto llvmValue = CreateXor(mask, negativeOne);
			const auto lhs = CreateAnd(llvmValue, base);
			const auto rhs = CreateAnd(mask, insert);
			return CreateOr(lhs, rhs);
		}

	case OpBitFieldSExtract:
		TODO_ERROR();

	case OpBitFieldUExtract:
		TODO_ERROR();


	case OpBitReverse:
		{
			const auto op = static_cast<SPIRV::SPIRVUnary*>(instruction);
			return CreateIntrinsic<1>(Intrinsics::bitreverse, {ConvertValue(op->getOperand(0), currentFunction)});
		}

	case OpBitCount:
		{
			const auto op = static_cast<SPIRV::SPIRVUnary*>(instruction);
			return CreateIntrinsic<1>(Intrinsics::ctpop, {ConvertValue(op->getOperand(0), currentFunction)});
		}

		// case OpDPdx: break;
		// case OpDPdy: break;
		// case OpFwidth: break;
		// case OpDPdxFine: break;
		// case OpDPdyFine: break;
		// case OpFwidthFine: break;
		// case OpDPdxCoarse: break;
		// case OpDPdyCoarse: break;
		// case OpFwidthCoarse: break;
		// case OpEmitVertex: break;
		// case OpEndPrimitive: break;
		// case OpEmitStreamVertex: break;
		// case OpEndStreamPrimitive: break;

	case OpControlBarrier:
		{
			TODO_ERROR();
		}

	case OpMemoryBarrier:
		{
			const auto op = static_cast<SPIRV::SPIRVAtomicInstBase*>(instruction);
			auto semantics = static_cast<SPIRV::SPIRVConstant*>(op->getOpValue(1))->getInt32Value();
			return CreateFence(ConvertSemantics(semantics), false);
		}

	case OpAtomicLoad:
		{
			const auto op = static_cast<SPIRV::SPIRVAtomicInstBase*>(instruction);
			auto pointer = ConvertValue(op->getOpValue(0), currentFunction);
			auto semantics = static_cast<SPIRV::SPIRVConstant*>(op->getOpValue(2))->getInt32Value();

			auto llvmValue = CreateLoad(pointer, semantics & 0x8000);
			LLVMSetOrdering(llvmValue, ConvertSemantics(semantics));
			return llvmValue;
		}

	case OpAtomicStore:
		{
			const auto op = static_cast<SPIRV::SPIRVAtomicInstBase*>(instruction);
			auto pointer = ConvertValue(op->getOpValue(0), currentFunction);
			auto semantics = static_cast<SPIRV::SPIRVConstant*>(op->getOpValue(2))->getInt32Value();
			auto value = ConvertValue(op->getOpValue(3), currentFunction);

			auto llvmValue = CreateStore(value, pointer, semantics & 0x8000);
			LLVMSetOrdering(llvmValue, ConvertSemantics(semantics));
			return llvmValue;
		}

	case OpAtomicCompareExchange:
	case OpAtomicCompareExchangeWeak:
		{
			const auto op = static_cast<SPIRV::SPIRVAtomicInstBase*>(instruction);
			auto pointer = ConvertValue(op->getOpValue(0), currentFunction);
			auto equalSemantics = static_cast<SPIRV::SPIRVConstant*>(op->getOpValue(2))->getInt32Value();
			auto unequalSemantics = static_cast<SPIRV::SPIRVConstant*>(op->getOpValue(3))->getInt32Value();
			auto value = ConvertValue(op->getOpValue(4), currentFunction);
			auto comparator = ConvertValue(op->getOpValue(5), currentFunction);

			auto llvmValue = CreateAtomicCmpXchg(pointer, comparator, value, ConvertSemantics(equalSemantics), ConvertSemantics(unequalSemantics), false);
			// TODO: When LLVM-C supports this for atomic instructions: LLVMSetVolatile(llvmValue, (equalSemantics & 0x8000) || (unequalSemantics & 0x8000));
			return CreateExtractValue(llvmValue, {0});
		}

	case OpAtomicIIncrement:
	case OpAtomicIDecrement:
		{
			const auto op = static_cast<SPIRV::SPIRVAtomicInstBase*>(instruction);
			auto pointer = ConvertValue(op->getOpValue(0), currentFunction);
			auto semantics = static_cast<SPIRV::SPIRVConstant*>(op->getOpValue(2))->getInt32Value();

			auto llvmValue = CreateAtomicRMW(op->getOpCode() == OpAtomicIIncrement ? LLVMAtomicRMWBinOpAdd : LLVMAtomicRMWBinOpSub,
			                                 pointer, ConstI32(1), ConvertSemantics(semantics), false);
			// TODO: When LLVM-C supports this for atomic instructions: LLVMSetVolatile(llvmValue, semantics & 0x8000);
			return llvmValue;
		}

	case OpAtomicExchange:
	case OpAtomicIAdd:
	case OpAtomicISub:
	case OpAtomicSMin:
	case OpAtomicUMin:
	case OpAtomicSMax:
	case OpAtomicUMax:
	case OpAtomicAnd:
	case OpAtomicOr:
	case OpAtomicXor:
		{
			const auto op = static_cast<SPIRV::SPIRVAtomicInstBase*>(instruction);
			auto pointer = ConvertValue(op->getOpValue(0), currentFunction);
			auto semantics = static_cast<SPIRV::SPIRVConstant*>(op->getOpValue(2))->getInt32Value();
			auto value = ConvertValue(op->getOpValue(3), currentFunction);

			auto llvmValue = CreateAtomicRMW(instructionLookupAtomic[op->getOpCode()], pointer, value, ConvertSemantics(semantics), false);
			// TODO: When LLVM-C supports this for atomic instructions: LLVMSetVolatile(llvmValue, semantics & 0x8000);
			return llvmValue;
		}

	case OpPhi:
		{
			auto phi = static_cast<SPIRV::SPIRVPhi*>(instruction);
			return CreatePhi(ConvertType(phi->getType()));
		}

	case OpBranch:
		{
			const auto branch = static_cast<SPIRV::SPIRVBranch*>(instruction);
			const auto block = GetBasicBlock(branch->getTargetLabel());
			const auto llvmBranch = CreateBr(block);
			if (branch->getPrevious() && branch->getPrevious()->getOpCode() == OpLoopMerge)
			{
				const auto loopMerge = static_cast<SPIRV::SPIRVLoopMerge*>(branch->getPrevious());
				SetLLVMLoopMetadata(loopMerge, llvmBranch);
			}
			return llvmBranch;
		}

	case OpBranchConditional:
		{
			const auto branch = static_cast<SPIRV::SPIRVBranchConditional*>(instruction);
			const auto trueBlock = GetBasicBlock(branch->getTrueLabel());
			const auto falseBlock = GetBasicBlock(branch->getFalseLabel());
			const auto llvmBranch = CreateCondBr(ConvertValue(branch->getCondition(), currentFunction), trueBlock, falseBlock);
			if (branch->getPrevious() && branch->getPrevious()->getOpCode() == OpLoopMerge)
			{
				const auto loopMerge = static_cast<SPIRV::SPIRVLoopMerge*>(branch->getPrevious());
				SetLLVMLoopMetadata(loopMerge, llvmBranch);
			}
			return llvmBranch;
		}

	case OpSwitch:
		{
			const auto swtch = static_cast<SPIRV::SPIRVSwitch*>(instruction);
			const auto select = ConvertValue(swtch->getSelect(), currentFunction);
			const auto defaultBlock = GetBasicBlock(swtch->getDefault());
			auto llvmSwitch = CreateSwitch(select, defaultBlock, swtch->getNumPairs());
			swtch->foreachPair([&](SPIRV::SPIRVSwitch::LiteralTy literals, SPIRV::SPIRVBasicBlock* label)
			{
				assert(!literals.empty());
				assert(literals.size() <= 2);
				auto literal = static_cast<uint64_t>(literals.at(0));
				if (literals.size() == 2)
				{
					literal += static_cast<uint64_t>(literals.at(1)) << 32;
				}
				LLVMAddCase(llvmSwitch, ConstU32(literal), GetBasicBlock(label));
			});
			return llvmSwitch;
		}

	case OpKill:
		// TODO: Handle case when OpKill not in entry function
		return CreateRet(LLVMConstInt(LLVMInt1TypeInContext(context), 1, false));

	case OpReturn:
		return LLVMBuildRetVoid(builder);

	case OpReturnValue:
		{
			const auto returnValue = static_cast<SPIRV::SPIRVReturnValue*>(instruction);
			return CreateRet(ConvertValue(returnValue->getReturnValue(), currentFunction));
		}

	case OpUnreachable:
		return CreateUnreachable();

		// case OpLifetimeStart: break;
		// case OpLifetimeStop: break;
		// case OpGroupAsyncCopy: break;
		// case OpGroupWaitEvents: break;
		// case OpGroupAll: break;
		// case OpGroupAny: break;
		// case OpGroupBroadcast: break;
		// case OpGroupIAdd: break;
		// case OpGroupFAdd: break;
		// case OpGroupFMin: break;
		// case OpGroupUMin: break;
		// case OpGroupSMin: break;
		// case OpGroupFMax: break;
		// case OpGroupUMax: break;
		// case OpGroupSMax: break;
		// case OpReadPipe: break;
		// case OpWritePipe: break;
		// case OpReservedReadPipe: break;
		// case OpReservedWritePipe: break;
		// case OpReserveReadPipePackets: break;
		// case OpReserveWritePipePackets: break;
		// case OpCommitReadPipe: break;
		// case OpCommitWritePipe: break;
		// case OpIsValidReserveId: break;
		// case OpGetNumPipePackets: break;
		// case OpGetMaxPipePackets: break;
		// case OpGroupReserveReadPipePackets: break;
		// case OpGroupReserveWritePipePackets: break;
		// case OpGroupCommitReadPipe: break;
		// case OpGroupCommitWritePipe: break;
		// case OpEnqueueMarker: break;
		// case OpEnqueueKernel: break;
		// case OpGetKernelNDrangeSubGroupCount: break;
		// case OpGetKernelNDrangeMaxSubGroupSize: break;
		// case OpGetKernelWorkGroupSize: break;
		// case OpGetKernelPreferredWorkGroupSizeMultiple: break;
		// case OpRetainEvent: break;
		// case OpReleaseEvent: break;
		// case OpCreateUserEvent: break;
		// case OpIsValidEvent: break;
		// case OpSetUserEventStatus: break;
		// case OpCaptureEventProfilingInfo: break;
		// case OpGetDefaultQueue: break;
		// case OpBuildNDRange: break;
		// case OpImageSparseSampleImplicitLod: break;
		// case OpImageSparseSampleExplicitLod: break;
		// case OpImageSparseSampleDrefImplicitLod: break;
		// case OpImageSparseSampleDrefExplicitLod: break;
		// case OpImageSparseSampleProjImplicitLod: break;
		// case OpImageSparseSampleProjExplicitLod: break;
		// case OpImageSparseSampleProjDrefImplicitLod: break;
		// case OpImageSparseSampleProjDrefExplicitLod: break;
		// case OpImageSparseFetch: break;
		// case OpImageSparseGather: break;
		// case OpImageSparseDrefGather: break;
		// case OpImageSparseTexelsResident: break;
		// case OpAtomicFlagTestAndSet: break;
		// case OpAtomicFlagClear: break;
		// case OpImageSparseRead: break;
		// case OpSizeOf: break;
		// case OpTypePipeStorage: break;
		// case OpConstantPipeStorage: break;
		// case OpCreatePipeFromPipeStorage: break;
		// case OpGetKernelLocalSizeForSubgroupCount: break;
		// case OpGetKernelMaxNumSubgroups: break;
		// case OpTypeNamedBarrier: break;
		// case OpNamedBarrierInitialize: break;
		// case OpMemoryNamedBarrier: break;
		// case OpModuleProcessed: break;
		// case OpForward: break;
		// case OpSubgroupBallotKHR: break;
		// case OpSubgroupFirstInvocationKHR: break;
		// case OpSubgroupAllKHR: break;
		// case OpSubgroupAnyKHR: break;
		// case OpSubgroupAllEqualKHR: break;
		// case OpSubgroupReadInvocationKHR: break;
		// case OpSubgroupShuffleINTEL: break;
		// case OpSubgroupShuffleDownINTEL: break;
		// case OpSubgroupShuffleUpINTEL: break;
		// case OpSubgroupShuffleXorINTEL: break;
		// case OpSubgroupBlockReadINTEL: break;
		// case OpSubgroupBlockWriteINTEL: break;
		// case OpSubgroupImageBlockReadINTEL: break;
		// case OpSubgroupImageBlockWriteINTEL: break;
		// case OpSubgroupImageMediaBlockReadINTEL: break;
		// case OpSubgroupImageMediaBlockWriteINTEL: break;
		// case OpVmeImageINTEL: break;
		// case OpTypeVmeImageINTEL: break;
		// case OpTypeAvcImePayloadINTEL: break;
		// case OpTypeAvcRefPayloadINTEL: break;
		// case OpTypeAvcSicPayloadINTEL: break;
		// case OpTypeAvcMcePayloadINTEL: break;
		// case OpTypeAvcMceResultINTEL: break;
		// case OpTypeAvcImeResultINTEL: break;
		// case OpTypeAvcImeResultSingleReferenceStreamoutINTEL: break;
		// case OpTypeAvcImeResultDualReferenceStreamoutINTEL: break;
		// case OpTypeAvcImeSingleReferenceStreaminINTEL: break;
		// case OpTypeAvcImeDualReferenceStreaminINTEL: break;
		// case OpTypeAvcRefResultINTEL: break;
		// case OpTypeAvcSicResultINTEL: break;
		// case OpSubgroupAvcMceGetDefaultInterBaseMultiReferencePenaltyINTEL: break;
		// case OpSubgroupAvcMceSetInterBaseMultiReferencePenaltyINTEL: break;
		// case OpSubgroupAvcMceGetDefaultInterShapePenaltyINTEL: break;
		// case OpSubgroupAvcMceSetInterShapePenaltyINTEL: break;
		// case OpSubgroupAvcMceGetDefaultInterDirectionPenaltyINTEL: break;
		// case OpSubgroupAvcMceSetInterDirectionPenaltyINTEL: break;
		// case OpSubgroupAvcMceGetDefaultIntraLumaShapePenaltyINTEL: break;
		// case OpSubgroupAvcMceGetDefaultInterMotionVectorCostTableINTEL: break;
		// case OpSubgroupAvcMceGetDefaultHighPenaltyCostTableINTEL: break;
		// case OpSubgroupAvcMceGetDefaultMediumPenaltyCostTableINTEL: break;
		// case OpSubgroupAvcMceGetDefaultLowPenaltyCostTableINTEL: break;
		// case OpSubgroupAvcMceSetMotionVectorCostFunctionINTEL: break;
		// case OpSubgroupAvcMceGetDefaultIntraLumaModePenaltyINTEL: break;
		// case OpSubgroupAvcMceGetDefaultNonDcLumaIntraPenaltyINTEL: break;
		// case OpSubgroupAvcMceGetDefaultIntraChromaModeBasePenaltyINTEL: break;
		// case OpSubgroupAvcMceSetAcOnlyHaarINTEL: break;
		// case OpSubgroupAvcMceSetSourceInterlacedFieldPolarityINTEL: break;
		// case OpSubgroupAvcMceSetSingleReferenceInterlacedFieldPolarityINTEL: break;
		// case OpSubgroupAvcMceSetDualReferenceInterlacedFieldPolaritiesINTEL: break;
		// case OpSubgroupAvcMceConvertToImePayloadINTEL: break;
		// case OpSubgroupAvcMceConvertToImeResultINTEL: break;
		// case OpSubgroupAvcMceConvertToRefPayloadINTEL: break;
		// case OpSubgroupAvcMceConvertToRefResultINTEL: break;
		// case OpSubgroupAvcMceConvertToSicPayloadINTEL: break;
		// case OpSubgroupAvcMceConvertToSicResultINTEL: break;
		// case OpSubgroupAvcMceGetMotionVectorsINTEL: break;
		// case OpSubgroupAvcMceGetInterDistortionsINTEL: break;
		// case OpSubgroupAvcMceGetBestInterDistortionsINTEL: break;
		// case OpSubgroupAvcMceGetInterMajorShapeINTEL: break;
		// case OpSubgroupAvcMceGetInterMinorShapeINTEL: break;
		// case OpSubgroupAvcMceGetInterDirectionsINTEL: break;
		// case OpSubgroupAvcMceGetInterMotionVectorCountINTEL: break;
		// case OpSubgroupAvcMceGetInterReferenceIdsINTEL: break;
		// case OpSubgroupAvcMceGetInterReferenceInterlacedFieldPolaritiesINTEL: break;
		// case OpSubgroupAvcImeInitializeINTEL: break;
		// case OpSubgroupAvcImeSetSingleReferenceINTEL: break;
		// case OpSubgroupAvcImeSetDualReferenceINTEL: break;
		// case OpSubgroupAvcImeRefWindowSizeINTEL: break;
		// case OpSubgroupAvcImeAdjustRefOffsetINTEL: break;
		// case OpSubgroupAvcImeConvertToMcePayloadINTEL: break;
		// case OpSubgroupAvcImeSetMaxMotionVectorCountINTEL: break;
		// case OpSubgroupAvcImeSetUnidirectionalMixDisableINTEL: break;
		// case OpSubgroupAvcImeSetEarlySearchTerminationThresholdINTEL: break;
		// case OpSubgroupAvcImeSetWeightedSadINTEL: break;
		// case OpSubgroupAvcImeEvaluateWithSingleReferenceINTEL: break;
		// case OpSubgroupAvcImeEvaluateWithDualReferenceINTEL: break;
		// case OpSubgroupAvcImeEvaluateWithSingleReferenceStreaminINTEL: break;
		// case OpSubgroupAvcImeEvaluateWithDualReferenceStreaminINTEL: break;
		// case OpSubgroupAvcImeEvaluateWithSingleReferenceStreamoutINTEL: break;
		// case OpSubgroupAvcImeEvaluateWithDualReferenceStreamoutINTEL: break;
		// case OpSubgroupAvcImeEvaluateWithSingleReferenceStreaminoutINTEL: break;
		// case OpSubgroupAvcImeEvaluateWithDualReferenceStreaminoutINTEL: break;
		// case OpSubgroupAvcImeConvertToMceResultINTEL: break;
		// case OpSubgroupAvcImeGetSingleReferenceStreaminINTEL: break;
		// case OpSubgroupAvcImeGetDualReferenceStreaminINTEL: break;
		// case OpSubgroupAvcImeStripSingleReferenceStreamoutINTEL: break;
		// case OpSubgroupAvcImeStripDualReferenceStreamoutINTEL: break;
		// case OpSubgroupAvcImeGetStreamoutSingleReferenceMajorShapeMotionVectorsINTEL: break;
		// case OpSubgroupAvcImeGetStreamoutSingleReferenceMajorShapeDistortionsINTEL: break;
		// case OpSubgroupAvcImeGetStreamoutSingleReferenceMajorShapeReferenceIdsINTEL: break;
		// case OpSubgroupAvcImeGetStreamoutDualReferenceMajorShapeMotionVectorsINTEL: break;
		// case OpSubgroupAvcImeGetStreamoutDualReferenceMajorShapeDistortionsINTEL: break;
		// case OpSubgroupAvcImeGetStreamoutDualReferenceMajorShapeReferenceIdsINTEL: break;
		// case OpSubgroupAvcImeGetBorderReachedINTEL: break;
		// case OpSubgroupAvcImeGetTruncatedSearchIndicationINTEL: break;
		// case OpSubgroupAvcImeGetUnidirectionalEarlySearchTerminationINTEL: break;
		// case OpSubgroupAvcImeGetWeightingPatternMinimumMotionVectorINTEL: break;
		// case OpSubgroupAvcImeGetWeightingPatternMinimumDistortionINTEL: break;
		// case OpSubgroupAvcFmeInitializeINTEL: break;
		// case OpSubgroupAvcBmeInitializeINTEL: break;
		// case OpSubgroupAvcRefConvertToMcePayloadINTEL: break;
		// case OpSubgroupAvcRefSetBidirectionalMixDisableINTEL: break;
		// case OpSubgroupAvcRefSetBilinearFilterEnableINTEL: break;
		// case OpSubgroupAvcRefEvaluateWithSingleReferenceINTEL: break;
		// case OpSubgroupAvcRefEvaluateWithDualReferenceINTEL: break;
		// case OpSubgroupAvcRefEvaluateWithMultiReferenceINTEL: break;
		// case OpSubgroupAvcRefEvaluateWithMultiReferenceInterlacedINTEL: break;
		// case OpSubgroupAvcRefConvertToMceResultINTEL: break;
		// case OpSubgroupAvcSicInitializeINTEL: break;
		// case OpSubgroupAvcSicConfigureSkcINTEL: break;
		// case OpSubgroupAvcSicConfigureIpeLumaINTEL: break;
		// case OpSubgroupAvcSicConfigureIpeLumaChromaINTEL: break;
		// case OpSubgroupAvcSicGetMotionVectorMaskINTEL: break;
		// case OpSubgroupAvcSicConvertToMcePayloadINTEL: break;
		// case OpSubgroupAvcSicSetIntraLumaShapePenaltyINTEL: break;
		// case OpSubgroupAvcSicSetIntraLumaModeCostFunctionINTEL: break;
		// case OpSubgroupAvcSicSetIntraChromaModeCostFunctionINTEL: break;
		// case OpSubgroupAvcSicSetBilinearFilterEnableINTEL: break;
		// case OpSubgroupAvcSicSetSkcForwardTransformEnableINTEL: break;
		// case OpSubgroupAvcSicSetBlockBasedRawSkipSadINTEL: break;
		// case OpSubgroupAvcSicEvaluateIpeINTEL: break;
		// case OpSubgroupAvcSicEvaluateWithSingleReferenceINTEL: break;
		// case OpSubgroupAvcSicEvaluateWithDualReferenceINTEL: break;
		// case OpSubgroupAvcSicEvaluateWithMultiReferenceINTEL: break;
		// case OpSubgroupAvcSicEvaluateWithMultiReferenceInterlacedINTEL: break;
		// case OpSubgroupAvcSicConvertToMceResultINTEL: break;
		// case OpSubgroupAvcSicGetIpeLumaShapeINTEL: break;
		// case OpSubgroupAvcSicGetBestIpeLumaDistortionINTEL: break;
		// case OpSubgroupAvcSicGetBestIpeChromaDistortionINTEL: break;
		// case OpSubgroupAvcSicGetPackedIpeLumaModesINTEL: break;
		// case OpSubgroupAvcSicGetIpeChromaModeINTEL: break;
		// case OpSubgroupAvcSicGetPackedSkcLumaCountThresholdINTEL: break;
		// case OpSubgroupAvcSicGetPackedSkcLumaSumThresholdINTEL: break;
		// case OpSubgroupAvcSicGetInterRawSadsINTEL: break;
	}
	TODO_ERROR();
}

LLVMBasicBlockRef SPIRVCompiledModuleBuilder::GetBasicBlock(const SPIRV::SPIRVBasicBlock* spirvBasicBlock)
{
	const auto cachedType = valueMapping.find(spirvBasicBlock->getId());
	if (cachedType != valueMapping.end())
	{
		return LLVMValueAsBasicBlock(cachedType->second);
	}

	FATAL_ERROR();
}

void SPIRVCompiledModuleBuilder::CompileBasicBlock(const SPIRV::SPIRVBasicBlock* spirvBasicBlock, LLVMValueRef currentFunction,
                                                   LLVMBasicBlockRef llvmBasicBlock)
{
	for (auto i = 0u; i < spirvBasicBlock->getNumInst(); i++)
	{
		//llvm::FastMathFlags flags{};
		const auto instruction = spirvBasicBlock->getInst(i);
		for (const auto decorate : instruction->getDecorates())
		{
			if (decorate.first == DecorationSaturatedConversion)
			{
				TODO_ERROR();
			}
			else if (decorate.first == DecorationFPRoundingMode)
			{
				TODO_ERROR();
			}
			else if (decorate.first == DecorationFPFastMathMode)
			{
				TODO_ERROR();
			}
			else if (decorate.first == DecorationNoContraction)
			{
				TODO_ERROR();
				// flags.setAllowContract(false);
			}
			else if (decorate.first == DecorationNoSignedWrap)
			{
				TODO_ERROR();
			}
			else if (decorate.first == DecorationNoUnsignedWrap)
			{
				TODO_ERROR();
			}
		}
		// TODO: state.builder.setFastMathFlags(flags);

		MoveBuilder(llvmBasicBlock, instruction);
		const auto llvmValue = ConvertInstruction(instruction, currentFunction);
		if (instruction->hasType())
		{
			assert(llvmValue);
			assert(ConvertType(instruction->getType()) == LLVMTypeOf(llvmValue));
		}

		if (llvmValue && instruction->hasId())
		{
			valueMapping[instruction->getId()] = llvmValue;
		}
	}
	currentBlock = nullptr;
}

LLVMValueRef SPIRVCompiledModuleBuilder::ConvertFunction(const SPIRV::SPIRVFunction* spirvFunction)
{
	const auto cachedType = functionMapping.find(spirvFunction->getId());
	if (cachedType != functionMapping.end())
	{
		return cachedType->second;
	}

	AddDebugInformation(spirvFunction);

	for (const auto decorate : spirvFunction->getDecorates())
	{
		TODO_ERROR();
	}

	auto returnType = ConvertType(spirvFunction->getType());

	if (spirvFunction == entryPoint && executionModel == ExecutionModelFragment)
	{
		assert(LLVMVoidTypeInContext(context) == returnType);
		returnType = LLVMInt1TypeInContext(context);
	}

	std::vector<LLVMTypeRef> parameters{spirvFunction->getNumArguments()};
	for (auto i = 0u; i < spirvFunction->getNumArguments(); i++)
	{
		assert(spirvFunction->getArgument(i)->getArgNo() == i);
		parameters[i] = ConvertType(spirvFunction->getArgument(i)->getType());
	}

	// TODO: Disable external linkage when no longer using directly
	const auto functionType = LLVMFunctionType(returnType, parameters.data(), static_cast<uint32_t>(parameters.size()), false);
	const auto llvmFunction = LLVMAddFunction(module, MangleName(spirvFunction).c_str(), functionType);
	LLVMSetLinkage(llvmFunction, LLVMExternalLinkage);

	for (auto i = 0u; i < spirvFunction->getNumBasicBlock(); i++)
	{
		const auto spirvBasicBlock = spirvFunction->getBasicBlock(i);
		const auto llvmBasicBlock = LLVMAppendBasicBlockInContext(context, llvmFunction, spirvBasicBlock->getName().c_str());
		valueMapping[spirvBasicBlock->getId()] = LLVMBasicBlockAsValue(llvmBasicBlock);
	}

	for (auto i = 0u; i < spirvFunction->getNumBasicBlock(); i++)
	{
		const auto basicBlock = GetBasicBlock(spirvFunction->getBasicBlock(i));
		CompileBasicBlock(spirvFunction->getBasicBlock(i), llvmFunction, basicBlock);
		if (spirvFunction == entryPoint && executionModel == ExecutionModelFragment)
		{
			// Turn all void terminations into return false
			if (spirvFunction->getBasicBlock(i)->getTerminateInstr()->getOpCode() == OpReturn)
			{
				const auto instruction = LLVMGetLastInstruction(basicBlock);
				LLVMInstructionEraseFromParent(instruction);
				LLVMPositionBuilderAtEnd(builder, basicBlock);
				CreateRet(LLVMConstInt(LLVMInt1TypeInContext(context), 0, false));
			}
		}
	}

	functionMapping[spirvFunction->getId()] = llvmFunction;
	return llvmFunction;
}

void SPIRVCompiledModuleBuilder::AddBuiltin()
{
	std::vector<LLVMTypeRef> inputMembers{};
	std::vector<LLVMTypeRef> outputMembers{};
	switch (executionModel)
	{
	case ExecutionModelVertex:
		builtinInputMapping.emplace_back(BuiltInVertexId, 0);
		builtinInputMapping.emplace_back(BuiltInVertexIndex, 0);
		inputMembers.push_back(LLVMInt32TypeInContext(context));
		builtinInputMapping.emplace_back(BuiltInInstanceId, 1);
		builtinInputMapping.emplace_back(BuiltInInstanceIndex, 1);
		inputMembers.push_back(LLVMInt32TypeInContext(context));

		builtinOutputMapping.emplace_back(BuiltInPosition, 0);
		outputMembers.push_back(LLVMVectorType(LLVMFloatTypeInContext(context), 4));
		builtinOutputMapping.emplace_back(BuiltInPointSize, 1);
		outputMembers.push_back(LLVMFloatTypeInContext(context));
		builtinOutputMapping.emplace_back(BuiltInClipDistance, 2);
		outputMembers.push_back(LLVMArrayType(LLVMFloatTypeInContext(context), 1));
		break;

	case ExecutionModelTessellationControl:
		break;

	case ExecutionModelTessellationEvaluation:
		break;

	case ExecutionModelGeometry:
		break;

	case ExecutionModelFragment:
		builtinInputMapping.emplace_back(BuiltInFragCoord, 0);
		inputMembers.push_back(LLVMVectorType(LLVMFloatTypeInContext(context), 4));

		// TODO: Only when extension enabled
		builtinOutputMapping.emplace_back(BuiltInFragStencilRefEXT, 0);
		outputMembers.push_back(LLVMInt32TypeInContext(context));
		break;

	case ExecutionModelGLCompute:
	case ExecutionModelKernel:
		builtinInputMapping.emplace_back(BuiltInGlobalInvocationId, 0);
		inputMembers.push_back(LLVMVectorType(LLVMInt32TypeInContext(context), 3));

		builtinInputMapping.emplace_back(BuiltInLocalInvocationId, 1);
		inputMembers.push_back(LLVMVectorType(LLVMInt32TypeInContext(context), 3));

		builtinInputMapping.emplace_back(BuiltInWorkgroupId, 2);
		inputMembers.push_back(LLVMVectorType(LLVMInt32TypeInContext(context), 3));
		break;

	default:
		TODO_ERROR();
	}

	auto llvmType = StructType(inputMembers, "_BuiltinInput", true);
	builtinInputVariable = GlobalVariable(llvmType, LLVMExternalLinkage, "_builtinInput");

	llvmType = StructType(outputMembers, "_BuiltinOutput", true);
	builtinOutputVariable = GlobalVariable(llvmType, LLVMExternalLinkage, "_builtinOutput");
}

CompiledModule* CompileSPIRVModule(CPJit* jit, const SPIRV::SPIRVModule* spirvModule, spv::ExecutionModel executionModel, const SPIRV::SPIRVFunction* entryPoint, const VkSpecializationInfo* specializationInfo)
{
	SPIRVCompiledModuleBuilder builder
	{
		spirvModule,
		executionModel,
		entryPoint,
		specializationInfo
	};
	return Compile(&builder, jit);
}

std::string MangleName(const SPIRV::SPIRVVariable* variable)
{
	auto name = variable->getName();
	if (name.empty())
	{
		switch (variable->getStorageClass())
		{
		case StorageClassInput:
		case StorageClassOutput:
			if (variable->hasDecorate(DecorationLocation))
			{
				name = "@location" + std::to_string(*variable->getDecorate(DecorationLocation).begin());
			}
			else
			{
				TODO_ERROR();
			}
			break;

		case StorageClassUniformConstant:
		case StorageClassUniform:
		case StorageClassStorageBuffer:
			if (variable->hasDecorate(DecorationDescriptorSet) && variable->hasDecorate(DecorationBinding))
			{
				const auto descriptorSet = *variable->getDecorate(DecorationDescriptorSet).begin();
				const auto binding = *variable->getDecorate(DecorationBinding).begin();
				name = "@set" + std::to_string(descriptorSet) + ":binding" + std::to_string(binding);
			}
			else
			{
				TODO_ERROR();
			}
			break;
			
		case StorageClassWorkgroup:
		case StorageClassCrossWorkgroup:
		case StorageClassPrivate:
		case StorageClassFunction:
		case StorageClassGeneric:
			return "@" + std::to_string(variable->getId());
			
		case StorageClassPushConstant:
			return "_pc_";
			
		case StorageClassAtomicCounter:
			TODO_ERROR();
			
		case StorageClassImage:
			TODO_ERROR();

		default:
			TODO_ERROR();
		}
	}

	name += ":" + std::to_string(variable->getId());
	
	switch (variable->getStorageClass())
	{
	case StorageClassUniformConstant:
		return "_uniformc_" + name;

	case StorageClassInput:
		return "_input_" + name;

	case StorageClassUniform:
		return "_uniform_" + name;

	case StorageClassOutput:
		return "_output_" + name;

	case StorageClassWorkgroup:
	case StorageClassCrossWorkgroup:
		TODO_ERROR();
		
	case StorageClassPrivate:
		return name;

	case StorageClassFunction:
		return name;

	case StorageClassGeneric:
		TODO_ERROR();
		
	case StorageClassPushConstant:
		return "_pc_" + name;
		
	case StorageClassAtomicCounter:
	case StorageClassImage:
		TODO_ERROR();
		
	case StorageClassStorageBuffer:
		return "_buffer_" + name;

	default:
		TODO_ERROR();
	}
}

std::string MangleName(const SPIRV::SPIRVFunction* function)
{
	const auto& name = function->getName();
	if (name.empty())
	{
		return "@" + std::to_string(function->getId());
	}

	return name + ":" + std::to_string(function->getId());
}
