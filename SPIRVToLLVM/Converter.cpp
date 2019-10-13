#include "Converter.h"

#include "SPIRVInstruction.h"
#include "SPIRVModule.h"

#include <llvm/ExecutionEngine/Orc/JITTargetMachineBuilder.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm-c/Target.h>

#include <cstdlib>

#if !defined(_MSC_VER)
#define __debugbreak() __asm__("int3")

inline void strcpy_s(char* destination, const char* source)
{
	strcpy(destination, source);
}
#endif
#define FATAL_ERROR() if (1) { __debugbreak(); abort(); } else (void)0

constexpr auto ALIGNMENT = 8;

struct State
{
	llvm::DataLayout& layout;
	llvm::LLVMContext& context;
	llvm::IRBuilder<>& builder;
	llvm::Module* module;
	
	std::unordered_map<uint32_t, llvm::Type*> typeMapping;
	std::unordered_map<uint32_t, llvm::Value*> valueMapping;
	std::unordered_map<uint32_t, llvm::Function*> functionMapping;
	
	llvm::GlobalVariable* builtinInputVariable;
	llvm::GlobalVariable* builtinOutputVariable;
	std::vector<std::pair<BuiltIn, uint32_t>> builtinInputMapping{};
	std::vector<std::pair<BuiltIn, uint32_t>> builtinOutputMapping{};
};

static void Dump(llvm::Module* llvmModule)
{
	std::string dump;
	llvm::raw_string_ostream DumpStream(dump);
	llvmModule->print(DumpStream, nullptr);
	std::cout << dump << std::endl;
}

static std::string DumpType(llvm::Type* llvmType)
{
	std::string type_str;
	llvm::raw_string_ostream rso(type_str);
	llvmType->print(rso);
	return rso.str();
}

static void HandleTypeDecorate(const std::pair<const Decoration, const SPIRV::SPIRVDecorateGeneric*> decorate)
{
	switch (decorate.first)
	{
	case DecorationBlock:
	case DecorationBufferBlock:
	case DecorationColMajor:
		break;
		
	case DecorationMatrixStride:
	case DecorationArrayStride:
	case DecorationOffset:
		// TODO
		break;

	default:
		FATAL_ERROR();
	}
}

static bool IsBuiltinStruct(SPIRV::SPIRVType* spirvType)
{
	const auto result = spirvType->isTypePointer() &&
		spirvType->getPointerElementType()->isTypeStruct() &&
		spirvType->getPointerElementType()->getStructMemberCount() > 0;

	if (result)
	{
		for (const auto& decorate : spirvType->getPointerElementType()->getMemberDecorates())
		{
			if (decorate.first.second == DecorationBuiltIn)
			{
				return true;
			}
		}
	}

	return false;
}

static llvm::Type* ConvertType(State& state, SPIRV::SPIRVType* spirvType, bool isClassMember = false)
{
	const auto cachedType = state.typeMapping.find(spirvType->getId());
	if (cachedType != state.typeMapping.end())
	{
		return cachedType->second;
	}
	
	// for (const auto decorate : spirvType->getDecorates())
	// {
	// 	HandleTypeDecorate(decorate);
	// }
	//
	// for (const auto decorate : memberDecorates)
	// {
	// 	HandleTypeDecorate(decorate);
	// }
	
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
			//   auto ST = static_cast<SPIRVTypeImage *>(T);
			//   if (ST->isOCLImage())
			//     return mapType(T, getOrCreateOpaquePtrType(M, transOCLImageTypeName(ST)));
			//   else
			//     llvm_unreachable("Unsupported image type");
			//   return nullptr;
			FATAL_ERROR();
		}
		
	case OpTypeSampledImage:
		{
			//   auto ST = static_cast<SPIRVTypeSampledImage *>(T);
			//   return mapType(
			//       T, getOrCreateOpaquePtrType(M, transOCLSampledImageTypeName(ST)));
			FATAL_ERROR();
		}
		
	case OpTypeStruct:
		{
			const auto strct = static_cast<SPIRV::SPIRVTypeStruct*>(spirvType);
			const auto name = strct->getName();
			auto llvmStruct = llvm::StructType::create(state.context, name);
			llvm::SmallVector<llvm::Type*, 4> types;
			for (size_t i = 0; i != strct->getMemberCount(); ++i)
			{
				types.push_back(ConvertType(state, strct->getMemberType(i), true));
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
			llvm::SmallVector<llvm::Type*, 4> types;
			for (auto i = 0u; i < spirvType->getMatrixColumnCount(); i++)
			{
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
		
		// // OpenCL Compiler does not use this instruction
		// case OpTypeVmeImageINTEL:
		//   return nullptr;
		// default: {
		//   auto OC = T->getOpCode();
		//   if (isOpaqueGenericTypeOpCode(OC) || isSubgroupAvcINTELTypeOpCode(OC)) {
		//     auto Name = isSubgroupAvcINTELTypeOpCode(OC)
		//                     ? OCLSubgroupINTELTypeOpCodeMap::rmap(OC)
		//                     : OCLOpaqueTypeOpCodeMap::rmap(OC);
		//     return mapType(
		//         T, getOrCreateOpaquePtrType(M, Name, getOCLOpaqueTypeAddrSpace(OC)));
		//   }
		//   llvm_unreachable("Not implemented");
		// }

		// case OpTypeImage:
		// case OpTypeSampler:
		// case OpTypeSampledImage:
		// 	llvmType = llvm::Type::getInt8PtrTy(state.context);
		// 	break;
		//
		// case OpTypeRuntimeArray:
		// 	llvmType = llvm::StructType::get(state.context, {
		// 		                                 llvm::Type::getInt64Ty(state.context),
		// 		                                 llvm::PointerType::get(ConvertType(state, spirvType->getArrayElementType()), 0),
		// 	                                 }, false);
		// 	break;
		//
		// case OpTypeStruct:
		// 	{
		// 		std::vector<llvm::Type*> members{};
		// 		for (auto i = 0u; i < spirvType->getStructMemberCount(); i++)
		// 		{
		// 			std::vector<std::pair<Decoration, const SPIRV::SPIRVMemberDecorate*>> newMemberDecorates{};
		// 			for (const auto decorate : spirvType->getMemberDecorates())
		// 			{
		// 				if (decorate.first.first == i)
		// 				{
		// 					newMemberDecorates.emplace_back(decorate.first.second, decorate.second);
		// 				}
		// 			}
		//
		// 			const auto member = spirvType->getStructMemberType(i);
		// 			members.push_back(ConvertType(state, member, newMemberDecorates));
		// 		}
		// 		llvmType = llvm::StructType::create(state.context, members, spirvType->getName());
		// 		break;
		// 	}
		//
		// case OpTypePointer:
		// 	if (IsBuiltinStruct(spirvType))
		// 	{
		// 		if (spirvType->getPointerStorageClass() == StorageClassOutput)
		// 		{
		// 			llvmType = state.builtinOutputVariable->getType();
		// 		}
		// 		else if (spirvType->getPointerStorageClass() == StorageClassInput)
		// 		{
		// 			llvmType = state.builtinInputVariable->getType();
		// 		}
		// 		else
		// 		{
		// 			FATAL_ERROR();
		// 		}
		// 	}
		// 	else if (spirvType->hasDecorate(DecorationBuiltIn))
		// 	{
		// 		FATAL_ERROR();
		// 	}
		// 	else
		// 	{
		// 		llvmType = llvm::PointerType::get(ConvertType(state, spirvType->getPointerElementType()), 0);
		// 	}
		// 	break;
		
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

static std::string ConvertName(SPIRV::SPIRVVariable* variable)
{
	switch (variable->getStorageClass())
	{
	case StorageClassUniformConstant:
		return variable->getName();
		
	case StorageClassInput:
		return "_input_" + variable->getName();
		
	case StorageClassUniform:
		return "_uniform_" + variable->getName();

	case StorageClassOutput:
		return "_output_" + variable->getName();

	case StorageClassWorkgroup:
	case StorageClassCrossWorkgroup:
	case StorageClassPrivate:
	case StorageClassFunction:
	case StorageClassGeneric:
	case StorageClassPushConstant:
	case StorageClassAtomicCounter:
	case StorageClassImage:
	case StorageClassStorageBuffer:
	default:
		FATAL_ERROR();
	}
}

static llvm::Value* ConvertValueNoDecoration(State& state, SPIRV::SPIRVValue* spirvValue)
{
	switch (spirvValue->getOpCode())
	{
	case OpConstant:
		{
			const auto spirvConstant = static_cast<SPIRV::SPIRVConstant*>(spirvValue);
			const auto spirvType = spirvConstant->getType();
			const auto llvmType = ConvertType(state, spirvType);
			switch (spirvType->getOpCode())
			{
			case OpTypeBool:
			case OpTypeInt:
				return llvm::ConstantInt::get(llvmType, spirvConstant->getInt64Value(), static_cast<SPIRV::SPIRVTypeInt*>(spirvType)->isSigned());

			case OpTypeFloat:
				{
					// 	const llvm::fltSemantics* FS = nullptr;
					// 	switch (BT->getFloatBitWidth()) {
					// 	case 16:
					// 		FS = &APFloat::IEEEhalf();
					// 		break;
					// 	case 32:
					// 		FS = &APFloat::IEEEsingle();
					// 		break;
					// 	case 64:
					// 		FS = &APFloat::IEEEdouble();
					// 		break;
					// 	default:
					// 		llvm_unreachable("invalid float type");
					// 	}
					// 	return mapValue(
					// 		BV, ConstantFP::get(*Context,
					// 		                    APFloat(*FS, APInt(BT->getFloatBitWidth(),
					// 		                                       BConst->getZExtIntValue()))));
					FATAL_ERROR();
				}
			}

			FATAL_ERROR();
		}

	case OpConstantTrue:
		// return mapValue(BV, ConstantInt::getTrue(*Context));
		FATAL_ERROR();

	case OpConstantFalse:
		// return mapValue(BV, ConstantInt::getFalse(*Context));
		FATAL_ERROR();

	case OpConstantNull:
		{
			// auto LT = transType(BV->getType());
			// return mapValue(BV, Constant::getNullValue(LT));
			FATAL_ERROR();
		}

	case OpConstantComposite:
		{
			// auto BCC = static_cast<SPIRVConstantComposite*>(BV);
			// std::vector<Constant*> CV;
			// for (auto& I : BCC->getElements())
			// 	CV.push_back(dyn_cast<Constant>(transValue(I, F, BB)));
			// switch (BV->getType()->getOpCode()) {
			// case OpTypeVector:
			// 	return mapValue(BV, ConstantVector::get(CV));
			// case OpTypeArray:
			// 	return mapValue(
			// 		BV, ConstantArray::get(dyn_cast<ArrayType>(transType(BCC->getType())),
			// 		                       CV));
			// case OpTypeStruct: {
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
			// }
			// default:
			// 	llvm_unreachable("not implemented");
			// 	return nullptr;
			// }
			FATAL_ERROR();
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
		{
			// auto BI =
			// 	createInstFromSpecConstantOp(static_cast<SPIRVSpecConstantOp*>(BV));
			// return mapValue(BV, transValue(BI, nullptr, nullptr, false));
			FATAL_ERROR();
		}

	case OpUndef:
		// return mapValue(BV, UndefValue::get(transType(BV->getType())));
		FATAL_ERROR();

	case OpVariable:
		{
			const auto variable = static_cast<SPIRV::SPIRVVariable*>(spirvValue);
			const auto llvmType = ConvertType(state, variable->getType());
			const auto linkage = ConvertLinkage(variable);
			const auto name = ConvertName(variable);
			llvm::Constant* initialiser = nullptr;
			const auto spirvInitialiser = variable->getInitializer();
			if (spirvInitialiser)
			{
				// 	Initializer = dyn_cast<Constant>(transValue(Init, F, BB, false));
				FATAL_ERROR();
			}
			else
			{
				initialiser = llvm::Constant::getNullValue(llvmType);
			}

			if (variable->getStorageClass() == StorageClassFunction && !initialiser)
			{
				FATAL_ERROR();
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

			SPIRV::SPIRVBuiltinVariableKind builtinKind;
			if (variable->isBuiltin(&builtinKind))
			{
				// 	BuiltinGVMap[LVar] = BVKind;
				FATAL_ERROR();
			}
			return llvmVariable;
		}

	case OpFunctionParameter:
		{
			// auto BA = static_cast<SPIRVFunctionParameter*>(BV);
			// assert(F && "Invalid function");
			// unsigned ArgNo = 0;
			// for (Function::arg_iterator I = F->arg_begin(), E = F->arg_end(); I != E;
			//      ++I, ++ArgNo) {
			// 	if (ArgNo == BA->getArgNo())
			// 		return mapValue(BV, &(*I));
			// }
			// llvm_unreachable("Invalid argument");
			// return nullptr;
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
		unsigned alignment;
		if (spirvValue->hasAlignment(&alignment))
		{
			alloca->setAlignment(alignment);
		}
	}

	if (auto globalVariable = llvm::dyn_cast<llvm::GlobalVariable>(llvmValue))
	{
		unsigned alignment;
		if (spirvValue->hasAlignment(&alignment))
		{
			globalVariable->setAlignment(alignment);
		}
	}
}

static llvm::Value* ConvertValue(State& state, SPIRV::SPIRVValue* spirvValue)
{
	const auto cachedType = state.valueMapping.find(spirvValue->getId());
	if (cachedType != state.valueMapping.end())
	{
		if (const auto variable = llvm::dyn_cast<llvm::GlobalVariable>(cachedType->second))
		{
			return state.builder.CreateAlignedLoad(variable, ALIGNMENT);
		}
		return cachedType->second;
	}

	if (spirvValue->isVariable() && spirvValue->getType()->isTypePointer())
	{
		if (spirvValue->getType()->getPointerElementType()->hasDecorate(DecorationBuiltIn))
		{
			FATAL_ERROR();
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

	auto llvmValue = ConvertValueNoDecoration(state, spirvValue);
	if (!spirvValue->getName().empty() && llvmValue->getName().empty())
	{
		llvmValue->setName(spirvValue->getName());
	}

	ConvertDecoration(llvmValue, spirvValue);

	state.valueMapping[spirvValue->getId()] = llvmValue;
	return llvmValue;
}

static std::vector<llvm::Value*> ConvertValue(State& state, std::vector<SPIRV::SPIRVValue*> spirvValues)
{
	std::vector<llvm::Value*> results{spirvValues.size(), nullptr};
	for (size_t i = 0; i < spirvValues.size(); i++)
	{
		results[i] = ConvertValue(state, spirvValues[i]);
	}
	return results;
}

static llvm::Function* GetInbuiltFunction(State& state, SPIRV::SPIRVMatrixTimesVector* matrixTimesVector)
{
	// TODO: Cache
	// TODO: Detect row/column major
	const auto matrix = matrixTimesVector->getMatrix()->getType();
	const auto vector = matrixTimesVector->getVector()->getType();

	if (matrix->getMatrixColumnCount() != 4 || matrix->getMatrixRowCount() != 4 || vector->getVectorComponentCount() != 4 || !vector->getVectorComponentType()->isTypeFloat(32))
	{
		FATAL_ERROR();
	}

	llvm::Type* params[3];
	params[0] = llvm::PointerType::get(ConvertType(state, vector), 0);
	params[1] = llvm::PointerType::get(ConvertType(state, matrix), 0);
	params[2] = llvm::PointerType::get(ConvertType(state, vector), 0);
	
	const auto functionType = llvm::FunctionType::get(llvm::Type::getVoidTy(state.context), params, false);
	auto function = llvm::Function::Create(functionType, llvm::GlobalValue::ExternalLinkage, 0, "_Matrix_4_4_F32_Col_Mult_Vector_4_F32", state.module);
	function->addDereferenceableAttr(0, 16);
	function->addDereferenceableAttr(1, 64);
	function->addDereferenceableAttr(2, 16);
	return function;
}

static llvm::Function* GetInbuiltFunction(State& state, SPIRV::SPIRVMatrixTimesMatrix* matrixTimesMatrix)
{
	// TODO: Cache
	// TODO: Detect row/column major
	const auto left = matrixTimesMatrix->getMatrixLeft()->getType();
	const auto right = matrixTimesMatrix->getMatrixRight()->getType();

	if (left->getMatrixColumnCount() != 4 || left->getMatrixRowCount() != 4 || !left->getMatrixComponentType()->isTypeFloat(32))
	{
		FATAL_ERROR();
	}

	if (right->getMatrixColumnCount() != 4 || right->getMatrixRowCount() != 4 || !right->getMatrixComponentType()->isTypeFloat(32))
	{
		FATAL_ERROR();
	}

	llvm::Type* params[3];
	params[0] = llvm::PointerType::get(ConvertType(state, left), 0);
	params[1] = llvm::PointerType::get(ConvertType(state, left), 0);
	params[2] = llvm::PointerType::get(ConvertType(state, right), 0);
	
	const auto functionType = llvm::FunctionType::get(llvm::Type::getVoidTy(state.context), params, false);
	auto function = llvm::Function::Create(functionType, llvm::GlobalValue::ExternalLinkage, 0, "_Matrix_4_4_F32_Col_Mult_Matrix_4_4_F32_Col", state.module);
	function->addDereferenceableAttr(0, 64);
	function->addDereferenceableAttr(1, 64);
	function->addDereferenceableAttr(2, 64);
	return function;
}

static llvm::Function* GetInbuiltFunction(State& state, SPIRV::SPIRVImageSampleExplicitLod* imageSampleExplicitLod, bool& isLod)
{
	// TODO: Cache
	const auto resultType = imageSampleExplicitLod->getType();
	const auto coordinateType = imageSampleExplicitLod->getOpValue(1)->getType();
	const auto operand = imageSampleExplicitLod->getOpWord(2);
	if (operand == 4)
	{
		isLod = false;
		FATAL_ERROR();
	}

	if (operand == 2)
	{
		isLod = true;

		if (resultType->getVectorComponentCount() != 4 || !resultType->getVectorComponentType()->isTypeFloat(32) || coordinateType->getVectorComponentCount() != 2 || !coordinateType->getVectorComponentType()->isTypeFloat(32))
		{
			FATAL_ERROR();
		}

		llvm::Type* params[4];
		params[0] = llvm::PointerType::get(ConvertType(state, resultType), 0);
		params[1] = llvm::Type::getInt8PtrTy(state.context);
		params[2] = llvm::PointerType::get(ConvertType(state, coordinateType), 0);
		params[3] = llvm::Type::getFloatTy(state.context);

		const auto functionType = llvm::FunctionType::get(llvm::Type::getVoidTy(state.context), params, false);
		auto function = llvm::Function::Create(functionType, llvm::GlobalValue::ExternalLinkage, 0, "_Image_Sample_4_F32_2_F32_Lod", state.module);
		function->addDereferenceableAttr(0, 16);
		function->addDereferenceableAttr(2, 16);
		return function;
	}

	FATAL_ERROR();
}

static void ConvertBasicBlock(State& state, llvm::Function* llvmFunction, const SPIRV::SPIRVBasicBlock* spirvBasicBlock)
{
	for (const auto decorate : spirvBasicBlock->getDecorates())
	{
		FATAL_ERROR();
	}
	
	const auto llvmBasicBlock = llvm::BasicBlock::Create(state.context, spirvBasicBlock->getName(), llvmFunction);
	state.builder.SetInsertPoint(llvmBasicBlock);

	for (auto i = 0u; i < spirvBasicBlock->getNumInst(); i++)
	{
		const auto instruction = spirvBasicBlock->getInst(i);
		for (const auto decorate : instruction->getDecorates())
		{
			FATAL_ERROR();
		}

		llvm::Value* llvmValue = nullptr;
		switch (instruction->getOpCode())
		{
		case OpNop:
			break;

			// case OpUndef: break;
			// case OpConstantTrue: break;
			// case OpConstantFalse: break;
			// case OpConstant: break;
			// case OpConstantComposite: break;
			// case OpConstantSampler: break;
			// case OpConstantNull: break;
			// case OpSpecConstantTrue: break;
			// case OpSpecConstantFalse: break;
			// case OpSpecConstant: break;
			// case OpSpecConstantComposite: break;
			// case OpSpecConstantOp: break;
			// case OpFunction: break;
			// case OpFunctionParameter: break;
			// case OpFunctionEnd: break;
			// case OpFunctionCall: break;
			// case OpVariable: break;
			// case OpImageTexelPointer: break;

		case OpLoad:
			{
				const auto load = reinterpret_cast<SPIRV::SPIRVLoad*>(instruction);
				llvmValue = state.builder.CreateAlignedLoad(ConvertValue(state, load->getSrc()), ALIGNMENT, load->SPIRVMemoryAccess::isVolatile(), load->getName());
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
				llvmValue = state.builder.CreateAlignedStore(ConvertValue(state, store->getSrc()), 
				                                             ConvertValue(state, store->getDst()), 
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
				const auto base = ConvertValue(state, accessChain->getBase());
				auto indices = ConvertValue(state, accessChain->getIndices());

				if (!accessChain->hasPtrIndex())
				{
					indices.insert(indices.begin(), state.builder.getInt32(0));
				}

				if (accessChain->isInBounds())
				{
					llvmValue = state.builder.CreateInBoundsGEP(nullptr, base, indices, accessChain->getName());
				}
				else
				{
					llvmValue = state.builder.CreateGEP(nullptr, base, indices, accessChain->getName());
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
			// case OpVectorExtractDynamic: break;
			// case OpVectorInsertDynamic: break;
			// case OpVectorShuffle: break;
			 
		case OpCompositeConstruct:
			{
				// const auto compositeConstruct = reinterpret_cast<SPIRV::SPIRVCompositeConstruct*>(instruction);
				// const auto type = compositeConstruct->getType();
				// if (type->isTypeVector())
				// {
				// 	llvmValue = llvm::UndefValue::get(ConvertType(state, type));
				// 	for (auto j = 0u; j < compositeConstruct->getConstituents().size(); j++)
				// 	{
				// 		llvmValue = state.builder.CreateInsertElement(llvmValue, ConvertValue(state, compositeConstruct->getConstituents()[j]), j);
				// 	}
				// }
				// else
				// {
				// 	FATAL_ERROR();
				// }
				 
				FATAL_ERROR();
				
				break;
			}
			 
		case OpCompositeExtract:
			{
				// const auto compositeExtract = reinterpret_cast<SPIRV::SPIRVCompositeExtract*>(instruction);
				// llvmValue = ConvertValue(state, compositeExtract->getComposite());
				// auto type = compositeExtract->getComposite()->getType();
				// auto indices = compositeExtract->getIndices();
				//
				// for (auto index : indices)
				// {
				// 	if (type->isTypeVector())
				// 	{
				// 		llvmValue = state.builder.CreateExtractElement(llvmValue, index);
				// 		type = type->getVectorComponentType();
				// 	}
				// 	else
				// 	{
				// 		FATAL_ERROR();
				// 	}
				// }

				FATAL_ERROR();
				
				break;
			}
			
			// case OpCompositeInsert: break;
			// case OpCopyObject: break;
			// case OpTranspose: break;
			// case OpSampledImage: break;
			// case OpImageSampleImplicitLod: break;

		case OpImageSampleExplicitLod:
			{
				// const auto imageSampleExplicitLod = reinterpret_cast<SPIRV::SPIRVImageSampleExplicitLod*>(instruction);
				// bool isLod;
				// const auto function = GetInbuildFunction(state, imageSampleExplicitLod, isLod);
				//
				// if (isLod)
				// {
				// 	llvm::Value* args[4];
				// 	args[1] = ConvertValue(state, imageSampleExplicitLod->getOpValue(0));
				// 	args[2] = ConvertValue(state, imageSampleExplicitLod->getOpValue(1));
				// 	args[3] = ConvertValue(state, imageSampleExplicitLod->getOpValue(3));
				//
				// 	const auto tmp0 = state.builder.CreateAlloca(ConvertType(state, imageSampleExplicitLod->getType()));
				// 	const auto tmp2 = state.builder.CreateAlloca(args[2]->getType());
				// 	state.builder.CreateStore(args[2], tmp2);
				// 	args[0] = tmp0;
				// 	args[2] = tmp2;
				//
				// 	state.builder.CreateCall(function, args);
				// 	llvmValue = state.builder.CreateLoad(tmp0);
				// }
				// else
				// {
				// 	FATAL_ERROR();
				// }

				FATAL_ERROR();
				
				break;
			}
			
			// case OpImageSampleDrefImplicitLod: break;
			// case OpImageSampleDrefExplicitLod: break;
			// case OpImageSampleProjImplicitLod: break;
			// case OpImageSampleProjExplicitLod: break;
			// case OpImageSampleProjDrefImplicitLod: break;
			// case OpImageSampleProjDrefExplicitLod: break;
			// case OpImageFetch: break;
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
			// case OpConvertFToU: break;
			// case OpConvertFToS: break;
			// case OpConvertSToF: break;
			// case OpConvertUToF: break;
			// case OpUConvert: break;
			// case OpSConvert: break;
			// case OpFConvert: break;
			// case OpQuantizeToF16: break;
			// case OpConvertPtrToU: break;
			// case OpSatConvertSToU: break;
			// case OpSatConvertUToS: break;
			// case OpConvertUToPtr: break;
			// case OpPtrCastToGeneric: break;
			// case OpGenericCastToPtr: break;
			// case OpGenericCastToPtrExplicit: break;
			// case OpBitcast: break;
			// case OpSNegate: break;
			// case OpFNegate: break;
			// case OpIAdd: break;
			// case OpFAdd: break;
			// case OpISub: break;
			// case OpFSub: break;
			// case OpIMul: break;
			// case OpFMul: break;
			// case OpUDiv: break;
			// case OpSDiv: break;
			// case OpFDiv: break;
			// case OpUMod: break;
			// case OpSRem: break;
			// case OpSMod: break;
			// case OpFRem: break;
			// case OpFMod: break;
			// case OpVectorTimesScalar: break;
			// case OpMatrixTimesScalar: break;
			// case OpVectorTimesMatrix: break;
			 
		case OpMatrixTimesVector:
			{
				const auto matrixTimesVector = reinterpret_cast<SPIRV::SPIRVMatrixTimesVector*>(instruction);
				llvm::Value* args[3];
				args[1] = ConvertValue(state, matrixTimesVector->getMatrix());
				args[2] = ConvertValue(state, matrixTimesVector->getVector());

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
				// const auto matrixTimesMatrix = reinterpret_cast<SPIRV::SPIRVMatrixTimesMatrix*>(instruction);
				// llvm::Value* args[3];
				// args[1] = ConvertValue(state, matrixTimesMatrix->getMatrixLeft());
				// args[2] = ConvertValue(state, matrixTimesMatrix->getMatrixRight());
				//
				// const auto tmp0 = state.builder.CreateAlloca(args[1]->getType());
				// const auto tmp1 = state.builder.CreateAlloca(args[1]->getType());
				// const auto tmp2 = state.builder.CreateAlloca(args[2]->getType());
				// state.builder.CreateStore(args[1], tmp1);
				// state.builder.CreateStore(args[2], tmp2);
				//
				// args[0] = tmp0;
				// args[1] = tmp1;
				// args[2] = tmp2;
				//
				// state.builder.CreateCall(GetInbuildFunction(state, matrixTimesMatrix), args);
				// llvmValue = state.builder.CreateLoad(tmp0);

				FATAL_ERROR();
				
				break;
			}
			
			// case OpMatrixTimesMatrix: break;
			// case OpOuterProduct: break;
			// case OpDot: break;
			// case OpIAddCarry: break;
			// case OpISubBorrow: break;
			// case OpUMulExtended: break;
			// case OpSMulExtended: break;
			// case OpAny: break;
			// case OpAll: break;
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
			// case OpLogicalOr: break;
			// case OpLogicalAnd: break;
			// case OpLogicalNot: break;
			// case OpSelect: break;
			// case OpIEqual: break;
			// case OpINotEqual: break;
			// case OpUGreaterThan: break;
			// case OpSGreaterThan: break;
			// case OpUGreaterThanEqual: break;
			// case OpSGreaterThanEqual: break;
			// case OpULessThan: break;
			// case OpSLessThan: break;
			// case OpULessThanEqual: break;
			// case OpSLessThanEqual: break;
			// case OpFOrdEqual: break;
			// case OpFUnordEqual: break;
			// case OpFOrdNotEqual: break;
			// case OpFUnordNotEqual: break;
			// case OpFOrdLessThan: break;
			// case OpFUnordLessThan: break;
			// case OpFOrdGreaterThan: break;
			// case OpFUnordGreaterThan: break;
			// case OpFOrdLessThanEqual: break;
			// case OpFUnordLessThanEqual: break;
			// case OpFOrdGreaterThanEqual: break;
			// case OpFUnordGreaterThanEqual: break;
			// case OpShiftRightLogical: break;
			// case OpShiftRightArithmetic: break;
			// case OpShiftLeftLogical: break;
			// case OpBitwiseOr: break;
			// case OpBitwiseXor: break;
			// case OpBitwiseAnd: break;
			// case OpNot: break;
			// case OpBitFieldInsert: break;
			// case OpBitFieldSExtract: break;
			// case OpBitFieldUExtract: break;
			// case OpBitReverse: break;
			// case OpBitCount: break;
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
			// case OpAtomicLoad: break;
			// case OpAtomicStore: break;
			// case OpAtomicExchange: break;
			// case OpAtomicCompareExchange: break;
			// case OpAtomicCompareExchangeWeak: break;
			// case OpAtomicIIncrement: break;
			// case OpAtomicIDecrement: break;
			// case OpAtomicIAdd: break;
			// case OpAtomicISub: break;
			// case OpAtomicSMin: break;
			// case OpAtomicUMin: break;
			// case OpAtomicSMax: break;
			// case OpAtomicUMax: break;
			// case OpAtomicAnd: break;
			// case OpAtomicOr: break;
			// case OpAtomicXor: break;
			// case OpPhi: break;
			// case OpLoopMerge: break;
			// case OpSelectionMerge: break;
			// case OpLabel: break;
			// case OpBranch: break;
			// case OpBranchConditional: break;
			// case OpSwitch: break;
			// case OpKill: break;
			 
		case OpReturn:
			llvmValue = state.builder.CreateRetVoid();
			break;
			
			// case OpReturnValue: break;
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

		if (llvmValue && instruction->hasId())
		{
			state.valueMapping[instruction->getId()] = llvmValue;
		}
	}
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
		state.builtinInputMapping.push_back(std::make_pair(BuiltInVertexId, 0));
		inputMembers.push_back(llvm::Type::getInt32Ty(state.context));
		state.builtinInputMapping.push_back(std::make_pair(BuiltInInstanceId, 1));
		inputMembers.push_back(llvm::Type::getInt32Ty(state.context));

		state.builtinOutputMapping.push_back(std::make_pair(BuiltInPosition, 0));
		outputMembers.push_back(llvm::VectorType::get(llvm::Type::getFloatTy(state.context), 4));
		state.builtinOutputMapping.push_back(std::make_pair(BuiltInPointSize, 1));
		outputMembers.push_back(llvm::Type::getFloatTy(state.context));
		state.builtinOutputMapping.push_back(std::make_pair(BuiltInClipDistance, 2));
		outputMembers.push_back(llvm::ArrayType::get(llvm::Type::getFloatTy(state.context), 1));
		break;

	case ExecutionModelTessellationControl:
		break;

	case ExecutionModelTessellationEvaluation:
		break;

	case ExecutionModelGeometry:
		break;

	case ExecutionModelFragment:
		break;

	case ExecutionModelGLCompute:
		inputMembers.push_back(llvm::Type::getInt32Ty(state.context));
		break;

	case ExecutionModelKernel:
		break;

	default:
		FATAL_ERROR();
	}

	const auto linkage = llvm::GlobalVariable::ExternalLinkage;
	const auto tlsModel = llvm::GlobalValue::NotThreadLocal;

	auto llvmType = llvm::PointerType::get(llvm::StructType::create(state.context, inputMembers, "_BuiltinInput"), 0);
	state.builtinInputVariable = new llvm::GlobalVariable(*state.module, llvmType, false, linkage, llvm::Constant::getNullValue(llvmType), "_builtinInput", nullptr, tlsModel, 0);
	state.builtinInputVariable->setAlignment(ALIGNMENT);

	llvmType = llvm::PointerType::get(llvm::StructType::create(state.context, outputMembers, "_BuiltinOutput"), 0);
	state.builtinOutputVariable = new llvm::GlobalVariable(*state.module, llvmType, false, linkage, llvm::Constant::getNullValue(llvmType), "_builtinOutput", nullptr, tlsModel, 0);
	state.builtinOutputVariable->setAlignment(ALIGNMENT);
}

std::unique_ptr<llvm::Module> ConvertSpirv(llvm::LLVMContext* context, const SPIRV::SPIRVModule* spirvModule, spv::ExecutionModel executionModel)
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
	
	State state
	{
		dataLayout.get(),
		*context,
		builder,
		llvmModule.get(),
	};

	AddBuiltin(state, executionModel);
	
	for (auto i = 0u; i < spirvModule->getNumVariables(); i++)
	{
		ConvertValue(state, spirvModule->getVariable(i));
	}

	for (auto i = 0u; i < spirvModule->getNumFunctions(); i++)
	{
		ConvertFunction(state, spirvModule->getFunction(i));
	}

	// TODO: Optimisations

	Dump(state.module);
	
	return llvmModule;
}
