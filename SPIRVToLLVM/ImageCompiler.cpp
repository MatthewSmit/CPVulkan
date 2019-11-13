#include "ImageCompiler.h"

#include <Formats.h>
#include <Half.h>

#include <llvm/ExecutionEngine/Orc/CompileUtils.h>
#include <llvm/ExecutionEngine/Orc/LLJIT.h>
#include <llvm/ExecutionEngine/SectionMemoryManager.h>
#include <llvm/IR/IRBuilder.h>

#include <iostream>

static void Dump(llvm::Module* llvmModule)
{
	std::string dump;
	llvm::raw_string_ostream DumpStream(dump);
	llvmModule->print(DumpStream, nullptr);
	std::cout << dump << std::endl;
}

template<typename T>
llvm::Type* GetType(llvm::IRBuilder<>& builder);

template<>
llvm::Type* GetType<half>(llvm::IRBuilder<>& builder)
{
	return builder.getHalfTy();
}

template<>
llvm::Type* GetType<float>(llvm::IRBuilder<>& builder)
{
	return builder.getFloatTy();
}

template<>
llvm::Type* GetType<double>(llvm::IRBuilder<>& builder)
{
	return builder.getDoubleTy();
}

template<>
llvm::Type* GetType<int8_t>(llvm::IRBuilder<>& builder)
{
	return builder.getInt8Ty();
}

template<>
llvm::Type* GetType<int16_t>(llvm::IRBuilder<>& builder)
{
	return builder.getInt16Ty();
}

template<>
llvm::Type* GetType<int32_t>(llvm::IRBuilder<>& builder)
{
	return builder.getInt32Ty();
}

template<>
llvm::Type* GetType<int64_t>(llvm::IRBuilder<>& builder)
{
	return builder.getInt64Ty();
}

template<>
llvm::Type* GetType<uint8_t>(llvm::IRBuilder<>& builder)
{
	return builder.getInt8Ty();
}

template<>
llvm::Type* GetType<uint16_t>(llvm::IRBuilder<>& builder)
{
	return builder.getInt16Ty();
}

template<>
llvm::Type* GetType<uint32_t>(llvm::IRBuilder<>& builder)
{
	return builder.getInt32Ty();
}

template<>
llvm::Type* GetType<uint64_t>(llvm::IRBuilder<>& builder)
{
	return builder.getInt64Ty();
}

static uint32_t GetPackedBits(const FormatInformation* information, int index)
{
	switch (index)
	{
	case 0:
		return information->Packed.RedBits;
	case 1:
		return information->Packed.GreenBits;
	case 2:
		return information->Packed.BlueBits;
	case 3:
		return information->Packed.AlphaBits;
	}
	FATAL_ERROR();
}

static uint32_t GetPackedOffset(const FormatInformation* information, int index)
{
	switch (index)
	{
	case 0:
		return information->Packed.RedOffset;
	case 1:
		return information->Packed.GreenOffset;
	case 2:
		return information->Packed.BlueOffset;
	case 3:
		return information->Packed.AlphaOffset;
	}
	FATAL_ERROR();
}

static llvm::Value* EmitConvertFloat(llvm::IRBuilder<>& builder, llvm::Value* inputValue, llvm::Type* outputType)
{
	if (inputValue->getType()->getPrimitiveSizeInBits() < outputType->getPrimitiveSizeInBits())
	{
		return builder.CreateFPExt(inputValue, outputType);
	}
	
	if (inputValue->getType()->getPrimitiveSizeInBits() > outputType->getPrimitiveSizeInBits())
	{
		return builder.CreateFPTrunc(inputValue, outputType);
	}

	return inputValue;
}

static llvm::Value* EmitConvertFloatInt(llvm::IRBuilder<>& builder, llvm::Value* inputValue, int64_t maxValue, llvm::Type* outputType)
{
	auto value = builder.CreateFMul(inputValue, llvm::ConstantFP::get(inputValue->getType(), maxValue));
	value = builder.CreateUnaryIntrinsic(llvm::Intrinsic::ID::round, value);
	return builder.CreateFPToSI(value, outputType);
}

static llvm::Value* EmitConvertFloatUInt(llvm::IRBuilder<>& builder, llvm::Value* inputValue, uint64_t maxValue, llvm::Type* outputType)
{
	auto value = builder.CreateFMul(inputValue, llvm::ConstantFP::get(inputValue->getType(), maxValue));
	value = builder.CreateUnaryIntrinsic(llvm::Intrinsic::ID::round, value);
	return builder.CreateFPToUI(value, outputType);
}

static llvm::Value* EmitConvertInt(llvm::IRBuilder<>& builder, llvm::Value* inputValue, llvm::Type* outputType, bool inputSigned)
{
	if (inputSigned)
	{
		return builder.CreateSExtOrTrunc(inputValue, outputType);
	}

	return builder.CreateZExtOrTrunc(inputValue, outputType);
}

static llvm::Value* EmitConvertUIntFloat(llvm::IRBuilder<>& builder, llvm::Value* inputValue, uint64_t maxValue, llvm::Type* outputType)
{
	const auto value = builder.CreateUIToFP(inputValue, outputType);
	return builder.CreateFDiv(value, llvm::ConstantFP::get(outputType, maxValue));
}

static llvm::Value* EmitLinearToSRGB(llvm::Function* currentFunction, llvm::IRBuilder<>& builder, llvm::Value* inputValue)
{
	const auto trueBlock = llvm::BasicBlock::Create(currentFunction->getContext(), "", currentFunction);
	const auto falseBlock = llvm::BasicBlock::Create(currentFunction->getContext(), "", currentFunction);
	const auto nextBlock = llvm::BasicBlock::Create(currentFunction->getContext(), "", currentFunction);

	const auto comparison = builder.CreateFCmpUGT(inputValue, llvm::ConstantFP::get(builder.getFloatTy(), 0.0031308));
	builder.CreateCondBr(comparison, trueBlock, falseBlock);

	builder.SetInsertPoint(falseBlock);
	const auto falseValue = builder.CreateFMul(inputValue, llvm::ConstantFP::get(builder.getFloatTy(), 12.92));
	builder.CreateBr(nextBlock);

	builder.SetInsertPoint(trueBlock);
	llvm::Value* trueValue = builder.CreateBinaryIntrinsic(llvm::Intrinsic::pow, inputValue, llvm::ConstantFP::get(builder.getFloatTy(), 1.0 / 2.4));
	trueValue = builder.CreateFMul(trueValue, llvm::ConstantFP::get(builder.getFloatTy(), 1.055));
	trueValue = builder.CreateFAdd(trueValue, llvm::ConstantFP::get(builder.getFloatTy(), -0.055));
	builder.CreateBr(nextBlock);

	builder.SetInsertPoint(nextBlock);
	auto value = builder.CreatePHI(builder.getFloatTy(), 2);
	value->addIncoming(trueValue, trueBlock);
	value->addIncoming(falseValue, falseBlock);
	return value;
}

template<typename InputType, typename OutputType>
static llvm::Value* EmitConvert(llvm::IRBuilder<>& builder, llvm::Value* inputValue)
{
	if constexpr (std::numeric_limits<InputType>::is_iec559)
	{
		if constexpr (std::numeric_limits<OutputType>::is_integer && std::numeric_limits<OutputType>::is_signed)
		{
			return EmitConvertFloatUInt(builder, inputValue, std::numeric_limits<OutputType>::max(), GetType<OutputType>(builder));
		}
		else if constexpr (std::numeric_limits<OutputType>::is_integer)
		{
			return EmitConvertFloatInt(builder, inputValue, std::numeric_limits<OutputType>::max(), GetType<OutputType>(builder));
		}
		else
		{
			static_assert(std::numeric_limits<OutputType>::is_iec559);
			return EmitConvertFloat(builder, inputValue, GetType<OutputType>(builder));
		}
	}
	else if (std::is_unsigned<InputType>::value)
	{
		static_assert(std::numeric_limits<InputType>::is_integer);
		if constexpr (std::numeric_limits<OutputType>::is_integer)
		{
			return EmitConvertInt(builder, inputValue, GetType<OutputType>(builder), false);
		}
		else
		{
			static_assert(std::numeric_limits<OutputType>::is_iec559);
			return EmitConvertUIntFloat(builder, inputValue, std::numeric_limits<InputType>::max(), GetType<OutputType>(builder));
		}
	}
	else
	{
		static_assert(std::numeric_limits<InputType>::is_integer);
		if constexpr (std::numeric_limits<OutputType>::is_integer)
		{
			return EmitConvertInt(builder, inputValue, GetType<OutputType>(builder), true);
		}
		else
		{
			static_assert(std::numeric_limits<OutputType>::is_iec559);
			FATAL_ERROR();
		}
	}
}

STL_DLL_EXPORT FunctionPointer CompileGetPixelDepth(SpirvJit* jit, const FormatInformation* information)
{
	assert(information->Type == FormatType::Normal);

	auto context = std::make_unique<llvm::LLVMContext>();
	llvm::IRBuilder<> builder(*context);
	auto module = std::make_unique<llvm::Module>("", *context);
	
	const auto functionType = llvm::FunctionType::get(builder.getFloatTy(), {
		                                                  builder.getInt8PtrTy()
	                                                  }, false);
	const auto function = llvm::Function::Create(static_cast<llvm::FunctionType*>(functionType),
	                                             llvm::GlobalVariable::ExternalLinkage,
	                                             "main",
	                                             module.get());
	
	const auto basicBlock = llvm::BasicBlock::Create(*context, "", function);
	builder.SetInsertPoint(basicBlock);
	
	llvm::Value* sourcePtr = &*function->arg_begin();

	if (information->Normal.RedOffset == INVALID_OFFSET)
	{
		FATAL_ERROR();
	}

	llvm::Value* value;
	switch (information->Format)
	{
	case VK_FORMAT_D16_UNORM:
	case VK_FORMAT_D16_UNORM_S8_UINT:
		sourcePtr = builder.CreateBitCast(sourcePtr, llvm::PointerType::get(builder.getInt16Ty(), 0));
		value = builder.CreateLoad(sourcePtr);
		value = EmitConvert<uint16_t, float>(builder, value);
		break;
	
	case VK_FORMAT_D24_UNORM_S8_UINT:
	case VK_FORMAT_X8_D24_UNORM_PACK32:
		FATAL_ERROR();
	
	case VK_FORMAT_D32_SFLOAT:
	case VK_FORMAT_D32_SFLOAT_S8_UINT:
		sourcePtr = builder.CreateBitCast(sourcePtr, llvm::PointerType::get(builder.getFloatTy(), 0));
		value = builder.CreateLoad(sourcePtr);
		break;
	
	default:
		FATAL_ERROR();
	}

	builder.CreateRet(value);
	
	// TODO: Optimise
	
	Dump(module.get());
	
	const auto compiledModule = jit->CompileModule(std::move(context), std::move(module));
	return jit->getFunctionPointer(compiledModule, "main");
}

STL_DLL_EXPORT FunctionPointer CompileGetPixel(SpirvJit* jit, const FormatInformation* information)
{
	auto context = std::make_unique<llvm::LLVMContext>();
	llvm::IRBuilder<> builder(*context);
	auto module = std::make_unique<llvm::Module>("", *context);

	llvm::Type* resultType;
	switch (information->ElementSize)
	{
	case 1:
		resultType = builder.getInt8Ty();
		break;

	case 2:
		resultType = builder.getInt16Ty();
		break;

	case 4:
		resultType = builder.getInt32Ty();
		break;

	case 8:
		resultType = builder.getInt64Ty();
		break;

	default:
		FATAL_ERROR();
	}
	
	const auto functionType = llvm::FunctionType::get(builder.getVoidTy(), {
		                                                  builder.getInt8PtrTy(),
		                                                  llvm::PointerType::get(resultType, 0),
	                                                  }, false);
	const auto function = llvm::Function::Create(static_cast<llvm::FunctionType*>(functionType),
	                                             llvm::GlobalVariable::ExternalLinkage,
	                                             "main",
	                                             module.get());
	
	const auto basicBlock = llvm::BasicBlock::Create(*context, "", function);
	builder.SetInsertPoint(basicBlock);
	
	llvm::Value* sourcePtr = &*function->arg_begin();
	llvm::Value* destinationPtr = &*(function->arg_begin() + 1);

	switch (information->Type)
	{
	case FormatType::Normal:
		{
			sourcePtr = builder.CreateBitCast(sourcePtr, llvm::PointerType::get(resultType, 0));

			if (information->Normal.RedOffset != INVALID_OFFSET)
			{
				auto value = builder.CreateConstGEP1_32(sourcePtr, information->Normal.RedOffset / information->ElementSize);
				value = builder.CreateLoad(value);
				const auto dst = builder.CreateConstGEP1_32(destinationPtr, 0);
				builder.CreateStore(value, dst);
			}

			if (information->Normal.GreenOffset != INVALID_OFFSET)
			{
				auto value = builder.CreateConstGEP1_32(sourcePtr, information->Normal.GreenOffset / information->ElementSize);
				value = builder.CreateLoad(value);
				const auto dst = builder.CreateConstGEP1_32(destinationPtr, 1);
				builder.CreateStore(value, dst);
			}

			if (information->Normal.BlueOffset != INVALID_OFFSET)
			{
				auto value = builder.CreateConstGEP1_32(sourcePtr, information->Normal.BlueOffset / information->ElementSize);
				value = builder.CreateLoad(value);
				const auto dst = builder.CreateConstGEP1_32(destinationPtr, 2);
				builder.CreateStore(value, dst);
			}

			if (information->Normal.AlphaOffset != INVALID_OFFSET)
			{
				auto value = builder.CreateConstGEP1_32(sourcePtr, information->Normal.AlphaOffset / information->ElementSize);
				value = builder.CreateLoad(value);
				const auto dst = builder.CreateConstGEP1_32(destinationPtr, 3);
				builder.CreateStore(value, dst);
			}
			break;
		}
		
	case FormatType::Packed: FATAL_ERROR();
	case FormatType::Compressed: FATAL_ERROR();
	case FormatType::Planar: FATAL_ERROR();
	case FormatType::PlanarSamplable: FATAL_ERROR();
	default: assert(false);
	}

	builder.CreateRetVoid();
	
	// TODO: Optimise
	
	Dump(module.get());
	
	const auto compiledModule = jit->CompileModule(std::move(context), std::move(module));
	return jit->getFunctionPointer(compiledModule, "main");
}

STL_DLL_EXPORT FunctionPointer CompileGetPixelF32(SpirvJit* jit, const FormatInformation* information)
{
	auto context = std::make_unique<llvm::LLVMContext>();
	llvm::IRBuilder<> builder(*context);
	auto module = std::make_unique<llvm::Module>("", *context);
	
	const auto functionType = llvm::FunctionType::get(builder.getVoidTy(), {
		                                                  builder.getInt8PtrTy(),
		                                                  llvm::PointerType::get(builder.getFloatTy(), 0),
	                                                  }, false);
	const auto function = llvm::Function::Create(static_cast<llvm::FunctionType*>(functionType),
	                                             llvm::GlobalVariable::ExternalLinkage,
	                                             "main",
	                                             module.get());
	
	const auto basicBlock = llvm::BasicBlock::Create(*context, "", function);
	builder.SetInsertPoint(basicBlock);
	
	llvm::Value* sourcePtr = &*function->arg_begin();
	llvm::Value* destinationPtr = &*(function->arg_begin() + 1);

	switch (information->Type)
	{
	case FormatType::Normal:
		{
			llvm::Type* sourceType;
			std::function<llvm::Value*(llvm::IRBuilder<>&, llvm::Value*)> process;
			switch (information->Base)
			{
			case BaseType::UNorm:
			case BaseType::UScaled:
			case BaseType::UInt:
				switch (information->ElementSize)
				{
				case 1:
					sourceType = builder.getInt8Ty();
					process = EmitConvert<uint8_t, float>;
					break;

				case 2:
					sourceType = builder.getInt16Ty();
					process = EmitConvert<uint16_t, float>;
					break;

				case 4:
					sourceType = builder.getInt32Ty();
					process = EmitConvert<uint32_t, float>;
					break;

				default:
					FATAL_ERROR();
				}
				break;

			default: FATAL_ERROR();
			}

			sourcePtr = builder.CreateBitCast(sourcePtr, llvm::PointerType::get(sourceType, 0));

			if (information->Normal.RedOffset != INVALID_OFFSET)
			{
				auto value = builder.CreateConstGEP1_32(sourcePtr, information->Normal.RedOffset / information->ElementSize);
				value = builder.CreateLoad(value);
				value = process(builder, value);
				const auto dst = builder.CreateConstGEP1_32(destinationPtr, 0);
				builder.CreateStore(value, dst);
			}

			if (information->Normal.GreenOffset != INVALID_OFFSET)
			{
				auto value = builder.CreateConstGEP1_32(sourcePtr, information->Normal.GreenOffset / information->ElementSize);
				value = builder.CreateLoad(value);
				value = process(builder, value);
				const auto dst = builder.CreateConstGEP1_32(destinationPtr, 1);
				builder.CreateStore(value, dst);
			}

			if (information->Normal.BlueOffset != INVALID_OFFSET)
			{
				auto value = builder.CreateConstGEP1_32(sourcePtr, information->Normal.BlueOffset / information->ElementSize);
				value = builder.CreateLoad(value);
				value = process(builder, value);
				const auto dst = builder.CreateConstGEP1_32(destinationPtr, 2);
				builder.CreateStore(value, dst);
			}

			if (information->Normal.AlphaOffset != INVALID_OFFSET)
			{
				auto value = builder.CreateConstGEP1_32(sourcePtr, information->Normal.AlphaOffset / information->ElementSize);
				value = builder.CreateLoad(value);
				value = process(builder, value);
				const auto dst = builder.CreateConstGEP1_32(destinationPtr, 3);
				builder.CreateStore(value, dst);
			}
			break;
		}
		
	case FormatType::Packed: FATAL_ERROR();
	case FormatType::Compressed: FATAL_ERROR();
	case FormatType::Planar: FATAL_ERROR();
	case FormatType::PlanarSamplable: FATAL_ERROR();
	default: assert(false);
	}

	builder.CreateRetVoid();
	
	// TODO: Optimise
	
	Dump(module.get());
	
	const auto compiledModule = jit->CompileModule(std::move(context), std::move(module));
	return jit->getFunctionPointer(compiledModule, "main");
}

STL_DLL_EXPORT FunctionPointer CompileGetPixelU32(SpirvJit* jit, const FormatInformation* information)
{
	auto context = std::make_unique<llvm::LLVMContext>();
	llvm::IRBuilder<> builder(*context);
	auto module = std::make_unique<llvm::Module>("", *context);
	
	const auto functionType = llvm::FunctionType::get(builder.getVoidTy(), {
		                                                  builder.getInt8PtrTy(),
		                                                  llvm::PointerType::get(builder.getInt32Ty(), 0),
	                                                  }, false);
	const auto function = llvm::Function::Create(static_cast<llvm::FunctionType*>(functionType),
	                                             llvm::GlobalVariable::ExternalLinkage,
	                                             "main",
	                                             module.get());
	
	const auto basicBlock = llvm::BasicBlock::Create(*context, "", function);
	builder.SetInsertPoint(basicBlock);
	
	llvm::Value* sourcePtr = &*function->arg_begin();
	llvm::Value* destinationPtr = &*(function->arg_begin() + 1);

	switch (information->Type)
	{
	case FormatType::Normal:
		{
			llvm::Type* sourceType;
			std::function<llvm::Value*(llvm::IRBuilder<>&, llvm::Value*)> process;
			switch (information->Base)
			{
			case BaseType::UNorm:
			case BaseType::UScaled:
			case BaseType::UInt:
				switch (information->ElementSize)
				{
				case 1:
					sourceType = builder.getInt8Ty();
					process = EmitConvert<uint8_t, uint32_t>;
					break;

				case 2:
					sourceType = builder.getInt16Ty();
					process = EmitConvert<uint16_t, uint32_t>;
					break;

				case 4:
					sourceType = builder.getInt32Ty();
					process = EmitConvert<uint32_t, uint32_t>;
					break;

				default:
					FATAL_ERROR();
				}
				break;

			default: FATAL_ERROR();
			}

			sourcePtr = builder.CreateBitCast(sourcePtr, llvm::PointerType::get(sourceType, 0));

			if (information->Normal.RedOffset != INVALID_OFFSET)
			{
				auto value = builder.CreateConstGEP1_32(sourcePtr, information->Normal.RedOffset / information->ElementSize);
				value = builder.CreateLoad(value);
				value = process(builder, value);
				const auto dst = builder.CreateConstGEP1_32(destinationPtr, 0);
				builder.CreateStore(value, dst);
			}

			if (information->Normal.GreenOffset != INVALID_OFFSET)
			{
				auto value = builder.CreateConstGEP1_32(sourcePtr, information->Normal.GreenOffset / information->ElementSize);
				value = builder.CreateLoad(value);
				value = process(builder, value);
				const auto dst = builder.CreateConstGEP1_32(destinationPtr, 1);
				builder.CreateStore(value, dst);
			}

			if (information->Normal.BlueOffset != INVALID_OFFSET)
			{
				auto value = builder.CreateConstGEP1_32(sourcePtr, information->Normal.BlueOffset / information->ElementSize);
				value = builder.CreateLoad(value);
				value = process(builder, value);
				const auto dst = builder.CreateConstGEP1_32(destinationPtr, 2);
				builder.CreateStore(value, dst);
			}

			if (information->Normal.AlphaOffset != INVALID_OFFSET)
			{
				auto value = builder.CreateConstGEP1_32(sourcePtr, information->Normal.AlphaOffset / information->ElementSize);
				value = builder.CreateLoad(value);
				value = process(builder, value);
				const auto dst = builder.CreateConstGEP1_32(destinationPtr, 3);
				builder.CreateStore(value, dst);
			}
			break;
		}
		
	case FormatType::Packed: FATAL_ERROR();
	case FormatType::Compressed: FATAL_ERROR();
	case FormatType::Planar: FATAL_ERROR();
	case FormatType::PlanarSamplable: FATAL_ERROR();
	default: assert(false);
	}

	builder.CreateRetVoid();
	
	// TODO: Optimise
	
	Dump(module.get());
	
	const auto compiledModule = jit->CompileModule(std::move(context), std::move(module));
	return jit->getFunctionPointer(compiledModule, "main");
}

STL_DLL_EXPORT FunctionPointer CompileSetPixelDepthStencil(SpirvJit* jit, const FormatInformation* information)
{
	assert(information->Type == FormatType::Normal);

	auto context = std::make_unique<llvm::LLVMContext>();
	llvm::IRBuilder<> builder(*context);
	auto module = std::make_unique<llvm::Module>("", *context);
	
	const auto functionType = llvm::FunctionType::get(builder.getVoidTy(), {
		                                                  builder.getInt8PtrTy(),
		                                                  builder.getFloatTy(),
		                                                  builder.getInt8Ty(),
	                                                  }, false);
	const auto function = llvm::Function::Create(static_cast<llvm::FunctionType*>(functionType),
	                                             llvm::GlobalVariable::ExternalLinkage,
	                                             "main",
	                                             module.get());
	
	const auto basicBlock = llvm::BasicBlock::Create(*context, "", function);
	builder.SetInsertPoint(basicBlock);
	
	const auto destinationPtr = &*function->arg_begin();
	const auto depthSource = &*(function->arg_begin() + 1);
	const auto stencilSource = &*(function->arg_begin() + 2);

	if (information->Normal.RedOffset != INVALID_OFFSET)
	{
		llvm::Value* value;
		auto dst = builder.CreateConstGEP1_32(destinationPtr, 0);
		switch (information->Format)
		{
		case VK_FORMAT_D16_UNORM:
		case VK_FORMAT_D16_UNORM_S8_UINT:
			value = builder.CreateMinNum(builder.CreateMaxNum(depthSource, llvm::ConstantFP::get(builder.getFloatTy(), 0)),
			                             llvm::ConstantFP::get(builder.getFloatTy(), 1));
			value = EmitConvert<float, uint16_t>(builder, value);
			dst = builder.CreateBitCast(dst, llvm::PointerType::get(builder.getInt16Ty(), 0));
			break;

		case VK_FORMAT_D24_UNORM_S8_UINT:
		case VK_FORMAT_X8_D24_UNORM_PACK32:
			FATAL_ERROR();

		case VK_FORMAT_D32_SFLOAT:
		case VK_FORMAT_D32_SFLOAT_S8_UINT:
			value = builder.CreateMinNum(builder.CreateMaxNum(depthSource, llvm::ConstantFP::get(builder.getFloatTy(), 0)),
			                             llvm::ConstantFP::get(builder.getFloatTy(), 1));
			dst = builder.CreateBitCast(dst, llvm::PointerType::get(builder.getFloatTy(), 0));
			break;

		default:
			FATAL_ERROR();
		}
		builder.CreateStore(value, dst);
	}
	
	if (information->Normal.GreenOffset != INVALID_OFFSET)
	{
		llvm::Value* dst;
		switch (information->Format)
		{
		case VK_FORMAT_S8_UINT:
			dst = builder.CreateConstGEP1_32(destinationPtr, 0);
			break;
			
		case VK_FORMAT_D16_UNORM_S8_UINT:
			dst = builder.CreateConstGEP1_32(destinationPtr, 2);
			break;

		case VK_FORMAT_D24_UNORM_S8_UINT:
			dst = builder.CreateConstGEP1_32(destinationPtr, 3);
			break;

		case VK_FORMAT_D32_SFLOAT_S8_UINT:
			dst = builder.CreateConstGEP1_32(destinationPtr, 4);
			break;

		default:
			FATAL_ERROR();
		}
		builder.CreateStore(stencilSource, dst);
	}

	builder.CreateRetVoid();
	
	// TODO: Optimise
	
	Dump(module.get());
	
	const auto compiledModule = jit->CompileModule(std::move(context), std::move(module));
	return jit->getFunctionPointer(compiledModule, "main");
}

static void EmitSetPackedPixelFloat(llvm::Function* function, llvm::Module* module, llvm::IRBuilder<>& builder, const FormatInformation* information, llvm::Value* destinationPtr, llvm::Value* sourcePtr)
{
	assert(information->Type == FormatType::Packed);
	
	llvm::Type* resultType;
	switch (information->TotalSize)
	{
	case 1:
		resultType = builder.getInt8Ty();
		break;

	case 2:
		resultType = builder.getInt16Ty();
		break;

	case 4:
		resultType = builder.getInt32Ty();
		break;

	default:
		FATAL_ERROR();
	}

	std::function<llvm::Value*(llvm::IRBuilder<>&, llvm::Value*, int)> process;
	switch (information->Base)
	{
	case BaseType::UNorm:
		process = [information, resultType, sourcePtr](llvm::IRBuilder<>& builder, llvm::Value* oldValue, int index)
		{
			const auto bits = GetPackedBits(information, index);
			const double size = (1ULL << bits) - 1;
			const auto offset = GetPackedOffset(information, index);

			// Get source float from array
			auto source = builder.CreateConstGEP1_32(sourcePtr, index);
			source = builder.CreateLoad(source);
			// Clamp between [0, 1]
			source = builder.CreateMinNum(builder.CreateMaxNum(source, llvm::ConstantFP::get(builder.getFloatTy(), 0)),
			                              llvm::ConstantFP::get(builder.getFloatTy(), 1));
			// Float to UInt
			auto value = builder.CreateFMul(source, llvm::ConstantFP::get(builder.getFloatTy(), size));
			value = builder.CreateUnaryIntrinsic(llvm::Intrinsic::round, value);
			value = builder.CreateFPToUI(value, resultType);
			// Shift and or with old value
			value = builder.CreateShl(value, llvm::ConstantInt::get(resultType, offset));
			return builder.CreateOr(value, oldValue);
		};
		break;
		 
	case BaseType::SNorm:
		process = [information, resultType, sourcePtr](llvm::IRBuilder<>& builder, llvm::Value* oldValue, int index)
		{
			const auto bits = GetPackedBits(information, index);
			const double size = (1ULL << (bits - 1)) - 1;
			const auto offset = GetPackedOffset(information, index);
			
			// Get source float from array
			auto source = builder.CreateConstGEP1_32(sourcePtr, index);
			source = builder.CreateLoad(source);
			// Clamp between [-1, 1]
			source = builder.CreateMinNum(builder.CreateMaxNum(source, llvm::ConstantFP::get(builder.getFloatTy(), -1)),
			                              llvm::ConstantFP::get(builder.getFloatTy(), 1));
			// Float to UInt
			auto value = builder.CreateFMul(source, llvm::ConstantFP::get(builder.getFloatTy(), size));
			value = builder.CreateUnaryIntrinsic(llvm::Intrinsic::round, value);
			value = builder.CreateFPToUI(value, resultType);
			// Shift and or with old value
			value = builder.CreateShl(value, llvm::ConstantInt::get(resultType, offset));
			return builder.CreateOr(value, oldValue);
		};
		break;
		
	case BaseType::UFloat:
		switch (information->Format)
		{
		case VK_FORMAT_B10G11R11_UFLOAT_PACK32:
			{
				const auto b10g11r11Function = llvm::Function::Create(llvm::FunctionType::get(resultType, {sourcePtr->getType()}, false), llvm::GlobalValue::ExternalLinkage, "FloatToB10G11R11", module);
				const auto value = builder.CreateCall(b10g11r11Function, {sourcePtr});
				const auto dst = builder.CreateBitCast(destinationPtr, llvm::PointerType::get(resultType, 0));
				builder.CreateStore(value, dst);
				return;
			}
			
		case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32:
			{
				const auto e5b9g9r9Function = llvm::Function::Create(llvm::FunctionType::get(resultType, {sourcePtr->getType()}, false), llvm::GlobalValue::ExternalLinkage, "FloatToE5B9G9R9", module);
				const auto value = builder.CreateCall(e5b9g9r9Function, {sourcePtr});
				const auto dst = builder.CreateBitCast(destinationPtr, llvm::PointerType::get(resultType, 0));
				builder.CreateStore(value, dst);
				return;
			}

		default:
			FATAL_ERROR();
		}
		
	case BaseType::SFloat:
		FATAL_ERROR();
		
	case BaseType::SRGB:
		process = [information, resultType, sourcePtr, function](llvm::IRBuilder<>& builder, llvm::Value* oldValue, int index)
		{
			const auto bits = GetPackedBits(information, index);
			const double size = (1ULL << bits) - 1;
			const auto offset = GetPackedOffset(information, index);

			// Get source float from array
			auto source = builder.CreateConstGEP1_32(sourcePtr, index);
			source = builder.CreateLoad(source);
			// Clamp between [0, 1]
			source = builder.CreateMinNum(builder.CreateMaxNum(source, llvm::ConstantFP::get(builder.getFloatTy(), 0)),
			                              llvm::ConstantFP::get(builder.getFloatTy(), 1));
			source = index == 3 ? source : EmitLinearToSRGB(function, builder, source);
			// Float to UInt
			auto value = builder.CreateFMul(source, llvm::ConstantFP::get(builder.getFloatTy(), size));
			value = builder.CreateUnaryIntrinsic(llvm::Intrinsic::round, value);
			value = builder.CreateFPToUI(value, resultType);
			// Shift and or with old value
			value = builder.CreateShl(value, llvm::ConstantInt::get(resultType, offset));
			return builder.CreateOr(value, oldValue);
		};
		break;
		
	default: FATAL_ERROR();
	}

	llvm::Value* value = llvm::ConstantInt::get(resultType, 0);

	if (information->Packed.RedBits)
	{
		value = process(builder, value, 0);
	}

	if (information->Packed.GreenBits)
	{
		value = process(builder, value, 1);
	}

	if (information->Packed.BlueBits)
	{
		value = process(builder, value, 2);
	}

	if (information->Packed.AlphaBits)
	{
		value = process(builder, value, 3);
	}

	const auto dst = builder.CreateBitCast(destinationPtr, llvm::PointerType::get(resultType, 0));
	builder.CreateStore(value, dst);
}

static void EmitSetPackedPixelInt32(llvm::IRBuilder<>& builder, const FormatInformation* information, llvm::Value* destinationPtr, llvm::Value* sourcePtr)
{
	assert(information->Type == FormatType::Packed);

	llvm::Type* resultType;
	switch (information->TotalSize)
	{
	case 1:
		resultType = builder.getInt8Ty();
		break;

	case 2:
		resultType = builder.getInt16Ty();
		break;

	case 4:
		resultType = builder.getInt32Ty();
		break;

	default:
		FATAL_ERROR();
	}
	
	const auto process = [information, resultType, sourcePtr](llvm::IRBuilder<>& builder, llvm::Value* oldValue, int index)
	{
		const auto bits = GetPackedBits(information, index);
		const auto mask = (1ULL << bits) - 1;
		const auto offset = GetPackedOffset(information, index);
				
		// Get source from array
		auto source = builder.CreateConstGEP1_32(sourcePtr, index);
		source = builder.CreateLoad(source);

		auto value = builder.CreateAnd(source, mask);
		value = builder.CreateZExtOrTrunc(value, resultType);
		
		// Shift and or with old value
		value = builder.CreateShl(value, llvm::ConstantInt::get(resultType, offset));
		return builder.CreateOr(value, oldValue);
	};

	llvm::Value* value = llvm::ConstantInt::get(resultType, 0);
	
	if (information->Packed.RedBits)
	{
		value = process(builder, value, 0);
	}
	
	if (information->Packed.GreenBits)
	{
		value = process(builder, value, 1);
	}
	
	if (information->Packed.BlueBits)
	{
		value = process(builder, value, 2);
	}
	
	if (information->Packed.AlphaBits)
	{
		value = process(builder, value, 3);
	}
	
	const auto dst = builder.CreateBitCast(destinationPtr, llvm::PointerType::get(resultType, 0));
	builder.CreateStore(value, dst);
}

STL_DLL_EXPORT FunctionPointer CompileSetPixelFloat(SpirvJit* jit, const FormatInformation* information)
{
	auto context = std::make_unique<llvm::LLVMContext>();
	llvm::IRBuilder<> builder(*context);
	auto module = std::make_unique<llvm::Module>("", *context);

	const auto functionType = llvm::FunctionType::get(builder.getVoidTy(), {
		                                                  builder.getInt8PtrTy(),
		                                                  llvm::PointerType::get(builder.getFloatTy(), 0),
	                                                  }, false);
	const auto function = llvm::Function::Create(static_cast<llvm::FunctionType*>(functionType),
	                                             llvm::GlobalVariable::ExternalLinkage,
	                                             "main",
	                                             module.get());

	const auto basicBlock = llvm::BasicBlock::Create(*context, "", function);
	builder.SetInsertPoint(basicBlock);

	llvm::Value* destinationPtr = &*function->arg_begin();
	const auto sourcePtr = &*(function->arg_begin() + 1);

	switch (information->Type)
	{
	case FormatType::Normal:
		{
			llvm::Type* resultType;

			if (information->Base == BaseType::SFloat)
			{
				switch (information->ElementSize)
				{
				case 2:
					resultType = llvm::Type::getHalfTy(*context);
					break;

				case 4:
					resultType = llvm::Type::getFloatTy(*context);
					break;

				case 8:
					resultType = llvm::Type::getDoubleTy(*context);
					break;

				default:
					FATAL_ERROR();
				}
			}
			else if (information->Base == BaseType::UFloat)
			{
				FATAL_ERROR();
			}
			else
			{
				switch (information->ElementSize)
				{
				case 1:
					resultType = builder.getInt8Ty();
					break;

				case 2:
					resultType = builder.getInt16Ty();
					break;

				case 4:
					resultType = builder.getInt32Ty();
					break;

				case 8:
					resultType = builder.getInt64Ty();
					break;

				default:
					FATAL_ERROR();
				}
			}

			destinationPtr = builder.CreateBitCast(destinationPtr, llvm::PointerType::get(resultType, 0));

			std::function<llvm::Value*(llvm::IRBuilder<>&, llvm::Value*, int)> process;
			switch (information->Base)
			{
			case BaseType::UNorm:
				switch (information->ElementSize)
				{
				case 1:
					process = [](llvm::IRBuilder<>& builder, llvm::Value* inputValue, int)
					{
						const auto value = builder.CreateMinNum(builder.CreateMaxNum(inputValue, llvm::ConstantFP::get(builder.getFloatTy(), 0)),
						                                        llvm::ConstantFP::get(builder.getFloatTy(), 1));
						return EmitConvert<float, uint8_t>(builder, value);
					};
					break;

				case 2:
					process = [](llvm::IRBuilder<>& builder, llvm::Value* inputValue, int)
					{
						const auto value = builder.CreateMinNum(builder.CreateMaxNum(inputValue, llvm::ConstantFP::get(builder.getFloatTy(), 0)),
						                                        llvm::ConstantFP::get(builder.getFloatTy(), 1));
						return EmitConvert<float, uint16_t>(builder, value);
					};
					break;

				case 4:
					process = [](llvm::IRBuilder<>& builder, llvm::Value* inputValue, int)
					{
						const auto value = builder.CreateMinNum(builder.CreateMaxNum(inputValue, llvm::ConstantFP::get(builder.getFloatTy(), 0)),
						                                        llvm::ConstantFP::get(builder.getFloatTy(), 1));
						return EmitConvert<float, uint32_t>(builder, value);
					};
					break;

				default:
					FATAL_ERROR();
				}
				break;

			case BaseType::SNorm:
				switch (information->ElementSize)
				{
				case 1:
					process = [](llvm::IRBuilder<>& builder, llvm::Value* inputValue, int)
					{
						const auto value = builder.CreateMinNum(builder.CreateMaxNum(inputValue, llvm::ConstantFP::get(builder.getFloatTy(), -1)),
						                                        llvm::ConstantFP::get(builder.getFloatTy(), 1));
						return EmitConvert<float, int8_t>(builder, value);
					};
					break;

				case 2:
					process = [](llvm::IRBuilder<>& builder, llvm::Value* inputValue, int)
					{
						const auto value = builder.CreateMinNum(builder.CreateMaxNum(inputValue, llvm::ConstantFP::get(builder.getFloatTy(), -1)),
						                                        llvm::ConstantFP::get(builder.getFloatTy(), 1));
						return EmitConvert<float, int16_t>(builder, value);
					};
					break;

				case 4:
					process = [](llvm::IRBuilder<>& builder, llvm::Value* inputValue, int)
					{
						const auto value = builder.CreateMinNum(builder.CreateMaxNum(inputValue, llvm::ConstantFP::get(builder.getFloatTy(), -1)),
						                                        llvm::ConstantFP::get(builder.getFloatTy(), 1));
						return EmitConvert<float, int32_t>(builder, value);
					};
					break;

				default:
					FATAL_ERROR();
				}
				break;

			case BaseType::UFloat: FATAL_ERROR();

			case BaseType::SFloat:
				switch (information->ElementSize)
				{
				case 2:
					process = [](llvm::IRBuilder<>& builder, llvm::Value* inputValue, int)
					{
						return EmitConvert<float, half>(builder, inputValue);
					};
					break;

				case 4:
					process = [](llvm::IRBuilder<>& builder, llvm::Value* inputValue, int)
					{
						return EmitConvert<float, float>(builder, inputValue);
					};
					break;

				case 8:
					process = [](llvm::IRBuilder<>& builder, llvm::Value* inputValue, int)
					{
						return EmitConvert<float, double>(builder, inputValue);
					};
					break;

				default:
					FATAL_ERROR();
				}
				break;

			case BaseType::SRGB:
				switch (information->ElementSize)
				{
				case 1:
					process = [function](llvm::IRBuilder<>& builder, llvm::Value* inputValue, int index)
					{
						llvm::Value* value = builder.CreateMinNum(builder.CreateMaxNum(inputValue, llvm::ConstantFP::get(builder.getFloatTy(), 0)),
						                                          llvm::ConstantFP::get(builder.getFloatTy(), 1));
						value = index == 3 ? inputValue : EmitLinearToSRGB(function, builder, inputValue);
						return EmitConvert<float, uint8_t>(builder, value);
					};
					break;

				case 2:
					process = [function](llvm::IRBuilder<>& builder, llvm::Value* inputValue, int index)
					{
						llvm::Value* value = builder.CreateMinNum(builder.CreateMaxNum(inputValue, llvm::ConstantFP::get(builder.getFloatTy(), 0)),
						                                          llvm::ConstantFP::get(builder.getFloatTy(), 1));
						value = index == 3 ? inputValue : EmitLinearToSRGB(function, builder, inputValue);
						return EmitConvert<float, uint16_t>(builder, value);
					};
					break;

				case 4:
					process = [function](llvm::IRBuilder<>& builder, llvm::Value* inputValue, int index)
					{
						llvm::Value* value = builder.CreateMinNum(builder.CreateMaxNum(inputValue, llvm::ConstantFP::get(builder.getFloatTy(), 0)),
						                                          llvm::ConstantFP::get(builder.getFloatTy(), 1));
						value = index == 3 ? inputValue : EmitLinearToSRGB(function, builder, inputValue);
						return EmitConvert<float, uint32_t>(builder, value);
					};
					break;

				default:
					FATAL_ERROR();
				}
				break;

			default: FATAL_ERROR();
			}

			if (information->Normal.RedOffset != INVALID_OFFSET)
			{
				auto value = builder.CreateConstGEP1_32(sourcePtr, 0);
				value = builder.CreateLoad(value);
				value = process(builder, value, 0);
				const auto dst = builder.CreateConstGEP1_32(destinationPtr, information->Normal.RedOffset / information->ElementSize);
				builder.CreateStore(value, dst);
			}

			if (information->Normal.GreenOffset != INVALID_OFFSET)
			{
				auto value = builder.CreateConstGEP1_32(sourcePtr, 1);
				value = builder.CreateLoad(value);
				value = process(builder, value, 1);
				const auto dst = builder.CreateConstGEP1_32(destinationPtr, information->Normal.GreenOffset / information->ElementSize);
				builder.CreateStore(value, dst);
			}

			if (information->Normal.BlueOffset != INVALID_OFFSET)
			{
				auto value = builder.CreateConstGEP1_32(sourcePtr, 2);
				value = builder.CreateLoad(value);
				value = process(builder, value, 2);
				const auto dst = builder.CreateConstGEP1_32(destinationPtr, information->Normal.BlueOffset / information->ElementSize);
				builder.CreateStore(value, dst);
			}

			if (information->Normal.AlphaOffset != INVALID_OFFSET)
			{
				auto value = builder.CreateConstGEP1_32(sourcePtr, 3);
				value = builder.CreateLoad(value);
				value = process(builder, value, 3);
				const auto dst = builder.CreateConstGEP1_32(destinationPtr, information->Normal.AlphaOffset / information->ElementSize);
				builder.CreateStore(value, dst);
			}
			break;
		}
		
	case FormatType::Packed:
		EmitSetPackedPixelFloat(function, module.get(), builder, information, destinationPtr, sourcePtr);
		break;
		
	case FormatType::Compressed: FATAL_ERROR();
	case FormatType::Planar: FATAL_ERROR();
	case FormatType::PlanarSamplable: FATAL_ERROR();
	default: assert(false);
	}

	builder.CreateRetVoid();

	// TODO: Optimise

	Dump(module.get());
	
	const auto compiledModule = jit->CompileModule(std::move(context), std::move(module));
	return jit->getFunctionPointer(compiledModule, "main");
}

STL_DLL_EXPORT FunctionPointer CompileSetPixelInt32(SpirvJit* jit, const FormatInformation* information)
{
	auto context = std::make_unique<llvm::LLVMContext>();
	llvm::IRBuilder<> builder(*context);
	auto module = std::make_unique<llvm::Module>("", *context);
	
	const auto functionType = llvm::FunctionType::get(builder.getVoidTy(), {
		                                                  builder.getInt8PtrTy(),
		                                                  llvm::PointerType::get(builder.getInt32Ty(), 0),
	                                                  }, false);
	const auto function = llvm::Function::Create(static_cast<llvm::FunctionType*>(functionType),
	                                             llvm::GlobalVariable::ExternalLinkage,
	                                             "main",
	                                             module.get());
	
	const auto basicBlock = llvm::BasicBlock::Create(*context, "", function);
	builder.SetInsertPoint(basicBlock);
	
	llvm::Value* destinationPtr = &*function->arg_begin();
	const auto sourcePtr = &*(function->arg_begin() + 1);

	switch (information->Type)
	{
	case FormatType::Normal:
		{
			llvm::Type* resultType;
			switch (information->ElementSize)
			{
			case 1:
				resultType = builder.getInt8Ty();
				break;

			case 2:
				resultType = builder.getInt16Ty();
				break;

			case 4:
				resultType = builder.getInt32Ty();
				break;

			default:
				FATAL_ERROR();
			}

			destinationPtr = builder.CreateBitCast(destinationPtr, llvm::PointerType::get(resultType, 0));

			std::function<llvm::Value*(llvm::IRBuilder<>&, llvm::Value*)> process;
			switch (information->Base)
			{
			case BaseType::SScaled:
			case BaseType::SInt:
				switch (information->ElementSize)
				{
				case 1:
					process = EmitConvert<int32_t, int8_t>;
					break;

				case 2:
					process = EmitConvert<int32_t, int16_t>;
					break;

				case 4:
					process = EmitConvert<int32_t, int32_t>;
					break;

				default:
					FATAL_ERROR();
				}
				break;

			default: FATAL_ERROR();
			}

			if (information->Normal.RedOffset != INVALID_OFFSET)
			{
				auto value = builder.CreateConstGEP1_32(sourcePtr, 0);
				value = builder.CreateLoad(value);
				value = process(builder, value);
				const auto dst = builder.CreateConstGEP1_32(destinationPtr, information->Normal.RedOffset / information->ElementSize);
				builder.CreateStore(value, dst);
			}

			if (information->Normal.GreenOffset != INVALID_OFFSET)
			{
				auto value = builder.CreateConstGEP1_32(sourcePtr, 1);
				value = builder.CreateLoad(value);
				value = process(builder, value);
				const auto dst = builder.CreateConstGEP1_32(destinationPtr, information->Normal.GreenOffset / information->ElementSize);
				builder.CreateStore(value, dst);
			}

			if (information->Normal.BlueOffset != INVALID_OFFSET)
			{
				auto value = builder.CreateConstGEP1_32(sourcePtr, 2);
				value = builder.CreateLoad(value);
				value = process(builder, value);
				const auto dst = builder.CreateConstGEP1_32(destinationPtr, information->Normal.BlueOffset / information->ElementSize);
				builder.CreateStore(value, dst);
			}

			if (information->Normal.AlphaOffset != INVALID_OFFSET)
			{
				auto value = builder.CreateConstGEP1_32(sourcePtr, 3);
				value = builder.CreateLoad(value);
				value = process(builder, value);
				const auto dst = builder.CreateConstGEP1_32(destinationPtr, information->Normal.AlphaOffset / information->ElementSize);
				builder.CreateStore(value, dst);
			}
			break;
		}
		
	case FormatType::Packed:
		EmitSetPackedPixelInt32(builder, information, destinationPtr, sourcePtr);
		break;
		
	case FormatType::Compressed: FATAL_ERROR();
	case FormatType::Planar: FATAL_ERROR();
	case FormatType::PlanarSamplable: FATAL_ERROR();
	default: assert(false);
	}
	
	builder.CreateRetVoid();
	
	// TODO: Optimise
	
	Dump(module.get());
	
	const auto compiledModule = jit->CompileModule(std::move(context), std::move(module));
	return jit->getFunctionPointer(compiledModule, "main");
}

STL_DLL_EXPORT FunctionPointer CompileSetPixelUInt32(SpirvJit* jit, const FormatInformation* information)
{
	auto context = std::make_unique<llvm::LLVMContext>();
	llvm::IRBuilder<> builder(*context);
	auto module = std::make_unique<llvm::Module>("", *context);
	
	const auto functionType = llvm::FunctionType::get(builder.getVoidTy(), {
		                                                  builder.getInt8PtrTy(),
		                                                  llvm::PointerType::get(builder.getInt32Ty(), 0),
	                                                  }, false);
	const auto function = llvm::Function::Create(static_cast<llvm::FunctionType*>(functionType),
	                                             llvm::GlobalVariable::ExternalLinkage,
	                                             "main",
	                                             module.get());
	
	const auto basicBlock = llvm::BasicBlock::Create(*context, "", function);
	builder.SetInsertPoint(basicBlock);

	llvm::Value* destinationPtr = &*function->arg_begin();
	const auto sourcePtr = &*(function->arg_begin() + 1);

	switch (information->Type)
	{
	case FormatType::Normal:
		{
			llvm::Type* resultType;
			switch (information->ElementSize)
			{
			case 1:
				resultType = builder.getInt8Ty();
				break;

			case 2:
				resultType = builder.getInt16Ty();
				break;

			case 4:
				resultType = builder.getInt32Ty();
				break;

			default:
				FATAL_ERROR();
			}

			destinationPtr = builder.CreateBitCast(destinationPtr, llvm::PointerType::get(resultType, 0));

			std::function<llvm::Value*(llvm::IRBuilder<>&, llvm::Value*)> process;
			switch (information->Base)
			{
			case BaseType::UScaled:
			case BaseType::UInt:
				switch (information->ElementSize)
				{
				case 1:
					process = EmitConvert<uint32_t, uint8_t>;
					break;

				case 2:
					process = EmitConvert<uint32_t, uint16_t>;
					break;

				case 4:
					process = EmitConvert<uint32_t, uint32_t>;
					break;

				default:
					FATAL_ERROR();
				}
				break;

			default: FATAL_ERROR();
			}

			if (information->Normal.RedOffset != INVALID_OFFSET)
			{
				auto value = builder.CreateConstGEP1_32(sourcePtr, 0);
				value = builder.CreateLoad(value);
				value = process(builder, value);
				const auto dst = builder.CreateConstGEP1_32(destinationPtr, information->Normal.RedOffset / information->ElementSize);
				builder.CreateStore(value, dst);
			}

			if (information->Normal.GreenOffset != INVALID_OFFSET)
			{
				auto value = builder.CreateConstGEP1_32(sourcePtr, 1);
				value = builder.CreateLoad(value);
				value = process(builder, value);
				const auto dst = builder.CreateConstGEP1_32(destinationPtr, information->Normal.GreenOffset / information->ElementSize);
				builder.CreateStore(value, dst);
			}

			if (information->Normal.BlueOffset != INVALID_OFFSET)
			{
				auto value = builder.CreateConstGEP1_32(sourcePtr, 2);
				value = builder.CreateLoad(value);
				value = process(builder, value);
				const auto dst = builder.CreateConstGEP1_32(destinationPtr, information->Normal.BlueOffset / information->ElementSize);
				builder.CreateStore(value, dst);
			}

			if (information->Normal.AlphaOffset != INVALID_OFFSET)
			{
				auto value = builder.CreateConstGEP1_32(sourcePtr, 3);
				value = builder.CreateLoad(value);
				value = process(builder, value);
				const auto dst = builder.CreateConstGEP1_32(destinationPtr, information->Normal.AlphaOffset / information->ElementSize);
				builder.CreateStore(value, dst);
			}
			break;
		}
		
	case FormatType::Packed:
		EmitSetPackedPixelInt32(builder, information, destinationPtr, sourcePtr);
		break;
		
	case FormatType::Compressed: FATAL_ERROR();
	case FormatType::Planar: FATAL_ERROR();
	case FormatType::PlanarSamplable: FATAL_ERROR();
	default: assert(false);
	}
	
	builder.CreateRetVoid();
	
	// TODO: Optimise
	
	Dump(module.get());
	
	const auto compiledModule = jit->CompileModule(std::move(context), std::move(module));
	return jit->getFunctionPointer(compiledModule, "main");
}