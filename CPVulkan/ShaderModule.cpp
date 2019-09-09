#include "ShaderModule.h"

#include <cassert>
#include <string>

struct DecompileState
{
	const std::vector<unsigned>& Data;
	uint32_t Size;
	uint32_t Word{};
	uint32_t MaxResults{};
};

static void Validate(DecompileState& state)
{
	state.Size = static_cast<int>(state.Data.size());
	if (state.Size < 4)
	{
		FATAL_ERROR();
	}

	// Magic number
	if (state.Data[state.Word++] != spv::MagicNumber)
	{
		FATAL_ERROR();
	}

	// Version
	state.Word++;

	// Generator's magic number
	state.Word++;

	// Result <id> bound
	state.MaxResults = state.Data[state.Word++];

	// Reserved schema, must be 0 for now
	const auto schema = state.Data[state.Word++];
	if (schema != 0)
	{
		FATAL_ERROR();
	}
}

static uint32_t DisassembleString(DecompileState& state, Instruction& instruction)
{
	const auto startWord = state.Word;

	auto done = false;
	do
	{
		auto content = state.Data[state.Word];
		instruction.arguments.push_back(content);
		auto wordString = reinterpret_cast<const char*>(&content);
		for (auto charCount = 0; charCount < 4; ++charCount)
		{
			if (*wordString == 0)
			{
				done = true;
				break;
			}
			wordString++;
		}
		++state.Word;
	}
	while (!done);

	return state.Word - startWord;
}

static void DisassembleInstruction(DecompileState& state, Instruction& instruction, uint32_t numOperands)
{
	// Handle all the parameterized operands
	for (auto op = 0; op < spv::InstructionDesc[instruction.opCode].operands.getNum() && numOperands > 0; ++op)
	{
		const auto operandClass = spv::InstructionDesc[instruction.opCode].operands.getClass(op);
		switch (operandClass)
		{
		case spv::OperandId:
		case spv::OperandScope:
		case spv::OperandMemorySemantics:
		case spv::OperandLiteralNumber:
			instruction.arguments.push_back(state.Data[state.Word++]);
			--numOperands;
			break;
		case spv::OperandVariableIds:
		case spv::OperandImageOperands:
		case spv::OperandOptionalLiteral:
		case spv::OperandVariableLiterals:
		case spv::OperandMemoryAccess:
		case spv::OperandVariableIdLiteral:
		case spv::OperandVariableLiteralId:
			for (auto i = 0u; i < numOperands; ++i)
			{
				instruction.arguments.push_back(state.Data[state.Word++]);
			}
			return;
		case spv::OperandOptionalLiteralString:
		case spv::OperandLiteralString:
			numOperands -= DisassembleString(state, instruction);
			break;
		default:
			assert(operandClass >= spv::OperandSource && operandClass < spv::OperandOpcode);
			instruction.arguments.push_back(state.Data[state.Word++]);
			--numOperands;
			break;
		}
	}
}

static std::vector<Instruction> ProcessInstructions(DecompileState& state)
{
	std::vector<Instruction> instructions{};
	while (state.Word < state.Size)
	{
		const auto instructionStart = state.Word;

		// Instruction wordCount and opcode
		const auto firstWord = state.Data[state.Word];
		const auto wordCount = firstWord >> spv::WordCountShift;
		const auto opCode = static_cast<spv::Op>(firstWord & spv::OpCodeMask);
		const auto nextInst = state.Word + wordCount;
		++state.Word;

		// Presence of full instruction
		if (nextInst > state.Size)
		{
			FATAL_ERROR();
		}

		// Base for computing number of operands; will be updated as more is learned
		auto numOperands = wordCount - 1;

		// Type <id>
		spv::Id typeId = 0;
		if (spv::InstructionDesc[opCode].hasType())
		{
			typeId = state.Data[state.Word++];
			--numOperands;
		}

		// Result <id>
		spv::Id resultId = 0;
		if (spv::InstructionDesc[opCode].hasResult())
		{
			resultId = state.Data[state.Word++];
			--numOperands;
		}

		auto instruction = Instruction{resultId, typeId, opCode};
		DisassembleInstruction(state, instruction, numOperands);
		if (state.Word != nextInst)
		{
			FATAL_ERROR();
		}
		
		instructions.push_back(instruction);
	}
	
	return instructions;
}

VkResult ShaderModule::Create(const VkShaderModuleCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkShaderModule* pShaderModule)
{
	assert(pCreateInfo->sType == VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO);
	assert(pCreateInfo->codeSize % 4 == 0);

	auto shaderModule = Allocate<ShaderModule>(pAllocator);

	auto next = pCreateInfo->pNext;
	while (next)
	{
		auto type = *static_cast<const VkStructureType*>(next);
		switch (type)
		{
		default:
			FATAL_ERROR();
		}
	}

	if (pCreateInfo->flags)
	{
		FATAL_ERROR();
	}

	std::vector<uint32_t> data{};
	data.resize(pCreateInfo->codeSize / 4);
	memcpy(data.data(), pCreateInfo->pCode, pCreateInfo->codeSize);

	shaderModule->Disassemble(data);

	*pShaderModule = reinterpret_cast<VkShaderModule>(shaderModule);
	return VK_SUCCESS;
}

void ShaderModule::Disassemble(const std::vector<unsigned>& data)
{
	spv::Parameterize();

	DecompileState state
	{
		data
	};
	Validate(state);
	const auto instructions = ProcessInstructions(state);

	auto index = 0u;
	HandleInitial(instructions, index);

	while (index < instructions.size())
	{
		const auto& instruction = instructions[index];
		if (instruction.opCode == spv::OpString ||
			instruction.opCode == spv::OpSourceExtension ||
			instruction.opCode == spv::OpSource ||
			instruction.opCode == spv::OpSourceContinued ||
			instruction.opCode == spv::OpName ||
			instruction.opCode == spv::OpMemberName ||
			instruction.opCode == spv::OpModuleProcessed)
		{
			index++;
		}
		else
		{
			break;
		}
	}

	HandleTypes(state.MaxResults, instructions, index);

	FATAL_ERROR();
}

void ShaderModule::HandleInitial(const std::vector<Instruction>& instructions, uint32_t& index)
{
	while (index < instructions.size())
	{
		const auto& instruction = instructions[index];
		if (instruction.opCode == spv::OpCapability)
		{
			HandleCapability(instructions[index]);
			index += 1;
		}
		else
		{
			break;
		}
	}

	if (instructions[index].opCode == spv::OpExtension)
	{
		FATAL_ERROR();
	}
	
	while (index < instructions.size())
	{
		const auto& instruction = instructions[index];
		if (instruction.opCode == spv::OpExtInstImport)
		{
			auto name = reinterpret_cast<const char*>(instruction.arguments.data());
			// TODO:
			index += 1;
		}
		else
		{
			break;
		}
	}

	assert(instructions[index].opCode == spv::OpMemoryModel);
	addressingModel = static_cast<spv::AddressingModel>(instructions[index].arguments[0]);
	memoryModel = static_cast<spv::MemoryModel>(instructions[index].arguments[1]);
	index += 1;

	while (index < instructions.size())
	{
		const auto& instruction = instructions[index];
		if (instruction.opCode == spv::OpEntryPoint)
		{
			ShaderEntryPoint entryPoint{};
			entryPoint.ExecutionModel = static_cast<spv::ExecutionModel>(instruction.arguments[0]);
			entryPoint.EntryPoint = instruction.arguments[1];
			entryPoint.Name = reinterpret_cast<const char*>(&instruction.arguments[2]);
			auto i = 2 + (entryPoint.Name.size() + 4) / 4;
			for (; i < instruction.arguments.size(); i++)
			{
				entryPoint.Interface.push_back(instruction.arguments[i]);
			}
			entryPoints.push_back(entryPoint);
			index += 1;
		}
		else
		{
			break;
		}
	}

	while (index < instructions.size())
	{
		const auto& instruction = instructions[index];
		if (instruction.opCode == spv::OpExecutionMode)
		{
			FATAL_ERROR();
			index += 1;
		}
		else if (instruction.opCode == spv::OpExecutionModeId)
		{
			FATAL_ERROR();
			index += 1;
		}
		else
		{
			break;
		}
	}
}

struct Decorate
{
	uint32_t Struct;
	uint32_t Target;
	spv::Decoration Decoration;
	std::vector<uint32_t> Arguments;
};

enum class ShaderVariableType
{
	Unknown,
	Void,
	Function,
};

struct ShaderVariable
{
	ShaderVariableType Type;
	std::vector<uint32_t> Arguments;

	union
	{
		struct
		{
			uint32_t ReturnType;
		} Function;
	};
};

void ShaderModule::HandleTypes(uint32_t maxResults, const std::vector<Instruction>& instructions, uint32_t& index)
{
	std::vector<Decorate> decorations{};
	std::vector<ShaderVariable> shaderVariables{maxResults};
	
	while (index < instructions.size())
	{
		const auto& instruction = instructions[index];
		if (instruction.opCode == spv::OpDecorate)
		{
			Decorate decoration{};
			decoration.Struct = 0xFFFFFFFF;
			decoration.Target = instruction.arguments[0];
			decoration.Decoration = static_cast<spv::Decoration>(instruction.arguments[1]);
			for (auto i = 2u; i < instruction.arguments.size(); i++)
			{
				decoration.Arguments.push_back(instruction.arguments[i]);
			}
			decorations.push_back(decoration);
			index += 1;
		}
		else if (instruction.opCode == spv::OpMemberDecorate)
		{
			Decorate decoration{};
			decoration.Struct = instruction.arguments[0];
			decoration.Target = instruction.arguments[1];
			decoration.Decoration = static_cast<spv::Decoration>(instruction.arguments[2]);
			for (auto i = 3u; i < instruction.arguments.size(); i++)
			{
				decoration.Arguments.push_back(instruction.arguments[i]);
			}
			decorations.push_back(decoration);
			index += 1;
		}
		else if (instruction.opCode == spv::OpGroupDecorate)
		{
			FATAL_ERROR();
			index += 1;
		}
		else if (instruction.opCode == spv::OpGroupMemberDecorate)
		{
			FATAL_ERROR();
			index += 1;
		}
		else if (instruction.opCode == spv::OpDecorationGroup)
		{
			FATAL_ERROR();
			index += 1;
		}
		else
		{
			break;
		}
	}

	while (index < instructions.size())
	{
		const auto& instruction = instructions[index];
		if (instruction.opCode == spv::OpTypeVoid)
		{
			for (const auto& decorate : decorations)
			{
				if (decorate.Struct == 0xFFFFFFFF && decorate.Target == instruction.resultId)
				{
					FATAL_ERROR();
				}
			}

			shaderVariables[instruction.resultId] = ShaderVariable
			{
				ShaderVariableType::Void,
			};

			index++;
		}
		else if (instruction.opCode == spv::OpTypeFunction)
		{
			for (const auto& decorate : decorations)
			{
				if (decorate.Struct == 0xFFFFFFFF && decorate.Target == instruction.resultId)
				{
					FATAL_ERROR();
				}
			}

			shaderVariables[instruction.resultId] = ShaderVariable
			{
				ShaderVariableType::Function,
			};
			shaderVariables[instruction.resultId].Function.ReturnType = instruction.arguments[0];

			index++;
		}
		else
		{
			FATAL_ERROR();
		}
	}

	//     All type declarations (OpTypeXXX instructions), all constant instructions, and all global variable declarations (all OpVariable instructions whose Storage Class is not Function). This is the preferred location for OpUndef instructions, though they can also appear in function bodies. All operands in all these instructions must be declared before being used. Otherwise, they can be in any order. This section is the first section to allow use of OpLine debug information.
	//
	//     All function declarations ("declarations" are functions without a body; there is no forward declaration to a function with a body). A function declaration is as follows.
	//
	//         Function declaration, using OpFunction.
	//
	//         Function parameter declarations, using OpFunctionParameter.
	//
	//         Function end, using OpFunctionEnd.
	//
	//     All function definitions (functions with a body). A function definition is as follows.
	//
	//         Function definition, using OpFunction.
	//
	//         Function parameter declarations, using OpFunctionParameter.
	//
	//         Block
	//
	//         Block
	//
	//         …
	//
	//         Function end, using OpFunctionEnd.
	//
	// Within a function definition:
	//
	//     A block always starts with an OpLabel instruction. This may be immediately preceded by an OpLine instruction, but the OpLabel is considered as the beginning of the block.
	//
	//     A block always ends with a termination instruction (see validation rules for more detail).
	//
	//     All OpVariable instructions in a function must have a Storage Class of Function.
	//
	//     All OpVariable instructions in a function must be in the first block in the function. These instructions, together with any immediately preceding OpLine instructions, must be the first instructions in that block. (Note the validation rules prevent OpPhi instructions in the first block of a function.)
	//
	//     A function definition (starts with OpFunction) can be immediately preceded by an OpLine instruction.
	//
	// Forward references (an operand <id> that appears before the Result <id> defining it) are allowed for:
	//
	//     Operands that are an OpFunction. This allows for recursion and early declaration of entry points.
	//
	//     Annotation-instruction operands. This is required to fully know everything about a type or variable once it is declared.
	//
	//     Labels.
	//
	//     OpPhi can contain forward references.
	//
	//     An OpTypeForwardPointer has a forward reference to an OpTypePointer.
	//
	//     An OpTypeStruct operand that’s a forward reference to the Pointer Type operand to an OpTypeForwardPointer.
	//
	//     The list of <id> provided in the OpEntryPoint instruction.
	//
	//     OpExecutionModeId.
	//
	// In all cases, there is enough type information to enable a single simple pass through a module to transform it. For example, function calls have all the type information in the call, phi-functions don’t change type, and labels don’t have type. The pointer forward reference allows structures to contain pointers to themselves or to be mutually recursive (through pointers), without needing additional type information.
	//
	// The Validation Rules section lists additional rules that must be satisfied.
	FATAL_ERROR();
}

void ShaderModule::HandleCapability(const Instruction& instruction)
{
	const auto capability = static_cast<spv::Capability>(instruction.arguments[0]);
	if (capability >= capabilities.size())
	{
		FATAL_ERROR();
	}

	capabilities[capability] = true;

	switch (capability)
	{
	case spv::CapabilityShader:
		capabilities[spv::CapabilityMatrix] = true;
		break;
	case spv::CapabilityGeometry:
	case spv::CapabilityTessellation:
	case spv::CapabilityAtomicStorage:
	case spv::CapabilityImageGatherExtended:
	case spv::CapabilityStorageImageMultisample:
	case spv::CapabilityUniformBufferArrayDynamicIndexing:
	case spv::CapabilitySampledImageArrayDynamicIndexing:
	case spv::CapabilityStorageBufferArrayDynamicIndexing:
	case spv::CapabilityStorageImageArrayDynamicIndexing:
	case spv::CapabilityClipDistance:
	case spv::CapabilityCullDistance:
	case spv::CapabilitySampleRateShading:
	case spv::CapabilitySampledRect:
	case spv::CapabilityInputAttachment:
	case spv::CapabilitySparseResidency:
	case spv::CapabilityMinLod:
	case spv::CapabilitySampledCubeArray:
	case spv::CapabilityImageMSArray:
	case spv::CapabilityStorageImageExtendedFormats:
	case spv::CapabilityImageQuery:
	case spv::CapabilityDerivativeControl:
	case spv::CapabilityInterpolationFunction:
	case spv::CapabilityTransformFeedback:
	case spv::CapabilityStorageImageReadWithoutFormat:
	case spv::CapabilityStorageImageWriteWithoutFormat:
		capabilities[spv::CapabilityMatrix] = true;
		capabilities[spv::CapabilityShader] = true;
		break;
	case spv::CapabilityVector16:
	case spv::CapabilityFloat16Buffer:
	case spv::CapabilityImageBasic:
	case spv::CapabilityPipes:
	case spv::CapabilityDeviceEnqueue:
	case spv::CapabilityLiteralSampler:
	case spv::CapabilityNamedBarrier:
		capabilities[spv::CapabilityKernel] = true;
		break;
	case spv::CapabilityInt64Atomics:
		capabilities[spv::CapabilityInt64] = true;
		break;
	case spv::CapabilityImageReadWrite:
	case spv::CapabilityImageMipmap:
		capabilities[spv::CapabilityKernel] = true;
		capabilities[spv::CapabilityImageBasic] = true;
		break;
	case spv::CapabilityTessellationPointSize:
		capabilities[spv::CapabilityMatrix] = true;
		capabilities[spv::CapabilityShader] = true;
		capabilities[spv::CapabilityTessellation] = true;
		break;
	case spv::CapabilityGeometryPointSize:
	case spv::CapabilityGeometryStreams:
	case spv::CapabilityMultiViewport:
		capabilities[spv::CapabilityMatrix] = true;
		capabilities[spv::CapabilityShader] = true;
		capabilities[spv::CapabilityGeometry] = true;
		break;
	case spv::CapabilityImageCubeArray:
		capabilities[spv::CapabilityMatrix] = true;
		capabilities[spv::CapabilityShader] = true;
		capabilities[spv::CapabilitySampledCubeArray] = true;
		break;
	case spv::CapabilityImageRect:
		capabilities[spv::CapabilityMatrix] = true;
		capabilities[spv::CapabilityShader] = true;
		capabilities[spv::CapabilitySampledRect] = true;
		break;
	case spv::CapabilityGenericPointer:
		capabilities[spv::CapabilityAddresses] = true;
		break;
	case spv::CapabilityImage1D:
		capabilities[spv::CapabilitySampled1D] = true;
		break;
	case spv::CapabilityImageBuffer:
		capabilities[spv::CapabilitySampledBuffer] = true;
		break;
	case spv::CapabilitySubgroupDispatch:
		capabilities[spv::CapabilityKernel] = true;
		capabilities[spv::CapabilityDeviceEnqueue] = true;
		break;
	case spv::CapabilityPipeStorage:
		capabilities[spv::CapabilityKernel] = true;
		capabilities[spv::CapabilityPipes] = true;
		break;
	case spv::CapabilityGroupNonUniformVote:
	case spv::CapabilityGroupNonUniformArithmetic:
	case spv::CapabilityGroupNonUniformBallot:
	case spv::CapabilityGroupNonUniformShuffle:
	case spv::CapabilityGroupNonUniformShuffleRelative:
	case spv::CapabilityGroupNonUniformClustered:
	case spv::CapabilityGroupNonUniformQuad:
		capabilities[spv::CapabilityGroupNonUniform] = true;
		break;
	}
}
