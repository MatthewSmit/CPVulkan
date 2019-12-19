#include "ImageCompiler.h"

#include "CompiledModule.h"
#include "CompiledModuleBuilder.h"
#include "Compilers.h"

#include <Formats.h>
#include <Half.h>

#include <llvm-c/Target.h>

#include <type_traits>

// Conversion functions
static LLVMValueRef EmitConvertFloat(CompiledModuleBuilder* moduleBuilder, LLVMValueRef inputValue, LLVMTypeRef outputType)
{
	return moduleBuilder->CreateFPExtOrTrunc(inputValue, outputType);
}

static LLVMValueRef EmitConvertFloatInt(CompiledModuleBuilder* moduleBuilder, LLVMValueRef inputValue, int64_t maxValue, LLVMTypeRef outputType)
{
	auto value = moduleBuilder->CreateFMul(inputValue, LLVMConstReal(LLVMTypeOf(inputValue), static_cast<double>(maxValue)));
	value = moduleBuilder->CreateIntrinsic<1>(Intrinsics::round, {value});
	return moduleBuilder->CreateFPToSI(value, outputType);
}

static LLVMValueRef EmitConvertFloatUInt(CompiledModuleBuilder* moduleBuilder, LLVMValueRef inputValue, uint64_t maxValue, LLVMTypeRef outputType)
{
	auto value = moduleBuilder->CreateFMul(inputValue, LLVMConstReal(LLVMTypeOf(inputValue), static_cast<double>(maxValue)));
	value = moduleBuilder->CreateIntrinsic<1>(Intrinsics::round, {value});
	return moduleBuilder->CreateFPToUI(value, outputType);
}

static LLVMValueRef EmitConvertInt(CompiledModuleBuilder* moduleBuilder, LLVMValueRef inputValue, LLVMTypeRef outputType, bool inputSigned)
{
	if (inputSigned)
	{
		return moduleBuilder->CreateSExtOrTrunc(inputValue, outputType);
	}
	return moduleBuilder->CreateZExtOrTrunc(inputValue, outputType);
}

static LLVMValueRef EmitConvertIntFloat(CompiledModuleBuilder* moduleBuilder, LLVMValueRef inputValue, uint64_t maxValue, LLVMTypeRef outputType)
{
	const auto value = moduleBuilder->CreateSIToFP(inputValue, outputType);
	return moduleBuilder->CreateFDiv(value, LLVMConstReal(outputType, static_cast<double>(maxValue)));
}

static LLVMValueRef EmitConvertUIntFloat(CompiledModuleBuilder* moduleBuilder, LLVMValueRef inputValue, uint64_t maxValue, LLVMTypeRef outputType)
{
	const auto value = moduleBuilder->CreateUIToFP(inputValue, outputType);
	return moduleBuilder->CreateFDiv(value, LLVMConstReal(outputType, static_cast<double>(maxValue)));
}

template<typename InputType, typename OutputType>
static LLVMValueRef EmitConvert(CompiledModuleBuilder* moduleBuilder, LLVMValueRef inputValue)
{
	if constexpr (std::numeric_limits<InputType>::is_iec559)
	{
		if constexpr (std::numeric_limits<OutputType>::is_integer && !std::numeric_limits<OutputType>::is_signed)
		{
			return EmitConvertFloatUInt(moduleBuilder, inputValue, std::numeric_limits<OutputType>::max(), moduleBuilder->GetType<OutputType>());
		}
		else if constexpr (std::numeric_limits<OutputType>::is_integer)
		{
			return EmitConvertFloatInt(moduleBuilder, inputValue, std::numeric_limits<OutputType>::max(), moduleBuilder->GetType<OutputType>());
		}
		else
		{
			static_assert(std::numeric_limits<OutputType>::is_iec559);
			return EmitConvertFloat(moduleBuilder, inputValue, moduleBuilder->GetType<OutputType>());
		}
	}
	else if (std::is_unsigned<InputType>::value)
	{
		static_assert(std::numeric_limits<InputType>::is_integer);
		if constexpr (std::numeric_limits<OutputType>::is_integer)
		{
			return EmitConvertInt(moduleBuilder, inputValue, moduleBuilder->GetType<OutputType>(), false);
		}
		else
		{
			static_assert(std::numeric_limits<OutputType>::is_iec559);
			return EmitConvertUIntFloat(moduleBuilder, inputValue, std::numeric_limits<InputType>::max(), moduleBuilder->GetType<OutputType>());
		}
	}
	else
	{
		static_assert(std::numeric_limits<InputType>::is_integer);
		if constexpr (std::numeric_limits<OutputType>::is_integer)
		{
			return EmitConvertInt(moduleBuilder, inputValue, moduleBuilder->GetType<OutputType>(), true);
		}
		else
		{
			static_assert(std::numeric_limits<OutputType>::is_iec559);
			return EmitConvertIntFloat(moduleBuilder, inputValue, std::numeric_limits<InputType>::max(), moduleBuilder->GetType<OutputType>());
		}
	}
}

// Getting pixel functions
LLVMValueRef EmitSRGBToLinear(CompiledModuleBuilder* moduleBuilder, LLVMValueRef inputValue)
{
	const auto trueBlock = LLVMAppendBasicBlockInContext(moduleBuilder->context, moduleBuilder->currentFunction, "");
	const auto falseBlock = LLVMAppendBasicBlockInContext(moduleBuilder->context, moduleBuilder->currentFunction, "");
	const auto nextBlock = LLVMAppendBasicBlockInContext(moduleBuilder->context, moduleBuilder->currentFunction, "");

	const auto comparison = moduleBuilder->CreateFCmpUGT(inputValue, moduleBuilder->ConstF32(0.04045f));
	moduleBuilder->CreateCondBr(comparison, trueBlock, falseBlock);

	LLVMPositionBuilderAtEnd(moduleBuilder->builder, falseBlock);
	const auto falseValue = moduleBuilder->CreateFDiv(inputValue, moduleBuilder->ConstF32(12.92f));
	moduleBuilder->CreateBr(nextBlock);

	LLVMPositionBuilderAtEnd(moduleBuilder->builder, trueBlock);
	auto trueValue = moduleBuilder->CreateFAdd(inputValue, moduleBuilder->ConstF32(0.055f));
	trueValue = moduleBuilder->CreateFDiv(trueValue, moduleBuilder->ConstF32(1.055f));
	trueValue = moduleBuilder->CreateIntrinsic<2>(Intrinsics::pow, {trueValue, moduleBuilder->ConstF32(2.4f)});
	moduleBuilder->CreateBr(nextBlock);

	LLVMPositionBuilderAtEnd(moduleBuilder->builder, nextBlock);
	const auto value = moduleBuilder->CreatePhi(LLVMFloatTypeInContext(moduleBuilder->context));
	std::array<LLVMValueRef, 2> incoming
	{
		trueValue,
		falseValue,
	};
	std::array<LLVMBasicBlockRef, 2> incomingBlocks
	{
		trueBlock,
		falseBlock,
	};
	LLVMAddIncoming(value, incoming.data(), incomingBlocks.data(), 2);
	return value;
}

template<typename ReturnType>
static LLVMValueRef EmitGetNormalChannel(CompiledModuleBuilder* moduleBuilder, const FormatInformation* information, LLVMValueRef sourcePtr, int channel)
{
	auto value = moduleBuilder->CreateGEP(sourcePtr, gsl::at(information->Normal.OffsetValues, channel) / information->ElementSize);
	value = moduleBuilder->CreateLoad(value);

	switch (information->Base)
	{
	case BaseType::UNorm:
	case BaseType::UScaled:
	case BaseType::UInt:
		switch (information->ElementSize)
		{
		case 1:
			return EmitConvert<uint8_t, ReturnType>(moduleBuilder, value);
		case 2:
			return EmitConvert<uint16_t, ReturnType>(moduleBuilder, value);
		case 4:
			return EmitConvert<uint32_t, ReturnType>(moduleBuilder, value);
		default:
			TODO_ERROR();
		}

	case BaseType::SNorm:
	case BaseType::SScaled:
	case BaseType::SInt:
		switch (information->ElementSize)
		{
		case 1:
			return EmitConvert<int8_t, ReturnType>(moduleBuilder, value);
		case 2:
			return EmitConvert<int16_t, ReturnType>(moduleBuilder, value);
		case 4:
			return EmitConvert<int32_t, ReturnType>(moduleBuilder, value);
		default:
			TODO_ERROR();
		}

	case BaseType::UFloat:
		TODO_ERROR();

	case BaseType::SFloat:
		assert((std::is_same<ReturnType, float>::value));
		switch (information->ElementSize)
		{
		case 2:
			return EmitConvert<half, ReturnType>(moduleBuilder, value);
		case 4:
			return EmitConvert<float, ReturnType>(moduleBuilder, value);
		default:
			TODO_ERROR();
		}

	case BaseType::SRGB:
		assert((std::is_same<ReturnType, float>::value));
		switch (information->ElementSize)
		{
		case 1:
			return channel == 3 ? EmitConvert<uint8_t, ReturnType>(moduleBuilder, value) : EmitSRGBToLinear(moduleBuilder, EmitConvert<uint8_t, ReturnType>(moduleBuilder, value));
		case 2:
			return channel == 3 ? EmitConvert<uint16_t, ReturnType>(moduleBuilder, value) : EmitSRGBToLinear(moduleBuilder, EmitConvert<uint16_t, ReturnType>(moduleBuilder, value));
		case 4:
			return channel == 3 ? EmitConvert<uint32_t, ReturnType>(moduleBuilder, value) : EmitSRGBToLinear(moduleBuilder, EmitConvert<uint32_t, ReturnType>(moduleBuilder, value));
		default:
			TODO_ERROR();
		}

	default:
		TODO_ERROR();
	}
}

static LLVMValueRef EmitGetPackedChannel(CompiledModuleBuilder* moduleBuilder, const FormatInformation* information, LLVMValueRef source, LLVMTypeRef destinationPtrType, int channel)
{
	const auto valueBits = LLVMSizeOfTypeInBits(moduleBuilder->jit->getDataLayout(), LLVMTypeOf(source));
	const auto bits = gsl::at(information->Packed.BitValues, channel);
	const auto mask = (1ULL << bits) - 1;
	const auto offset = gsl::at(information->Packed.OffsetValues, channel);

	// Shift and mask value
	auto value = moduleBuilder->CreateLShr(source, LLVMConstInt(LLVMTypeOf(source), offset, false));
	value = moduleBuilder->CreateAnd(value, LLVMConstInt(LLVMTypeOf(source), mask, false));

	switch (information->Base)
	{
	case BaseType::UNorm:
		value = moduleBuilder->CreateUIToFP(value, LLVMFloatTypeInContext(moduleBuilder->context));
		value = moduleBuilder->CreateFDiv(value, LLVMConstReal(LLVMFloatTypeInContext(moduleBuilder->context), mask));
		return value;

	case BaseType::SNorm:
		value = moduleBuilder->CreateSIToFP(value, LLVMFloatTypeInContext(moduleBuilder->context));
		value = moduleBuilder->CreateFDiv(value, LLVMConstReal(LLVMFloatTypeInContext(moduleBuilder->context), mask >> 1));
		return value;

	case BaseType::UScaled:
	case BaseType::SScaled:
		TODO_ERROR();

	case BaseType::UInt:
		return value;

	case BaseType::SInt:
		if (valueBits != bits)
		{
			value = moduleBuilder->CreateShl(value, moduleBuilder->ConstI32(valueBits - bits));
			value = moduleBuilder->CreateAShr(value, moduleBuilder->ConstI32(valueBits - bits));
			// TODO: ASHR can be exact
		}
		else
		{
			value = moduleBuilder->CreateSExt(value, destinationPtrType);
		}
		return value;

	case BaseType::UFloat:
	case BaseType::SFloat:
		TODO_ERROR();

	case BaseType::SRGB:
		value = moduleBuilder->CreateUIToFP(value, LLVMFloatTypeInContext(moduleBuilder->context));
		value = moduleBuilder->CreateFDiv(value, LLVMConstReal(LLVMFloatTypeInContext(moduleBuilder->context), mask));
		if (channel != 3)
		{
			value = EmitSRGBToLinear(moduleBuilder, value);
		}
		return value;

	default:
		FATAL_ERROR();
	}
}

static LLVMValueRef EmitGetNormalPixel(CompiledModuleBuilder* moduleBuilder, const FormatInformation* information, LLVMValueRef sourcePointer, LLVMTypeRef resultType)
{
	LLVMTypeRef sourceType{};
	switch (information->Base)
	{
	case BaseType::UFloat:
		TODO_ERROR();

	case BaseType::SFloat:
		switch (information->ElementSize)
		{
		case 2:
			sourceType = LLVMHalfTypeInContext(moduleBuilder->context);
			break;

		case 4:
			sourceType = LLVMFloatTypeInContext(moduleBuilder->context);
			break;

		case 8:
			sourceType = LLVMDoubleTypeInContext(moduleBuilder->context);
			break;

		default:
			TODO_ERROR();
		}
		break;

	default:
		sourceType = LLVMIntTypeInContext(moduleBuilder->context, information->ElementSize * 8);
		break;
	}

	sourcePointer = LLVMBuildBitCast(moduleBuilder->builder, sourcePointer, LLVMPointerType(sourceType, 0), "");

	if (LLVMGetTypeKind(LLVMGetElementType(resultType)) == LLVMFloatTypeKind)
	{
		LLVMValueRef values[4];
		values[0] = moduleBuilder->ConstF32(0);
		values[1] = moduleBuilder->ConstF32(0);
		values[2] = moduleBuilder->ConstF32(0);
		values[3] = moduleBuilder->ConstF32(1);

		auto vector = LLVMConstVector(values, 4);
		for (auto i = 0u; i < 4; i++)
		{
			if (gsl::at(information->Normal.OffsetValues, i) != INVALID_OFFSET)
			{
				const auto value = EmitGetNormalChannel<float>(moduleBuilder, information, sourcePointer, i);
				vector = moduleBuilder->CreateInsertElement(vector, value, moduleBuilder->ConstI32(i));
			}
		}

		return vector;
	}
	else
	{
		LLVMValueRef values[4];
		values[0] = moduleBuilder->ConstI32(0);
		values[1] = moduleBuilder->ConstI32(0);
		values[2] = moduleBuilder->ConstI32(0);
		values[3] = moduleBuilder->ConstI32(1);

		const auto isSigned = information->Base == BaseType::SInt;

		auto vector = LLVMConstVector(values, 4);
		for (auto i = 0u; i < 4; i++)
		{
			if (gsl::at(information->Normal.OffsetValues, i) != INVALID_OFFSET)
			{
				const auto value = isSigned
					                   ? EmitGetNormalChannel<int32_t>(moduleBuilder, information, sourcePointer, i)
					                   : EmitGetNormalChannel<uint32_t>(moduleBuilder, information, sourcePointer, i);
				vector = moduleBuilder->CreateInsertElement(vector, value, moduleBuilder->ConstI32(i));
			}
		}

		return vector;
	}
}

static LLVMValueRef EmitGetPackedPixel(CompiledModuleBuilder* moduleBuilder, const FormatInformation* information, LLVMValueRef sourcePointer, LLVMTypeRef resultType)
{
	const auto sourceType = LLVMIntTypeInContext(moduleBuilder->context, information->TotalSize * 8);
	sourcePointer = moduleBuilder->CreateBitCast(sourcePointer, LLVMPointerType(sourceType, 0));
	const auto source = moduleBuilder->CreateLoad(sourcePointer);

	switch (information->Format)
	{
	case VK_FORMAT_B10G11R11_UFLOAT_PACK32:
		{
			const auto destinationPtr = moduleBuilder->CreateAlloca(resultType);
			LLVMTypeRef argumentTypes[]
			{
				LLVMTypeOf(source),
				LLVMTypeOf(destinationPtr),
			};
			const auto functionType = LLVMFunctionType(LLVMVoidTypeInContext(moduleBuilder->context), argumentTypes, 2, false);
			const auto conversionFunction = LLVMAddFunction(moduleBuilder->module, "B10G11R11ToFloat", functionType);
			LLVMSetLinkage(conversionFunction, LLVMExternalLinkage);
			moduleBuilder->CreateCall(conversionFunction, {source, destinationPtr});
			return moduleBuilder->CreateLoad(destinationPtr);
		}

	case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32:
		{
			const auto destinationPtr = moduleBuilder->CreateAlloca(resultType);
			LLVMTypeRef argumentTypes[]
			{
				LLVMTypeOf(source),
				LLVMTypeOf(destinationPtr),
			};
			const auto functionType = LLVMFunctionType(LLVMVoidTypeInContext(moduleBuilder->context), argumentTypes, 2, false);
			const auto conversionFunction = LLVMAddFunction(moduleBuilder->module, "E5B9G9R9ToFloat", functionType);
			LLVMSetLinkage(conversionFunction, LLVMExternalLinkage);
			moduleBuilder->CreateCall(conversionFunction, {source, destinationPtr});
			return moduleBuilder->CreateLoad(destinationPtr);
		}
	}

	LLVMValueRef values[4];
	if (LLVMGetTypeKind(LLVMGetElementType(resultType)) == LLVMFloatTypeKind)
	{
		values[0] = moduleBuilder->ConstF32(0);
		values[1] = moduleBuilder->ConstF32(0);
		values[2] = moduleBuilder->ConstF32(0);
		values[3] = moduleBuilder->ConstF32(1);
	}
	else
	{
		values[0] = moduleBuilder->ConstI32(0);
		values[1] = moduleBuilder->ConstI32(0);
		values[2] = moduleBuilder->ConstI32(0);
		values[3] = moduleBuilder->ConstI32(1);
	}
	
	auto vector = LLVMConstVector(values, 4);
	for (auto i = 0u; i < 4; i++)
	{
		if (gsl::at(information->Packed.BitValues, i) != INVALID_OFFSET)
		{
			const auto value = EmitGetPackedChannel(moduleBuilder, information, source, LLVMGetElementType(resultType), i);
			vector = moduleBuilder->CreateInsertElement(vector, value, moduleBuilder->ConstI32(i));
		}
	}

	return vector;
}

static LLVMValueRef EmitGetDepthStencilPixel(CompiledModuleBuilder* moduleBuilder, const FormatInformation* information, LLVMValueRef sourcePointer, LLVMTypeRef resultType)
{
	LLVMValueRef value;
	switch (information->Format)
	{
	case VK_FORMAT_D16_UNORM:
	case VK_FORMAT_D16_UNORM_S8_UINT:
		sourcePointer = moduleBuilder->CreateBitCast(sourcePointer, LLVMPointerType(LLVMInt16TypeInContext(moduleBuilder->context), 0));
		value = moduleBuilder->CreateLoad(sourcePointer);
		value = EmitConvert<uint16_t, float>(moduleBuilder, value);
		break;

	case VK_FORMAT_D24_UNORM_S8_UINT:
	case VK_FORMAT_X8_D24_UNORM_PACK32:
		sourcePointer = moduleBuilder->CreateBitCast(sourcePointer, LLVMPointerType(LLVMInt32TypeInContext(moduleBuilder->context), 0));
		value = moduleBuilder->CreateLoad(sourcePointer);
		value = moduleBuilder->CreateAnd(value, moduleBuilder->ConstU32(0x00FFFFFF));
		value = moduleBuilder->CreateUIToFP(value, LLVMFloatTypeInContext(moduleBuilder->context));
		value = moduleBuilder->CreateFDiv(value, moduleBuilder->ConstF32(0x00FFFFFF));
		break;

	case VK_FORMAT_D32_SFLOAT:
	case VK_FORMAT_D32_SFLOAT_S8_UINT:
		sourcePointer = moduleBuilder->CreateBitCast(sourcePointer, LLVMPointerType(LLVMFloatTypeInContext(moduleBuilder->context), 0));
		value = moduleBuilder->CreateLoad(sourcePointer);
		break;

	default:
		TODO_ERROR();
	}

	LLVMValueRef values[]
	{
		moduleBuilder->ConstF32(0),
		moduleBuilder->ConstF32(0),
		moduleBuilder->ConstF32(0),
		moduleBuilder->ConstF32(1),
	};
	auto vector = LLVMConstVector(values, 4);
	vector = moduleBuilder->CreateInsertElement(vector, value, moduleBuilder->ConstI32(0));
	return vector;
}

LLVMValueRef EmitGetPixel(CompiledModuleBuilder* moduleBuilder, LLVMValueRef sourcePointer, LLVMTypeRef resultType, const FormatInformation* information)
{
	switch (information->Type)
	{
	case FormatType::Normal:
		return EmitGetNormalPixel(moduleBuilder, information, sourcePointer, resultType);
		
	case FormatType::Packed:
		return EmitGetPackedPixel(moduleBuilder, information, sourcePointer, resultType);
		
	case FormatType::DepthStencil:
		return EmitGetDepthStencilPixel(moduleBuilder, information, sourcePointer, resultType);
			
	default:
		FATAL_ERROR();
	}
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

	LLVMValueRef CreateMaxNum(LLVMValueRef lhs, LLVMValueRef rhs)
	{
		return CreateIntrinsic<2>(Intrinsics::maxnum, {
			                          lhs,
			                          rhs,
		                          });
	}

	LLVMValueRef CreateMinNum(LLVMValueRef lhs, LLVMValueRef rhs)
	{
		return CreateIntrinsic<2>(Intrinsics::minnum, {
			                          lhs,
			                          rhs,
		                          });
	}
};

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
		this->currentFunction = function;

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
			value = EmitConvert<uint16_t, float>(this, value);
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

class GetStencilPixelCompiledModuleBuilder final : public PixelCompiledModuleBuilder
{
public:
	GetStencilPixelCompiledModuleBuilder(CPJit* jit, const FormatInformation* information) :
		PixelCompiledModuleBuilder{jit, information}
	{
		assert(information->Type == FormatType::DepthStencil);
		assert(information->DepthStencil.StencilOffset != INVALID_OFFSET);
	}

protected:
	void MainCompilation() override
	{
		std::vector<LLVMTypeRef> parameters
		{
			LLVMPointerType(LLVMInt8TypeInContext(context), 0),
		};
		const auto functionType = LLVMFunctionType(LLVMInt8TypeInContext(context), parameters.data(), static_cast<uint32_t>(parameters.size()), false);
		const auto function = LLVMAddFunction(module, "main", functionType);
		LLVMSetLinkage(function, LLVMExternalLinkage);
		this->currentFunction = function;

		const auto basicBlock = LLVMAppendBasicBlockInContext(context, function, "");
		LLVMPositionBuilderAtEnd(builder, basicBlock);

		auto sourcePtr = LLVMGetParam(function, 0);
		sourcePtr = CreateGEP(sourcePtr, information->DepthStencil.StencilOffset);
		const auto value = CreateLoad(sourcePtr);

		LLVMBuildRet(builder, value);
	}
};

CP_DLL_EXPORT FunctionPointer CompileGetPixelStencil(CPJit* jit, const FormatInformation* information)
{
	return GetStencilPixelCompiledModuleBuilder(jit, information).Compile()->getFunctionPointer("main");
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
		this->currentFunction = function;

		const auto basicBlock = LLVMAppendBasicBlockInContext(context, function, "");
		LLVMPositionBuilderAtEnd(builder, basicBlock);

		const auto sourcePtr = LLVMGetParam(function, 0);
		const auto destinationPtr = LLVMGetParam(function, 1);

		// case FormatType::Compressed:
		// 	// TODO
		// 	// {
		// 	// 	const auto subX = &*(function->arg_begin() + 2);
		// 	// 	const auto subY = &*(function->arg_begin() + 3);
		// 	//
		// 	// 	// The type of the block
		// 	// 	llvm::Type* sourceType;
		// 	// 	switch (information->TotalSize)
		// 	// 	{
		// 	// 	case 4:
		// 	// 		sourceType = builder.getInt32Ty();
		// 	// 		break;
		// 	//
		// 	// 	case 8:
		// 	// 		sourceType = builder.getInt64Ty();
		// 	// 		break;
		// 	//
		// 	// 	case 16:
		// 	// 		sourceType = builder.getInt128Ty();
		// 	// 		break;
		// 	//
		// 	// 	default:
		// 	// 		TODO_ERROR();
		// 	// 	}
		// 	//
		// 	// 	sourcePtr = builder.CreateBitCast(sourcePtr, llvm::PointerType::get(sourceType, 0));
		// 	// 	const auto source = builder.CreateLoad(sourcePtr);
		// 	// 	const auto pixels = builder.CreateAlloca(builder.getFloatTy(), builder.getInt32(4 * information->Compressed.BlockWidth * information->Compressed.BlockHeight));
		// 	//
		// 	// 	// Get function that handles decompression
		// 	// 	llvm::Function* decompress;
		// 	// 	switch (information->Format)
		// 	// 	{
		// 	// 	case VK_FORMAT_BC1_RGB_UNORM_BLOCK:
		// 	// 	case VK_FORMAT_BC1_RGB_SRGB_BLOCK:
		// 	// 	case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:
		// 	// 	case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:
		// 	// 	case VK_FORMAT_BC2_UNORM_BLOCK:
		// 	// 	case VK_FORMAT_BC2_SRGB_BLOCK:
		// 	// 	case VK_FORMAT_BC3_UNORM_BLOCK:
		// 	// 	case VK_FORMAT_BC3_SRGB_BLOCK:
		// 	// 	case VK_FORMAT_BC4_UNORM_BLOCK:
		// 	// 	case VK_FORMAT_BC4_SNORM_BLOCK:
		// 	// 	case VK_FORMAT_BC5_UNORM_BLOCK:
		// 	// 	case VK_FORMAT_BC5_SNORM_BLOCK:
		// 	// 	case VK_FORMAT_BC6H_UFLOAT_BLOCK:
		// 	// 	case VK_FORMAT_BC6H_SFLOAT_BLOCK:
		// 	// 	case VK_FORMAT_BC7_UNORM_BLOCK:
		// 	// 	case VK_FORMAT_BC7_SRGB_BLOCK:
		// 	// 		TODO_ERROR();
		// 	// 		
		// 	// 	case VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK:
		// 	// 	case VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK:
		// 	// 		decompress = llvm::Function::Create(
		// 	// 			llvm::FunctionType::get(builder.getVoidTy(), {source->getType(), pixels->getType()}, false), 
		// 	// 			llvm::GlobalValue::ExternalLinkage, "DecompressETC2R8G8B8", module.get());
		// 	// 		break;
		// 	// 		
		// 	// 	case VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK:
		// 	// 	case VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK:
		// 	// 	case VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK:
		// 	// 	case VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK:
		// 	// 		TODO_ERROR();
		// 	// 		
		// 	// 	case VK_FORMAT_EAC_R11_UNORM_BLOCK:
		// 	// 	case VK_FORMAT_EAC_R11_SNORM_BLOCK:
		// 	// 	case VK_FORMAT_EAC_R11G11_UNORM_BLOCK:
		// 	// 	case VK_FORMAT_EAC_R11G11_SNORM_BLOCK:
		// 	// 		TODO_ERROR();
		// 	// 		
		// 	// 	case VK_FORMAT_ASTC_4x4_UNORM_BLOCK:
		// 	// 	case VK_FORMAT_ASTC_4x4_SRGB_BLOCK:
		// 	// 	case VK_FORMAT_ASTC_5x4_UNORM_BLOCK:
		// 	// 	case VK_FORMAT_ASTC_5x4_SRGB_BLOCK:
		// 	// 	case VK_FORMAT_ASTC_5x5_UNORM_BLOCK:
		// 	// 	case VK_FORMAT_ASTC_5x5_SRGB_BLOCK:
		// 	// 	case VK_FORMAT_ASTC_6x5_UNORM_BLOCK:
		// 	// 	case VK_FORMAT_ASTC_6x5_SRGB_BLOCK:
		// 	// 	case VK_FORMAT_ASTC_6x6_UNORM_BLOCK:
		// 	// 	case VK_FORMAT_ASTC_6x6_SRGB_BLOCK:
		// 	// 	case VK_FORMAT_ASTC_8x5_UNORM_BLOCK:
		// 	// 	case VK_FORMAT_ASTC_8x5_SRGB_BLOCK:
		// 	// 	case VK_FORMAT_ASTC_8x6_UNORM_BLOCK:
		// 	// 	case VK_FORMAT_ASTC_8x6_SRGB_BLOCK:
		// 	// 	case VK_FORMAT_ASTC_8x8_UNORM_BLOCK:
		// 	// 	case VK_FORMAT_ASTC_8x8_SRGB_BLOCK:
		// 	// 	case VK_FORMAT_ASTC_10x5_UNORM_BLOCK:
		// 	// 	case VK_FORMAT_ASTC_10x5_SRGB_BLOCK:
		// 	// 	case VK_FORMAT_ASTC_10x6_UNORM_BLOCK:
		// 	// 	case VK_FORMAT_ASTC_10x6_SRGB_BLOCK:
		// 	// 	case VK_FORMAT_ASTC_10x8_UNORM_BLOCK:
		// 	// 	case VK_FORMAT_ASTC_10x8_SRGB_BLOCK:
		// 	// 	case VK_FORMAT_ASTC_10x10_UNORM_BLOCK:
		// 	// 	case VK_FORMAT_ASTC_10x10_SRGB_BLOCK:
		// 	// 	case VK_FORMAT_ASTC_12x10_UNORM_BLOCK:
		// 	// 	case VK_FORMAT_ASTC_12x10_SRGB_BLOCK:
		// 	// 	case VK_FORMAT_ASTC_12x12_UNORM_BLOCK:
		// 	// 	case VK_FORMAT_ASTC_12x12_SRGB_BLOCK:
		// 	// 		TODO_ERROR();
		// 	// 		
		// 	// 	case VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG:
		// 	// 	case VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG:
		// 	// 	case VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG:
		// 	// 	case VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG:
		// 	// 	case VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG:
		// 	// 	case VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG:
		// 	// 	case VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG:
		// 	// 	case VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG:
		// 	// 		TODO_ERROR();
		// 	// 		
		// 	// 	default:
		// 	// 		TODO_ERROR();
		// 	// 	}
		// 	//
		// 	// 	builder.CreateCall(decompress, {source, pixels});
		// 	//
		// 	// 	// Get the offset to the pixels we need
		// 	// 	auto offset = builder.CreateMul(subY, builder.getInt32(information->Compressed.BlockWidth));
		// 	// 	offset = builder.CreateAdd(offset, subX);
		// 	// 	offset = builder.CreateMul(offset, builder.getInt32(4));
		// 	//
		// 	// 	// Set R channel
		// 	// 	auto dst = builder.CreateConstGEP1_32(destinationPtr, 0);
		// 	// 	llvm::Value* value = builder.CreateLoad(builder.CreateGEP(pixels, offset));
		// 	// 	if (information->Base == BaseType::SRGB)
		// 	// 	{
		// 	// 		value = EmitSRGBToLinear(function, builder, value);
		// 	// 	}
		// 	// 	builder.CreateStore(value, dst);
		// 	//
		// 	// 	// Set G channel
		// 	// 	dst = builder.CreateConstGEP1_32(destinationPtr, 1);
		// 	// 	value = builder.CreateLoad(builder.CreateGEP(pixels, builder.CreateAdd(offset, builder.getInt32(1))));
		// 	// 	if (information->Base == BaseType::SRGB)
		// 	// 	{
		// 	// 		value = EmitSRGBToLinear(function, builder, value);
		// 	// 	}
		// 	// 	builder.CreateStore(value, dst);
		// 	//
		// 	// 	// Set B channel
		// 	// 	dst = builder.CreateConstGEP1_32(destinationPtr, 2);
		// 	// 	value = builder.CreateLoad(builder.CreateGEP(pixels, builder.CreateAdd(offset, builder.getInt32(2))));
		// 	// 	if (information->Base == BaseType::SRGB)
		// 	// 	{
		// 	// 		value = EmitSRGBToLinear(function, builder, value);
		// 	// 	}
		// 	// 	builder.CreateStore(value, dst);
		// 	//
		// 	// 	// Set A channel
		// 	// 	dst = builder.CreateConstGEP1_32(destinationPtr, 3);
		// 	// 	value = builder.CreateLoad(builder.CreateGEP(pixels, builder.CreateAdd(offset, builder.getInt32(3))));
		// 	// 	builder.CreateStore(value, dst);
		// 	// 	
		// 	// 	break;
		// 	// }
		// 	TODO_ERROR();
		const auto result = EmitGetPixel(this, sourcePtr, LLVMVectorType(GetType<ReturnType>(), 4), information);
		
		for (auto i = 0u; i < 4; i++)
		{
			const auto destination = CreateGEP(destinationPtr, i);
			CreateStore(CreateExtractElement(result, ConstI32(i)), destination);
		}

		LLVMBuildRetVoid(builder);
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
		this->currentFunction = function;

		const auto basicBlock = LLVMAppendBasicBlockInContext(context, function, "");
		LLVMPositionBuilderAtEnd(builder, basicBlock);

		const auto destinationPtr = LLVMGetParam(function, 0);
		const auto depthSource = LLVMGetParam(function, 1);
		const auto stencilSource = LLVMGetParam(function, 2);
		
		if (information->DepthStencil.DepthOffset != INVALID_OFFSET)
		{
			auto dst = destinationPtr;
			auto value = CreateMinNum(CreateMaxNum(depthSource, ConstF32(0)), ConstF32(1));
			switch (information->Format)
			{
			case VK_FORMAT_D16_UNORM:
			case VK_FORMAT_D16_UNORM_S8_UINT:
				value = EmitConvert<float, uint16_t>(this, value);
				dst = LLVMBuildBitCast(builder, dst, LLVMPointerType(LLVMInt16TypeInContext(context), 0), "");
				break;
		
			case VK_FORMAT_D24_UNORM_S8_UINT:
			case VK_FORMAT_X8_D24_UNORM_PACK32:
				value = LLVMBuildFMul(builder, value, ConstF32(0x00FFFFFF), "");
				value = CreateIntrinsic<1>(Intrinsics::round, {value});
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
				TODO_ERROR();
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

template<typename SourceType>
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
			LLVMPointerType(GetType<SourceType>(), 0),
		};
		const auto functionType = LLVMFunctionType(LLVMVoidType(), parameters.data(), static_cast<uint32_t>(parameters.size()), false);
		const auto function = LLVMAddFunction(module, "main", functionType);
		LLVMSetLinkage(function, LLVMExternalLinkage);
		this->currentFunction = function;

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
			TODO_ERROR();

		case FormatType::Planar:
			TODO_ERROR();

		case FormatType::PlanarSamplable:
			TODO_ERROR();

		default: TODO_ERROR();
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
			TODO_ERROR();
			
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
				TODO_ERROR();
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

	void CompileSetPacked(LLVMValueRef function, LLVMValueRef destinationPtr, LLVMValueRef sourcePtr)
	{
		assert(information->Type == FormatType::Packed);

		const auto resultType = LLVMIntTypeInContext(context, information->TotalSize * 8);
		auto value = LLVMConstInt(resultType, 0, false);

		for (auto i = 0u; i < 4; i++)
		{
			if (gsl::at(information->Packed.BitValues, i))
			{
				value = CompileSetPackedChannel(function, value, resultType, sourcePtr, destinationPtr, i);
			}
		}

		const auto dst = CreateBitCast(destinationPtr, LLVMPointerType(resultType, 0));
		CreateStore(value, dst);
	}

	LLVMValueRef CompileSetNormalChannel(LLVMValueRef function, LLVMValueRef inputValue, int channel)
	{
		switch (information->Base)
		{
		case BaseType::UNorm:
			{
				assert((std::is_same<SourceType, float>::value));
				const auto value = CreateMinNum(CreateMaxNum(inputValue, ConstF32(0)), ConstF32(1));
				switch (information->ElementSize)
				{
				case 1:
					return EmitConvert<SourceType, uint8_t>(this, value);
				case 2:
					return EmitConvert<SourceType, uint16_t>(this, value);
				case 4:
					return EmitConvert<SourceType, uint32_t>(this, value);
				default:
					TODO_ERROR();
				}
			}
			
		case BaseType::SNorm:
			{
				assert((std::is_same<SourceType, float>::value));
				const auto value = CreateMinNum(CreateMaxNum(inputValue, ConstF32(-1)), ConstF32(1));
				switch (information->ElementSize)
				{
				case 1:
					return EmitConvert<SourceType, int8_t>(this, value);
				case 2:
					return EmitConvert<SourceType, int16_t>(this, value);
				case 4:
					return EmitConvert<SourceType, int32_t>(this, value);
				default:
					TODO_ERROR();
				}
			}
		
		case BaseType::UScaled:
			TODO_ERROR();
			
		case BaseType::SScaled:
			TODO_ERROR();
			
		case BaseType::UInt:
			assert((std::is_same<SourceType, uint32_t>::value));
			switch (information->ElementSize)
			{
			case 1:
				{
					const auto tmp = CreateICmpULT(inputValue, ConstU32(std::numeric_limits<uint8_t>::max()));
					inputValue = CreateSelect(tmp, inputValue, ConstU32(std::numeric_limits<uint8_t>::max()));
					return EmitConvert<SourceType, uint8_t>(this, inputValue);
				}
			
			case 2:
				{
					const auto tmp = CreateICmpULT(inputValue, ConstU32(std::numeric_limits<uint16_t>::max()));
					inputValue = CreateSelect(tmp, inputValue, ConstU32(std::numeric_limits<uint16_t>::max()));
					return EmitConvert<SourceType, uint16_t>(this, inputValue);
				}
			
			case 4:
				return EmitConvert<SourceType, uint32_t>(this, inputValue);
			
			default:
				TODO_ERROR();
			}
			
		case BaseType::SInt:
			assert((std::is_same<SourceType, int32_t>::value));
			switch (information->ElementSize)
			{
			case 1:
				{
					auto tmp = CreateICmpSGT(inputValue, ConstI32(std::numeric_limits<int8_t>::min()));
					inputValue = CreateSelect(tmp, inputValue, ConstI32(std::numeric_limits<int8_t>::min()));

					tmp = CreateICmpSLT(inputValue, ConstI32(std::numeric_limits<int8_t>::max()));
					inputValue = CreateSelect(tmp, inputValue, ConstI32(std::numeric_limits<int8_t>::max()));
					
					return EmitConvert<SourceType, int8_t>(this, inputValue);
				}
			
			case 2:
				{
					auto tmp = CreateICmpSGT(inputValue, ConstI32(std::numeric_limits<int16_t>::min()));
					inputValue = CreateSelect(tmp, inputValue, ConstI32(std::numeric_limits<int16_t>::min()));

					tmp = CreateICmpSLT(inputValue, ConstI32(std::numeric_limits<int16_t>::max()));
					inputValue = CreateSelect(tmp, inputValue, ConstI32(std::numeric_limits<int16_t>::max()));
					
					return EmitConvert<SourceType, int16_t>(this, inputValue);
				}
			
			case 4:
				return EmitConvert<SourceType, int32_t>(this, inputValue);
			
			default:
				TODO_ERROR();
			}
			
		case BaseType::UFloat:
			TODO_ERROR();
			
		case BaseType::SFloat:
			assert((std::is_same<SourceType, float>::value));
			switch (information->ElementSize)
			{
			case 2:
				return EmitConvert<SourceType, half>(this, inputValue);
			
			case 4:
				return EmitConvert<SourceType, float>(this, inputValue);
			
			case 8:
				return EmitConvert<SourceType, double>(this, inputValue);
			
			default:
				TODO_ERROR();
			}
			
		case BaseType::SRGB:
			{
				assert((std::is_same<SourceType, float>::value));
				auto value = CreateMinNum(CreateMaxNum(inputValue, ConstF32(0)), ConstF32(1));
				value = channel == 3 ? value : EmitLinearToSRGB(function, value);
				switch (information->ElementSize)
				{
				case 1:
					return EmitConvert<SourceType, uint8_t>(this, value);
				case 2:
					return EmitConvert<SourceType, uint16_t>(this, value);
				case 4:
					return EmitConvert<SourceType, uint32_t>(this, value);
				default:
					TODO_ERROR();
				}
			}
		
		default:
			FATAL_ERROR();
		}
	}

	LLVMValueRef CompileSetPackedChannel(LLVMValueRef function, LLVMValueRef inputValue, LLVMTypeRef resultType, LLVMValueRef sourcePtr, LLVMValueRef destinationPtr, int channel)
	{
		switch (information->Format)
		{
		case VK_FORMAT_B10G11R11_UFLOAT_PACK32:
			{
				assert((std::is_same<SourceType, float>::value));
				auto conversionFunction = LLVMGetNamedFunction(module, "FloatToB10G11R11");
				if (!conversionFunction)
				{
					auto sourcePtrType = LLVMTypeOf(sourcePtr);
					const auto functionType = LLVMFunctionType(resultType, &sourcePtrType, 1, false);
					conversionFunction = LLVMAddFunction(module, "FloatToB10G11R11", functionType);
					LLVMSetLinkage(conversionFunction, LLVMExternalLinkage);
				}
				return CreateCall(conversionFunction, {sourcePtr});
			}
					
		case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32:
			{
				assert((std::is_same<SourceType, float>::value));
				auto conversionFunction = LLVMGetNamedFunction(module, "FloatToE5B9G9R9");
				if (!conversionFunction)
				{
					auto sourcePtrType = LLVMTypeOf(sourcePtr);
					const auto functionType = LLVMFunctionType(resultType, &sourcePtrType, 1, false);
					conversionFunction = LLVMAddFunction(module, "FloatToE5B9G9R9", functionType);
					LLVMSetLinkage(conversionFunction, LLVMExternalLinkage);
				}
				return CreateCall(conversionFunction, {sourcePtr});
			}
		}
		
		const auto bits = gsl::at(information->Packed.BitValues, channel);
		const auto mask = (1ULL << bits) - 1;
		const auto offset = gsl::at(information->Packed.OffsetValues, channel);

		// Get source from array
		auto source = CreateGEP(sourcePtr, channel);
		source = CreateLoad(source);

		LLVMValueRef value;
		switch (information->Base)
		{
		case BaseType::UNorm:
			assert((std::is_same<SourceType, float>::value));
			
			// Clamp between [0, 1]
			source = CreateMinNum(CreateMaxNum(source, ConstF32(0)), ConstF32(1));
			
			// Float to UInt
			value = CreateFMul(source, ConstF32(mask));
			value = CreateIntrinsic<1>(Intrinsics::round, {value});
			value = CreateFPToUI(value, resultType);
			break;
			
		case BaseType::SNorm:
			assert((std::is_same<SourceType, float>::value));

			// Clamp between [-1, 1]
			source = CreateMinNum(CreateMaxNum(source, ConstF32(-1)), ConstF32(1));
			
			// Float to UInt
			value = CreateFMul(source, ConstF32(mask >> 1));
			value = CreateIntrinsic<1>(Intrinsics::round, {value});
			value = CreateFPToUI(value, resultType);
			break;
		
		case BaseType::UScaled:
			TODO_ERROR();
			
		case BaseType::SScaled:
			TODO_ERROR();
			
		case BaseType::UInt:
			assert((std::is_same<SourceType, uint32_t>::value));

			source = CreateSelect(CreateICmpULT(source, ConstU32(mask)), source, ConstU32(mask));

			value = CreateAnd(source, ConstI32(mask));
			value = CreateZExtOrTrunc(value, resultType);
			break;
			
		case BaseType::SInt:
			{
				assert((std::is_same<SourceType, int32_t>::value));
				const auto min = -(1LL << (bits - 1));
				const auto max = (1LL << (bits - 1)) - 1;

				source = CreateSelect(CreateICmpSGT(source, ConstI32(min)), source, ConstI32(min));
				source = CreateSelect(CreateICmpSLT(source, ConstI32(max)), source, ConstI32(max));

				value = CreateAnd(source, ConstI32(mask));
				value = CreateZExtOrTrunc(value, resultType);
				break;
			}
			
		case BaseType::UFloat:
			assert((std::is_same<SourceType, float>::value));
			TODO_ERROR();
			
		case BaseType::SFloat:
			assert((std::is_same<SourceType, float>::value));
			TODO_ERROR();
			
		case BaseType::SRGB:
			assert((std::is_same<SourceType, float>::value));
			
			// Clamp between [0, 1]
			source = CreateMinNum(CreateMaxNum(source, ConstF32(0)), ConstF32(1));

			source = channel == 3 ? source : EmitLinearToSRGB(function, source);
						
			// Float to UInt
			value = CreateFMul(source, ConstF32(mask));
			value = CreateIntrinsic<1>(Intrinsics::round, {value});
			value = CreateFPToUI(value, resultType);
			break;
		
		default:
			FATAL_ERROR();
		}

		// Shift and or with old value
		value = CreateShl(value, LLVMConstInt(resultType, offset, false));
		return CreateOr(value, inputValue);
	}

	LLVMValueRef EmitLinearToSRGB(LLVMValueRef function, LLVMValueRef inputValue)
	{
		const auto trueBlock = LLVMAppendBasicBlockInContext(context, function, "");
		const auto falseBlock = LLVMAppendBasicBlockInContext(context, function, "");
		const auto nextBlock = LLVMAppendBasicBlockInContext(context, function, "");

		const auto comparison = CreateFCmpUGT(inputValue, ConstF32(0.0031308f));
		CreateCondBr(comparison, trueBlock, falseBlock);

		LLVMPositionBuilderAtEnd(builder, falseBlock);
		const auto falseValue = CreateFMul(inputValue, ConstF32(12.92f));
		CreateBr(nextBlock);

		LLVMPositionBuilderAtEnd(builder, trueBlock);
		auto trueValue = CreateIntrinsic<2>(Intrinsics::pow, {inputValue, ConstF32(1.0f / 2.4f)});
		trueValue = CreateFMul(trueValue, ConstF32(1.055f));
		trueValue = CreateFAdd(trueValue, ConstF32(-0.055f));
		CreateBr(nextBlock);

		LLVMPositionBuilderAtEnd(builder, nextBlock);
		const auto value = CreatePhi(LLVMFloatTypeInContext(context));
		std::array<LLVMValueRef, 2> incoming
		{
			trueValue,
			falseValue,
		};
		std::array<LLVMBasicBlockRef, 2> incomingBlocks
		{
			trueBlock,
			falseBlock,
		};
		LLVMAddIncoming(value, incoming.data(), incomingBlocks.data(), 2);
		return value;
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