#include "Converter.h"

#include "SPIRVInstruction.h"
#include "SPIRVModule.h"

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>

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
	llvm::LLVMContext& context;
	llvm::IRBuilder<>& builder;
	llvm::Module* module;
	
	std::unordered_map<uint32_t, llvm::Type*> typeMapping;
	std::unordered_map<uint32_t, llvm::GlobalVariable*> variableMapping;
	std::unordered_map<uint32_t, llvm::Value*> valueMapping;
	std::unordered_map<uint32_t, llvm::Function*> functionMapping;
	
	llvm::GlobalVariable* builtinInputVariable;
	llvm::GlobalVariable* builtinOutputVariable;
};

static void HandleTypeDecorate(const std::pair<const Decoration, const SPIRV::SPIRVDecorateGeneric*> decorate)
{
	switch (decorate.first)
	{
	case DecorationBlock:
	case DecorationBufferBlock:
	case DecorationBuiltIn:
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

static llvm::Type* ConvertType(State& state, SPIRV::SPIRVType* spirvType, const std::vector<std::pair<Decoration, const SPIRV::SPIRVMemberDecorate*>> memberDecorates = {})
{
	for (const auto decorate : spirvType->getDecorates())
	{
		HandleTypeDecorate(decorate);
	}

	for (const auto decorate : memberDecorates)
	{
		HandleTypeDecorate(decorate);
	}
	
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
		llvmType = llvm::Type::getInt8Ty(state.context);
		break;

	case OpTypeInt:
		switch (spirvType->getIntegerBitWidth())
		{
		case 8:
			llvmType = llvm::Type::getInt8Ty(state.context);
			break;

		case 16:
			llvmType = llvm::Type::getInt16Ty(state.context);
			break;

		case 32:
			llvmType = llvm::Type::getInt32Ty(state.context);
			break;

		case 64:
			llvmType = llvm::Type::getInt64Ty(state.context);
			break;

		default:
			FATAL_ERROR();
		}
		break;

	case OpTypeFloat:
		switch (spirvType->getFloatBitWidth())
		{
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

	case OpTypeVector:
		llvmType = llvm::VectorType::get(ConvertType(state, spirvType->getVectorComponentType()), spirvType->getVectorComponentCount());
		break;

	case OpTypeMatrix:
		{
			std::vector<llvm::Type*> members{};
			for (auto i = 0u; i < spirvType->getMatrixColumnCount(); i++)
			{
				const auto member = spirvType->getMatrixVectorType();
				members.push_back(ConvertType(state, member));
			}

			llvmType = llvm::StructType::create(state.context, members, spirvType->getName());
			break;
		}

	case OpTypeImage:
	case OpTypeSampler:
	case OpTypeSampledImage:
		llvmType = llvm::Type::getInt8PtrTy(state.context);
		break;

	case OpTypeArray:
		llvmType = llvm::ArrayType::get(ConvertType(state, spirvType->getArrayElementType()), spirvType->getArrayLength());
		break;

	case OpTypeRuntimeArray:
		llvmType = llvm::StructType::get(state.context, {
			                                 llvm::Type::getInt64Ty(state.context),
			                                 llvm::PointerType::get(ConvertType(state, spirvType->getArrayElementType()), 0),
		                                 }, false);
		break;

	case OpTypeStruct:
		{
			std::vector<llvm::Type*> members{};
			auto isBuiltinStruct = false;
			for (auto i = 0u; i < spirvType->getStructMemberCount(); i++)
			{
				std::vector<std::pair<Decoration, const SPIRV::SPIRVMemberDecorate*>> newMemberDecorates{};
				for (const auto decorate : spirvType->getMemberDecorates())
				{
					if (decorate.first.second == DecorationBuiltIn)
					{
						isBuiltinStruct = true;
					}
					if (decorate.first.first == i)
					{
						newMemberDecorates.push_back(std::make_pair(decorate.first.second, decorate.second));
					}
				}

				const auto member = spirvType->getStructMemberType(i);
				members.push_back(ConvertType(state, member, newMemberDecorates));
			}

			if (isBuiltinStruct)
			{
				// TODO: Verify same layout
				// TODO: Check if input/output
				llvmType = state.builtinOutputVariable->getType()->getElementType();
			}
			else
			{
				llvmType = llvm::StructType::create(state.context, members, spirvType->getName());
			}
			break;
		}

	case OpTypeOpaque:
		FATAL_ERROR();
		break;

	case OpTypePointer:
		llvmType = llvm::PointerType::get(ConvertType(state, spirvType->getPointerElementType()), 0);
		break;

	case OpTypeFunction:
		{
			std::vector<llvm::Type*> params{};
			const auto functionType = static_cast<SPIRV::SPIRVTypeFunction*>(spirvType);
			for (auto i = 0u; i < functionType->getNumParameters(); i++)
			{
				const auto param = functionType->getParameterType(i);
				params.push_back(ConvertType(state, param));
			}
			
			llvmType = llvm::FunctionType::get(ConvertType(state, spirvType->getFunctionReturnType()), params, false);
			break;
		}

	case OpTypeEvent:
		FATAL_ERROR();
		break;

	case OpTypeDeviceEvent:
		FATAL_ERROR();
		break;

	case OpTypeReserveId:
		FATAL_ERROR();
		break;

	case OpTypeQueue:
		FATAL_ERROR();
		break;

	case OpTypePipe:
		FATAL_ERROR();
		break;

	case OpTypeForwardPointer:
		FATAL_ERROR();
		break;

	case OpTypePipeStorage:
		FATAL_ERROR();
		break;

	case OpTypeNamedBarrier:
		FATAL_ERROR();
		break;

	case OpTypeVmeImageINTEL:
		FATAL_ERROR();
		break;

	case OpTypeAvcImePayloadINTEL:
		FATAL_ERROR();
		break;

	case OpTypeAvcRefPayloadINTEL:
		FATAL_ERROR();
		break;

	case OpTypeAvcSicPayloadINTEL:
		FATAL_ERROR();
		break;

	case OpTypeAvcMcePayloadINTEL:
		FATAL_ERROR();
		break;

	case OpTypeAvcMceResultINTEL:
		FATAL_ERROR();
		break;

	case OpTypeAvcImeResultINTEL:
		FATAL_ERROR();
		break;

	case OpTypeAvcImeResultSingleReferenceStreamoutINTEL:
		FATAL_ERROR();
		break;

	case OpTypeAvcImeResultDualReferenceStreamoutINTEL:
		FATAL_ERROR();
		break;

	case OpTypeAvcImeSingleReferenceStreaminINTEL:
		FATAL_ERROR();
		break;

	case OpTypeAvcImeDualReferenceStreaminINTEL:
		FATAL_ERROR();
		break;

	case OpTypeAvcRefResultINTEL:
		FATAL_ERROR();
		break;

	case OpTypeAvcSicResultINTEL:
		FATAL_ERROR();
		break;
		
	default:
		FATAL_ERROR();
	}

	state.typeMapping[spirvType->getId()] = llvmType;
	
	return llvmType;
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

static llvm::GlobalVariable* ConvertVariable(State& state, const SPIRV::SPIRVVariable* spirvVariable)
{
	const auto cachedType = state.variableMapping.find(spirvVariable->getId());
	if (cachedType != state.variableMapping.end())
	{
		return cachedType->second;
	}

	const auto llvmType = ConvertType(state, spirvVariable->getType());
	const auto storage = spirvVariable->getStorageClass();

	const auto isBuiltinStruct = IsBuiltinStruct(spirvVariable->getType());
	if (isBuiltinStruct)
	{
		if (storage == StorageClassInput)
		{
			state.variableMapping[spirvVariable->getId()] = state.builtinInputVariable;
			return state.builtinInputVariable;
		}

		if (storage == StorageClassOutput)
		{
			state.variableMapping[spirvVariable->getId()] = state.builtinOutputVariable;
			return state.builtinOutputVariable;
		}
		FATAL_ERROR();
	}

	for (const auto decorate : spirvVariable->getDecorates())
	{
		switch (decorate.first)
		{
		case DecorationBinding:
		case DecorationDescriptorSet:
		case DecorationLocation:
		case DecorationRelaxedPrecision:
			break;
			
		default:
			FATAL_ERROR();
		}
	}

	if (spirvVariable->getInitializer())
	{
		FATAL_ERROR();
	}

	llvm::GlobalValue::LinkageTypes linkage;
	llvm::GlobalValue::ThreadLocalMode tlsModel;
	std::string name{};
	
	switch (storage)
	{
	case StorageClassUniformConstant:
		linkage = llvm::GlobalVariable::ExternalLinkage;
		tlsModel = llvm::GlobalValue::GeneralDynamicTLSModel;
		name = "_uniformc_" + spirvVariable->getName();
		break;

	case StorageClassInput:
		linkage = llvm::GlobalVariable::ExternalLinkage;
		tlsModel = llvm::GlobalValue::GeneralDynamicTLSModel;
		name = "_input_" + spirvVariable->getName();
		break;

	case StorageClassUniform:
		linkage = llvm::GlobalVariable::ExternalLinkage;
		tlsModel = llvm::GlobalValue::GeneralDynamicTLSModel;
		name = "_uniform_" + spirvVariable->getName();
		break;

	case StorageClassOutput:
		linkage = llvm::GlobalVariable::ExternalLinkage;
		tlsModel = llvm::GlobalValue::GeneralDynamicTLSModel;
		name = "_output_" + spirvVariable->getName();
		break;

	case StorageClassWorkgroup:
		FATAL_ERROR();
		break;

	case StorageClassCrossWorkgroup:
		FATAL_ERROR();
		break;

	case StorageClassPrivate:
		FATAL_ERROR();
		break;

	case StorageClassFunction:
		FATAL_ERROR();
		break;

	case StorageClassGeneric:
		FATAL_ERROR();
		break;

	case StorageClassPushConstant:
		FATAL_ERROR();
		break;

	case StorageClassAtomicCounter:
		FATAL_ERROR();
		break;

	case StorageClassImage:
		FATAL_ERROR();
		break;

	case StorageClassStorageBuffer:
		FATAL_ERROR();
		break;
		
	default:
		FATAL_ERROR();
	}

	tlsModel = llvm::GlobalValue::NotThreadLocal;
	const auto llvmVariable = new llvm::GlobalVariable(*state.module, llvmType, spirvVariable->isConstant(), linkage, llvm::Constant::getNullValue(llvmType), name, nullptr, tlsModel, 0);
	state.variableMapping[spirvVariable->getId()] = llvmVariable;
	llvmVariable->setAlignment(ALIGNMENT);
	return llvmVariable;
}

static llvm::Value* ConvertValue(State& state, SPIRV::SPIRVValue* spirvValue)
{
	const auto cachedType = state.valueMapping.find(spirvValue->getId());
	if (cachedType != state.valueMapping.end())
	{
		return cachedType->second;
	}

	llvm::Value* llvmValue;
	switch (spirvValue->getOpCode())
	{
	case OpVariable:
		{
			const auto variable = ConvertVariable(state, static_cast<SPIRV::SPIRVVariable*>(spirvValue));
			llvmValue = state.builder.CreateAlignedLoad(variable, ALIGNMENT);
			break;
		}

	case OpConstant:
		{
			const auto constant = static_cast<SPIRV::SPIRVConstant*>(spirvValue);
			if (constant->getType()->isTypeFloat(32))
			{
				llvmValue = llvm::ConstantFP::get(llvm::Type::getFloatTy(state.context), constant->getFloatValue());
			}
			else if (constant->getType()->isTypeFloat(64))
			{
				llvmValue = llvm::ConstantFP::get(llvm::Type::getDoubleTy(state.context), constant->getDoubleValue());
			}
			else
			{
				FATAL_ERROR();
			}
			break;
		}

	case OpConstantComposite:
		{
			const auto constantComposite = static_cast<SPIRV::SPIRVConstantComposite*>(spirvValue);
			if (constantComposite->getType()->isTypeVector())
			{
				auto elements = constantComposite->getElements();
				std::vector<llvm::Constant*> values(elements.size());
				for (auto i = 0u; i < elements.size(); i++)
				{
					values[i] = static_cast<llvm::Constant*>(ConvertValue(state, elements[i]));
				}
				llvmValue = llvm::ConstantVector::get(values);
			}
			else
			{
				FATAL_ERROR();
			}
			break;
		}
		
	default:
		FATAL_ERROR();
	}

	state.valueMapping[spirvValue->getId()] = llvmValue;
	return llvmValue;
}

static llvm::Function* GetInbuildFunction(State& state, SPIRV::SPIRVMatrixTimesVector* matrixTimesVector)
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

static llvm::Function* GetInbuildFunction(State& state, SPIRV::SPIRVMatrixTimesMatrix* matrixTimesMatrix)
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

static llvm::Function* GetInbuildFunction(State& state, SPIRV::SPIRVImageSampleExplicitLod* imageSampleExplicitLod, bool& isLod)
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
				llvmValue = state.builder.CreateAlignedLoad(ConvertValue(state, load->getSrc()), ALIGNMENT);
				break;
			}

		case OpStore:
			{
				const auto store = reinterpret_cast<SPIRV::SPIRVStore*>(instruction);
				llvmValue = state.builder.CreateAlignedStore(ConvertValue(state, store->getSrc()), ConvertValue(state, store->getDst()), ALIGNMENT);
				break;
			}

			// case OpCopyMemory: break;
			// case OpCopyMemorySized: break;
			 
		case OpAccessChain:
			{
				const auto accessChain = reinterpret_cast<SPIRV::SPIRVAccessChain*>(instruction);
				llvmValue = ConvertValue(state, accessChain->getBase());
				auto indices = accessChain->getIndices();

				for (auto spirvValue : indices)
				{
					const auto index = static_cast<SPIRV::SPIRVConstant*>(spirvValue)->getInt32Value();
					if (accessChain->isInBounds())
					{
						llvmValue = state.builder.CreateConstInBoundsGEP2_32(nullptr, llvmValue, 0, index);
					}
					else
					{
						llvmValue = state.builder.CreateConstGEP2_32(nullptr, llvmValue, 0, index);
					}
				}
				break;
			}

			// case OpInBoundsAccessChain: break;
			// case OpPtrAccessChain: break;
			// case OpArrayLength: break;
			// case OpGenericPtrMemSemantics: break;
			// case OpInBoundsPtrAccessChain: break;
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
				const auto compositeConstruct = reinterpret_cast<SPIRV::SPIRVCompositeConstruct*>(instruction);
				const auto type = compositeConstruct->getType();
				if (type->isTypeVector())
				{
					llvmValue = llvm::UndefValue::get(ConvertType(state, type));
					for (auto j = 0u; j < compositeConstruct->getConstituents().size(); j++)
					{
						llvmValue = state.builder.CreateInsertElement(llvmValue, ConvertValue(state, compositeConstruct->getConstituents()[j]), j);
					}
				}
				else
				{
					FATAL_ERROR();
				}
				
				break;
			}
			 
		case OpCompositeExtract:
			{
				const auto compositeExtract = reinterpret_cast<SPIRV::SPIRVCompositeExtract*>(instruction);
				llvmValue = ConvertValue(state, compositeExtract->getComposite());
				auto type = compositeExtract->getComposite()->getType();
				auto indices = compositeExtract->getIndices();

				for (auto index : indices)
				{
					if (type->isTypeVector())
					{
						llvmValue = state.builder.CreateExtractElement(llvmValue, index);
						type = type->getVectorComponentType();
					}
					else
					{
						FATAL_ERROR();
					}
				}
				
				break;
			}
			
			// case OpCompositeInsert: break;
			// case OpCopyObject: break;
			// case OpTranspose: break;
			// case OpSampledImage: break;
			// case OpImageSampleImplicitLod: break;

		case OpImageSampleExplicitLod:
			{
				const auto imageSampleExplicitLod = reinterpret_cast<SPIRV::SPIRVImageSampleExplicitLod*>(instruction);
				bool isLod;
				const auto function = GetInbuildFunction(state, imageSampleExplicitLod, isLod);
				
				if (isLod)
				{
					llvm::Value* args[4];
					args[1] = ConvertValue(state, imageSampleExplicitLod->getOpValue(0));
					args[2] = ConvertValue(state, imageSampleExplicitLod->getOpValue(1));
					args[3] = ConvertValue(state, imageSampleExplicitLod->getOpValue(3));

					const auto tmp0 = state.builder.CreateAlloca(ConvertType(state, imageSampleExplicitLod->getType()));
					const auto tmp2 = state.builder.CreateAlloca(args[2]->getType());
					state.builder.CreateStore(args[2], tmp2);
					args[0] = tmp0;
					args[2] = tmp2;

					state.builder.CreateCall(function, args);
					llvmValue = state.builder.CreateLoad(tmp0);
				}
				else
				{
					FATAL_ERROR();
				}
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

				const auto tmp0 = state.builder.CreateAlloca(args[2]->getType());
				const auto tmp1 = state.builder.CreateAlloca(args[1]->getType());
				const auto tmp2 = state.builder.CreateAlloca(args[2]->getType());
				state.builder.CreateStore(args[1], tmp1);
				state.builder.CreateStore(args[2], tmp2);
				
				args[0] = tmp0;
				args[1] = tmp1;
				args[2] = tmp2;

				state.builder.CreateCall(GetInbuildFunction(state, matrixTimesVector), args);
				llvmValue = state.builder.CreateLoad(tmp0);
				break;
			}
			 
		case OpMatrixTimesMatrix:
			{
				const auto matrixTimesMatrix = reinterpret_cast<SPIRV::SPIRVMatrixTimesMatrix*>(instruction);
				llvm::Value* args[3];
				args[1] = ConvertValue(state, matrixTimesMatrix->getMatrixLeft());
				args[2] = ConvertValue(state, matrixTimesMatrix->getMatrixRight());

				const auto tmp0 = state.builder.CreateAlloca(args[1]->getType());
				const auto tmp1 = state.builder.CreateAlloca(args[1]->getType());
				const auto tmp2 = state.builder.CreateAlloca(args[2]->getType());
				state.builder.CreateStore(args[1], tmp1);
				state.builder.CreateStore(args[2], tmp2);
				
				args[0] = tmp0;
				args[1] = tmp1;
				args[2] = tmp2;

				state.builder.CreateCall(GetInbuildFunction(state, matrixTimesMatrix), args);
				llvmValue = state.builder.CreateLoad(tmp0);
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
		inputMembers.push_back(llvm::Type::getInt32Ty(state.context));
		inputMembers.push_back(llvm::Type::getInt32Ty(state.context));
		
		outputMembers.push_back(llvm::VectorType::get(llvm::Type::getFloatTy(state.context), 4));
		outputMembers.push_back(llvm::Type::getFloatTy(state.context));
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
	llvm::IRBuilder<> builder(*context);
	auto llvmModule = std::make_unique<llvm::Module>("", *context);
	
	State state
	{
		*context,
		builder,
		llvmModule.get(),
	};
	
	const auto addressingModel = spirvModule->getAddressingModel();
	const auto memoryModel = spirvModule->getMemoryModel();

	AddBuiltin(state, executionModel);
	
	for (auto i = 0u; i < spirvModule->getNumVariables(); i++)
	{
		ConvertVariable(state, spirvModule->getVariable(i));
	}

	for (auto i = 0u; i < spirvModule->getNumFunctions(); i++)
	{
		ConvertFunction(state, spirvModule->getFunction(i));
	}

	// TODO: Optimisations

	std::string dump;
	llvm::raw_string_ostream DumpStream(dump);
	llvmModule->print(DumpStream, nullptr);
	std::cout << dump << std::endl;
	
	return std::move(llvmModule);
}
