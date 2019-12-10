#include "ImageCompiler.h"

#include "CompiledModule.h"
#include "CompiledModuleBuilder.h"

#include <Formats.h>
#include <Half.h>

#include <llvm-c/Target.h>

#include <type_traits>

namespace Intrinsics
{
	enum ID
	{
		not_intrinsic = 0,

		// Get the intrinsic enums generated from Intrinsics.td
#define GET_INTRINSIC_ENUM_VALUES
#include "llvm/IR/IntrinsicEnums.inc"
#undef GET_INTRINSIC_ENUM_VALUES
	};
}

template<int NumberValues>
static LLVMValueRef CallIntrinsic(LLVMBuilderRef builder, LLVMModuleRef module, Intrinsics::ID intrinsic, std::array<LLVMValueRef, NumberValues> values)
{
	std::array<LLVMTypeRef, NumberValues> parameterTypes;
	for (auto i = 0u; i < NumberValues; i++)
	{
		parameterTypes[i] = LLVMTypeOf(values[i]);
	}

	const auto declaration = LLVMGetIntrinsicDeclaration(module, intrinsic, parameterTypes.data(), parameterTypes.size());
	return LLVMBuildCall(builder, declaration, values.data(), static_cast<uint32_t>(values.size()), "");
}

class PixelCompiledModuleBuilder : public CompiledModuleBuilder
{
public:
	PixelCompiledModuleBuilder(CPJit* jit, const FormatInformation* information) :
		CompiledModuleBuilder{jit},
		information{information}
	{
	}

protected:
	const FormatInformation* information;

	LLVMValueRef EmitConvertFloat(LLVMValueRef inputValue, LLVMTypeRef outputType)
	{
		const auto inputBits = LLVMSizeOfTypeInBits(jit->getDataLayout(), LLVMTypeOf(inputValue));
		const auto outputBits = LLVMSizeOfTypeInBits(jit->getDataLayout(), outputType);

		if (inputBits < outputBits)
		{
			return LLVMBuildFPExt(builder, inputValue, outputType, "");
		}

		if (inputBits > outputBits)
		{
			return LLVMBuildFPTrunc(builder, inputValue, outputType, "");
		}

		return inputValue;
	}

	LLVMValueRef EmitConvertFloatInt(LLVMValueRef inputValue, int64_t maxValue, LLVMTypeRef outputType)
	{
		auto value = LLVMBuildFMul(builder, inputValue, LLVMConstReal(LLVMTypeOf(inputValue), static_cast<double>(maxValue)), "");
		value = CallIntrinsic<1>(builder, module, Intrinsics::round, {value});
		return LLVMBuildFPToSI(builder, value, outputType, "");
	}

	LLVMValueRef EmitConvertFloatUInt(LLVMValueRef inputValue, uint64_t maxValue, LLVMTypeRef outputType)
	{
		auto value = LLVMBuildFMul(builder, inputValue, LLVMConstReal(LLVMTypeOf(inputValue), static_cast<double>(maxValue)), "");
		value = CallIntrinsic<1>(builder, module, Intrinsics::round, {value});
		return LLVMBuildFPToUI(builder, value, outputType, "");
	}

	LLVMValueRef EmitConvertInt(LLVMValueRef inputValue, LLVMTypeRef outputType, bool inputSigned)
	{
		const auto inputBits = LLVMSizeOfTypeInBits(jit->getDataLayout(), LLVMTypeOf(inputValue));
		const auto outputBits = LLVMSizeOfTypeInBits(jit->getDataLayout(), outputType);

		if (inputBits < outputBits)
		{
			if (inputSigned)
			{
				return LLVMBuildSExt(builder, inputValue, outputType, "");
			}

			return LLVMBuildZExt(builder, inputValue, outputType, "");
		}

		if (inputBits > outputBits)
		{
			return LLVMBuildTrunc(builder, inputValue, outputType, "");
		}

		return inputValue;
	}

	LLVMValueRef EmitConvertIntFloat(LLVMValueRef inputValue, uint64_t maxValue, LLVMTypeRef outputType)
	{
		const auto value = LLVMBuildSIToFP(builder, inputValue, outputType, "");
		return LLVMBuildFDiv(builder, value, LLVMConstReal(outputType, static_cast<double>(maxValue)), "");
	}

	LLVMValueRef EmitConvertUIntFloat(LLVMValueRef inputValue, uint64_t maxValue, LLVMTypeRef outputType)
	{
		const auto value = LLVMBuildUIToFP(builder, inputValue, outputType, "");
		return LLVMBuildFDiv(builder, value, LLVMConstReal(outputType, static_cast<double>(maxValue)), "");
	}

	template<typename InputType, typename OutputType>
	LLVMValueRef EmitConvert(LLVMValueRef inputValue)
	{
		if constexpr (std::numeric_limits<InputType>::is_iec559)
		{
			if constexpr (std::numeric_limits<OutputType>::is_integer && !std::numeric_limits<OutputType>::is_signed)
			{
				return EmitConvertFloatUInt(inputValue, std::numeric_limits<OutputType>::max(), GetType<OutputType>());
			}
			else if constexpr (std::numeric_limits<OutputType>::is_integer)
			{
				return EmitConvertFloatInt(inputValue, std::numeric_limits<OutputType>::max(), GetType<OutputType>());
			}
			else
			{
				static_assert(std::numeric_limits<OutputType>::is_iec559);
				return EmitConvertFloat(inputValue, GetType<OutputType>());
			}
		}
		else if (std::is_unsigned<InputType>::value)
		{
			static_assert(std::numeric_limits<InputType>::is_integer);
			if constexpr (std::numeric_limits<OutputType>::is_integer)
			{
				return EmitConvertInt(inputValue, GetType<OutputType>(), false);
			}
			else
			{
				static_assert(std::numeric_limits<OutputType>::is_iec559);
				return EmitConvertUIntFloat(inputValue, std::numeric_limits<InputType>::max(), GetType<OutputType>());
			}
		}
		else
		{
			static_assert(std::numeric_limits<InputType>::is_integer);
			if constexpr (std::numeric_limits<OutputType>::is_integer)
			{
				return EmitConvertInt(inputValue, GetType<OutputType>(), true);
			}
			else
			{
				static_assert(std::numeric_limits<OutputType>::is_iec559);
				return EmitConvertIntFloat(inputValue, std::numeric_limits<InputType>::max(), GetType<OutputType>());
			}
		}
	}

	LLVMValueRef CreateMaxNum(LLVMValueRef lhs, LLVMValueRef rhs)
	{
		return CallIntrinsic<2>(builder, module, Intrinsics::maxnum, {
			                        lhs,
			                        rhs,
		                        });
	}

	LLVMValueRef CreateMinNum(LLVMValueRef lhs, LLVMValueRef rhs)
	{
		return CallIntrinsic<2>(builder, module, Intrinsics::minnum, {
			                        lhs,
			                        rhs,
		                        });
	}
};

// static uint32_t GetPackedBits(const FormatInformation* information, int index)
// {
// 	switch (index)
// 	{
// 	case 0:
// 		return information->Packed.RedBits;
// 	case 1:
// 		return information->Packed.GreenBits;
// 	case 2:
// 		return information->Packed.BlueBits;
// 	case 3:
// 		return information->Packed.AlphaBits;
// 	}
// 	FATAL_ERROR();
// }
//
// static uint32_t GetPackedOffset(const FormatInformation* information, int index)
// {
// 	switch (index)
// 	{
// 	case 0:
// 		return information->Packed.RedOffset;
// 	case 1:
// 		return information->Packed.GreenOffset;
// 	case 2:
// 		return information->Packed.BlueOffset;
// 	case 3:
// 		return information->Packed.AlphaOffset;
// 	}
// 	FATAL_ERROR();
// }

// static llvm::Value* EmitLinearToSRGB(llvm::Function* currentFunction, llvm::IRBuilder<>& builder, llvm::Value* inputValue)
// {
// 	const auto trueBlock = llvm::BasicBlock::Create(currentFunction->getContext(), "", currentFunction);
// 	const auto falseBlock = llvm::BasicBlock::Create(currentFunction->getContext(), "", currentFunction);
// 	const auto nextBlock = llvm::BasicBlock::Create(currentFunction->getContext(), "", currentFunction);
//
// 	const auto comparison = builder.CreateFCmpUGT(inputValue, llvm::ConstantFP::get(builder.getFloatTy(), 0.0031308));
// 	builder.CreateCondBr(comparison, trueBlock, falseBlock);
//
// 	builder.SetInsertPoint(falseBlock);
// 	const auto falseValue = builder.CreateFMul(inputValue, llvm::ConstantFP::get(builder.getFloatTy(), 12.92));
// 	builder.CreateBr(nextBlock);
//
// 	builder.SetInsertPoint(trueBlock);
// 	llvm::Value* trueValue = builder.CreateBinaryIntrinsic(llvm::Intrinsic::pow, inputValue, llvm::ConstantFP::get(builder.getFloatTy(), 1.0 / 2.4));
// 	trueValue = builder.CreateFMul(trueValue, llvm::ConstantFP::get(builder.getFloatTy(), 1.055));
// 	trueValue = builder.CreateFAdd(trueValue, llvm::ConstantFP::get(builder.getFloatTy(), -0.055));
// 	builder.CreateBr(nextBlock);
//
// 	builder.SetInsertPoint(nextBlock);
// 	auto value = builder.CreatePHI(builder.getFloatTy(), 2);
// 	value->addIncoming(trueValue, trueBlock);
// 	value->addIncoming(falseValue, falseBlock);
// 	return value;
// }
//
// static llvm::Value* EmitSRGBToLinear(llvm::Function* currentFunction, llvm::IRBuilder<>& builder, llvm::Value* inputValue)
// {
// 	const auto trueBlock = llvm::BasicBlock::Create(currentFunction->getContext(), "", currentFunction);
// 	const auto falseBlock = llvm::BasicBlock::Create(currentFunction->getContext(), "", currentFunction);
// 	const auto nextBlock = llvm::BasicBlock::Create(currentFunction->getContext(), "", currentFunction);
// 	
// 	const auto comparison = builder.CreateFCmpUGT(inputValue, llvm::ConstantFP::get(builder.getFloatTy(), 0.04045));
// 	builder.CreateCondBr(comparison, trueBlock, falseBlock);
// 	
// 	builder.SetInsertPoint(falseBlock);
// 	const auto falseValue = builder.CreateFDiv(inputValue, llvm::ConstantFP::get(builder.getFloatTy(), 12.92));
// 	builder.CreateBr(nextBlock);
// 	
// 	builder.SetInsertPoint(trueBlock);
// 	auto trueValue = builder.CreateFAdd(inputValue, llvm::ConstantFP::get(builder.getFloatTy(), 0.055));
// 	trueValue = builder.CreateFDiv(trueValue, llvm::ConstantFP::get(builder.getFloatTy(), 1.055));
// 	trueValue = builder.CreateBinaryIntrinsic(llvm::Intrinsic::pow, trueValue, llvm::ConstantFP::get(builder.getFloatTy(), 2.4));
// 	builder.CreateBr(nextBlock);
// 	
// 	builder.SetInsertPoint(nextBlock);
// 	auto value = builder.CreatePHI(builder.getFloatTy(), 2);
// 	value->addIncoming(trueValue, trueBlock);
// 	value->addIncoming(falseValue, falseBlock);
// 	return value;
// }

class GetDepthPixelCompiledModuleBuilder final : public PixelCompiledModuleBuilder
{
public:
	GetDepthPixelCompiledModuleBuilder(CPJit* jit, const FormatInformation* information) :
		PixelCompiledModuleBuilder{jit, information}
	{
		assert(information->Type == FormatType::DepthStencil);
		assert(information->DepthStencil.DepthOffset != INVALID_OFFSET);
	}

protected:
	void MainCompilation() override
	{
		std::vector<LLVMTypeRef> parameters
		{
			LLVMPointerType(LLVMInt8TypeInContext(context), 0),
		};
		const auto functionType = LLVMFunctionType(LLVMFloatTypeInContext(context), parameters.data(), static_cast<uint32_t>(parameters.size()), false);
		const auto function = LLVMAddFunction(module, "main", functionType);
		LLVMSetLinkage(function, LLVMExternalLinkage);

		const auto basicBlock = LLVMAppendBasicBlockInContext(context, function, "");
		LLVMPositionBuilderAtEnd(builder, basicBlock);

		auto sourcePtr = LLVMGetParam(function, 0);

		LLVMValueRef value{};
		switch (information->Format)
		{
		case VK_FORMAT_D16_UNORM:
		case VK_FORMAT_D16_UNORM_S8_UINT:
			sourcePtr = LLVMBuildBitCast(builder, sourcePtr, LLVMPointerType(LLVMInt16TypeInContext(context), 0), "");
			value = CreateLoad(sourcePtr);
			value = EmitConvert<uint16_t, float>(value);
			break;
			
		case VK_FORMAT_D24_UNORM_S8_UINT:
		case VK_FORMAT_X8_D24_UNORM_PACK32:
			sourcePtr = LLVMBuildBitCast(builder, sourcePtr, LLVMPointerType(LLVMInt32TypeInContext(context), 0), "");
			value = CreateLoad(sourcePtr);
			value = LLVMBuildAnd(builder, value, ConstU32(0x00FFFFFF), "");
			value = LLVMBuildUIToFP(builder, value, LLVMFloatTypeInContext(context), "");
			value = LLVMBuildFDiv(builder, value, ConstF32(0x00FFFFFF), "");
			break;
			
		case VK_FORMAT_D32_SFLOAT:
		case VK_FORMAT_D32_SFLOAT_S8_UINT:
			sourcePtr = LLVMBuildBitCast(builder, sourcePtr, LLVMPointerType(LLVMFloatTypeInContext(context), 0), "");
			value = CreateLoad(sourcePtr);
			break;
			
		default:
			FATAL_ERROR();
		}

		LLVMBuildRet(builder, value);
	}
};

CP_DLL_EXPORT FunctionPointer CompileGetPixelDepth(CPJit* jit, const FormatInformation* information)
{
	return GetDepthPixelCompiledModuleBuilder(jit, information).Compile()->getFunctionPointer("main");
}

CP_DLL_EXPORT FunctionPointer CompileGetPixelStencil(CPJit* jit, const FormatInformation* information)
{
	// assert(information->Type == FormatType::DepthStencil);
	// assert(information->DepthStencil.StencilOffset != INVALID_OFFSET);
	//
	// auto context = std::make_unique<llvm::LLVMContext>();
	// llvm::IRBuilder<> builder(*context);
	// auto module = std::make_unique<llvm::Module>("", *context);
	//
	// const auto functionType = llvm::FunctionType::get(builder.getInt8Ty(), {
	// 	                                                  builder.getInt8PtrTy()
	//                                                   }, false);
	// const auto function = llvm::Function::Create(static_cast<llvm::FunctionType*>(functionType),
	//                                              llvm::GlobalVariable::ExternalLinkage,
	//                                              "main",
	//                                              module.get());
	//
	// const auto basicBlock = llvm::BasicBlock::Create(*context, "", function);
	// builder.SetInsertPoint(basicBlock);
	//
	// llvm::Value* sourcePtr = &*function->arg_begin();
	// sourcePtr = builder.CreateConstGEP1_32(sourcePtr, information->DepthStencil.StencilOffset);
	// const auto value = builder.CreateLoad(sourcePtr);
	//
	// builder.CreateRet(value);
	//
	// // TODO: Optimise
	//
	// Dump(module.get());
	//
	// const auto compiledModule = jit->CompileModule(std::move(context), std::move(module));
	// return jit->getFunctionPointer(compiledModule, "main");
	FATAL_ERROR();
}

template<typename ReturnType>
class GetPixelCompiledModuleBuilder final : public PixelCompiledModuleBuilder
{
public:
	GetPixelCompiledModuleBuilder(CPJit* jit, const FormatInformation* information) :
		PixelCompiledModuleBuilder{jit, information}
	{
	}

protected:
	void MainCompilation() override
	{
		std::vector<LLVMTypeRef> parameters
		{
			LLVMPointerType(LLVMInt8TypeInContext(context), 0),
			LLVMPointerType(GetType<ReturnType>(), 0),
		};
		if (information->Type == FormatType::Compressed)
		{
			parameters.push_back(LLVMInt32TypeInContext(context));
			parameters.push_back(LLVMInt32TypeInContext(context));
		}
		const auto functionType = LLVMFunctionType(LLVMVoidType(), parameters.data(), static_cast<uint32_t>(parameters.size()), false);
		const auto function = LLVMAddFunction(module, "main", functionType);
		LLVMSetLinkage(function, LLVMExternalLinkage);

		const auto basicBlock = LLVMAppendBasicBlockInContext(context, function, "");
		LLVMPositionBuilderAtEnd(builder, basicBlock);

		const auto sourcePtr = LLVMGetParam(function, 0);
		const auto destinationPtr = LLVMGetParam(function, 1);
		
		switch (information->Type)
		{
		case FormatType::Normal:
			CompileGetNormal(sourcePtr, destinationPtr);
			break;
		
		case FormatType::Packed:
			// {
			// 	switch (information->Base)
			// 	{
			// 	case BaseType::UNorm:
			// 	case BaseType::SNorm:
			// 		{
			// 			llvm::Type* sourceType;
			// 			switch (information->TotalSize)
			// 			{
			// 			case 1:
			// 				sourceType = builder.getInt8Ty();
			// 				break;
			//
			// 			case 2:
			// 				sourceType = builder.getInt16Ty();
			// 				break;
			//
			// 			case 4:
			// 				sourceType = builder.getInt32Ty();
			// 				break;
			//
			// 			default:
			// 				FATAL_ERROR();
			// 			}
			//
			// 			sourcePtr = builder.CreateBitCast(sourcePtr, llvm::PointerType::get(sourceType, 0));
			// 			const auto source = builder.CreateLoad(sourcePtr);
			//
			// 			const auto process = [information, destinationPtr, source](llvm::IRBuilder<>& builder, int index)
			// 			{
			// 				const auto bits = GetPackedBits(information, index);
			// 				const auto mask = (1ULL << bits) - 1;
			// 				const auto offset = GetPackedOffset(information, index);
			//
			// 				// Shift and mask value
			// 				auto value = builder.CreateLShr(source, llvm::ConstantInt::get(source->getType(), offset));
			// 				value = builder.CreateAnd(value, llvm::ConstantInt::get(source->getType(), mask));
			//
			// 				// Convert to float
			// 				if (information->Base == BaseType::SNorm)
			// 				{
			// 					value = builder.CreateSIToFP(value, builder.getFloatTy());
			// 					value = builder.CreateFDiv(value, llvm::ConstantFP::get(builder.getFloatTy(), mask >> 1));
			// 				}
			// 				else
			// 				{
			// 					value = builder.CreateUIToFP(value, builder.getFloatTy());
			// 					value = builder.CreateFDiv(value, llvm::ConstantFP::get(builder.getFloatTy(), mask));
			// 				}
			//
			// 				const auto dst = builder.CreateConstGEP1_32(destinationPtr, index);
			// 				builder.CreateStore(value, dst);
			// 			};
			//
			// 			if (information->Packed.RedBits)
			// 			{
			// 				process(builder, 0);
			// 			}
			// 			else
			// 			{
			// 				const auto dst = builder.CreateConstGEP1_32(destinationPtr, 0);
			// 				builder.CreateStore(llvm::ConstantFP::get(builder.getFloatTy(), 0), dst);
			// 			}
			//
			// 			if (information->Packed.GreenBits)
			// 			{
			// 				process(builder, 1);
			// 			}
			// 			else
			// 			{
			// 				const auto dst = builder.CreateConstGEP1_32(destinationPtr, 1);
			// 				builder.CreateStore(llvm::ConstantFP::get(builder.getFloatTy(), 0), dst);
			// 			}
			//
			// 			if (information->Packed.BlueBits)
			// 			{
			// 				process(builder, 2);
			// 			}
			// 			else
			// 			{
			// 				const auto dst = builder.CreateConstGEP1_32(destinationPtr, 2);
			// 				builder.CreateStore(llvm::ConstantFP::get(builder.getFloatTy(), 0), dst);
			// 			}
			//
			// 			if (information->Packed.AlphaBits)
			// 			{
			// 				process(builder, 3);
			// 			}
			// 			else
			// 			{
			// 				const auto dst = builder.CreateConstGEP1_32(destinationPtr, 3);
			// 				builder.CreateStore(llvm::ConstantFP::get(builder.getFloatTy(), 1), dst);
			// 			}
			// 			break;
			// 		}
			// 		
			// 		// case BaseType::UScaled: break;
			// 		// case BaseType::SScaled: break;
			// 		// case BaseType::UInt: break;
			// 		// case BaseType::SInt: break;
			// 		 
			// 	case BaseType::UFloat:
			// 		{
			// 			sourcePtr = builder.CreateBitCast(sourcePtr, llvm::PointerType::get(builder.getInt32Ty(), 0));
			// 			const auto source = builder.CreateLoad(sourcePtr);
			// 			switch (information->Format)
			// 			{
			// 			case VK_FORMAT_B10G11R11_UFLOAT_PACK32:
			// 				{
			// 					const auto b10g11r11Function = llvm::Function::Create(llvm::FunctionType::get(builder.getVoidTy(), {source->getType(), destinationPtr->getType()}, false), llvm::GlobalValue::ExternalLinkage, "B10G11R11ToFloat", module.get());
			// 					builder.CreateCall(b10g11r11Function, {source, destinationPtr});
			// 					break;
			// 				}
			// 	
			// 			case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32:
			// 				{
			// 					const auto e5b9g9r9Function = llvm::Function::Create(llvm::FunctionType::get(builder.getVoidTy(), {source->getType(), destinationPtr->getType()}, false), llvm::GlobalValue::ExternalLinkage, "E5B9G9R9ToFloat", module.get());
			// 					builder.CreateCall(e5b9g9r9Function, {source, destinationPtr});
			// 					break;
			// 				}
			//
			// 			default:
			// 				FATAL_ERROR();
			// 			}
			// 			break;
			// 		}
			// 		
			// 		// case BaseType::SFloat: break;
			// 		
			// 	case BaseType::SRGB:
			// 		{
			// 			llvm::Type* sourceType;
			// 			switch (information->TotalSize)
			// 			{
			// 			case 1:
			// 				sourceType = builder.getInt8Ty();
			// 				break;
			//
			// 			case 2:
			// 				sourceType = builder.getInt16Ty();
			// 				break;
			//
			// 			case 4:
			// 				sourceType = builder.getInt32Ty();
			// 				break;
			//
			// 			default:
			// 				FATAL_ERROR();
			// 			}
			//
			// 			sourcePtr = builder.CreateBitCast(sourcePtr, llvm::PointerType::get(sourceType, 0));
			// 			const auto source = builder.CreateLoad(sourcePtr);
			//
			// 			const auto process = [information, function, destinationPtr, source](llvm::IRBuilder<>& builder, int index)
			// 			{
			// 				const auto bits = GetPackedBits(information, index);
			// 				const auto mask = (1ULL << bits) - 1;
			// 				const auto offset = GetPackedOffset(information, index);
			//
			// 				// Shift and mask value
			// 				auto value = builder.CreateLShr(source, llvm::ConstantInt::get(source->getType(), offset));
			// 				value = builder.CreateAnd(value, llvm::ConstantInt::get(source->getType(), mask));
			//
			// 				// Convert to float
			// 				value = builder.CreateUIToFP(value, builder.getFloatTy());
			// 				value = builder.CreateFDiv(value, llvm::ConstantFP::get(builder.getFloatTy(), mask));
			//
			// 				if (index != 3)
			// 				{
			// 					value = EmitSRGBToLinear(function, builder, value);
			// 				}
			//
			// 				const auto dst = builder.CreateConstGEP1_32(destinationPtr, index);
			// 				builder.CreateStore(value, dst);
			// 			};
			//
			// 			if (information->Packed.RedBits)
			// 			{
			// 				process(builder, 0);
			// 			}
			// 			else
			// 			{
			// 				const auto dst = builder.CreateConstGEP1_32(destinationPtr, 0);
			// 				builder.CreateStore(llvm::ConstantFP::get(builder.getFloatTy(), 0), dst);
			// 			}
			//
			// 			if (information->Packed.GreenBits)
			// 			{
			// 				process(builder, 1);
			// 			}
			// 			else
			// 			{
			// 				const auto dst = builder.CreateConstGEP1_32(destinationPtr, 1);
			// 				builder.CreateStore(llvm::ConstantFP::get(builder.getFloatTy(), 0), dst);
			// 			}
			//
			// 			if (information->Packed.BlueBits)
			// 			{
			// 				process(builder, 2);
			// 			}
			// 			else
			// 			{
			// 				const auto dst = builder.CreateConstGEP1_32(destinationPtr, 2);
			// 				builder.CreateStore(llvm::ConstantFP::get(builder.getFloatTy(), 0), dst);
			// 			}
			//
			// 			if (information->Packed.AlphaBits)
			// 			{
			// 				process(builder, 3);
			// 			}
			// 			else
			// 			{
			// 				const auto dst = builder.CreateConstGEP1_32(destinationPtr, 3);
			// 				builder.CreateStore(llvm::ConstantFP::get(builder.getFloatTy(), 1), dst);
			// 			}
			// 			break;
			// 		}
			// 		 
			// 	default: FATAL_ERROR();
			// 	}
			//
			// 	break;
			// }
			FATAL_ERROR();
			// 	{
			// 		switch (information->Base)
			// 		{
			// 			// case BaseType::UNorm: break;
			// 			// case BaseType::SNorm: break;
			// 			// case BaseType::UScaled: break;
			// 		case BaseType::SScaled: break;
			// 			// case BaseType::UInt: break;
			// 		case BaseType::SInt: break;
			// 			// case BaseType::UFloat: break;
			// 			// case BaseType::SFloat: break;
			// 			// case BaseType::SRGB: break;
			// 		default: FATAL_ERROR();
			// 		}
			//
			// 		llvm::Type* sourceType;
			// 		switch (information->TotalSize)
			// 		{
			// 		case 1:
			// 			sourceType = builder.getInt8Ty();
			// 			break;
			//
			// 		case 2:
			// 			sourceType = builder.getInt16Ty();
			// 			break;
			//
			// 		case 4:
			// 			sourceType = builder.getInt32Ty();
			// 			break;
			//
			// 		default:
			// 			FATAL_ERROR();
			// 		}
			//
			// 		sourcePtr = builder.CreateBitCast(sourcePtr, llvm::PointerType::get(sourceType, 0));
			// 		const auto source = builder.CreateLoad(sourcePtr);
			// 		
			// 		const auto process = [information, destinationPtr, source](llvm::IRBuilder<>& builder, int index)
			// 		{
			// 			const auto bits = GetPackedBits(information, index);
			// 			const auto mask = (1ULL << bits) - 1;
			// 			const auto offset = GetPackedOffset(information, index);
			//
			// 			// Shift and mask value
			// 			auto value = builder.CreateLShr(source, llvm::ConstantInt::get(source->getType(), offset));
			// 			value = builder.CreateAnd(value, llvm::ConstantInt::get(source->getType(), mask));
			// 			value = builder.CreateSExt(value, destinationPtr->getType()->getPointerElementType());
			// 			
			// 			const auto dst = builder.CreateConstGEP1_32(destinationPtr, index);
			// 			builder.CreateStore(value, dst);
			// 		};
			//
			// 		if (information->Packed.RedBits)
			// 		{
			// 			process(builder, 0);
			// 		}
			// 		else
			// 		{
			// 			const auto dst = builder.CreateConstGEP1_32(destinationPtr, 0);
			// 			builder.CreateStore(builder.getInt32(0), dst);
			// 		}
			//
			// 		if (information->Packed.GreenBits)
			// 		{
			// 			process(builder, 1);
			// 		}
			// 		else
			// 		{
			// 			const auto dst = builder.CreateConstGEP1_32(destinationPtr, 1);
			// 			builder.CreateStore(builder.getInt32(0), dst);
			// 		}
			//
			// 		if (information->Packed.BlueBits)
			// 		{
			// 			process(builder, 2);
			// 		}
			// 		else
			// 		{
			// 			const auto dst = builder.CreateConstGEP1_32(destinationPtr, 2);
			// 			builder.CreateStore(builder.getInt32(0), dst);
			// 		}
			//
			// 		if (information->Packed.AlphaBits)
			// 		{
			// 			process(builder, 3);
			// 		}
			// 		else
			// 		{
			// 			const auto dst = builder.CreateConstGEP1_32(destinationPtr, 3);
			// 			builder.CreateStore(builder.getInt32(0), dst);
			// 		}
			//
			// 		break;
			// 	}
			FATAL_ERROR();
			// 	{
			// 		switch (information->Base)
			// 		{
			// 			// case BaseType::UNorm: break;
			// 			// case BaseType::SNorm: break;
			// 		case BaseType::UScaled: break;
			// 			// case BaseType::SScaled: break;
			// 		case BaseType::UInt: break;
			// 			// case BaseType::SInt: break;
			// 			// case BaseType::UFloat: break;
			// 			// case BaseType::SFloat: break;
			// 			// case BaseType::SRGB: break;
			// 		default: FATAL_ERROR();
			// 		}
			// 		
			// 		llvm::Type* sourceType;
			// 		switch (information->TotalSize)
			// 		{
			// 		case 1:
			// 			sourceType = builder.getInt8Ty();
			// 			break;
			//
			// 		case 2:
			// 			sourceType = builder.getInt16Ty();
			// 			break;
			//
			// 		case 4:
			// 			sourceType = builder.getInt32Ty();
			// 			break;
			//
			// 		default:
			// 			FATAL_ERROR();
			// 		}
			//
			// 		sourcePtr = builder.CreateBitCast(sourcePtr, llvm::PointerType::get(sourceType, 0));
			// 		const auto source = builder.CreateLoad(sourcePtr);
			// 		
			// 		const auto process = [information, destinationPtr, source](llvm::IRBuilder<>& builder, int index)
			// 		{
			// 			const auto bits = GetPackedBits(information, index);
			// 			const auto mask = (1ULL << bits) - 1;
			// 			const auto offset = GetPackedOffset(information, index);
			//
			// 			// Shift and mask value
			// 			auto value = builder.CreateLShr(source, llvm::ConstantInt::get(source->getType(), offset));
			// 			value = builder.CreateAnd(value, llvm::ConstantInt::get(source->getType(), mask));
			// 			
			// 			const auto dst = builder.CreateConstGEP1_32(destinationPtr, index);
			// 			builder.CreateStore(value, dst);
			// 		};
			//
			// 		if (information->Packed.RedBits)
			// 		{
			// 			process(builder, 0);
			// 		}
			// 		else
			// 		{
			// 			const auto dst = builder.CreateConstGEP1_32(destinationPtr, 0);
			// 			builder.CreateStore(builder.getInt32(0), dst);
			// 		}
			//
			// 		if (information->Packed.GreenBits)
			// 		{
			// 			process(builder, 1);
			// 		}
			// 		else
			// 		{
			// 			const auto dst = builder.CreateConstGEP1_32(destinationPtr, 1);
			// 			builder.CreateStore(builder.getInt32(0), dst);
			// 		}
			//
			// 		if (information->Packed.BlueBits)
			// 		{
			// 			process(builder, 2);
			// 		}
			// 		else
			// 		{
			// 			const auto dst = builder.CreateConstGEP1_32(destinationPtr, 2);
			// 			builder.CreateStore(builder.getInt32(0), dst);
			// 		}
			//
			// 		if (information->Packed.AlphaBits)
			// 		{
			// 			process(builder, 3);
			// 		}
			// 		else
			// 		{
			// 			const auto dst = builder.CreateConstGEP1_32(destinationPtr, 3);
			// 			builder.CreateStore(builder.getInt32(0), dst);
			// 		}
			//
			// 		break;
			// 	}
			FATAL_ERROR();
		
		case FormatType::DepthStencil:
			// {
			// 	llvm::Value* value;
			// 	switch (information->Format)
			// 	{
			// 	case VK_FORMAT_D16_UNORM:
			// 	case VK_FORMAT_D16_UNORM_S8_UINT:
			// 		sourcePtr = builder.CreateBitCast(sourcePtr, llvm::PointerType::get(builder.getInt16Ty(), 0));
			// 		value = builder.CreateLoad(sourcePtr);
			// 		value = EmitConvert<uint16_t, float>(builder, value);
			// 		break;
			//
			// 	case VK_FORMAT_D24_UNORM_S8_UINT:
			// 	case VK_FORMAT_X8_D24_UNORM_PACK32:
			// 		sourcePtr = builder.CreateBitCast(sourcePtr, llvm::PointerType::get(builder.getInt32Ty(), 0));
			// 		value = builder.CreateLoad(sourcePtr);
			// 		value = builder.CreateAnd(value, builder.getInt32(0x00FFFFFF));
			// 		value = builder.CreateUIToFP(value, builder.getFloatTy());
			// 		value = builder.CreateFDiv(value, llvm::ConstantFP::get(builder.getFloatTy(), 0x00FFFFFF));
			// 		break;
			//
			// 	case VK_FORMAT_D32_SFLOAT:
			// 	case VK_FORMAT_D32_SFLOAT_S8_UINT:
			// 		sourcePtr = builder.CreateBitCast(sourcePtr, llvm::PointerType::get(builder.getFloatTy(), 0));
			// 		value = builder.CreateLoad(sourcePtr);
			// 		break;
			//
			// 	default:
			// 		FATAL_ERROR();
			// 	}
			// 	
			// 	auto dst = builder.CreateConstGEP1_32(destinationPtr, 0);
			// 	builder.CreateStore(value, dst);
			//
			// 	dst = builder.CreateConstGEP1_32(destinationPtr, 1);
			// 	builder.CreateStore(llvm::ConstantFP::get(builder.getFloatTy(), 0), dst);
			//
			// 	dst = builder.CreateConstGEP1_32(destinationPtr, 2);
			// 	builder.CreateStore(llvm::ConstantFP::get(builder.getFloatTy(), 0), dst);
			//
			// 	dst = builder.CreateConstGEP1_32(destinationPtr, 3);
			// 	builder.CreateStore(llvm::ConstantFP::get(builder.getFloatTy(), 1), dst);
			// 	
			// 	break;
			// }
			FATAL_ERROR();
			
		case FormatType::Compressed:
			// {
			// 	const auto subX = &*(function->arg_begin() + 2);
			// 	const auto subY = &*(function->arg_begin() + 3);
			//
			// 	// The type of the block
			// 	llvm::Type* sourceType;
			// 	switch (information->TotalSize)
			// 	{
			// 	case 4:
			// 		sourceType = builder.getInt32Ty();
			// 		break;
			//
			// 	case 8:
			// 		sourceType = builder.getInt64Ty();
			// 		break;
			//
			// 	case 16:
			// 		sourceType = builder.getInt128Ty();
			// 		break;
			//
			// 	default:
			// 		FATAL_ERROR();
			// 	}
			//
			// 	sourcePtr = builder.CreateBitCast(sourcePtr, llvm::PointerType::get(sourceType, 0));
			// 	const auto source = builder.CreateLoad(sourcePtr);
			// 	const auto pixels = builder.CreateAlloca(builder.getFloatTy(), builder.getInt32(4 * information->Compressed.BlockWidth * information->Compressed.BlockHeight));
			//
			// 	// Get function that handles decompression
			// 	llvm::Function* decompress;
			// 	switch (information->Format)
			// 	{
			// 	case VK_FORMAT_BC1_RGB_UNORM_BLOCK:
			// 	case VK_FORMAT_BC1_RGB_SRGB_BLOCK:
			// 	case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:
			// 	case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:
			// 	case VK_FORMAT_BC2_UNORM_BLOCK:
			// 	case VK_FORMAT_BC2_SRGB_BLOCK:
			// 	case VK_FORMAT_BC3_UNORM_BLOCK:
			// 	case VK_FORMAT_BC3_SRGB_BLOCK:
			// 	case VK_FORMAT_BC4_UNORM_BLOCK:
			// 	case VK_FORMAT_BC4_SNORM_BLOCK:
			// 	case VK_FORMAT_BC5_UNORM_BLOCK:
			// 	case VK_FORMAT_BC5_SNORM_BLOCK:
			// 	case VK_FORMAT_BC6H_UFLOAT_BLOCK:
			// 	case VK_FORMAT_BC6H_SFLOAT_BLOCK:
			// 	case VK_FORMAT_BC7_UNORM_BLOCK:
			// 	case VK_FORMAT_BC7_SRGB_BLOCK:
			// 		FATAL_ERROR();
			// 		
			// 	case VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK:
			// 	case VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK:
			// 		decompress = llvm::Function::Create(
			// 			llvm::FunctionType::get(builder.getVoidTy(), {source->getType(), pixels->getType()}, false), 
			// 			llvm::GlobalValue::ExternalLinkage, "DecompressETC2R8G8B8", module.get());
			// 		break;
			// 		
			// 	case VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK:
			// 	case VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK:
			// 	case VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK:
			// 	case VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK:
			// 		FATAL_ERROR();
			// 		
			// 	case VK_FORMAT_EAC_R11_UNORM_BLOCK:
			// 	case VK_FORMAT_EAC_R11_SNORM_BLOCK:
			// 	case VK_FORMAT_EAC_R11G11_UNORM_BLOCK:
			// 	case VK_FORMAT_EAC_R11G11_SNORM_BLOCK:
			// 		FATAL_ERROR();
			// 		
			// 	case VK_FORMAT_ASTC_4x4_UNORM_BLOCK:
			// 	case VK_FORMAT_ASTC_4x4_SRGB_BLOCK:
			// 	case VK_FORMAT_ASTC_5x4_UNORM_BLOCK:
			// 	case VK_FORMAT_ASTC_5x4_SRGB_BLOCK:
			// 	case VK_FORMAT_ASTC_5x5_UNORM_BLOCK:
			// 	case VK_FORMAT_ASTC_5x5_SRGB_BLOCK:
			// 	case VK_FORMAT_ASTC_6x5_UNORM_BLOCK:
			// 	case VK_FORMAT_ASTC_6x5_SRGB_BLOCK:
			// 	case VK_FORMAT_ASTC_6x6_UNORM_BLOCK:
			// 	case VK_FORMAT_ASTC_6x6_SRGB_BLOCK:
			// 	case VK_FORMAT_ASTC_8x5_UNORM_BLOCK:
			// 	case VK_FORMAT_ASTC_8x5_SRGB_BLOCK:
			// 	case VK_FORMAT_ASTC_8x6_UNORM_BLOCK:
			// 	case VK_FORMAT_ASTC_8x6_SRGB_BLOCK:
			// 	case VK_FORMAT_ASTC_8x8_UNORM_BLOCK:
			// 	case VK_FORMAT_ASTC_8x8_SRGB_BLOCK:
			// 	case VK_FORMAT_ASTC_10x5_UNORM_BLOCK:
			// 	case VK_FORMAT_ASTC_10x5_SRGB_BLOCK:
			// 	case VK_FORMAT_ASTC_10x6_UNORM_BLOCK:
			// 	case VK_FORMAT_ASTC_10x6_SRGB_BLOCK:
			// 	case VK_FORMAT_ASTC_10x8_UNORM_BLOCK:
			// 	case VK_FORMAT_ASTC_10x8_SRGB_BLOCK:
			// 	case VK_FORMAT_ASTC_10x10_UNORM_BLOCK:
			// 	case VK_FORMAT_ASTC_10x10_SRGB_BLOCK:
			// 	case VK_FORMAT_ASTC_12x10_UNORM_BLOCK:
			// 	case VK_FORMAT_ASTC_12x10_SRGB_BLOCK:
			// 	case VK_FORMAT_ASTC_12x12_UNORM_BLOCK:
			// 	case VK_FORMAT_ASTC_12x12_SRGB_BLOCK:
			// 		FATAL_ERROR();
			// 		
			// 	case VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG:
			// 	case VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG:
			// 	case VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG:
			// 	case VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG:
			// 	case VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG:
			// 	case VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG:
			// 	case VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG:
			// 	case VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG:
			// 		FATAL_ERROR();
			// 		
			// 	default:
			// 		FATAL_ERROR();
			// 	}
			//
			// 	builder.CreateCall(decompress, {source, pixels});
			//
			// 	// Get the offset to the pixels we need
			// 	auto offset = builder.CreateMul(subY, builder.getInt32(information->Compressed.BlockWidth));
			// 	offset = builder.CreateAdd(offset, subX);
			// 	offset = builder.CreateMul(offset, builder.getInt32(4));
			//
			// 	// Set R channel
			// 	auto dst = builder.CreateConstGEP1_32(destinationPtr, 0);
			// 	llvm::Value* value = builder.CreateLoad(builder.CreateGEP(pixels, offset));
			// 	if (information->Base == BaseType::SRGB)
			// 	{
			// 		value = EmitSRGBToLinear(function, builder, value);
			// 	}
			// 	builder.CreateStore(value, dst);
			//
			// 	// Set G channel
			// 	dst = builder.CreateConstGEP1_32(destinationPtr, 1);
			// 	value = builder.CreateLoad(builder.CreateGEP(pixels, builder.CreateAdd(offset, builder.getInt32(1))));
			// 	if (information->Base == BaseType::SRGB)
			// 	{
			// 		value = EmitSRGBToLinear(function, builder, value);
			// 	}
			// 	builder.CreateStore(value, dst);
			//
			// 	// Set B channel
			// 	dst = builder.CreateConstGEP1_32(destinationPtr, 2);
			// 	value = builder.CreateLoad(builder.CreateGEP(pixels, builder.CreateAdd(offset, builder.getInt32(2))));
			// 	if (information->Base == BaseType::SRGB)
			// 	{
			// 		value = EmitSRGBToLinear(function, builder, value);
			// 	}
			// 	builder.CreateStore(value, dst);
			//
			// 	// Set A channel
			// 	dst = builder.CreateConstGEP1_32(destinationPtr, 3);
			// 	value = builder.CreateLoad(builder.CreateGEP(pixels, builder.CreateAdd(offset, builder.getInt32(3))));
			// 	builder.CreateStore(value, dst);
			// 	
			// 	break;
			// }
			FATAL_ERROR();
			
		case FormatType::Planar:
			FATAL_ERROR();
			
		case FormatType::PlanarSamplable:
			FATAL_ERROR();
			
		default:
			FATAL_ERROR();
		}

		LLVMBuildRetVoid(builder);
	}

private:
	LLVMValueRef Const(ReturnType value)
	{
		if constexpr (std::is_same<ReturnType, float>::value)
		{
			return ConstF32(value);
		}
		else if constexpr (std::is_same<ReturnType, int32_t>::value)
		{
			return ConstI32(value);
		}
		else if constexpr (std::is_same<ReturnType, uint32_t>::value)
		{
			return ConstU32(value);
		}
		else
		{
			static_assert(false);
		}
	}
	
	void CompileGetNormal(LLVMValueRef sourcePtr, LLVMValueRef destinationPtr)
	{
		LLVMTypeRef sourceType{};
		switch (information->Base)
		{
		case BaseType::UFloat:
			FATAL_ERROR();
			
		case BaseType::SFloat:
			switch (information->ElementSize)
			{
			case 2:
				sourceType = LLVMHalfTypeInContext(context);
				break;

			case 4:
				sourceType = LLVMFloatTypeInContext(context);
				break;

			case 8:
				sourceType = LLVMDoubleTypeInContext(context);
				break;

			default:
				FATAL_ERROR();
			}
			break;

		default:
			sourceType = LLVMIntTypeInContext(context, information->ElementSize * 8);
			break;
		}

		sourcePtr = LLVMBuildBitCast(builder, sourcePtr, LLVMPointerType(sourceType, 0), "");

		for (auto i = 0u; i < 4; i++)
		{
			const auto dst = CreateGEP(destinationPtr, i);
			if (gsl::at(information->Normal.OffsetValues, i) != INVALID_OFFSET)
			{
				auto value = CreateGEP(sourcePtr, gsl::at(information->Normal.OffsetValues, i) / information->ElementSize);
				value = CreateLoad(value);
				value = CompileGetNormalChannel(value, i);
				CreateStore(value, dst);
			}
			else
			{
				// Set empty channel to 0 (1 for alpha channel)
				CreateStore(Const(static_cast<ReturnType>(i == 3 ? 1 : 0)), dst);
			}
		}
	}

	LLVMValueRef CompileGetNormalChannel(LLVMValueRef value, int channel)
	{
		switch (information->Base)
		{
		case BaseType::UNorm:
		case BaseType::UScaled:
		case BaseType::UInt:
			{
				switch (information->ElementSize)
				{
				case 1:
					return EmitConvert<uint8_t, ReturnType>(value);
				case 2:
					return EmitConvert<uint16_t, ReturnType>(value);
				case 4:
					return EmitConvert<uint32_t, ReturnType>(value);
				default:
					FATAL_ERROR();
				}
			}

		case BaseType::SNorm:
		case BaseType::SScaled:
		case BaseType::SInt:
			{
				switch (information->ElementSize)
				{
				case 1:
					return EmitConvert<int8_t, ReturnType>(value);
				case 2:
					return EmitConvert<int16_t, ReturnType>(value);
				case 4:
					return EmitConvert<int32_t, ReturnType>(value);
				default:
					FATAL_ERROR();
				}
			}

		case BaseType::UFloat:
			FATAL_ERROR();

		case BaseType::SFloat:
			assert((std::is_same<ReturnType, float>::value));
			// 		switch (information->ElementSize)
			// 		{
			// 		case 2:
			// 			sourceType = builder.getHalfTy();
			// 			process = [](llvm::IRBuilder<>& builder, llvm::Value* inputValue, int)
			// 			{
			// 				return EmitConvert<half, float>(builder, inputValue);
			// 			};
			// 			break;
			//
			// 		case 4:
			// 			sourceType = builder.getFloatTy();
			// 			process = [](llvm::IRBuilder<>& builder, llvm::Value* inputValue, int)
			// 			{
			// 				return EmitConvert<float, float>(builder, inputValue);
			// 			};
			// 			break;
			//
			// 		default:
			// 			FATAL_ERROR();
			// 		}
			// 		break;
			FATAL_ERROR();

		case BaseType::SRGB:
			assert((std::is_same<ReturnType, float>::value));
			// 		switch (information->ElementSize)
			// 		{
			// 		case 1:
			// 			sourceType = builder.getInt8Ty();
			// 			process = [function](llvm::IRBuilder<>& builder, llvm::Value* inputValue, int index)
			// 			{
			// 				inputValue = EmitConvert<uint8_t, float>(builder, inputValue);
			// 				return index == 3 ? inputValue : EmitSRGBToLinear(function, builder, inputValue);
			// 			};
			// 			break;
			//
			// 		case 2:
			// 			sourceType = builder.getInt16Ty();
			// 			process = [function](llvm::IRBuilder<>& builder, llvm::Value* inputValue, int index)
			// 			{
			// 				inputValue = EmitConvert<uint16_t, float>(builder, inputValue);
			// 				return index == 3 ? inputValue : EmitSRGBToLinear(function, builder, inputValue);
			// 			};
			// 			break;
			//
			// 		case 4:
			// 			sourceType = builder.getInt32Ty();
			// 			process = [function](llvm::IRBuilder<>& builder, llvm::Value* inputValue, int index)
			// 			{
			// 				inputValue = EmitConvert<uint32_t, float>(builder, inputValue);
			// 				return index == 3 ? inputValue : EmitSRGBToLinear(function, builder, inputValue);
			// 			};
			// 			break;
			//
			// 		default:
			// 			FATAL_ERROR();
			// 		}
			// 		break;
			FATAL_ERROR();

		default:
			FATAL_ERROR();
		}
	}
};

CP_DLL_EXPORT FunctionPointer CompileGetPixelF32(CPJit* jit, const FormatInformation* information)
{
	return GetPixelCompiledModuleBuilder<float>(jit, information).Compile()->getFunctionPointer("main");
}

CP_DLL_EXPORT FunctionPointer CompileGetPixelI32(CPJit* jit, const FormatInformation* information)
{
	return GetPixelCompiledModuleBuilder<int32_t>(jit, information).Compile()->getFunctionPointer("main");
}

CP_DLL_EXPORT FunctionPointer CompileGetPixelU32(CPJit* jit, const FormatInformation* information)
{
	return GetPixelCompiledModuleBuilder<uint32_t>(jit, information).Compile()->getFunctionPointer("main");
}

class SetDSPixelCompiledModuleBuilder final : public PixelCompiledModuleBuilder
{
public:
	SetDSPixelCompiledModuleBuilder(CPJit* jit, const FormatInformation* information) :
		PixelCompiledModuleBuilder{jit, information}
	{
		assert(information->Type == FormatType::DepthStencil);
	}

protected:
	void MainCompilation() override
	{
		std::vector<LLVMTypeRef> parameters
		{
			LLVMPointerType(LLVMInt8TypeInContext(context), 0),
			GetType<float>(),
			GetType<uint8_t>(),
		};
		const auto functionType = LLVMFunctionType(LLVMVoidType(), parameters.data(), static_cast<uint32_t>(parameters.size()), false);
		const auto function = LLVMAddFunction(module, "main", functionType);
		LLVMSetLinkage(function, LLVMExternalLinkage);

		const auto basicBlock = LLVMAppendBasicBlockInContext(context, function, "");
		LLVMPositionBuilderAtEnd(builder, basicBlock);

		const auto destinationPtr = LLVMGetParam(function, 0);
		const auto depthSource = LLVMGetParam(function, 1);
		const auto stencilSource = LLVMGetParam(function, 2);
		
		if (information->DepthStencil.DepthOffset != INVALID_OFFSET)
		{
			auto dst = destinationPtr;
			auto value = CreateMinNum(CreateMaxNum(depthSource,  ConstF32(0)), ConstF32(1));
			switch (information->Format)
			{
			case VK_FORMAT_D16_UNORM:
			case VK_FORMAT_D16_UNORM_S8_UINT:
				value = EmitConvert<float, uint16_t>(value);
				dst = LLVMBuildBitCast(builder, dst, LLVMPointerType(LLVMInt16TypeInContext(context), 0), "");
				break;
		
			case VK_FORMAT_D24_UNORM_S8_UINT:
			case VK_FORMAT_X8_D24_UNORM_PACK32:
				value = LLVMBuildFMul(builder, value, ConstF32(0x00FFFFFF), "");
				value = CallIntrinsic<1>(builder, module, Intrinsics::round, {value});
				value = LLVMBuildFPToUI(builder, value, LLVMInt32TypeInContext(context), "");
				dst = LLVMBuildBitCast(builder, dst, LLVMPointerType(LLVMInt32TypeInContext(context), 0), "");
				if (information->Format == VK_FORMAT_D24_UNORM_S8_UINT)
				{
					auto stencil = LLVMBuildZExt(builder, stencilSource, LLVMInt32TypeInContext(context), "");
					stencil = LLVMBuildShl(builder, stencil, ConstU32(24), "");
					value = LLVMBuildOr(builder, value, stencil, "");
				}
				break;
		
			case VK_FORMAT_D32_SFLOAT:
			case VK_FORMAT_D32_SFLOAT_S8_UINT:
				dst = LLVMBuildBitCast(builder, dst, LLVMPointerType(LLVMFloatTypeInContext(context), 0), "");
				break;
		
			default:
				FATAL_ERROR();
			}
			CreateStore(value, dst);
		}
		
		if (information->DepthStencil.StencilOffset != INVALID_OFFSET && information->Format != VK_FORMAT_D24_UNORM_S8_UINT)
		{
			const auto dst = CreateGEP(destinationPtr, information->DepthStencil.StencilOffset);
			CreateStore(stencilSource, dst);
		}

		LLVMBuildRetVoid(builder);
	}
};

CP_DLL_EXPORT FunctionPointer CompileSetPixelDepthStencil(CPJit* jit, const FormatInformation* information)
{
	return SetDSPixelCompiledModuleBuilder(jit, information).Compile()->getFunctionPointer("main");
}

// static void EmitSetPackedPixelFloat(llvm::Function* function, llvm::Module* module, llvm::IRBuilder<>& builder, const FormatInformation* information, llvm::Value* destinationPtr, llvm::Value* sourcePtr)
// {
// 	assert(information->Type == FormatType::Packed);
// 	
// 	llvm::Type* resultType;
// 	switch (information->TotalSize)
// 	{
// 	case 1:
// 		resultType = builder.getInt8Ty();
// 		break;
//
// 	case 2:
// 		resultType = builder.getInt16Ty();
// 		break;
//
// 	case 4:
// 		resultType = builder.getInt32Ty();
// 		break;
//
// 	default:
// 		FATAL_ERROR();
// 	}
//
// 	std::function<llvm::Value*(llvm::IRBuilder<>&, llvm::Value*, int)> process;
// 	switch (information->Base)
// 	{
// 	case BaseType::UNorm:
// 		process = [information, resultType, sourcePtr](llvm::IRBuilder<>& builder, llvm::Value* oldValue, int index)
// 		{
// 			const auto bits = GetPackedBits(information, index);
// 			const double size = (1ULL << bits) - 1;
// 			const auto offset = GetPackedOffset(information, index);
//
// 			// Get source float from array
// 			auto source = builder.CreateConstGEP1_32(sourcePtr, index);
// 			source = builder.CreateLoad(source);
// 			// Clamp between [0, 1]
// 			source = builder.CreateMinNum(builder.CreateMaxNum(source, llvm::ConstantFP::get(builder.getFloatTy(), 0)),
// 			                              llvm::ConstantFP::get(builder.getFloatTy(), 1));
// 			// Float to UInt
// 			auto value = builder.CreateFMul(source, llvm::ConstantFP::get(builder.getFloatTy(), size));
// 			value = builder.CreateUnaryIntrinsic(llvm::Intrinsic::round, value);
// 			value = builder.CreateFPToUI(value, resultType);
// 			// Shift and or with old value
// 			value = builder.CreateShl(value, llvm::ConstantInt::get(resultType, offset));
// 			return builder.CreateOr(value, oldValue);
// 		};
// 		break;
// 		 
// 	case BaseType::SNorm:
// 		process = [information, resultType, sourcePtr](llvm::IRBuilder<>& builder, llvm::Value* oldValue, int index)
// 		{
// 			const auto bits = GetPackedBits(information, index);
// 			const double size = (1ULL << (bits - 1)) - 1;
// 			const auto offset = GetPackedOffset(information, index);
// 			
// 			// Get source float from array
// 			auto source = builder.CreateConstGEP1_32(sourcePtr, index);
// 			source = builder.CreateLoad(source);
// 			// Clamp between [-1, 1]
// 			source = builder.CreateMinNum(builder.CreateMaxNum(source, llvm::ConstantFP::get(builder.getFloatTy(), -1)),
// 			                              llvm::ConstantFP::get(builder.getFloatTy(), 1));
// 			// Float to UInt
// 			auto value = builder.CreateFMul(source, llvm::ConstantFP::get(builder.getFloatTy(), size));
// 			value = builder.CreateUnaryIntrinsic(llvm::Intrinsic::round, value);
// 			value = builder.CreateFPToUI(value, resultType);
// 			// Shift and or with old value
// 			value = builder.CreateShl(value, llvm::ConstantInt::get(resultType, offset));
// 			return builder.CreateOr(value, oldValue);
// 		};
// 		break;
// 		
// 	case BaseType::UFloat:
// 		switch (information->Format)
// 		{
// 		case VK_FORMAT_B10G11R11_UFLOAT_PACK32:
// 			{
// 				const auto b10g11r11Function = llvm::Function::Create(llvm::FunctionType::get(resultType, {sourcePtr->getType()}, false), llvm::GlobalValue::ExternalLinkage, "FloatToB10G11R11", module);
// 				const auto value = builder.CreateCall(b10g11r11Function, {sourcePtr});
// 				const auto dst = builder.CreateBitCast(destinationPtr, llvm::PointerType::get(resultType, 0));
// 				builder.CreateStore(value, dst);
// 				return;
// 			}
// 			
// 		case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32:
// 			{
// 				const auto e5b9g9r9Function = llvm::Function::Create(llvm::FunctionType::get(resultType, {sourcePtr->getType()}, false), llvm::GlobalValue::ExternalLinkage, "FloatToE5B9G9R9", module);
// 				const auto value = builder.CreateCall(e5b9g9r9Function, {sourcePtr});
// 				const auto dst = builder.CreateBitCast(destinationPtr, llvm::PointerType::get(resultType, 0));
// 				builder.CreateStore(value, dst);
// 				return;
// 			}
//
// 		default:
// 			FATAL_ERROR();
// 		}
// 		
// 	case BaseType::SFloat:
// 		FATAL_ERROR();
// 		
// 	case BaseType::SRGB:
// 		process = [information, resultType, sourcePtr, function](llvm::IRBuilder<>& builder, llvm::Value* oldValue, int index)
// 		{
// 			const auto bits = GetPackedBits(information, index);
// 			const double size = (1ULL << bits) - 1;
// 			const auto offset = GetPackedOffset(information, index);
//
// 			// Get source float from array
// 			auto source = builder.CreateConstGEP1_32(sourcePtr, index);
// 			source = builder.CreateLoad(source);
// 			// Clamp between [0, 1]
// 			source = builder.CreateMinNum(builder.CreateMaxNum(source, llvm::ConstantFP::get(builder.getFloatTy(), 0)),
// 			                              llvm::ConstantFP::get(builder.getFloatTy(), 1));
// 			source = index == 3 ? source : EmitLinearToSRGB(function, builder, source);
// 			// Float to UInt
// 			auto value = builder.CreateFMul(source, llvm::ConstantFP::get(builder.getFloatTy(), size));
// 			value = builder.CreateUnaryIntrinsic(llvm::Intrinsic::round, value);
// 			value = builder.CreateFPToUI(value, resultType);
// 			// Shift and or with old value
// 			value = builder.CreateShl(value, llvm::ConstantInt::get(resultType, offset));
// 			return builder.CreateOr(value, oldValue);
// 		};
// 		break;
// 		
// 	default: FATAL_ERROR();
// 	}
//
// 	llvm::Value* value = llvm::ConstantInt::get(resultType, 0);
//
// 	if (information->Packed.RedBits)
// 	{
// 		value = process(builder, value, 0);
// 	}
//
// 	if (information->Packed.GreenBits)
// 	{
// 		value = process(builder, value, 1);
// 	}
//
// 	if (information->Packed.BlueBits)
// 	{
// 		value = process(builder, value, 2);
// 	}
//
// 	if (information->Packed.AlphaBits)
// 	{
// 		value = process(builder, value, 3);
// 	}
//
// 	const auto dst = builder.CreateBitCast(destinationPtr, llvm::PointerType::get(resultType, 0));
// 	builder.CreateStore(value, dst);
// }
//
// static void EmitSetPackedPixelInt32(llvm::IRBuilder<>& builder, const FormatInformation* information, llvm::Value* destinationPtr, llvm::Value* sourcePtr)
// {
// 	assert(information->Type == FormatType::Packed);
//
// 	llvm::Type* resultType;
// 	switch (information->TotalSize)
// 	{
// 	case 1:
// 		resultType = builder.getInt8Ty();
// 		break;
//
// 	case 2:
// 		resultType = builder.getInt16Ty();
// 		break;
//
// 	case 4:
// 		resultType = builder.getInt32Ty();
// 		break;
//
// 	default:
// 		FATAL_ERROR();
// 	}
//
// 	const auto process = [information, resultType, sourcePtr](llvm::IRBuilder<>& builder, llvm::Value* oldValue, int index)
// 	{
// 		const auto bits = GetPackedBits(information, index);
// 		const auto mask = (1ULL << bits) - 1;
// 		const auto offset = GetPackedOffset(information, index);
//
// 		const auto min = -(1LL << (bits - 1));
// 		const auto max = (1LL << (bits - 1)) - 1;
//
// 		// Get source from array
// 		auto source = builder.CreateConstGEP1_32(sourcePtr, index);
// 		source = builder.CreateLoad(source);
// 		
// 		auto tmp = builder.CreateICmpSGT(source, builder.getInt32(min));
// 		source = builder.CreateSelect(tmp, source, builder.getInt32(min));
//
// 		tmp = builder.CreateICmpSLT(source, builder.getInt32(max));
// 		source = builder.CreateSelect(tmp, source, builder.getInt32(max));
//
// 		auto value = builder.CreateAnd(source, mask);
// 		value = builder.CreateZExtOrTrunc(value, resultType);
//
// 		// Shift and or with old value
// 		value = builder.CreateShl(value, llvm::ConstantInt::get(resultType, offset));
// 		return builder.CreateOr(value, oldValue);
// 	};
//
// 	llvm::Value* value = llvm::ConstantInt::get(resultType, 0);
//
// 	if (information->Packed.RedBits)
// 	{
// 		value = process(builder, value, 0);
// 	}
//
// 	if (information->Packed.GreenBits)
// 	{
// 		value = process(builder, value, 1);
// 	}
//
// 	if (information->Packed.BlueBits)
// 	{
// 		value = process(builder, value, 2);
// 	}
//
// 	if (information->Packed.AlphaBits)
// 	{
// 		value = process(builder, value, 3);
// 	}
//
// 	const auto dst = builder.CreateBitCast(destinationPtr, llvm::PointerType::get(resultType, 0));
// 	builder.CreateStore(value, dst);
// }
//
// static void EmitSetPackedPixelUInt32(llvm::IRBuilder<>& builder, const FormatInformation* information, llvm::Value* destinationPtr, llvm::Value* sourcePtr)
// {
// 	assert(information->Type == FormatType::Packed);
//
// 	llvm::Type* resultType;
// 	switch (information->TotalSize)
// 	{
// 	case 1:
// 		resultType = builder.getInt8Ty();
// 		break;
//
// 	case 2:
// 		resultType = builder.getInt16Ty();
// 		break;
//
// 	case 4:
// 		resultType = builder.getInt32Ty();
// 		break;
//
// 	default:
// 		FATAL_ERROR();
// 	}
//
// 	const auto process = [information, resultType, sourcePtr](llvm::IRBuilder<>& builder, llvm::Value* oldValue, int index)
// 	{
// 		const auto bits = GetPackedBits(information, index);
// 		const auto mask = (1ULL << bits) - 1;
// 		const auto offset = GetPackedOffset(information, index);
//
// 		// Get source from array
// 		auto source = builder.CreateConstGEP1_32(sourcePtr, index);
// 		source = builder.CreateLoad(source);
//
// 		const auto tmp = builder.CreateICmpULT(source, builder.getInt32(mask));
// 		source = builder.CreateSelect(tmp, source, builder.getInt32(mask));
//
// 		auto value = builder.CreateAnd(source, mask);
// 		value = builder.CreateZExtOrTrunc(value, resultType);
//
// 		// Shift and or with old value
// 		value = builder.CreateShl(value, llvm::ConstantInt::get(resultType, offset));
// 		return builder.CreateOr(value, oldValue);
// 	};
//
// 	llvm::Value* value = llvm::ConstantInt::get(resultType, 0);
//
// 	if (information->Packed.RedBits)
// 	{
// 		value = process(builder, value, 0);
// 	}
//
// 	if (information->Packed.GreenBits)
// 	{
// 		value = process(builder, value, 1);
// 	}
//
// 	if (information->Packed.BlueBits)
// 	{
// 		value = process(builder, value, 2);
// 	}
//
// 	if (information->Packed.AlphaBits)
// 	{
// 		value = process(builder, value, 3);
// 	}
//
// 	const auto dst = builder.CreateBitCast(destinationPtr, llvm::PointerType::get(resultType, 0));
// 	builder.CreateStore(value, dst);
// }

template<typename ReturnType>
class SetPixelCompiledModuleBuilder final : public PixelCompiledModuleBuilder
{
public:
	SetPixelCompiledModuleBuilder(CPJit* jit, const FormatInformation* information) :
		PixelCompiledModuleBuilder{jit, information}
	{
	}

protected:
	void MainCompilation() override
	{
		std::vector<LLVMTypeRef> parameters
		{
			LLVMPointerType(LLVMInt8TypeInContext(context), 0),
			LLVMPointerType(GetType<ReturnType>(), 0),
		};
		const auto functionType = LLVMFunctionType(LLVMVoidType(), parameters.data(), static_cast<uint32_t>(parameters.size()), false);
		const auto function = LLVMAddFunction(module, "main", functionType);
		LLVMSetLinkage(function, LLVMExternalLinkage);

		const auto basicBlock = LLVMAppendBasicBlockInContext(context, function, "");
		LLVMPositionBuilderAtEnd(builder, basicBlock);

		const auto destinationPtr = LLVMGetParam(function, 0);
		const auto sourcePtr = LLVMGetParam(function, 1);

		switch (information->Type)
		{
		case FormatType::Normal:
			CompileSetNormal(function, destinationPtr, sourcePtr);
			break;

		case FormatType::Packed:
			CompileSetPacked(function, destinationPtr, sourcePtr);
			break;

		case FormatType::Compressed:
			FATAL_ERROR();

		case FormatType::Planar:
			FATAL_ERROR();

		case FormatType::PlanarSamplable:
			FATAL_ERROR();

		default: FATAL_ERROR();
		}

		LLVMBuildRetVoid(builder);
	}

private:
	void CompileSetNormal(LLVMValueRef function, LLVMValueRef destinationPtr, LLVMValueRef sourcePtr)
	{
		LLVMTypeRef resultType{};
		switch (information->Base)
		{
		case BaseType::UFloat:
			FATAL_ERROR();
			
		case BaseType::SFloat:
			switch (information->ElementSize)
			{
			case 2:
				resultType = LLVMHalfTypeInContext(context);
				break;
			
			case 4:
				resultType = LLVMFloatTypeInContext(context);
				break;
			
			case 8:
				resultType = LLVMDoubleTypeInContext(context);
				break;
			
			default:
				FATAL_ERROR();
			}
			break;
			
		default:
			resultType = LLVMIntTypeInContext(context, information->ElementSize * 8);
			break;
		}

		destinationPtr = LLVMBuildBitCast(builder, destinationPtr, LLVMPointerType(resultType, 0), "");

		for (auto i = 0u; i < 4; i++)
		{
			if (gsl::at(information->Normal.OffsetValues, i) != INVALID_OFFSET)
			{
				auto value = CreateGEP(sourcePtr, i);
				value = CreateLoad(value);
				value = CompileSetNormalChannel(function, value, i);
				const auto dst = CreateGEP(destinationPtr, gsl::at(information->Normal.OffsetValues, i) / information->ElementSize);
				CreateStore(value, dst);
			}
		}
	}

	LLVMValueRef CompileSetNormalChannel(LLVMValueRef function, LLVMValueRef inputValue, int channel)
	{
		switch (information->Base)
		{
		case BaseType::UNorm:
			{
				assert((std::is_same<ReturnType, float>::value));
				const auto value = CreateMinNum(CreateMaxNum(inputValue, ConstF32(0)), ConstF32(1));
				switch (information->ElementSize)
				{
				case 1:
					return EmitConvert<float, uint8_t>(value);
				case 2:
					return EmitConvert<float, uint16_t>(value);
				case 4:
					return EmitConvert<float, uint32_t>(value);
				default:
					FATAL_ERROR();
				}
			}
			
		case BaseType::SNorm:
			assert((std::is_same<ReturnType, float>::value));
			switch (information->ElementSize)
			{
			case 1:
				//			process = [](llvm::IRBuilder<>& builder, llvm::Value* inputValue, int)
				//			{
				//				const auto value = builder.CreateMinNum(builder.CreateMaxNum(inputValue, llvm::ConstantFP::get(builder.getFloatTy(), -1)),
				//				                                        llvm::ConstantFP::get(builder.getFloatTy(), 1));
				//				return EmitConvert<float, int8_t>(builder, value);
				//			};
				//			break;
			
			case 2:
				//			process = [](llvm::IRBuilder<>& builder, llvm::Value* inputValue, int)
				//			{
				//				const auto value = builder.CreateMinNum(builder.CreateMaxNum(inputValue, llvm::ConstantFP::get(builder.getFloatTy(), -1)),
				//				                                        llvm::ConstantFP::get(builder.getFloatTy(), 1));
				//				return EmitConvert<float, int16_t>(builder, value);
				//			};
				//			break;
			
			case 4:
				//			process = [](llvm::IRBuilder<>& builder, llvm::Value* inputValue, int)
				//			{
				//				const auto value = builder.CreateMinNum(builder.CreateMaxNum(inputValue, llvm::ConstantFP::get(builder.getFloatTy(), -1)),
				//				                                        llvm::ConstantFP::get(builder.getFloatTy(), 1));
				//				return EmitConvert<float, int32_t>(builder, value);
				//			};
				//			break;
			
			default:
				FATAL_ERROR();
			}
		
		case BaseType::UScaled:
			FATAL_ERROR();
			
		case BaseType::SScaled:
			FATAL_ERROR();
			
		case BaseType::UInt:
			assert((std::is_same<ReturnType, uint32_t>::value));
			switch (information->ElementSize)
			{
			case 1:
				// 				process = [](llvm::IRBuilder<>& builder, llvm::Value* inputValue)
				// 				{
				// 					const auto tmp = builder.CreateICmpULT(inputValue, builder.getInt32(std::numeric_limits<uint8_t>::max()));
				// 					inputValue = builder.CreateSelect(tmp, inputValue, builder.getInt32(std::numeric_limits<uint8_t>::max()));
				// 					return EmitConvert<uint32_t, uint8_t>(builder, inputValue);
				// 				};
				// 				break;
			
			case 2:
				// 				process = [](llvm::IRBuilder<>& builder, llvm::Value* inputValue)
				// 				{
				// 					const auto tmp = builder.CreateICmpULT(inputValue, builder.getInt32(std::numeric_limits<uint16_t>::max()));
				// 					inputValue = builder.CreateSelect(tmp, inputValue, builder.getInt32(std::numeric_limits<uint16_t>::max()));
				// 					return EmitConvert<uint32_t, uint16_t>(builder, inputValue);
				// 				};
				// 				break;
			
			case 4:
				// 				process = EmitConvert<uint32_t, uint32_t>;
				// 				break;
			
			default:
				FATAL_ERROR();
			}
			
		case BaseType::SInt:
			assert((std::is_same<ReturnType, int32_t>::value));
			switch (information->ElementSize)
			{
			case 1:
				// 				process = [](llvm::IRBuilder<>& builder, llvm::Value* inputValue)
				// 				{
				// 					auto tmp = builder.CreateICmpSGT(inputValue, builder.getInt32(std::numeric_limits<int8_t>::min()));
				// 					inputValue = builder.CreateSelect(tmp, inputValue, builder.getInt32(std::numeric_limits<int8_t>::min()));
				// 				
				// 					tmp = builder.CreateICmpSLT(inputValue, builder.getInt32(std::numeric_limits<int8_t>::max()));
				// 					inputValue = builder.CreateSelect(tmp, inputValue, builder.getInt32(std::numeric_limits<int8_t>::max()));
				//
				// 					return EmitConvert<int32_t, int8_t>(builder, inputValue);
				// 				};
				// 				break;
			
			case 2:
				// 				process = [](llvm::IRBuilder<>& builder, llvm::Value* inputValue)
				// 				{
				// 					auto tmp = builder.CreateICmpSGT(inputValue, builder.getInt32(std::numeric_limits<int16_t>::min()));
				// 					inputValue = builder.CreateSelect(tmp, inputValue, builder.getInt32(std::numeric_limits<int16_t>::min()));
				//
				// 					tmp = builder.CreateICmpSLT(inputValue, builder.getInt32(std::numeric_limits<int16_t>::max()));
				// 					inputValue = builder.CreateSelect(tmp, inputValue, builder.getInt32(std::numeric_limits<int16_t>::max()));
				//
				// 					return EmitConvert<int32_t, int16_t>(builder, inputValue);
				// 				};
				// 				break;
			
			case 4:
				// 				process = EmitConvert<int32_t, int32_t>;
				// 				break;
			
			default:
				FATAL_ERROR();
			}
			
		case BaseType::UFloat:
			FATAL_ERROR();
			
		case BaseType::SFloat:
			assert((std::is_same<ReturnType, float>::value));
			switch (information->ElementSize)
			{
			case 2:
				//			process = [](llvm::IRBuilder<>& builder, llvm::Value* inputValue, int)
				//			{
				//				return EmitConvert<float, half>(builder, inputValue);
				//			};
				//			break;
			
			case 4:
				//			process = [](llvm::IRBuilder<>& builder, llvm::Value* inputValue, int)
				//			{
				//				return EmitConvert<float, float>(builder, inputValue);
				//			};
				//			break;
			
			case 8:
				//			process = [](llvm::IRBuilder<>& builder, llvm::Value* inputValue, int)
				//			{
				//				return EmitConvert<float, double>(builder, inputValue);
				//			};
				//			break;
			
			default:
				FATAL_ERROR();
			}
			
		case BaseType::SRGB:
			assert((std::is_same<ReturnType, float>::value));
			switch (information->ElementSize)
			{
			case 1:
				//			process = [function](llvm::IRBuilder<>& builder, llvm::Value* inputValue, int index)
				//			{
				//				llvm::Value* value = builder.CreateMinNum(builder.CreateMaxNum(inputValue, llvm::ConstantFP::get(builder.getFloatTy(), 0)),
				//				                                          llvm::ConstantFP::get(builder.getFloatTy(), 1));
				//				value = index == 3 ? inputValue : EmitLinearToSRGB(function, builder, inputValue);
				//				return EmitConvert<float, uint8_t>(builder, value);
				//			};
				//			break;
			
			case 2:
				//			process = [function](llvm::IRBuilder<>& builder, llvm::Value* inputValue, int index)
				//			{
				//				llvm::Value* value = builder.CreateMinNum(builder.CreateMaxNum(inputValue, llvm::ConstantFP::get(builder.getFloatTy(), 0)),
				//				                                          llvm::ConstantFP::get(builder.getFloatTy(), 1));
				//				value = index == 3 ? inputValue : EmitLinearToSRGB(function, builder, inputValue);
				//				return EmitConvert<float, uint16_t>(builder, value);
				//			};
				//			break;
			
			case 4:
				//			process = [function](llvm::IRBuilder<>& builder, llvm::Value* inputValue, int index)
				//			{
				//				llvm::Value* value = builder.CreateMinNum(builder.CreateMaxNum(inputValue, llvm::ConstantFP::get(builder.getFloatTy(), 0)),
				//				                                          llvm::ConstantFP::get(builder.getFloatTy(), 1));
				//				value = index == 3 ? inputValue : EmitLinearToSRGB(function, builder, inputValue);
				//				return EmitConvert<float, uint32_t>(builder, value);
				//			};
				//			break;
			
			default:
				FATAL_ERROR();
			}
		
		default:
			FATAL_ERROR();
		}
	}
	
	void CompileSetPacked(LLVMValueRef function, LLVMValueRef destinationPtr, LLVMValueRef sourcePtr)
	{
		//EmitSetPackedPixelFloat(function, module.get(), builder, information, destinationPtr, sourcePtr);
		//EmitSetPackedPixelUInt32(builder, information, destinationPtr, sourcePtr);
		//EmitSetPackedPixelInt32(builder, information, destinationPtr, sourcePtr);
		FATAL_ERROR();
	}
};

CP_DLL_EXPORT FunctionPointer CompileSetPixelF32(CPJit* jit, const FormatInformation* information)
{
	return SetPixelCompiledModuleBuilder<float>(jit, information).Compile()->getFunctionPointer("main");
}

CP_DLL_EXPORT FunctionPointer CompileSetPixelI32(CPJit* jit, const FormatInformation* information)
{
	return SetPixelCompiledModuleBuilder<int32_t>(jit, information).Compile()->getFunctionPointer("main");
}

CP_DLL_EXPORT FunctionPointer CompileSetPixelU32(CPJit* jit, const FormatInformation* information)
{
	return SetPixelCompiledModuleBuilder<uint32_t>(jit, information).Compile()->getFunctionPointer("main");
}