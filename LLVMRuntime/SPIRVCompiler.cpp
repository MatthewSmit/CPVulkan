#include "SPIRVCompiler.h"

#include "SPIRVInstruction.h"
#include "SPIRVModule.h"

#include <Base.h>

#include <llvm/ExecutionEngine/Orc/JITTargetMachineBuilder.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>

#include <llvm-c/Target.h>

#include <gsl/gsl>

#include <cstdlib>

constexpr auto ALIGNMENT = 8;

struct State
{
	llvm::DataLayout& layout;
	llvm::LLVMContext& context;
	llvm::IRBuilder<>& builder;
	llvm::Module* module;
	const VkSpecializationInfo* specializationInfo;
	
	std::unordered_map<uint32_t, llvm::Type*> typeMapping;
	std::unordered_map<uint32_t, llvm::Value*> valueMapping;
	std::unordered_map<llvm::Type*, std::vector<uint32_t>> structIndexMapping;
	std::unordered_set<uint32_t> variablePointers;
	std::unordered_map<uint32_t, llvm::Function*> functionMapping;
	std::unordered_map<std::string, llvm::Function*> functionCache;
	
	llvm::GlobalVariable* builtinInputVariable;
	llvm::GlobalVariable* builtinOutputVariable;
	std::vector<std::pair<BuiltIn, uint32_t>> builtinInputMapping{};
	std::vector<std::pair<BuiltIn, uint32_t>> builtinOutputMapping{};
	llvm::GlobalVariable* userData;
};

static llvm::Value* ConvertValue(State& state, SPIRV::SPIRVValue* spirvValue, llvm::Function* currentFunction);
static llvm::Function* ConvertFunction(State& state, const SPIRV::SPIRVFunction* spirvFunction);

static void Dump(llvm::Module* llvmModule)
{
	std::string dump;
	llvm::raw_string_ostream DumpStream(dump);
	llvmModule->print(DumpStream, nullptr);
	std::cout << dump << std::endl;
}

CP_DLL_EXPORT std::string DumpType(llvm::Type* llvmType)
{
	std::string type_str;
	llvm::raw_string_ostream rso(type_str);
	llvmType->print(rso);
	return rso.str();
}

static std::string GetTypeName(const char* baseName, const std::string& postfixes)
{
	if (postfixes.empty())
	{
		return baseName;
	}

	return std::string{baseName} + "_" + postfixes;
}

static std::string GetTypeName(SPIRV::SPIRVType* type)
{
	switch (type->getOpCode())
	{
	case OpTypeInt:
		return (static_cast<SPIRV::SPIRVTypeInt*>(type)->isSigned() ? "I" : "U") + std::to_string(type->getIntegerBitWidth());
		
	case OpTypeFloat:
		return "F" + std::to_string(type->getFloatBitWidth());
		
	case OpTypeVector:
		return GetTypeName(type->getVectorComponentType()) + "[" + std::to_string(type->getVectorComponentCount()) + "]";
		
	case OpTypeMatrix:
		return GetTypeName(type->getMatrixComponentType()) + "[" + std::to_string(type->getMatrixColumnCount()) + "," + std::to_string(type->getMatrixRowCount()) + "]";
		
	default:
		FATAL_ERROR();
	}
}

static std::string GetImageTypeName(const SPIRV::SPIRVTypeImage* imageType)
{
	static const char* accessQualifierStr[3]
	{
		"ro",
		"wo",
		"rw",
	};
	
	const char* sampledType;
	switch (imageType->getSampledType()->getOpCode())
	{
	case OpTypeFloat:
		switch (imageType->getSampledType()->getFloatBitWidth())
		{
		case 32:
			sampledType = "float";
			break;
			
		default:
			FATAL_ERROR();
		}
		break;
		
	case OpTypeInt:
		if (static_cast<SPIRV::SPIRVTypeInt*>(imageType->getSampledType())->isSigned())
		{
			switch (imageType->getSampledType()->getIntegerBitWidth())
			{
			case 32:
				sampledType = "int32";
				break;

			default:
				FATAL_ERROR();
			}
		}
		else
		{
			switch (imageType->getSampledType()->getIntegerBitWidth())
			{
			case 32:
				sampledType = "uint32";
				break;

			default:
				FATAL_ERROR();
			}
		}
		break;
		
	default:
		FATAL_ERROR();
	}

	const auto& descriptor = imageType->getDescriptor();
	
	std::string postfixStr;
	llvm::raw_string_ostream outputStream(postfixStr);
	outputStream << sampledType
		<< '_' << descriptor.Dim
		<< '_' << descriptor.Depth
		<< '_' << descriptor.Arrayed
		<< '_' << descriptor.MS
		<< '_' << descriptor.Sampled
		<< '_' << descriptor.Format
		<< '_' << gsl::at(accessQualifierStr, imageType->hasAccessQualifier() ? imageType->getAccessQualifier() : AccessQualifierReadOnly);

	return GetTypeName("Image", outputStream.str());
}

llvm::Type* CreateOpaqueImageType(State& state, const std::string& name)
{
	auto opaqueType = state.module->getTypeByName(name);
	if (!opaqueType)
	{
		opaqueType = llvm::StructType::create(state.module->getContext(), name);
		llvm::SmallVector<llvm::Type*, 4> types;
		types.push_back(state.builder.getInt32Ty());
		types.push_back(state.builder.getInt8PtrTy());
		types.push_back(state.builder.getInt8PtrTy());
		opaqueType->setBody(types, false);
	}

	return llvm::PointerType::get(opaqueType, 0);
}

static llvm::Type* ConvertType(State& state, SPIRV::SPIRVType* spirvType, bool isClassMember = false)
{
	const auto cachedType = state.typeMapping.find(spirvType->getId());
	if (cachedType != state.typeMapping.end())
	{
		return cachedType->second;
	}
	
	llvm::Type* llvmType;
	
	switch (spirvType->getOpCode())
	{
	case OpTypeVoid:
		llvmType = llvm::Type::getVoidTy(state.context);
		break;
		
	case OpTypeBool:
		llvmType = llvm::Type::getInt1Ty(state.context);
		break;
		
	case OpTypeInt:
		llvmType = llvm::Type::getIntNTy(state.context, spirvType->getIntegerBitWidth());
		break;
		
	case OpTypeFloat:
		switch (spirvType->getFloatBitWidth())
		{
		case 16:
			llvmType = llvm::Type::getHalfTy(state.context);
			break;
			
		case 32:
			llvmType = llvm::Type::getFloatTy(state.context);
			break;

		case 64:
			llvmType = llvm::Type::getDoubleTy(state.context);
			break;

		default:
			FATAL_ERROR();
		}
		
		break;
		
	case OpTypeArray:
		// TODO: stride member variables
		llvmType = llvm::ArrayType::get(ConvertType(state, spirvType->getArrayElementType()), spirvType->getArrayLength());
		break;
		
	case OpTypePointer:
		llvmType = llvm::PointerType::get(ConvertType(state, spirvType->getPointerElementType(), isClassMember), 0);
		break;
		
	case OpTypeVector:
		llvmType = llvm::VectorType::get(ConvertType(state, spirvType->getVectorComponentType()), spirvType->getVectorComponentCount());
		break;
		
	case OpTypeOpaque:
		llvmType = llvm::StructType::create(state.context, spirvType->getName());
		break;
		
	case OpTypeFunction:
		{
			const auto functionType = static_cast<SPIRV::SPIRVTypeFunction*>(spirvType);
			const auto returnType = ConvertType(state, functionType->getReturnType());
			std::vector<llvm::Type*> parameters;
			for (size_t i = 0; i != functionType->getNumParameters(); ++i)
			{
				parameters.push_back(ConvertType(state, functionType->getParameterType(i)));
			}
			llvmType = llvm::FunctionType::get(returnType, parameters, false);
			break;
		}
		
	case OpTypeImage:
		{
			const auto image = static_cast<SPIRV::SPIRVTypeImage*>(spirvType);
			llvmType = CreateOpaqueImageType(state, GetImageTypeName(image));
			break;
		}
		
	case OpTypeSampler:
		{
			llvmType = CreateOpaqueImageType(state, "Sampler");
			break;
		}
		
	case OpTypeSampledImage:
		{
			const auto sampledImage = static_cast<SPIRV::SPIRVTypeSampledImage*>(spirvType);
			llvmType = CreateOpaqueImageType(state, "Sampled" + GetImageTypeName(sampledImage->getImageType()));
			break;
		}
		
	case OpTypeStruct:
		{
			const auto strct = static_cast<SPIRV::SPIRVTypeStruct*>(spirvType);
			const auto& name = strct->getName();
			auto llvmStruct = llvm::StructType::create(state.context, name);
			auto& indexMapping = state.structIndexMapping[llvmStruct];
			llvm::SmallVector<llvm::Type*, 4> types;
			uint32_t currentOffset = 0;
			for (size_t i = 0; i != strct->getMemberCount(); ++i)
			{
				const auto decorate = strct->getMemberDecorate(i, DecorationOffset);
				if (decorate && decorate->getLiteral(0) != currentOffset)
				{
					const auto offset = decorate->getLiteral(0);
					if (offset < currentOffset)
					{
						FATAL_ERROR();
					}
					else
					{
						auto difference = offset - currentOffset;
						while (difference >= 8)
						{
							types.push_back(state.builder.getInt64Ty());
							difference -= 8;
						}
						while (difference >= 4)
						{
							types.push_back(state.builder.getInt32Ty());
							difference -= 4;
						}
						while (difference >= 2)
						{
							types.push_back(state.builder.getInt16Ty());
							difference -= 2;
						}
						while (difference >= 1)
						{
							types.push_back(state.builder.getInt8Ty());
							difference -= 1;
						}
					}
					currentOffset = offset;
				}
				const auto llvmMemberType = ConvertType(state, strct->getMemberType(i), true);
				indexMapping.push_back(types.size());
				types.push_back(llvmMemberType);
				currentOffset += (state.layout.getTypeSizeInBits(llvmMemberType) + 7) / 8;
			}
			llvmStruct->setBody(types, strct->isPacked());
			llvmType = llvmStruct;
			break;
		}
		
	case OpTypeMatrix:
		{
			auto name = std::string{"Matrix"};
			name += std::to_string(spirvType->getMatrixColumnCount());
			name += 'x';
			name += std::to_string(spirvType->getMatrixRowCount());

			auto llvmStruct = llvm::StructType::create(state.context, name);
			auto& indexMapping = state.structIndexMapping[llvmStruct];
			llvm::SmallVector<llvm::Type*, 4> types;
			for (auto i = 0u; i < spirvType->getMatrixColumnCount(); i++)
			{
				indexMapping.push_back(types.size());
				types.push_back(ConvertType(state, spirvType->getMatrixVectorType(), true));
			}
			llvmStruct->setBody(types);
			llvmType = llvmStruct;
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
			FATAL_ERROR();
		}
		
	case OpTypePipeStorage:
		{
			//   auto PST = static_cast<SPIRVTypePipeStorage *>(T);
			//   return mapType(
			//       T, getOrCreateOpaquePtrType(M, transOCLPipeStorageTypeName(PST),
			//                                   getOCLOpaqueTypeAddrSpace(T->getOpCode())));
			FATAL_ERROR();
		}

	case OpTypeRuntimeArray:
		{
			const auto runtimeArray = static_cast<SPIRV::SPIRVTypeRuntimeArray*>(spirvType);
			const auto llvmElementType = ConvertType(state, runtimeArray->getElementType(), isClassMember);
			// const auto name = DumpType(llvmElementType) + "[]";
			llvmType = llvm::PointerType::get(llvmElementType, 0);

			// auto llvmStruct = llvm::StructType::create(state.context, name);
			// llvm::SmallVector<llvm::Type*, 2> types;
			// types.push_back(llvm::Type::getInt32Ty(state.context));
			// types.push_back(llvm::PointerType::get(llvmElementType, 0));
			// llvmStruct->setBody(types);
			// llvmType = llvmStruct;
			break;
		}
		
	default:
		FATAL_ERROR();
	}
	
	state.typeMapping[spirvType->getId()] = llvmType;
	
	return llvmType;
}

static llvm::GlobalValue::LinkageTypes ConvertLinkage(SPIRV::SPIRVValue* value)
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
		FATAL_ERROR();
		
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
		FATAL_ERROR();
		
	case LinkageTypeInternal:
		return llvm::GlobalValue::ExternalLinkage;
		
	default:
		FATAL_ERROR();
	}
}

static bool IsOpaqueType(SPIRV::SPIRVType* spirvType)
{
	switch (spirvType->getOpCode())
	{
	case OpTypeVoid:
		FATAL_ERROR();

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

	case OpTypeStruct:
		for (auto i = 0u; i < spirvType->getStructMemberCount(); i++)
		{
			if (IsOpaqueType(spirvType->getStructMemberType(i)))
			{
				FATAL_ERROR();
			}
		}
		return false;
		
	case OpTypeArray:
	case OpTypeRuntimeArray:
		return IsOpaqueType(spirvType->getArrayElementType());
		
	case OpTypePointer:
	case OpTypeOpaque:
	case OpTypeFunction:
	case OpTypePipe:
	case OpTypePipeStorage:
		FATAL_ERROR();
		
	default:
		FATAL_ERROR();
	}
}

static bool HasSpecOverride(const State& state, SPIRV::SPIRVValue* spirvValue)
{
	if (state.specializationInfo == nullptr || !spirvValue->hasDecorate(DecorationSpecId))
	{
		return false;
	}

	const auto id = *spirvValue->getDecorate(DecorationSpecId).begin();
	for (auto i = 0u; i < state.specializationInfo->mapEntryCount; i++)
	{
		const auto& entry = state.specializationInfo->pMapEntries[i];
		if (entry.constantID == id)
		{
			return true;
		}
	}
	return false;
}

template<typename T>
static T GetSpecOverride(const State& state, SPIRV::SPIRVValue* spirvValue)
{
	const auto id = *spirvValue->getDecorate(DecorationSpecId).begin();
	for (auto i = 0u; i < state.specializationInfo->mapEntryCount; i++)
	{
		const auto& entry = state.specializationInfo->pMapEntries[i];
		if (entry.constantID == id)
		{
			if (entry.size != sizeof(T))
			{
				FATAL_ERROR();
			}

			return *reinterpret_cast<const T*>(static_cast<const uint8_t*>(state.specializationInfo->pData) + entry.offset);
		}
	}

	FATAL_ERROR();
}

template<typename T>
static T GetConstantValue(llvm::Constant* constant)
{
	if constexpr (std::is_same<T, bool>::value)
	{
		FATAL_ERROR();
	}
	
	if constexpr (std::numeric_limits<T>::is_integer && std::numeric_limits<T>::is_signed)
	{
		return constant->getUniqueInteger().getSExtValue();
	}

	if constexpr (std::numeric_limits<T>::is_integer && !std::numeric_limits<T>::is_signed)
	{
		return constant->getUniqueInteger().getZExtValue();
	}
	
	FATAL_ERROR();
}

template<typename T>
static llvm::Constant* SetConstantValue(State& state, llvm::Type* type, T value)
{
	if constexpr (std::is_same<T, bool>::value)
	{
		return value ? llvm::ConstantInt::getTrue(state.context) : llvm::ConstantInt::getFalse(state.context);
	}

	if constexpr (std::numeric_limits<T>::is_integer)
	{
		return llvm::ConstantInt::get(type, value, std::numeric_limits<T>::is_signed);
	}

	FATAL_ERROR();
}

template<typename Input, typename Output>
static llvm::Constant* SpecOperation(State& state, llvm::Type* type, SPIRV::SPIRVValue* lhs, SPIRV::SPIRVValue* rhs, std::function<Output(Input, Input)> operation)
{
	const auto llvmLhs = llvm::dyn_cast<llvm::Constant>(ConvertValue(state, lhs, nullptr));
	const auto llvmRhs = llvm::dyn_cast<llvm::Constant>(ConvertValue(state, rhs, nullptr));
	if (llvmLhs->getType()->isVectorTy())
	{
		FATAL_ERROR();
	}
	
	if (llvmLhs->getType()->isStructTy())
	{
		FATAL_ERROR();
	}

	if (llvmLhs->getType() != llvmRhs->getType())
	{
		FATAL_ERROR();
	}

	const auto realLhs = GetConstantValue<Input>(llvmLhs);
	const auto realRhs = GetConstantValue<Input>(llvmRhs);
	const auto realResult = operation(realLhs, realRhs);
	return SetConstantValue(state, type, realResult);
}

static llvm::Value* HandleSpecConstantOperation(State& state, SPIRV::SPIRVSpecConstantOp* spirvValue)
{
	const auto operation = static_cast<Op>(spirvValue->getOpWord(0));
	const auto type = ConvertType(state, spirvValue->getType());

	switch (operation)
	{
	case OpSConvert:
	case OpUConvert:
	case OpFConvert:
	case OpSNegate:
	case OpNot:
		FATAL_ERROR();
	case OpIAdd:
		return SpecOperation<int64_t, int64_t>(state, type, spirvValue->getOpValue(1), spirvValue->getOpValue(2), [](auto x, auto y) { return x + y; });
	case OpISub:
		return SpecOperation<int64_t, int64_t>(state, type, spirvValue->getOpValue(1), spirvValue->getOpValue(2), [](auto x, auto y) { return x - y; });
	case OpIMul:
		return SpecOperation<int64_t, int64_t>(state, type, spirvValue->getOpValue(1), spirvValue->getOpValue(2), [](auto x, auto y) { return x * y; });
	case OpUDiv:
		return SpecOperation<uint64_t, uint64_t>(state, type, spirvValue->getOpValue(1), spirvValue->getOpValue(2), [](auto x, auto y) { return x / y; });
	case OpSDiv:
		return SpecOperation<int64_t, int64_t>(state, type, spirvValue->getOpValue(1), spirvValue->getOpValue(2), [](auto x, auto y) { return x + y; });
	case OpUMod:
	case OpSRem:
	case OpSMod:
	case OpShiftRightLogical:
	case OpShiftRightArithmetic:
	case OpShiftLeftLogical:
	case OpBitwiseOr:
	case OpBitwiseXor:
	case OpBitwiseAnd:
	case OpVectorShuffle:
	case OpCompositeExtract:
	case OpCompositeInsert:
	case OpLogicalOr:
	case OpLogicalAnd:
	case OpLogicalNot:
	case OpLogicalEqual:
	case OpLogicalNotEqual:
	case OpSelect:
	case OpIEqual:
	case OpINotEqual:
	case OpULessThan:
	case OpSLessThan:
	case OpUGreaterThan:
	case OpSGreaterThan:
	case OpULessThanEqual:
	case OpSLessThanEqual:
	case OpUGreaterThanEqual:
		FATAL_ERROR();
	case OpSGreaterThanEqual:
		return SpecOperation<int64_t, bool>(state, type, spirvValue->getOpValue(1), spirvValue->getOpValue(2), [](auto x, auto y) { return x >= y; });
	case OpQuantizeToF16:
	case OpConvertFToS:
	case OpConvertSToF:
	case OpConvertFToU:
	case OpConvertUToF:
	case OpConvertPtrToU:
	case OpConvertUToPtr:
	case OpGenericCastToPtr:
	case OpPtrCastToGeneric:
	case OpBitcast:
	case OpFNegate:
	case OpFAdd:
	case OpFSub:
	case OpFMul:
	case OpFDiv:
	case OpFRem:
	case OpFMod:
	case OpAccessChain:
	case OpInBoundsAccessChain:
	case OpPtrAccessChain:
	case OpInBoundsPtrAccessChain:
	default:
		FATAL_ERROR();
	}
}

static llvm::Value* ConvertValueNoDecoration(State& state, SPIRV::SPIRVValue* spirvValue, llvm::Function* currentFunction)
{
	switch (spirvValue->getOpCode())
	{
	case OpSpecConstant:
		if (HasSpecOverride(state, spirvValue))
		{
			const auto spirvConstant = static_cast<SPIRV::SPIRVConstant*>(spirvValue);
			const auto spirvType = spirvConstant->getType();
			const auto llvmType = ConvertType(state, spirvType);
			switch (spirvType->getOpCode())
			{
			case OpTypeInt:
				switch (spirvType->getIntegerBitWidth())
				{
				case 8:
					return llvm::ConstantInt::get(llvmType, GetSpecOverride<uint8_t>(state, spirvValue), static_cast<SPIRV::SPIRVTypeInt*>(spirvType)->isSigned());
					
				case 16:
					return llvm::ConstantInt::get(llvmType, GetSpecOverride<uint16_t>(state, spirvValue), static_cast<SPIRV::SPIRVTypeInt*>(spirvType)->isSigned());
					
				case 32:
					return llvm::ConstantInt::get(llvmType, GetSpecOverride<uint32_t>(state, spirvValue), static_cast<SPIRV::SPIRVTypeInt*>(spirvType)->isSigned());
					
				case 64:
					return llvm::ConstantInt::get(llvmType, GetSpecOverride<uint64_t>(state, spirvValue), static_cast<SPIRV::SPIRVTypeInt*>(spirvType)->isSigned());
					
				default:
					FATAL_ERROR();
				}

			case OpTypeFloat:
				switch (spirvType->getFloatBitWidth())
				{
				case 16:
					return llvm::ConstantFP::get(llvmType, llvm::APFloat(llvm::APFloat::IEEEhalf(), llvm::APInt(spirvType->getFloatBitWidth(), GetSpecOverride<uint16_t>(state, spirvValue))));

				case 32:
					return llvm::ConstantFP::get(llvmType, llvm::APFloat(llvm::APFloat::IEEEsingle(), llvm::APInt(spirvType->getFloatBitWidth(), GetSpecOverride<uint32_t>(state, spirvValue))));

				case 64:
					return llvm::ConstantFP::get(llvmType, llvm::APFloat(llvm::APFloat::IEEEdouble(), llvm::APInt(spirvType->getFloatBitWidth(), GetSpecOverride<uint64_t>(state, spirvValue))));

				default:
					FATAL_ERROR();
				}

			default:
				FATAL_ERROR();
			}
		}
		
	case OpConstant:
		{
			const auto spirvConstant = static_cast<SPIRV::SPIRVConstant*>(spirvValue);
			const auto spirvType = spirvConstant->getType();
			const auto llvmType = ConvertType(state, spirvType);
			switch (spirvType->getOpCode())
			{
			case OpTypeInt:
				return llvm::ConstantInt::get(llvmType, spirvConstant->getInt64Value(), static_cast<SPIRV::SPIRVTypeInt*>(spirvType)->isSigned());

			case OpTypeFloat:
				{
					const llvm::fltSemantics* semantics;
					switch (spirvType->getFloatBitWidth())
					{
					case 16:
						semantics = &llvm::APFloat::IEEEhalf();
						break;
						
					case 32:
						semantics = &llvm::APFloat::IEEEsingle();
						break;
						
					case 64:
						semantics = &llvm::APFloat::IEEEdouble();
						break;
						
					default:
						FATAL_ERROR();
					}
					
					return llvm::ConstantFP::get(llvmType, llvm::APFloat(*semantics,
					                                                     llvm::APInt(spirvType->getFloatBitWidth(), spirvConstant->getInt64Value())));
				}

			default:
				FATAL_ERROR();
			}
		}

	case OpSpecConstantTrue:
		if (HasSpecOverride(state, spirvValue))
		{
			return GetSpecOverride<VkBool32>(state, spirvValue)
				       ? llvm::ConstantInt::getTrue(state.context)
				       : llvm::ConstantInt::getFalse(state.context);
		}
		
	case OpConstantTrue:
		return llvm::ConstantInt::getTrue(state.context);

	case OpSpecConstantFalse:
		if (HasSpecOverride(state, spirvValue))
		{
			return GetSpecOverride<VkBool32>(state, spirvValue)
				       ? llvm::ConstantInt::getTrue(state.context)
				       : llvm::ConstantInt::getFalse(state.context);
		}
		
	case OpConstantFalse:
		return llvm::ConstantInt::getFalse(state.context);

	case OpConstantNull:
		return llvm::Constant::getNullValue(ConvertType(state, spirvValue->getType()));

	case OpSpecConstantComposite:
		if (HasSpecOverride(state, spirvValue))
		{
			FATAL_ERROR();
		}
		
	case OpConstantComposite:
		{
			const auto constantComposite = static_cast<SPIRV::SPIRVConstantComposite*>(spirvValue);
			std::vector<llvm::Constant*> constants;
			for (auto& element : constantComposite->getElements())
			{
				constants.push_back(llvm::dyn_cast<llvm::Constant>(ConvertValue(state, element, currentFunction)));
			}
			switch (constantComposite->getType()->getOpCode())
			{
			case OpTypeVector:
				return llvm::ConstantVector::get(constants);

			case OpTypeArray:
				return llvm::ConstantArray::get(llvm::dyn_cast<llvm::ArrayType>(ConvertType(state, constantComposite->getType())), constants);
				
			case OpTypeStruct:
				// 	auto BCCTy = dyn_cast<StructType>(transType(BCC->getType()));
				// 	auto Members = BCCTy->getNumElements();
				// 	auto Constants = CV.size();
				// 	// if we try to initialize constant TypeStruct, add bitcasts
				// 	// if src and dst types are both pointers but to different types
				// 	if (Members == Constants) {
				// 		for (unsigned I = 0; I < Members; ++I) {
				// 			if (CV[I]->getType() == BCCTy->getElementType(I))
				// 				continue;
				// 			if (!CV[I]->getType()->isPointerTy() ||
				// 				!BCCTy->getElementType(I)->isPointerTy())
				// 				continue;
				//
				// 			CV[I] = ConstantExpr::getBitCast(CV[I], BCCTy->getElementType(I));
				// 		}
				// 	}
				//
				// 	return mapValue(BV,
				// 	                ConstantStruct::get(
				// 		                dyn_cast<StructType>(transType(BCC->getType())), CV));
				FATAL_ERROR();

			default:
				FATAL_ERROR();
			}
		}

	case OpConstantSampler:
		{
			// auto BCS = static_cast<SPIRVConstantSampler*>(BV);
			// return mapValue(BV, oclTransConstantSampler(BCS, BB));
			FATAL_ERROR();
		}

	case OpConstantPipeStorage:
		{
			// auto BCPS = static_cast<SPIRVConstantPipeStorage*>(BV);
			// return mapValue(BV, oclTransConstantPipeStorage(BCPS));
			FATAL_ERROR();
		}
		
	case OpSpecConstantOp:
		return HandleSpecConstantOperation(state, static_cast<SPIRV::SPIRVSpecConstantOp*>(spirvValue));

	case OpUndef:
		// return mapValue(BV, UndefValue::get(transType(BV->getType())));
		FATAL_ERROR();

	case OpVariable:
		{
			const auto variable = static_cast<SPIRV::SPIRVVariable*>(spirvValue);
			auto isPointer = (variable->getStorageClass() == StorageClassUniform || variable->getStorageClass() == StorageClassUniformConstant) && !IsOpaqueType(variable->getType()->getPointerElementType());
			if (isPointer)
			{
				if (variable->getType()->getPointerElementType()->hasDecorate(DecorationBufferBlock))
				{
					isPointer = false;
				}
			}
			const auto llvmType = isPointer
				                      ? ConvertType(state, variable->getType())
				                      : ConvertType(state, variable->getType()->getPointerElementType());
			const auto linkage = ConvertLinkage(variable);
			const auto name = MangleName(variable);
			llvm::Constant* initialiser = nullptr;
			const auto spirvInitialiser = variable->getInitializer();
			if (spirvInitialiser)
			{
				initialiser = llvm::dyn_cast<llvm::Constant>(ConvertValue(state, spirvInitialiser, currentFunction));
			}
			else if (variable->getStorageClass() == StorageClassFunction)
			{
				return state.builder.CreateAlloca(llvmType);
			}
			else
			{
				initialiser = llvm::Constant::getNullValue(llvmType);
			}

			if (isPointer)
			{
				state.variablePointers.insert(variable->getId());
			}

			auto llvmVariable = new llvm::GlobalVariable(*state.module,
			                                             llvmType,
			                                             variable->isConstant(),
			                                             linkage,
			                                             initialiser,
			                                             name,
			                                             nullptr,
			                                             llvm::GlobalValue::NotThreadLocal);
			llvmVariable->setUnnamedAddr(variable->isConstant() && llvmType->isArrayTy() && llvmType->getArrayElementType()->isIntegerTy(8)
				                             ? llvm::GlobalValue::UnnamedAddr::Global
				                             : llvm::GlobalValue::UnnamedAddr::None);
			return llvmVariable;
		}

	case OpFunctionParameter:
		{
			const auto functionParameter = static_cast<SPIRV::SPIRVFunctionParameter*>(spirvValue);
			assert(currentFunction);
			uint32_t argNumber = 0;
			for (auto i = currentFunction->arg_begin(), end = currentFunction->arg_end(); i != end; ++i, ++argNumber)
			{
				if (argNumber == functionParameter->getArgNo())
				{
					return &*i;
				}
			}

			FATAL_ERROR();
		}

	case OpFunction:
		// return mapValue(BV, transFunction(static_cast<SPIRVFunction*>(BV)));
		FATAL_ERROR();

	case OpLabel:
		// return mapValue(BV, BasicBlock::Create(*Context, BV->getName(), F));
		FATAL_ERROR();

	default:
		break;
	}

	FATAL_ERROR();
}

static void ConvertDecoration(llvm::Value* llvmValue, SPIRV::SPIRVValue* spirvValue)
{
	if (auto alloca = llvm::dyn_cast<llvm::AllocaInst>(llvmValue))
	{
		uint32_t alignment;
		if (spirvValue->hasAlignment(&alignment))
		{
			alloca->setAlignment(alignment);
		}
	}

	if (auto globalVariable = llvm::dyn_cast<llvm::GlobalVariable>(llvmValue))
	{
		uint32_t alignment;
		if (spirvValue->hasAlignment(&alignment))
		{
			globalVariable->setAlignment(alignment);
		}
	}
}

static llvm::Value* ConvertBuiltin(State& state, BuiltIn builtin)
{
	for (const auto mapping : state.builtinInputMapping)
	{
		if (mapping.first == builtin)
		{
			llvm::Value* indices[]
			{
				llvm::ConstantInt::get(state.context, llvm::APInt(32, 0)),
				llvm::ConstantInt::get(state.context, llvm::APInt(32, mapping.second)),
			};
			return state.builder.CreateInBoundsGEP(state.builtinInputVariable, indices);
		}
	}

	for (const auto mapping : state.builtinOutputMapping)
	{
		if (mapping.first == builtin)
		{
			llvm::Value* indices[]
			{
				llvm::ConstantInt::get(state.context, llvm::APInt(32, 0)),
				llvm::ConstantInt::get(state.context, llvm::APInt(32, mapping.second)),
			};
			return state.builder.CreateInBoundsGEP(state.builtinOutputVariable, indices);
		}
	}
	
	FATAL_ERROR();
}

static llvm::Value* ConvertValue(State& state, SPIRV::SPIRVValue* spirvValue, llvm::Function* currentFunction)
{
	const auto cachedType = state.valueMapping.find(spirvValue->getId());
	if (cachedType != state.valueMapping.end())
	{
		if (spirvValue->hasId() && state.variablePointers.find(spirvValue->getId()) != state.variablePointers.end())
		{
			if (const auto variable = llvm::dyn_cast<llvm::GlobalVariable>(cachedType->second))
			{
				return state.builder.CreateAlignedLoad(variable, ALIGNMENT);
			}
		}
		return cachedType->second;
	}

	if (spirvValue->isVariable() && spirvValue->getType()->isTypePointer())
	{
		if (spirvValue->hasDecorate(DecorationBuiltIn))
		{
			const auto builtin = static_cast<BuiltIn>(*spirvValue->getDecorate(DecorationBuiltIn).begin());
			if (state.builder.GetInsertBlock() == nullptr)
			{
				return nullptr;
			}
			return ConvertBuiltin(state, builtin);
		}

		if (spirvValue->getType()->getPointerElementType()->hasMemberDecorate(DecorationBuiltIn))
		{
			if (static_cast<SPIRV::SPIRVVariable*>(spirvValue)->getStorageClass() == StorageClassInput)
			{
				return state.valueMapping[spirvValue->getId()] = state.builtinInputVariable;
			}
			if (static_cast<SPIRV::SPIRVVariable*>(spirvValue)->getStorageClass() == StorageClassOutput)
			{
				return state.valueMapping[spirvValue->getId()] = state.builtinOutputVariable;
			}
			FATAL_ERROR();
		}
	}

	auto llvmValue = ConvertValueNoDecoration(state, spirvValue, currentFunction);
	if (!spirvValue->getName().empty() && llvmValue->getName().empty())
	{
		llvmValue->setName(spirvValue->getName());
	}

	ConvertDecoration(llvmValue, spirvValue);

	state.valueMapping[spirvValue->getId()] = llvmValue;
	return llvmValue;
}

static std::vector<llvm::Value*> ConvertValue(State& state, std::vector<SPIRV::SPIRVValue*> spirvValues, llvm::Function* currentFunction)
{
	std::vector<llvm::Value*> results{spirvValues.size(), nullptr};
	for (size_t i = 0; i < spirvValues.size(); i++)
	{
		results[i] = ConvertValue(state, spirvValues[i], currentFunction);
	}
	return results;
}

static llvm::Function* GetInbuiltFunction(State& state, SPIRV::SPIRVMatrixTimesVector* matrixTimesVector)
{
	// TODO: Detect row/column major
	const auto matrix = matrixTimesVector->getMatrix()->getType();
	const auto vector = matrixTimesVector->getVector()->getType();

	const auto functionName = "@Matrix.Mult." + GetTypeName(matrix) + "." + GetTypeName(vector) + ".col";
	const auto cachedFunction = state.functionCache.find(functionName);
	if (cachedFunction != state.functionCache.end())
	{
		return cachedFunction->second;
	}

	llvm::Type* params[3];
	params[0] = llvm::PointerType::get(ConvertType(state, matrixTimesVector->getType()), 0);
	params[1] = llvm::PointerType::get(ConvertType(state, matrix), 0);
	params[2] = llvm::PointerType::get(ConvertType(state, vector), 0);
	
	const auto functionType = llvm::FunctionType::get(llvm::Type::getVoidTy(state.context), params, false);
	auto function = llvm::Function::Create(functionType, llvm::GlobalValue::ExternalLinkage, 0, functionName, state.module);
	function->addDereferenceableAttr(0, 16);
	function->addDereferenceableAttr(1, 64);
	function->addDereferenceableAttr(2, 16);
	state.functionCache[functionName] = function;
	return function;
}

static llvm::Function* GetInbuiltFunction(State& state, SPIRV::SPIRVMatrixTimesMatrix* matrixTimesMatrix)
{
	// TODO: Detect row/column major
	const auto left = matrixTimesMatrix->getMatrixLeft()->getType();
	const auto right = matrixTimesMatrix->getMatrixRight()->getType();

	const auto functionName = "@Matrix.Mult." + GetTypeName(left) + "." + GetTypeName(right) + ".col";
	const auto cachedFunction = state.functionCache.find(functionName);
	if (cachedFunction != state.functionCache.end())
	{
		return cachedFunction->second;
	}

	llvm::Type* params[3];
	params[0] = llvm::PointerType::get(ConvertType(state, matrixTimesMatrix->getType()), 0);
	params[1] = llvm::PointerType::get(ConvertType(state, left), 0);
	params[2] = llvm::PointerType::get(ConvertType(state, right), 0);
	
	const auto functionType = llvm::FunctionType::get(llvm::Type::getVoidTy(state.context), params, false);
	auto function = llvm::Function::Create(functionType, llvm::GlobalValue::ExternalLinkage, 0, functionName, state.module);
	function->addDereferenceableAttr(0, 64);
	function->addDereferenceableAttr(1, 64);
	function->addDereferenceableAttr(2, 64);
	state.functionCache[functionName] = function;
	return function;
}

static llvm::Function* GetInbuiltFunction(State& state, SPIRV::SPIRVDot* dot)
{
	const auto argumentType = dot->getOperand(0)->getType();

	const auto functionName = "@Vector.Dot." + GetTypeName(argumentType);
	const auto cachedFunction = state.functionCache.find(functionName);
	if (cachedFunction != state.functionCache.end())
	{
		return cachedFunction->second;
	}

	llvm::Type* params[]
	{
		llvm::PointerType::get(ConvertType(state, dot->getType()), 0),
		llvm::PointerType::get(ConvertType(state, argumentType), 0),
		llvm::PointerType::get(ConvertType(state, argumentType), 0),
	};
	
	const auto functionType = llvm::FunctionType::get(llvm::Type::getVoidTy(state.context), params, false);
	const auto function = llvm::Function::Create(functionType, llvm::GlobalValue::ExternalLinkage, 0, functionName, state.module);
	state.functionCache[functionName] = function;
	function->addDereferenceableAttr(0, 16);
	function->addDereferenceableAttr(1, 16);
	function->addDereferenceableAttr(2, 16);
	return function;
}

static llvm::Function* GetInbuiltFunction(State& state, SPIRV::SPIRVImageSampleImplicitLod* imageSampleImplicitLod)
{
	const auto spirvImageType = static_cast<SPIRV::SPIRVTypeSampledImage*>(imageSampleImplicitLod->getOperandTypes()[0])->getImageType();
	const auto llvmImageType = CreateOpaqueImageType(state, "Sampled" + GetImageTypeName(spirvImageType));

	const auto resultType = imageSampleImplicitLod->getType();
	const auto coordinateType = imageSampleImplicitLod->getOpValue(1)->getType();

	const auto cube = spirvImageType->getDescriptor().Dim == DimCube;
	const auto array = spirvImageType->getDescriptor().Arrayed;

	if (imageSampleImplicitLod->getOpWords().size() > 2)
	{
		FATAL_ERROR();
	}

	const auto functionName = "@Image.Sample.Implicit." + GetTypeName(resultType) + "." + GetTypeName(coordinateType) + (cube ? ".Cube" : "") + (array ? ".Array" : "");
	const auto cachedFunction = state.functionCache.find(functionName);
	if (cachedFunction != state.functionCache.end())
	{
		return cachedFunction->second;
	}

	llvm::Type* params[]
	{
		state.builder.getInt8PtrTy(),
		llvm::PointerType::get(ConvertType(state, resultType), 0),
		llvmImageType,
		llvm::PointerType::get(ConvertType(state, coordinateType), 0),
	};

	const auto functionType = llvm::FunctionType::get(llvm::Type::getVoidTy(state.context), params, false);
	auto function = llvm::Function::Create(functionType, llvm::GlobalValue::ExternalLinkage, 0, functionName, state.module);
	function->addDereferenceableAttr(1, 16);
	function->addDereferenceableAttr(3, 16);
	state.functionCache[functionName] = function;
	return function;
}

static llvm::Function* GetInbuiltFunction(State& state, SPIRV::SPIRVImageSampleExplicitLod* imageSampleExplicitLod)
{
	const auto spirvImageType = static_cast<SPIRV::SPIRVTypeSampledImage*>(imageSampleExplicitLod->getOperandTypes()[0])->getImageType();
	const auto llvmImageType = CreateOpaqueImageType(state, "Sampled" + GetImageTypeName(spirvImageType));
	
	const auto resultType = imageSampleExplicitLod->getType();
	const auto coordinateType = imageSampleExplicitLod->getOpValue(1)->getType();
	const auto operand = imageSampleExplicitLod->getOpWord(2);

	const auto cube = spirvImageType->getDescriptor().Dim == DimCube;
	const auto array = spirvImageType->getDescriptor().Arrayed;

	if (operand == 2)
	{
		if (imageSampleExplicitLod->getOpWords().size() > 4)
		{
			FATAL_ERROR();
		}

		const auto functionName = "@Image.Sample.Explicit." + GetTypeName(resultType) + "." + GetTypeName(coordinateType) + ".Lod" + (cube ? ".Cube" : "") + (array ? ".Array" : "");
		const auto cachedFunction = state.functionCache.find(functionName);
		if (cachedFunction != state.functionCache.end())
		{
			return cachedFunction->second;
		}

		llvm::Type* params[]
		{
			state.builder.getInt8PtrTy(),
			llvm::PointerType::get(ConvertType(state, resultType), 0),
			llvmImageType,
			llvm::PointerType::get(ConvertType(state, coordinateType), 0),
			llvm::Type::getFloatTy(state.context),
		};

		const auto functionType = llvm::FunctionType::get(llvm::Type::getVoidTy(state.context), params, false);
		auto function = llvm::Function::Create(functionType, llvm::GlobalValue::ExternalLinkage, 0, functionName, state.module);
		function->addDereferenceableAttr(1, 16);
		function->addDereferenceableAttr(3, 16);
		state.functionCache[functionName] = function;
		return function;
	}

	FATAL_ERROR();
}

static llvm::Function* GetInbuiltFunction(State& state, SPIRV::SPIRVImageFetch* imageFetch)
{
	const auto spirvImageType = static_cast<SPIRV::SPIRVTypeImage*>(imageFetch->getOperandTypes()[0]);
	const auto llvmImageType = CreateOpaqueImageType(state, GetImageTypeName(spirvImageType));
	
	const auto resultType = imageFetch->getType();
	const auto coordinateType = imageFetch->getOpValue(1)->getType();

	const auto cube = spirvImageType->getDescriptor().Dim == DimCube;
	const auto array = spirvImageType->getDescriptor().Arrayed;

	if (imageFetch->getOpWords().size() > 2)
	{
		FATAL_ERROR();
	}
	
	const auto functionName = "@Image.Fetch." + GetTypeName(resultType) + "." + GetTypeName(coordinateType) + (cube ? ".Cube" : "") + (array ? ".Array" : "");
	const auto cachedFunction = state.functionCache.find(functionName);
	if (cachedFunction != state.functionCache.end())
	{
		return cachedFunction->second;
	}
	
	llvm::Type* params[]
	{
		state.builder.getInt8PtrTy(),
		llvm::PointerType::get(ConvertType(state, resultType), 0),
		llvmImageType,
		llvm::PointerType::get(ConvertType(state, coordinateType), 0),
	};
	
	const auto functionType = llvm::FunctionType::get(llvm::Type::getVoidTy(state.context), params, false);
	auto function = llvm::Function::Create(functionType, llvm::GlobalValue::ExternalLinkage, 0, functionName, state.module);
	function->addDereferenceableAttr(1, 16);
	function->addDereferenceableAttr(3, 16);
	state.functionCache[functionName] = function;
	return function;
}

std::vector<llvm::Value*> MapBuiltin(State& state, BuiltIn builtin, const std::vector<std::pair<BuiltIn, uint32_t>>& builtinMapping)
{
	for (const auto mapping : builtinMapping)
	{
		if (mapping.first == builtin)
		{
			return std::vector<llvm::Value*>
			{
				llvm::ConstantInt::get(llvm::Type::getInt32Ty(state.context), mapping.second, false)
			};
		}
	}
	
	FATAL_ERROR();
}

std::vector<llvm::Value*> MapBuiltin(State& state, const std::vector<llvm::Value*>& indices, SPIRV::SPIRVType* type, const std::vector<std::pair<BuiltIn, uint32_t>>& builtinMapping)
{
	if (indices.size() != 1)
	{
		FATAL_ERROR();
	}

	const auto structType = type->getPointerElementType();
	if (!structType->isTypeStruct())
	{
		FATAL_ERROR();
	}

	auto decorates = static_cast<SPIRV::SPIRVTypeStruct*>(structType)->getMemberDecorates();
	
	if (const auto constant = llvm::dyn_cast<llvm::Constant>(indices[0]))
	{
		const auto index = constant->getUniqueInteger().getLimitedValue(0xFFFFFFFF);
		for (const auto decorate : decorates)
		{
			if (decorate.first.first == index && decorate.first.second == DecorationBuiltIn)
			{
				return MapBuiltin(state, static_cast<BuiltIn>(decorate.second->getLiteral(0)), builtinMapping);
			}
		}
		
		FATAL_ERROR();
	}
	else
	{
		FATAL_ERROR();
	}
}

static void SetLLVMLoopMetadata(State& state, SPIRV::SPIRVLoopMerge* loopMerge, llvm::BranchInst* branchInstruction)
{
	if (!loopMerge)
	{
		return;
	}

	const auto temp = llvm::MDNode::getTemporary(state.context, llvm::None);
	auto self = llvm::MDNode::get(state.context, temp.get());
	self->replaceOperandWith(0, self);

	const auto loopControl = loopMerge->getLoopControl();
	if (loopControl == LoopControlMaskNone)
	{
		branchInstruction->setMetadata("llvm.loop", self);
		return;
	}
	
	unsigned numParam = 0;
	std::vector<llvm::Metadata*> metadata;
	auto loopControlParameters = loopMerge->getLoopControlParameters();
	metadata.push_back(llvm::MDNode::get(state.context, self));
	
	// // To correctly decode loop control parameters, order of checks for loop
	// // control masks must match with the order given in the spec (see 3.23),
	// // i.e. check smaller-numbered bits first.
	// // Unroll and UnrollCount loop controls can't be applied simultaneously with
	// // DontUnroll loop control.
	// if (LC & LoopControlUnrollMask)
	//   Metadata.push_back(getMetadataFromName("llvm.loop.unroll.enable"));
	// else if (LC & LoopControlDontUnrollMask)
	//   Metadata.push_back(getMetadataFromName("llvm.loop.unroll.disable"));
	// if (LC & LoopControlDependencyInfiniteMask)
	//   Metadata.push_back(getMetadataFromName("llvm.loop.ivdep.enable"));
	// if (LC & LoopControlDependencyLengthMask) {
	//   if (!LoopControlParameters.empty()) {
	//     Metadata.push_back(llvm::MDNode::get(
	//         *Context,
	//         getMetadataFromNameAndParameter("llvm.loop.ivdep.safelen",
	//                                         LoopControlParameters[NumParam])));
	//     ++NumParam;
	//     assert(NumParam <= LoopControlParameters.size() &&
	//            "Missing loop control parameter!");
	//   }
	// }
	// // Placeholder for LoopControls added in SPIR-V 1.4 spec (see 3.23)
	// if (LC & LoopControlMinIterationsMask) {
	//   ++NumParam;
	//   assert(NumParam <= LoopControlParameters.size() &&
	//          "Missing loop control parameter!");
	// }
	// if (LC & LoopControlMaxIterationsMask) {
	//   ++NumParam;
	//   assert(NumParam <= LoopControlParameters.size() &&
	//          "Missing loop control parameter!");
	// }
	// if (LC & LoopControlIterationMultipleMask) {
	//   ++NumParam;
	//   assert(NumParam <= LoopControlParameters.size() &&
	//          "Missing loop control parameter!");
	// }
	// if (LC & LoopControlPeelCountMask) {
	//   ++NumParam;
	//   assert(NumParam <= LoopControlParameters.size() &&
	//          "Missing loop control parameter!");
	// }
	// if (LC & LoopControlPartialCountMask && !(LC & LoopControlDontUnrollMask)) {
	//   // If unroll factor is set as '1' - disable loop unrolling
	//   if (1 == LoopControlParameters[NumParam])
	//     Metadata.push_back(getMetadataFromName("llvm.loop.unroll.disable"));
	//   else
	//     Metadata.push_back(llvm::MDNode::get(
	//         *Context,
	//         getMetadataFromNameAndParameter("llvm.loop.unroll.count",
	//                                         LoopControlParameters[NumParam])));
	//   ++NumParam;
	//   assert(NumParam <= LoopControlParameters.size() &&
	//          "Missing loop control parameter!");
	// }
	FATAL_ERROR();
	llvm::MDNode* node = llvm::MDNode::get(state.context, metadata);
	
	// Set the first operand to refer itself
	node->replaceOperandWith(0, node);
	branchInstruction->setMetadata("llvm.loop", node);
}

static llvm::Value* ConvertOCLFromExtensionInstruction(State& state, const SPIRV::SPIRVExtInst* extensionInstruction, llvm::Function* currentFunction)
{
	FATAL_ERROR();
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

static llvm::Value* ConvertOGLFromExtensionInstruction(State& state, const SPIRV::SPIRVExtInst* extensionInstruction, llvm::Function* currentFunction)
{
	const auto entryPoint = static_cast<OpenGL::Entrypoints>(extensionInstruction->getExtOp());
	auto functionName = "@" + SPIRV::OGLExtOpMap::map(entryPoint);
	std::vector<llvm::Type*> argumentTypes{};
	for (const auto type : extensionInstruction->getArgumentValueTypes())
	{
		functionName += "." + GetTypeName(type);
		argumentTypes.push_back(ConvertType(state, type));
	}
	
	const auto cachedFunction = state.functionCache.find(functionName);
	llvm::Function* function;
	if (cachedFunction == state.functionCache.end())
	{
		const auto functionType = llvm::FunctionType::get(ConvertType(state, extensionInstruction->getType()), argumentTypes, false);
		function = llvm::Function::Create(functionType, llvm::GlobalValue::ExternalLinkage, 0, functionName, state.module);
		state.functionCache[functionName] = function;
	}
	else
	{
		function = cachedFunction->second;
	}

	return state.builder.CreateCall(function, ConvertValue(state, extensionInstruction->getArgumentValues(), currentFunction));
}

static llvm::Value* ConvertDebugFromExtensionInstruction(State& state, const SPIRV::SPIRVExtInst* extensionInstruction, llvm::Function* currentFunction)
{
	FATAL_ERROR();
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

static llvm::BasicBlock* ConvertBasicBlock(State& state, llvm::Function* currentFunction, const SPIRV::SPIRVBasicBlock* spirvBasicBlock)
{
	const auto cachedType = state.valueMapping.find(spirvBasicBlock->getId());
	if (cachedType != state.valueMapping.end())
	{
		return static_cast<llvm::BasicBlock*>(cachedType->second);
	}

	for (const auto decorate : spirvBasicBlock->getDecorates())
	{
		FATAL_ERROR();
	}
	
	const auto llvmBasicBlock = llvm::BasicBlock::Create(state.context, spirvBasicBlock->getName(), currentFunction);
	state.valueMapping[spirvBasicBlock->getId()] = llvmBasicBlock;
	state.builder.SetInsertPoint(llvmBasicBlock);

	for (auto i = 0u; i < spirvBasicBlock->getNumInst(); i++)
	{
		const auto instruction = spirvBasicBlock->getInst(i);
		for (const auto decorate : instruction->getDecorates())
		{
			if (decorate.first == DecorationRelaxedPrecision)
			{
			}
			else
			{
				FATAL_ERROR();
			}
		}

		llvm::Value* llvmValue = nullptr;
		switch (instruction->getOpCode())
		{
		case OpNop:
			break;

		case OpExtInst:
			{
				const auto extensionInstruction = reinterpret_cast<SPIRV::SPIRVExtInst*>(instruction);
				switch (extensionInstruction->getExtSetKind())
				{
				case SPIRV::SPIRVEIS_OpenCL:
					llvmValue = ConvertOCLFromExtensionInstruction(state, extensionInstruction, currentFunction);
					break;

				case SPIRV::SPIRVEIS_OpenGL:
					llvmValue = ConvertOGLFromExtensionInstruction(state, extensionInstruction, currentFunction);
					break;

				case SPIRV::SPIRVEIS_Debug:
					llvmValue = ConvertDebugFromExtensionInstruction(state, extensionInstruction, currentFunction);
					break;

				default:
					FATAL_ERROR();
				}
				break;
			}

		case OpFunctionCall:
			{
				const auto functionCall = reinterpret_cast<SPIRV::SPIRVFunctionCall*>(instruction);
				const auto llvmFunctionType = ConvertFunction(state, functionCall->getFunction());
				state.builder.SetInsertPoint(llvmBasicBlock);
				llvmValue = state.builder.CreateCall(llvmFunctionType, ConvertValue(state, functionCall->getArgumentValues(), currentFunction));
				break;
			}

		case OpVariable:
			// TODO: Alloca?
			llvmValue = ConvertValueNoDecoration(state, instruction, currentFunction);
			break;

			// case OpImageTexelPointer: break;

		case OpLoad:
			{
				const auto load = reinterpret_cast<SPIRV::SPIRVLoad*>(instruction);
				llvmValue = state.builder.CreateAlignedLoad(ConvertValue(state, load->getSrc(), currentFunction), ALIGNMENT, load->SPIRVMemoryAccess::isVolatile(), load->getName());
				if (load->isNonTemporal())
				{
					//   transNonTemporalMetadata(LI);
					FATAL_ERROR();
				}
				break;
			}

		case OpStore:
			{
				const auto store = reinterpret_cast<SPIRV::SPIRVStore*>(instruction);
				llvmValue = state.builder.CreateAlignedStore(ConvertValue(state, store->getSrc(), currentFunction),
				                                             ConvertValue(state, store->getDst(), currentFunction),
				                                             ALIGNMENT,
				                                             store->SPIRVMemoryAccess::isVolatile());
				if (store->isNonTemporal())
				{
					//   transNonTemporalMetadata(LI);
					FATAL_ERROR();
				}
				break;
			}

			// case OpCopyMemory: break;
			// case OpCopyMemorySized: break;

		case OpAccessChain:
		case OpInBoundsAccessChain:
		case OpPtrAccessChain:
		case OpInBoundsPtrAccessChain:
			{
				const auto accessChain = reinterpret_cast<SPIRV::SPIRVAccessChainBase*>(instruction);
				const auto base = ConvertValue(state, accessChain->getBase(), currentFunction);
				auto indices = ConvertValue(state, accessChain->getIndices(), currentFunction);

				if (base->getType() == state.builtinInputVariable->getType())
				{
					indices = MapBuiltin(state, indices, accessChain->getBase()->getType(), state.builtinInputMapping);
				}

				if (base->getType() == state.builtinOutputVariable->getType())
				{
					indices = MapBuiltin(state, indices, accessChain->getBase()->getType(), state.builtinOutputMapping);
				}

				if (!accessChain->hasPtrIndex())
				{
					indices.insert(indices.begin(), state.builder.getInt32(0));
				}
				
				// TODO: In Bounds

				auto currentType = base->getType();
				llvmValue = base;
				assert(currentType->isPointerTy());
				currentType = currentType->getPointerElementType();
				auto startIndex = 0u;
				for (auto k = 1u; k <= indices.size(); k++)
				{
					if (k == indices.size())
					{
						llvm::ArrayRef<llvm::Value*> range{indices.data() + startIndex, k - startIndex};
						llvmValue = state.builder.CreateGEP(llvmValue, range);
						break;
					}
					
					if (currentType->isStructTy())
					{
						auto index = llvm::cast<llvm::ConstantInt>(indices[k])->getZExtValue();
						if (state.structIndexMapping.find(currentType) != state.structIndexMapping.end())
						{
							index = state.structIndexMapping.at(currentType).at(index);
							indices[k] = state.builder.getInt32(index);
						}

						currentType = currentType->getStructElementType(index);
					}
					else if (currentType->isVectorTy())
					{
						currentType = currentType->getVectorElementType();
					}
					else if (currentType->isArrayTy())
					{
						currentType = currentType->getArrayElementType();
					}
					else if (currentType->isPointerTy())
					{
						// We assume this is a runtime array
						// TODO: Bounds check?
						llvm::ArrayRef<llvm::Value*> range{indices.data() + startIndex, k - startIndex};
						startIndex = k;
						llvmValue = state.builder.CreateLoad(state.builder.CreateGEP(llvmValue, range));
						currentType = currentType->getPointerElementType();
					}
					else
					{
						FATAL_ERROR();
					}
				}

				break;
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
				llvmValue = state.builder.CreateExtractElement(ConvertValue(state, vectorExtractDynamic->getVector(), currentFunction),
				                                               ConvertValue(state, vectorExtractDynamic->getIndex(), currentFunction));
				break;
			}
			
		case OpVectorInsertDynamic:
			{
				auto vectorInsertDynamic = static_cast<SPIRV::SPIRVVectorInsertDynamic*>(instruction);
				llvmValue = state.builder.CreateInsertElement(ConvertValue(state, vectorInsertDynamic->getVector(), currentFunction),
				                                              ConvertValue(state, vectorInsertDynamic->getComponent(), currentFunction),
				                                              ConvertValue(state, vectorInsertDynamic->getIndex(), currentFunction));
				break;
			}

		case OpVectorShuffle:
			{
				const auto vectorShuffle = reinterpret_cast<SPIRV::SPIRVVectorShuffle*>(instruction);
				std::vector<llvm::Constant*> components{};
				for (auto component : vectorShuffle->getComponents())
				{
					if (component == 0xFFFFFFFF)
					{
						components.push_back(llvm::UndefValue::get(state.builder.getInt32Ty()));
					}
					else
					{
						components.push_back(llvm::ConstantInt::get(state.builder.getInt32Ty(), component));
					}
				}
				llvmValue = state.builder.CreateShuffleVector(ConvertValue(state, vectorShuffle->getVector1(), currentFunction),
				                                              ConvertValue(state, vectorShuffle->getVector2(), currentFunction),
				                                              llvm::ConstantVector::get(components));
				break;
			}

		case OpCompositeConstruct:
			{
				// TODO: Optimise if all constant (ie, ConstantVector/ConstantArray/ConstantStruct)
				const auto compositeConstruct = reinterpret_cast<SPIRV::SPIRVCompositeConstruct*>(instruction);

				switch (compositeConstruct->getType()->getOpCode())
				{
				case OpTypeVector:
					llvmValue = llvm::UndefValue::get(ConvertType(state, compositeConstruct->getType()));
					for (auto j = 0u; j < compositeConstruct->getConstituents().size(); j++)
					{
						// TODO: Can be vectors or floats
						llvmValue = state.builder.CreateInsertElement(llvmValue, ConvertValue(state, compositeConstruct->getConstituents()[j], currentFunction), j);
					}
					break;

				case OpTypeMatrix:
				case OpTypeArray:
					llvmValue = state.builder.CreateAlloca(ConvertType(state, compositeConstruct->getType()));
					for (auto j = 0u; j < compositeConstruct->getConstituents().size(); j++)
					{
						auto destination = state.builder.CreateConstInBoundsGEP2_32(nullptr, llvmValue, 0, j);
						auto value = ConvertValue(state, compositeConstruct->getConstituents()[j], currentFunction);
						state.builder.CreateStore(value, destination);
					}
					llvmValue = state.builder.CreateLoad(llvmValue);
					break;

				case OpTypeStruct:
					{
						auto structType = ConvertType(state, compositeConstruct->getType());
						llvmValue = state.builder.CreateAlloca(structType);
						for (auto j = 0u; j < compositeConstruct->getConstituents().size(); j++)
						{
							const auto index = state.structIndexMapping.find(structType) != state.structIndexMapping.end()
								                   ? state.structIndexMapping.at(structType).at(j)
								                   : j;

							auto destination = state.builder.CreateConstInBoundsGEP2_32(nullptr, llvmValue, 0, index);
							auto value = ConvertValue(state, compositeConstruct->getConstituents()[j], currentFunction);
							state.builder.CreateStore(value, destination);
						}
						llvmValue = state.builder.CreateLoad(llvmValue);
						break;
					}

				default:
					FATAL_ERROR();
				}
				break;
			}

		case OpCompositeExtract:
			{
				const auto compositeExtract = reinterpret_cast<SPIRV::SPIRVCompositeExtract*>(instruction);
				const auto composite = ConvertValue(state, compositeExtract->getComposite(), currentFunction);
				if (compositeExtract->getComposite()->getType()->isTypeVector())
				{
					assert(compositeExtract->getIndices().size() == 1);
					llvmValue = state.builder.CreateExtractElement(composite, compositeExtract->getIndices()[0]);
				}
				else
				{
					auto currentType = composite->getType();
					auto indices = compositeExtract->getIndices();
					for (auto k = 0u; k <= indices.size(); k++)
					{
						if (k == indices.size())
						{
							llvmValue = state.builder.CreateExtractValue(composite, indices);
							break;
						}

						if (currentType->isStructTy())
						{
							if (state.structIndexMapping.find(currentType) != state.structIndexMapping.end())
							{
								indices[k] = state.structIndexMapping.at(currentType).at(indices[k]);
							}
							currentType = currentType->getStructElementType(indices[k]);
						}
						else if (currentType->isArrayTy())
						{
							currentType = currentType->getArrayElementType();
						}
						else if (currentType->isVectorTy())
						{
							assert(k + 1 == indices.size());
							const auto vectorIndex = indices[k];
							indices.pop_back();
							llvmValue = state.builder.CreateExtractValue(composite, indices);
							llvmValue = state.builder.CreateExtractElement(llvmValue, vectorIndex);
							break;
						}
						else
						{
							FATAL_ERROR();
						}
					}
				}

				break;
			}

		case OpCompositeInsert:
			{
				const auto compositeInsert = static_cast<SPIRV::SPIRVCompositeInsert*>(instruction);
				const auto composite = ConvertValue(state, compositeInsert->getComposite(), currentFunction);
				const auto value = ConvertValue(state, compositeInsert->getObject(), currentFunction);
				if (compositeInsert->getComposite()->getType()->isTypeVector())
				{
					assert(compositeInsert->getIndices().size() == 1);
					llvmValue = state.builder.CreateInsertElement(composite, value, compositeInsert->getIndices()[0]);
				}
				else
				{
					auto currentType = composite->getType();
					auto indices = compositeInsert->getIndices();
					for (auto k = 0u; k <= indices.size(); k++)
					{
						if (k == indices.size())
						{
							llvmValue = state.builder.CreateInsertValue(composite, value, indices);
							break;
						}
						
						if (currentType->isStructTy())
						{
							if (state.structIndexMapping.find(currentType) != state.structIndexMapping.end())
							{
								indices[k] = state.structIndexMapping.at(currentType).at(indices[k]);
							}
							currentType = currentType->getStructElementType(indices[k]);
						}
						else if (currentType->isArrayTy())
						{
							currentType = currentType->getArrayElementType();
						}
						else if (currentType->isVectorTy())
						{
							assert(k + 1 == indices.size());
							const auto vectorIndex = indices[k];
							indices.pop_back();
							llvmValue = state.builder.CreateExtractValue(composite, indices);
							llvmValue = state.builder.CreateInsertElement(llvmValue, value, vectorIndex);
							llvmValue = state.builder.CreateInsertValue(composite, llvmValue, indices);
							break;
						}
						else
						{
							FATAL_ERROR();
						}
					}
				}
				break;
			}
			
			// case OpCopyObject: break;
			// case OpTranspose: break;
			 
		case OpSampledImage:
			{
				const auto sampledImage = reinterpret_cast<SPIRV::SPIRVSampledImage*>(instruction);

				// Get storage 3 pointers in size
				llvm::Value* storage = state.builder.CreateAlloca(state.builder.getInt8PtrTy(), state.builder.getInt32(3));
				storage = state.builder.CreateBitCast(storage, ConvertType(state, sampledImage->getType()));

				auto image = ConvertValue(state, sampledImage->getOpValue(0), currentFunction);
				auto sampler = ConvertValue(state, sampledImage->getOpValue(1), currentFunction);

				const auto functionName = "@ImageCombine";
				const auto cachedFunction = state.functionCache.find(functionName);
				llvm::Function* function;
				if (cachedFunction != state.functionCache.end())
				{
					function = cachedFunction->second;
				}
				else
				{
					llvm::Type* params[]
					{
						image->getType(),
						sampler->getType(),
						storage->getType(),
					};

					const auto functionType = llvm::FunctionType::get(llvm::Type::getVoidTy(state.context), params, false);
					function = llvm::Function::Create(functionType, llvm::GlobalValue::ExternalLinkage, 0, functionName, state.module);
					function->addDereferenceableAttr(0, 16);
					function->addDereferenceableAttr(1, 16);
					function->addDereferenceableAttr(2, 16);
					state.functionCache[functionName] = function;
				}

				state.builder.CreateCall(function, 
				                         {
					                         image,
					                         sampler,
					                         storage,
				                         });
				llvmValue = storage;
				break;
			}

		case OpImageSampleImplicitLod:
			{
				const auto imageSampleImplicitLod = reinterpret_cast<SPIRV::SPIRVImageSampleImplicitLod*>(instruction);
				const auto function = GetInbuiltFunction(state, imageSampleImplicitLod);

				llvm::Value* args[4];
				args[0] = state.builder.CreateLoad(state.userData);
				args[1] = state.builder.CreateAlloca(ConvertType(state, imageSampleImplicitLod->getType()));
				args[2] = ConvertValue(state, imageSampleImplicitLod->getOpValue(0), currentFunction);
				args[3] = ConvertValue(state, imageSampleImplicitLod->getOpValue(1), currentFunction);

				const auto tmp3 = state.builder.CreateAlloca(args[3]->getType());
				state.builder.CreateStore(args[3], tmp3);
				args[3] = tmp3;

				state.builder.CreateCall(function, args);
				llvmValue = state.builder.CreateLoad(args[1]);
				
				break;
			}

		case OpImageSampleExplicitLod:
			{
				const auto imageSampleExplicitLod = reinterpret_cast<SPIRV::SPIRVImageSampleExplicitLod*>(instruction);
				const auto function = GetInbuiltFunction(state, imageSampleExplicitLod);

				llvm::Value* args[5];
				args[0] = state.builder.CreateLoad(state.userData);
				args[1] = state.builder.CreateAlloca(ConvertType(state, imageSampleExplicitLod->getType()));
				args[2] = ConvertValue(state, imageSampleExplicitLod->getOpValue(0), currentFunction);
				args[3] = ConvertValue(state, imageSampleExplicitLod->getOpValue(1), currentFunction);
				args[4] = ConvertValue(state, imageSampleExplicitLod->getOpValue(3), currentFunction);

				const auto tmp3 = state.builder.CreateAlloca(args[3]->getType());
				state.builder.CreateStore(args[3], tmp3);
				args[3] = tmp3;

				state.builder.CreateCall(function, args);
				llvmValue = state.builder.CreateLoad(args[1]);

				break;
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
				const auto function = GetInbuiltFunction(state, imageFetch);
				
				llvm::Value* args[4];
				args[0] = state.builder.CreateLoad(state.userData);
				args[1] = state.builder.CreateAlloca(ConvertType(state, imageFetch->getType()));
				args[2] = ConvertValue(state, imageFetch->getOpValue(0), currentFunction);
				args[3] = ConvertValue(state, imageFetch->getOpValue(1), currentFunction);

				const auto tmp3 = state.builder.CreateAlloca(args[3]->getType());
				state.builder.CreateStore(args[3], tmp3);
				args[3] = tmp3;

				state.builder.CreateCall(function, args);
				llvmValue = state.builder.CreateLoad(args[1]);
				
				break;
			}

			// case OpImageGather: break;
			// case OpImageDrefGather: break;
			// case OpImageRead: break;
			// case OpImageWrite: break;
			// case OpImage: break;
			// case OpImageQueryFormat: break;
			// case OpImageQueryOrder: break;
			// case OpImageQuerySizeLod: break;
			// case OpImageQuerySize: break;
			// case OpImageQueryLod: break;
			// case OpImageQueryLevels: break;
			// case OpImageQuerySamples: break;

		case OpConvertFToU:
			{
				const auto op = static_cast<SPIRV::SPIRVUnary*>(instruction);
				llvmValue = state.builder.CreateFPToUI(ConvertValue(state, op->getOperand(0), currentFunction),
				                                       ConvertType(state, op->getType()));
				break;
			}

		case OpConvertFToS:
			{
				const auto op = static_cast<SPIRV::SPIRVUnary*>(instruction);
				llvmValue = state.builder.CreateFPToSI(ConvertValue(state, op->getOperand(0), currentFunction),
				                                       ConvertType(state, op->getType()));
				break;
			}
			
		case OpConvertSToF:
			{
				const auto op = static_cast<SPIRV::SPIRVUnary*>(instruction);
				llvmValue = state.builder.CreateSIToFP(ConvertValue(state, op->getOperand(0), currentFunction),
				                                       ConvertType(state, op->getType()));
				break;
			}
			
		case OpConvertUToF:
			{
				const auto op = static_cast<SPIRV::SPIRVUnary*>(instruction);
				llvmValue = state.builder.CreateUIToFP(ConvertValue(state, op->getOperand(0), currentFunction),
				                                       ConvertType(state, op->getType()));
				break;
			}

		case OpUConvert:
			{
				const auto op = static_cast<SPIRV::SPIRVUnary*>(instruction);
				const auto value = ConvertValue(state, op->getOperand(0), currentFunction);
				const auto type = ConvertType(state, op->getType());
				if (value->getType()->getScalarSizeInBits() < type->getScalarSizeInBits())
				{
					llvmValue = state.builder.CreateZExt(value, type);
				}
				else
				{
					llvmValue = state.builder.CreateTrunc(value, type);
				}
				break;
			}

		case OpSConvert:
			{
				const auto op = static_cast<SPIRV::SPIRVUnary*>(instruction);
				const auto value = ConvertValue(state, op->getOperand(0), currentFunction);
				const auto type = ConvertType(state, op->getType());
				if (value->getType()->getScalarSizeInBits() < type->getScalarSizeInBits())
				{
					llvmValue = state.builder.CreateSExt(value, type);
				}
				else
				{
					llvmValue = state.builder.CreateTrunc(value, type);
				}
				break;
			}

			// case OpFConvert: break;
			// case OpQuantizeToF16: break;
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
				llvmValue = state.builder.CreateBitCast(ConvertValue(state, op->getOperand(0), currentFunction),
				                                        ConvertType(state, op->getType()));
				break;
			}

		case OpSNegate:
			{
				const auto op = static_cast<SPIRV::SPIRVUnary*>(instruction);
				llvmValue = state.builder.CreateNSWNeg(ConvertValue(state, op->getOperand(0), currentFunction));
				break;
			}

		case OpFNegate:
			{
				const auto op = static_cast<SPIRV::SPIRVUnary*>(instruction);
				llvmValue = state.builder.CreateFNeg(ConvertValue(state, op->getOperand(0), currentFunction));
				break;
			}

		case OpIAdd:
			{
				const auto op = static_cast<SPIRV::SPIRVBinary*>(instruction);
				llvmValue = state.builder.CreateAdd(ConvertValue(state, op->getOperand(0), currentFunction),
				                                    ConvertValue(state, op->getOperand(1), currentFunction));
				break;
			}

		case OpFAdd:
			{
				const auto op = static_cast<SPIRV::SPIRVBinary*>(instruction);
				llvmValue = state.builder.CreateFAdd(ConvertValue(state, op->getOperand(0), currentFunction),
				                                     ConvertValue(state, op->getOperand(1), currentFunction));
				break;
			}

		case OpISub:
			{
				const auto op = static_cast<SPIRV::SPIRVBinary*>(instruction);
				llvmValue = state.builder.CreateSub(ConvertValue(state, op->getOperand(0), currentFunction),
				                                    ConvertValue(state, op->getOperand(1), currentFunction));
				break;
			}

		case OpFSub:
			{
				const auto op = static_cast<SPIRV::SPIRVBinary*>(instruction);
				llvmValue = state.builder.CreateFSub(ConvertValue(state, op->getOperand(0), currentFunction),
				                                     ConvertValue(state, op->getOperand(1), currentFunction));
				break;
			}

		case OpIMul:
			{
				const auto op = static_cast<SPIRV::SPIRVBinary*>(instruction);
				llvmValue = state.builder.CreateMul(ConvertValue(state, op->getOperand(0), currentFunction),
				                                    ConvertValue(state, op->getOperand(1), currentFunction));
				break;
			}

		case OpFMul:
			{
				const auto op = static_cast<SPIRV::SPIRVBinary*>(instruction);
				llvmValue = state.builder.CreateFMul(ConvertValue(state, op->getOperand(0), currentFunction),
				                                     ConvertValue(state, op->getOperand(1), currentFunction));
				break;
			}

		case OpUDiv:
			{
				const auto op = static_cast<SPIRV::SPIRVBinary*>(instruction);
				llvmValue = state.builder.CreateUDiv(ConvertValue(state, op->getOperand(0), currentFunction),
				                                     ConvertValue(state, op->getOperand(1), currentFunction));
				break;
			}

		case OpSDiv:
			{
				const auto op = static_cast<SPIRV::SPIRVBinary*>(instruction);
				llvmValue = state.builder.CreateSDiv(ConvertValue(state, op->getOperand(0), currentFunction),
				                                     ConvertValue(state, op->getOperand(1), currentFunction));
				break;
			}

		case OpFDiv:
			{
				const auto op = static_cast<SPIRV::SPIRVBinary*>(instruction);
				llvmValue = state.builder.CreateFDiv(ConvertValue(state, op->getOperand(0), currentFunction),
				                                     ConvertValue(state, op->getOperand(1), currentFunction));
				break;
			}

		case OpUMod:
			{
				const auto op = static_cast<SPIRV::SPIRVBinary*>(instruction);
				llvmValue = state.builder.CreateURem(ConvertValue(state, op->getOperand(0), currentFunction),
				                                     ConvertValue(state, op->getOperand(1), currentFunction));
				break;
			}

		case OpSRem:
			{
				const auto op = static_cast<SPIRV::SPIRVBinary*>(instruction);
				llvmValue = state.builder.CreateSRem(ConvertValue(state, op->getOperand(0), currentFunction),
				                                     ConvertValue(state, op->getOperand(1), currentFunction));
				break;
			}

		case OpSMod:
			{
				const auto op = static_cast<SPIRV::SPIRVBinary*>(instruction);
				auto left = ConvertValue(state, op->getOperand(0), currentFunction);
				auto right = ConvertValue(state, op->getOperand(1), currentFunction);
				llvmValue = state.builder.CreateSDiv(left, right);
				llvmValue = state.builder.CreateMul(right, llvmValue);
				llvmValue = state.builder.CreateSub(left, llvmValue);
				break;
			}

		case OpFRem:
			{
				const auto op = static_cast<SPIRV::SPIRVBinary*>(instruction);
				llvmValue = state.builder.CreateFRem(ConvertValue(state, op->getOperand(0), currentFunction),
				                                     ConvertValue(state, op->getOperand(1), currentFunction));
				break;
			}

		case OpFMod:
			{
				FATAL_ERROR();
				break;
			}

		case OpVectorTimesScalar:
			{
				const auto vectorTimesScalar = reinterpret_cast<SPIRV::SPIRVVectorTimesScalar*>(instruction);
				auto vector = ConvertValue(state, vectorTimesScalar->getVector(), currentFunction);
				auto scalar = ConvertValue(state, vectorTimesScalar->getScalar(), currentFunction);
				llvmValue = state.builder.CreateVectorSplat(vector->getType()->getVectorNumElements(), scalar);
				llvmValue = state.builder.CreateFMul(vector, llvmValue);
				break;
			}
			
			// case OpMatrixTimesScalar: break;
			// case OpVectorTimesMatrix: break;

		case OpMatrixTimesVector:
			{
				const auto matrixTimesVector = reinterpret_cast<SPIRV::SPIRVMatrixTimesVector*>(instruction);
				llvm::Value* args[3];
				args[1] = ConvertValue(state, matrixTimesVector->getMatrix(), currentFunction);
				args[2] = ConvertValue(state, matrixTimesVector->getVector(), currentFunction);

				const auto function = GetInbuiltFunction(state, matrixTimesVector);

				const auto tmp0 = state.builder.CreateAlloca(function->getFunctionType()->getParamType(0)->getPointerElementType());
				const auto tmp1 = state.builder.CreateAlloca(args[1]->getType());
				const auto tmp2 = state.builder.CreateAlloca(args[2]->getType());
				state.builder.CreateStore(args[1], tmp1);
				state.builder.CreateStore(args[2], tmp2);

				args[0] = tmp0;
				args[1] = tmp1;
				args[2] = tmp2;

				state.builder.CreateCall(function, args);
				llvmValue = state.builder.CreateLoad(tmp0);
				break;
			}

		case OpMatrixTimesMatrix:
			{
				const auto matrixTimesMatrix = reinterpret_cast<SPIRV::SPIRVMatrixTimesMatrix*>(instruction);
				llvm::Value* args[3];
				args[1] = ConvertValue(state, matrixTimesMatrix->getMatrixLeft(), currentFunction);
				args[2] = ConvertValue(state, matrixTimesMatrix->getMatrixRight(), currentFunction);

				const auto function = GetInbuiltFunction(state, matrixTimesMatrix);

				const auto tmp0 = state.builder.CreateAlloca(function->getFunctionType()->getParamType(0)->getPointerElementType());
				const auto tmp1 = state.builder.CreateAlloca(args[1]->getType());
				const auto tmp2 = state.builder.CreateAlloca(args[2]->getType());
				state.builder.CreateStore(args[1], tmp1);
				state.builder.CreateStore(args[2], tmp2);
				
				args[0] = tmp0;
				args[1] = tmp1;
				args[2] = tmp2;
				
				state.builder.CreateCall(function, args);
				llvmValue = state.builder.CreateLoad(tmp0);

				break;
			}

			// case OpOuterProduct: break;
			 
		case OpDot:
			{
				const auto dot = reinterpret_cast<SPIRV::SPIRVDot*>(instruction);
				llvm::Value* args[3];
				args[1] = ConvertValue(state, dot->getOperand(0), currentFunction);
				args[2] = ConvertValue(state, dot->getOperand(1), currentFunction);

				const auto function = GetInbuiltFunction(state, dot);

				const auto tmp0 = state.builder.CreateAlloca(function->getFunctionType()->getParamType(0)->getPointerElementType());
				const auto tmp1 = state.builder.CreateAlloca(args[1]->getType());
				const auto tmp2 = state.builder.CreateAlloca(args[2]->getType());
				state.builder.CreateStore(args[1], tmp1);
				state.builder.CreateStore(args[2], tmp2);

				args[0] = tmp0;
				args[1] = tmp1;
				args[2] = tmp2;

				state.builder.CreateCall(function, args);
				llvmValue = state.builder.CreateLoad(tmp0);
				break;
			}

			// case OpIAddCarry: break;
			// case OpISubBorrow: break;
			// case OpUMulExtended: break;
			// case OpSMulExtended: break;
			 
		case OpAny:
			{
				const auto op = reinterpret_cast<SPIRV::SPIRVUnary*>(instruction);
				auto type = llvm::VectorType::get(state.builder.getInt32Ty(), op->getOperand(0)->getType()->getVectorComponentCount());
				llvmValue = state.builder.CreateZExt(ConvertValue(state, op->getOperand(0), currentFunction), type);
				llvmValue = state.builder.CreateUnaryIntrinsic(llvm::Intrinsic::experimental_vector_reduce_or, llvmValue);
				llvmValue = state.builder.CreateICmpNE(llvmValue, state.builder.getInt32(0));
				break;
			}
			
		case OpAll:
			{
				const auto op = reinterpret_cast<SPIRV::SPIRVUnary*>(instruction);
				auto type = llvm::VectorType::get(state.builder.getInt32Ty(), op->getOperand(0)->getType()->getVectorComponentCount());
				llvmValue = state.builder.CreateZExt(ConvertValue(state, op->getOperand(0), currentFunction), type);
				llvmValue = state.builder.CreateUnaryIntrinsic(llvm::Intrinsic::experimental_vector_reduce_and, llvmValue);
				llvmValue = state.builder.CreateICmpNE(llvmValue, state.builder.getInt32(0));
				break;
			}
			 
			// case OpIsNan: break;
			// case OpIsInf: break;
			// case OpIsFinite: break;
			// case OpIsNormal: break;
			// case OpSignBitSet: break;
			// case OpLessOrGreater: break;
			// case OpOrdered: break;
			// case OpUnordered: break;
			// case OpLogicalEqual: break;
			// case OpLogicalNotEqual: break;
			 
		case OpLogicalOr:
			{
				const auto op = static_cast<SPIRV::SPIRVBinary*>(instruction);
				llvmValue = state.builder.CreateOr(ConvertValue(state, op->getOperand(0), currentFunction),
				                                   ConvertValue(state, op->getOperand(1), currentFunction));
				break;
			}
			 
		case OpLogicalAnd:
			{
				const auto op = static_cast<SPIRV::SPIRVBinary*>(instruction);
				llvmValue = state.builder.CreateAnd(ConvertValue(state, op->getOperand(0), currentFunction),
				                                    ConvertValue(state, op->getOperand(1), currentFunction));
				break;
			}

		case OpLogicalNot:
			{
				const auto op = static_cast<SPIRV::SPIRVUnary*>(instruction);
				llvmValue = state.builder.CreateNot(ConvertValue(state, op->getOperand(0), currentFunction));
				break;
			}

		case OpSelect:
			{
				const auto op = static_cast<SPIRV::SPIRVSelect*>(instruction);
				llvmValue = state.builder.CreateSelect(ConvertValue(state, op->getCondition(), currentFunction),
				                                       ConvertValue(state, op->getTrueValue(), currentFunction),
				                                       ConvertValue(state, op->getFalseValue(), currentFunction));
				break;
			}
			 
		case OpIEqual:
			{
				const auto op = static_cast<SPIRV::SPIRVCompare*>(instruction);
				llvmValue = state.builder.CreateICmpEQ(ConvertValue(state, op->getOperand(0), currentFunction), 
				                                       ConvertValue(state, op->getOperand(1), currentFunction));
				break;
			}
			 
		case OpINotEqual:
			{
				const auto op = static_cast<SPIRV::SPIRVCompare*>(instruction);
				llvmValue = state.builder.CreateICmpNE(ConvertValue(state, op->getOperand(0), currentFunction), 
				                                       ConvertValue(state, op->getOperand(1), currentFunction));
				break;
			}
			 
		case OpUGreaterThan:
			{
				const auto op = static_cast<SPIRV::SPIRVCompare*>(instruction);
				llvmValue = state.builder.CreateICmpUGT(ConvertValue(state, op->getOperand(0), currentFunction), 
				                                        ConvertValue(state, op->getOperand(1), currentFunction));
				break;
			}
			 
		case OpSGreaterThan:
			{
				const auto op = static_cast<SPIRV::SPIRVCompare*>(instruction);
				llvmValue = state.builder.CreateICmpSGT(ConvertValue(state, op->getOperand(0), currentFunction), 
				                                        ConvertValue(state, op->getOperand(1), currentFunction));
				break;
			}
			 
		case OpUGreaterThanEqual:
			{
				const auto op = static_cast<SPIRV::SPIRVCompare*>(instruction);
				llvmValue = state.builder.CreateICmpUGE(ConvertValue(state, op->getOperand(0), currentFunction), 
				                                        ConvertValue(state, op->getOperand(1), currentFunction));
				break;
			}
			 
		case OpSGreaterThanEqual:
			{
				const auto op = static_cast<SPIRV::SPIRVCompare*>(instruction);
				llvmValue = state.builder.CreateICmpSGE(ConvertValue(state, op->getOperand(0), currentFunction), 
				                                        ConvertValue(state, op->getOperand(1), currentFunction));
				break;
			}
			 
		case OpULessThan:
			{
				const auto op = static_cast<SPIRV::SPIRVCompare*>(instruction);
				llvmValue = state.builder.CreateICmpULT(ConvertValue(state, op->getOperand(0), currentFunction), 
				                                        ConvertValue(state, op->getOperand(1), currentFunction));
				break;
			}
			 
		case OpSLessThan:
			{
				const auto op = static_cast<SPIRV::SPIRVCompare*>(instruction);
				llvmValue = state.builder.CreateICmpSLT(ConvertValue(state, op->getOperand(0), currentFunction), 
				                                        ConvertValue(state, op->getOperand(1), currentFunction));
				break;
			}
			 
		case OpULessThanEqual:
			{
				const auto op = static_cast<SPIRV::SPIRVCompare*>(instruction);
				llvmValue = state.builder.CreateICmpULE(ConvertValue(state, op->getOperand(0), currentFunction), 
				                                        ConvertValue(state, op->getOperand(1), currentFunction));
				break;
			}
			 
		case OpSLessThanEqual:
			{
				const auto op = static_cast<SPIRV::SPIRVCompare*>(instruction);
				llvmValue = state.builder.CreateICmpSLE(ConvertValue(state, op->getOperand(0), currentFunction), 
				                                        ConvertValue(state, op->getOperand(1), currentFunction));
				break;
			}
			 
		case OpFOrdEqual:
			{
				const auto op = static_cast<SPIRV::SPIRVCompare*>(instruction);
				llvmValue = state.builder.CreateFCmpOEQ(ConvertValue(state, op->getOperand(0), currentFunction), 
				                                        ConvertValue(state, op->getOperand(1), currentFunction));
				break;
			}
			 
		case OpFUnordEqual:
			{
				const auto op = static_cast<SPIRV::SPIRVCompare*>(instruction);
				llvmValue = state.builder.CreateFCmpUEQ(ConvertValue(state, op->getOperand(0), currentFunction), 
				                                        ConvertValue(state, op->getOperand(1), currentFunction));
				break;
			}
			 
		case OpFOrdNotEqual:
			{
				const auto op = static_cast<SPIRV::SPIRVCompare*>(instruction);
				llvmValue = state.builder.CreateFCmpONE(ConvertValue(state, op->getOperand(0), currentFunction), 
				                                        ConvertValue(state, op->getOperand(1), currentFunction));
				break;
			}
			 
		case OpFUnordNotEqual:
			{
				const auto op = static_cast<SPIRV::SPIRVCompare*>(instruction);
				llvmValue = state.builder.CreateFCmpUNE(ConvertValue(state, op->getOperand(0), currentFunction), 
				                                        ConvertValue(state, op->getOperand(1), currentFunction));
				break;
			}

		case OpFOrdLessThan:
			{
				const auto op = static_cast<SPIRV::SPIRVCompare*>(instruction);
				llvmValue = state.builder.CreateFCmpOLT(ConvertValue(state, op->getOperand(0), currentFunction), 
				                                        ConvertValue(state, op->getOperand(1), currentFunction));
				break;
			}

		case OpFUnordLessThan:
			{
				const auto op = static_cast<SPIRV::SPIRVCompare*>(instruction);
				llvmValue = state.builder.CreateFCmpULT(ConvertValue(state, op->getOperand(0), currentFunction), 
				                                        ConvertValue(state, op->getOperand(1), currentFunction));
				break;
			}

		case OpFOrdGreaterThan:
			{
				const auto op = static_cast<SPIRV::SPIRVCompare*>(instruction);
				llvmValue = state.builder.CreateFCmpOGT(ConvertValue(state, op->getOperand(0), currentFunction), 
				                                        ConvertValue(state, op->getOperand(1), currentFunction));
				break;
			}

			// case OpFUnordGreaterThan: break;
			// case OpFOrdLessThanEqual: break;
			// case OpFUnordLessThanEqual: break;
			// case OpFOrdGreaterThanEqual: break;
			// case OpFUnordGreaterThanEqual: break;
			 
		case OpShiftRightLogical:
			{
				const auto op = static_cast<SPIRV::SPIRVBinary*>(instruction);
				const auto base = ConvertValue(state, op->getOperand(0), currentFunction);
				const auto shift = ConvertValue(state, op->getOperand(1), currentFunction);
				if (base->getType() != shift->getType())
				{
					if (base->getType()->getScalarSizeInBits() < shift->getType()->getScalarSizeInBits())
					{
						llvmValue = state.builder.CreateZExt(base, shift->getType());
						llvmValue = state.builder.CreateLShr(llvmValue, shift);
						llvmValue = state.builder.CreateTrunc(llvmValue, base->getType());
					}
					else
					{
						auto isSigned = static_cast<SPIRV::SPIRVTypeInt*>(op->getOperand(1)->getType()->isTypeVector()
							                                                  ? op->getOperand(1)->getType()->getVectorComponentType()
							                                                  : op->getOperand(1)->getType())->isSigned();
						if (isSigned)
						{
							llvmValue = state.builder.CreateSExt(shift, base->getType());
						}
						else
						{
							llvmValue = state.builder.CreateZExt(shift, base->getType());
						}
						llvmValue = state.builder.CreateLShr(base, llvmValue);
					}
				}
				else
				{
					llvmValue = state.builder.CreateLShr(base, shift);
				}
				break;
			}
			 
		case OpShiftRightArithmetic:
			{
				const auto op = static_cast<SPIRV::SPIRVBinary*>(instruction);
				const auto base = ConvertValue(state, op->getOperand(0), currentFunction);
				const auto shift = ConvertValue(state, op->getOperand(1), currentFunction);
				if (base->getType() != shift->getType())
				{
					if (base->getType()->getScalarSizeInBits() < shift->getType()->getScalarSizeInBits())
					{
						llvmValue = state.builder.CreateSExt(base, shift->getType());
						llvmValue = state.builder.CreateAShr(llvmValue, shift);
						llvmValue = state.builder.CreateTrunc(llvmValue, base->getType());
					}
					else
					{
						auto isSigned = static_cast<SPIRV::SPIRVTypeInt*>(op->getOperand(1)->getType()->isTypeVector()
							                                                  ? op->getOperand(1)->getType()->getVectorComponentType()
							                                                  : op->getOperand(1)->getType())->isSigned();
						if (isSigned)
						{
							llvmValue = state.builder.CreateSExt(shift, base->getType());
						}
						else
						{
							llvmValue = state.builder.CreateZExt(shift, base->getType());
						}
						llvmValue = state.builder.CreateAShr(base, llvmValue);
					}
				}
				else
				{
					llvmValue = state.builder.CreateAShr(base, shift);
				}
				break;
			}
			 
		case OpShiftLeftLogical:
			{
				const auto op = static_cast<SPIRV::SPIRVBinary*>(instruction);
				const auto base = ConvertValue(state, op->getOperand(0), currentFunction);
				const auto shift = ConvertValue(state, op->getOperand(1), currentFunction);
				if (base->getType() != shift->getType())
				{
					if (base->getType()->getScalarSizeInBits() < shift->getType()->getScalarSizeInBits())
					{
						auto isSigned = static_cast<SPIRV::SPIRVTypeInt*>(op->getOperand(0)->getType()->isTypeVector()
							                                                  ? op->getOperand(0)->getType()->getVectorComponentType()
							                                                  : op->getOperand(0)->getType())->isSigned();
						if (isSigned)
						{
							llvmValue = state.builder.CreateSExt(base, shift->getType());
						}
						else
						{
							llvmValue = state.builder.CreateZExt(base, shift->getType());
						}
						llvmValue = state.builder.CreateShl(llvmValue, shift);
						llvmValue = state.builder.CreateTrunc(llvmValue, base->getType());
					}
					else
					{
						auto isSigned = static_cast<SPIRV::SPIRVTypeInt*>(op->getOperand(1)->getType()->isTypeVector()
							                                                  ? op->getOperand(1)->getType()->getVectorComponentType()
							                                                  : op->getOperand(1)->getType())->isSigned();
						if (isSigned)
						{
							llvmValue = state.builder.CreateSExt(shift, base->getType());
						}
						else
						{
							llvmValue = state.builder.CreateZExt(shift, base->getType());
						}
						llvmValue = state.builder.CreateShl(base, llvmValue);
					}
				}
				else
				{
					llvmValue = state.builder.CreateShl(base, shift);
				}
				break;
			}
			 
		case OpBitwiseOr:
			{
				const auto op = static_cast<SPIRV::SPIRVBinary*>(instruction);
				llvmValue = state.builder.CreateOr(ConvertValue(state, op->getOperand(0), currentFunction), 
				                                   ConvertValue(state, op->getOperand(1), currentFunction));
				break;
			}
			 
		case OpBitwiseXor:
			{
				const auto op = static_cast<SPIRV::SPIRVBinary*>(instruction);
				llvmValue = state.builder.CreateXor(ConvertValue(state, op->getOperand(0), currentFunction), 
				                                    ConvertValue(state, op->getOperand(1), currentFunction));
				break;
			}
			 
		case OpBitwiseAnd:
			{
				const auto op = static_cast<SPIRV::SPIRVBinary*>(instruction);
				llvmValue = state.builder.CreateAnd(ConvertValue(state, op->getOperand(0), currentFunction), 
				                                    ConvertValue(state, op->getOperand(1), currentFunction));
				break;
			}
			 
		case OpNot:
			{
				const auto op = static_cast<SPIRV::SPIRVUnary*>(instruction);
				llvmValue = state.builder.CreateNot(ConvertValue(state, op->getOperand(0), currentFunction));
				break;
			}
			
		case OpBitFieldInsert:
			{
				const auto op = static_cast<SPIRV::SPIRVBitFieldInsert*>(instruction);
				const auto base = ConvertValue(state, op->getBase(), currentFunction);
				const auto insert = ConvertValue(state, op->getInsert(), currentFunction);
				auto offset = ConvertValue(state, op->getOffset(), currentFunction);
				auto count = ConvertValue(state, op->getCount(), currentFunction);
				llvm::Value* mask;
				
				if (offset->getType()->getScalarSizeInBits() < base->getType()->getScalarSizeInBits())
				{
					offset = state.builder.CreateZExt(offset, base->getType()->getScalarType());
				}
				else if (offset->getType()->getScalarSizeInBits() > base->getType()->getScalarSizeInBits())
				{
					offset = state.builder.CreateTrunc(offset, base->getType()->getScalarType());
				}

				if (count->getType()->getScalarSizeInBits() < base->getType()->getScalarSizeInBits())
				{
					count = state.builder.CreateZExt(count, base->getType()->getScalarType());
				}
				else if (count->getType()->getScalarSizeInBits() > base->getType()->getScalarSizeInBits())
				{
					count = state.builder.CreateTrunc(count, base->getType()->getScalarType());
				}

				const auto negativeOne = state.builder.getInt(llvm::APInt(base->getType()->getScalarSizeInBits(), -1, true));
				mask = state.builder.CreateShl(negativeOne, count, "", false, true);
				mask = state.builder.CreateXor(mask, negativeOne);
				mask = state.builder.CreateShl(mask, offset);

				llvmValue = state.builder.CreateXor(mask, negativeOne);
				const auto lhs = state.builder.CreateAnd(llvmValue, base);
				const auto rhs = state.builder.CreateAnd(mask, insert);
				llvmValue = state.builder.CreateOr(lhs, rhs);
				break;
			}
			
		case OpBitFieldSExtract:
			FATAL_ERROR();
			
		case OpBitFieldUExtract:
			FATAL_ERROR();
			
			 
		case OpBitReverse:
			{
				const auto op = static_cast<SPIRV::SPIRVUnary*>(instruction);
				llvmValue = state.builder.CreateUnaryIntrinsic(llvm::Intrinsic::bitreverse, ConvertValue(state, op->getOperand(0), currentFunction));
				break;
			}
			
		case OpBitCount:
			{
				const auto op = static_cast<SPIRV::SPIRVUnary*>(instruction);
				llvmValue = state.builder.CreateUnaryIntrinsic(llvm::Intrinsic::ctpop, ConvertValue(state, op->getOperand(0), currentFunction));
				break;
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
			// case OpControlBarrier: break;
			// case OpMemoryBarrier: break;
			 
		case OpAtomicLoad:
			FATAL_ERROR();

		case OpAtomicStore:
			FATAL_ERROR();

		case OpAtomicExchange:
			FATAL_ERROR();

		case OpAtomicCompareExchange:
			FATAL_ERROR();

		case OpAtomicCompareExchangeWeak:
			FATAL_ERROR();

		case OpAtomicIIncrement:
			FATAL_ERROR();
			
		case OpAtomicIDecrement:
			FATAL_ERROR();
			
			 
		case OpAtomicIAdd:
			{
				const auto op = static_cast<SPIRV::SPIRVAtomicInstBase*>(instruction);
				auto pointer = ConvertValue(state, op->getOpValue(0), currentFunction);
				auto scope = static_cast<SPIRV::SPIRVConstant*>(op->getOpValue(1))->getInt32Value();
				auto semantics = static_cast<SPIRV::SPIRVConstant*>(op->getOpValue(2))->getInt32Value();
				auto value = ConvertValue(state, op->getOpValue(3), currentFunction);

				if (scope != 1)
				{
					FATAL_ERROR();
				}
				
				if (semantics != 0)
				{
					FATAL_ERROR();
				}

				llvmValue = state.builder.CreateAtomicRMW(llvm::AtomicRMWInst::Add, pointer, value, llvm::AtomicOrdering::Monotonic);
				break;
			}
			
		case OpAtomicISub:
			FATAL_ERROR();
			
		case OpAtomicSMin:
			FATAL_ERROR();

		case OpAtomicUMin:
			FATAL_ERROR();
			
		case OpAtomicSMax:
			FATAL_ERROR();
			
		case OpAtomicUMax:
			FATAL_ERROR();
			
		case OpAtomicAnd:
			FATAL_ERROR();
			
		case OpAtomicOr:
			FATAL_ERROR();
			
		case OpAtomicXor:
			FATAL_ERROR();
			

		case OpPhi:
			{
				auto phi = static_cast<SPIRV::SPIRVPhi*>(instruction);
				auto llvmPhi = state.builder.CreatePHI(ConvertType(state, phi->getType()), phi->getPairs().size() / 2);
				phi->foreachPair([&](SPIRV::SPIRVValue* incomingValue, SPIRV::SPIRVBasicBlock* incomingBasicBlock, size_t)
				{
					const auto basicBlock = ConvertBasicBlock(state, currentFunction, incomingBasicBlock);
					const auto translated = ConvertValue(state, incomingValue, currentFunction);
					llvmPhi->addIncoming(translated, basicBlock);
				});
				state.builder.SetInsertPoint(llvmBasicBlock);
				llvmValue = llvmPhi;
				break;
			}
			
		case OpLoopMerge:
		case OpSelectionMerge:
		case OpLabel:
			llvmValue = nullptr;
			break;
			
		case OpBranch:
			{
				const auto branch = static_cast<SPIRV::SPIRVBranch*>(instruction);
				const auto block = ConvertBasicBlock(state, currentFunction, branch->getTargetLabel());
				state.builder.SetInsertPoint(llvmBasicBlock);
				const auto llvmBranch = state.builder.CreateBr(block);
				llvmValue = llvmBranch;
				if (branch->getPrevious() && branch->getPrevious()->getOpCode() == OpLoopMerge)
				{
					const auto loopMerge = static_cast<SPIRV::SPIRVLoopMerge*>(branch->getPrevious());
					SetLLVMLoopMetadata(state, loopMerge, llvmBranch);
				}
				break;
			}
			 
		case OpBranchConditional:
			{
				const auto branch = static_cast<SPIRV::SPIRVBranchConditional*>(instruction);
				const auto trueBlock = ConvertBasicBlock(state, currentFunction, branch->getTrueLabel());
				const auto falseBlock = ConvertBasicBlock(state, currentFunction, branch->getFalseLabel());
				state.builder.SetInsertPoint(llvmBasicBlock);
				const auto llvmBranch = state.builder.CreateCondBr(ConvertValue(state, branch->getCondition(), currentFunction), trueBlock, falseBlock);
				llvmValue = llvmBranch;
				if (branch->getPrevious() && branch->getPrevious()->getOpCode() == OpLoopMerge)
				{
					const auto loopMerge = static_cast<SPIRV::SPIRVLoopMerge*>(branch->getPrevious());
					SetLLVMLoopMetadata(state, loopMerge, llvmBranch);
				}
				break;
			}
			
		case OpSwitch:
			{
				const auto swtch = static_cast<SPIRV::SPIRVSwitch*>(instruction);
				const auto select = ConvertValue(state, swtch->getSelect(), currentFunction);
				const auto defaultBlock = ConvertBasicBlock(state, currentFunction, swtch->getDefault());
				state.builder.SetInsertPoint(llvmBasicBlock);
				auto llvmSwitch = state.builder.CreateSwitch(select, defaultBlock, swtch->getNumPairs());
				swtch->foreachPair([&](SPIRV::SPIRVSwitch::LiteralTy literals, SPIRV::SPIRVBasicBlock* label)
				{
					assert(!literals.empty());
					assert(literals.size() <= 2);
					auto literal = static_cast<uint64_t>(literals.at(0));
					if (literals.size() == 2)
					{
						literal += static_cast<uint64_t>(literals.at(1)) << 32;
					}
					llvmSwitch->addCase(llvm::ConstantInt::get(llvm::dyn_cast<llvm::IntegerType>(select->getType()), literal), ConvertBasicBlock(state, currentFunction, label));
				});
				state.builder.SetInsertPoint(llvmBasicBlock);
				llvmValue = llvmSwitch;
				break;
			}
			
			// case OpKill: break;
			 
		case OpReturn:
			llvmValue = state.builder.CreateRetVoid();
			break;
			
		case OpReturnValue:
			{
				const auto returnValue = static_cast<SPIRV::SPIRVReturnValue*>(instruction);
				llvmValue = state.builder.CreateRet(ConvertValue(state, returnValue->getReturnValue(), currentFunction));
				break;
			}
			
			// case OpUnreachable: break;
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
			// case OpNoLine: break;
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
		 
		default:
			FATAL_ERROR();
		}

		if (instruction->hasType())
		{
			assert(ConvertType(state, instruction->getType()) == llvmValue->getType());
		}

		if (llvmValue && instruction->hasId())
		{
			state.valueMapping[instruction->getId()] = llvmValue;
		}
	}

	return llvmBasicBlock;
}

static llvm::Function* ConvertFunction(State& state, const SPIRV::SPIRVFunction* spirvFunction)
{
	const auto cachedType = state.functionMapping.find(spirvFunction->getId());
	if (cachedType != state.functionMapping.end())
	{
		return cachedType->second;
	}

	for (const auto decorate : spirvFunction->getDecorates())
	{
		FATAL_ERROR();
	}

	const auto llvmFunction = llvm::Function::Create(static_cast<llvm::FunctionType*>(ConvertType(state, spirvFunction->getFunctionType())), 
	                                                 llvm::GlobalVariable::ExternalLinkage,
	                                                 spirvFunction->getName(), 
	                                                 state.module);

	for (auto i = 0u; i < spirvFunction->getNumBasicBlock(); i++)
	{
		ConvertBasicBlock(state, llvmFunction, spirvFunction->getBasicBlock(i));
	}

	state.functionMapping[spirvFunction->getId()] = llvmFunction;
	return llvmFunction;
}

static void AddBuiltin(State& state, ExecutionModel executionModel)
{
	std::vector<llvm::Type*> inputMembers{};
	std::vector<llvm::Type*> outputMembers{};
	switch (executionModel)
	{
	case ExecutionModelVertex:
		state.builtinInputMapping.emplace_back(BuiltInVertexId, 0);
		state.builtinInputMapping.emplace_back(BuiltInVertexIndex, 0);
		inputMembers.push_back(llvm::Type::getInt32Ty(state.context));
		state.builtinInputMapping.emplace_back(BuiltInInstanceId, 1);
		state.builtinInputMapping.emplace_back(BuiltInInstanceIndex, 1);
		inputMembers.push_back(llvm::Type::getInt32Ty(state.context));

		state.builtinOutputMapping.emplace_back(BuiltInPosition, 0);
		outputMembers.push_back(llvm::VectorType::get(llvm::Type::getFloatTy(state.context), 4));
		state.builtinOutputMapping.emplace_back(BuiltInPointSize, 1);
		outputMembers.push_back(llvm::Type::getFloatTy(state.context));
		state.builtinOutputMapping.emplace_back(BuiltInClipDistance, 2);
		outputMembers.push_back(llvm::ArrayType::get(llvm::Type::getFloatTy(state.context), 1));
		break;

	case ExecutionModelTessellationControl:
		break;

	case ExecutionModelTessellationEvaluation:
		break;

	case ExecutionModelGeometry:
		break;

	case ExecutionModelFragment:
		state.builtinInputMapping.emplace_back(BuiltInFragCoord, 0);
		inputMembers.push_back(llvm::VectorType::get(llvm::Type::getFloatTy(state.context), 4));
		break;

	case ExecutionModelGLCompute:
		state.builtinInputMapping.emplace_back(BuiltInGlobalInvocationId, 0);
		inputMembers.push_back(llvm::VectorType::get(llvm::Type::getInt32Ty(state.context), 3));

		state.builtinInputMapping.emplace_back(BuiltInLocalInvocationId, 1);
		inputMembers.push_back(llvm::VectorType::get(llvm::Type::getInt32Ty(state.context), 3));

		state.builtinInputMapping.emplace_back(BuiltInWorkgroupId, 2);
		inputMembers.push_back(llvm::VectorType::get(llvm::Type::getInt32Ty(state.context), 3));
		break;

	case ExecutionModelKernel:
		state.builtinInputMapping.emplace_back(BuiltInGlobalInvocationId, 0);
		inputMembers.push_back(llvm::VectorType::get(llvm::Type::getInt32Ty(state.context), 3));

		state.builtinInputMapping.emplace_back(BuiltInLocalInvocationId, 1);
		inputMembers.push_back(llvm::VectorType::get(llvm::Type::getInt32Ty(state.context), 3));

		state.builtinInputMapping.emplace_back(BuiltInWorkgroupId, 2);
		inputMembers.push_back(llvm::VectorType::get(llvm::Type::getInt32Ty(state.context), 3));
		break;

	default:
		FATAL_ERROR();
	}

	const auto linkage = llvm::GlobalVariable::ExternalLinkage;
	const auto tlsModel = llvm::GlobalValue::NotThreadLocal;

	auto llvmType = llvm::StructType::create(state.context, inputMembers, "_BuiltinInput", true);
	state.builtinInputVariable = new llvm::GlobalVariable(*state.module, llvmType, false, linkage, llvm::Constant::getNullValue(llvmType), "_builtinInput", nullptr, tlsModel, 0);
	state.builtinInputVariable->setAlignment(ALIGNMENT);

	llvmType = llvm::StructType::create(state.context, outputMembers, "_BuiltinOutput", true);
	state.builtinOutputVariable = new llvm::GlobalVariable(*state.module, llvmType, false, linkage, llvm::Constant::getNullValue(llvmType), "_builtinOutput", nullptr, tlsModel, 0);
	state.builtinOutputVariable->setAlignment(ALIGNMENT);
}

std::unique_ptr<llvm::Module> ConvertSpirv(llvm::LLVMContext* context, const SPIRV::SPIRVModule* spirvModule, spv::ExecutionModel executionModel, const VkSpecializationInfo* specializationInfo)
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
	
	llvm::IRBuilder<> builder(*context);
	auto llvmModule = std::make_unique<llvm::Module>("", *context);

	if (spirvModule->getAddressingModel() != AddressingModelLogical)
	{
		FATAL_ERROR();
	}
	
	State state
	{
		dataLayout.get(),
		*context,
		builder,
		llvmModule.get(),
		specializationInfo,
	};

	AddBuiltin(state, executionModel);
	
	state.userData = new llvm::GlobalVariable(*state.module, state.builder.getInt8PtrTy(), false, llvm::GlobalValue::ExternalLinkage, llvm::Constant::getNullValue(state.builder.getInt8PtrTy()), "@userData", nullptr, llvm::GlobalValue::NotThreadLocal, 0);
	state.userData->setAlignment(ALIGNMENT);
	
	for (auto i = 0u; i < spirvModule->getNumVariables(); i++)
	{
		ConvertValue(state, spirvModule->getVariable(i), nullptr);
	}

	for (auto i = 0u; i < spirvModule->getNumFunctions(); i++)
	{
		ConvertFunction(state, spirvModule->getFunction(i));
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
						const auto llvmValue = ConvertValue(state, spirvModule->getValue(entry.first), nullptr);
						if (!llvmValue->getType()->isVectorTy() || llvmValue->getType()->getVectorNumElements() != 3 || !llvmValue->getType()->getVectorElementType()->isIntegerTy())
						{
							FATAL_ERROR();
						}

						new llvm::GlobalVariable(*state.module,
						                         llvmValue->getType(),
						                         true,
						                         llvm::GlobalValue::ExternalLinkage,
						                         llvm::dyn_cast<llvm::Constant>(llvmValue),
						                         "@WorkgroupSize",
						                         nullptr,
						                         llvm::GlobalValue::NotThreadLocal);
					}
				}
			}
		}
	}

	// TODO: Optimisations

	Dump(state.module);
	
	return llvmModule;
}

std::string CP_DLL_EXPORT MangleName(const SPIRV::SPIRVVariable* variable)
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
				FATAL_ERROR();
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
				FATAL_ERROR();
			}
			break;
			
		case StorageClassWorkgroup:
		case StorageClassCrossWorkgroup:
		case StorageClassPrivate:
			FATAL_ERROR();
			
		case StorageClassFunction:
			return "";
			
		case StorageClassGeneric:
			FATAL_ERROR();
			
		case StorageClassPushConstant:
			return "_pc_";
			
		case StorageClassAtomicCounter:
			FATAL_ERROR();
			
		case StorageClassImage:
			FATAL_ERROR();

		default:
			FATAL_ERROR();
		}
	}
	
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
		FATAL_ERROR();
		
	case StorageClassPrivate:
		return name;

	case StorageClassFunction:
		return name;

	case StorageClassGeneric:
		FATAL_ERROR();
		
	case StorageClassPushConstant:
		return "_pc_" + name;
		
	case StorageClassAtomicCounter:
	case StorageClassImage:
		FATAL_ERROR();
		
	case StorageClassStorageBuffer:
		return "_buffer_" + name;

	default:
		FATAL_ERROR();
	}
}
