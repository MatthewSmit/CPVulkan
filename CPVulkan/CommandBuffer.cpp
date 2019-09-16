#include "CommandBuffer.h"

#include "Buffer.h"
#include "DescriptorSet.h"
#include "DeviceState.h"
#include "Event.h"
#include "Formats.h"
#include "Framebuffer.h"
#include "Image.h"
#include "ImageView.h"
#include "Pipeline.h"
#include "RenderPass.h"

#include <SPIRVFunction.h>
#include <SPIRVInstruction.h>
#include <SPIRVModule.h>

#include <glm/glm.hpp>

#include <cassert>
#include <stack>

constexpr auto MAX_FRAGMENT_ATTACHMENTS = 4;

class PipelineLayout;

struct Variable
{
	enum class Type
	{
		Unknown,
		Pointer,
		UniformBuffer,
		BuiltinBuffer,
		Vector4Float,
		Matrix4x4Float,
	} type;

	Variable() = default;

	Variable(uint8_t* pointer)
	{
		type = Type::Pointer;
		this->pointer = pointer;
	}

	Variable(const glm::vec4& vector)
	{
		type = Type::Vector4Float;
		f32.v4 = vector;
	}

	Variable(const VkDescriptorBufferInfo& bufferInfo, SPIRV::SPIRVTypeStruct* structType)
	{
		type = Type::UniformBuffer;
		buffer.info = bufferInfo;
		buffer.type = structType;
	}

	Variable(const glm::mat4x4& matrix)
	{
		type = Type::Matrix4x4Float;
		f32.m4x4 = matrix;
	}

	Variable(const std::vector<Variable>* builtins, SPIRV::SPIRVTypeStruct* structType)
	{
		type = Type::BuiltinBuffer;
		builtin.vector = builtins;
		builtin.type = structType;
	}

	union
	{
		union
		{
			glm::vec4 v4;
			glm::mat4x4 m4x4;
		} f32;

		struct
		{
			VkDescriptorBufferInfo info;
			SPIRV::SPIRVTypeStruct* type;
		} buffer;

		struct
		{
			const std::vector<Variable>* vector;
			SPIRV::SPIRVTypeStruct* type;
		} builtin;

		uint8_t* pointer;
	};

	Variable GetChild(int index) const
	{
		switch (type)
		{
		case Type::UniformBuffer:
			{
				auto columnMajor = false;
				auto rowMajor = false;
				auto offset = 0u;
				auto matrixStride = 0u;
				for (const auto& memberDecorate : buffer.type->getMemberDecorates())
				{
					if (memberDecorate.first.first == index && memberDecorate.first.second == DecorationColMajor)
					{
						columnMajor = true;
					}
					else if (memberDecorate.first.first == index && memberDecorate.first.second == DecorationRowMajor)
					{
						rowMajor = true;
					}
					else if (memberDecorate.first.first == index && memberDecorate.first.second == DecorationOffset)
					{
						offset = memberDecorate.second->getLiteral(0);
					}
					else if (memberDecorate.first.first == index && memberDecorate.first.second == DecorationMatrixStride)
					{
						matrixStride = memberDecorate.second->getLiteral(0);
					}
				}

				const auto memberType = buffer.type->getMemberType(index);
				switch (memberType->getOpCode())
				{
				case OpTypeMatrix:
					{
						if ((columnMajor && rowMajor) || (!columnMajor && !rowMajor))
						{
							FATAL_ERROR();
						}

						if (rowMajor)
						{
							FATAL_ERROR();
						}

						if (matrixStride != 16)
						{
							FATAL_ERROR();
						}

						if (memberType->getMatrixColumnCount() != 4 || memberType->getMatrixRowCount() != 4)
						{
							FATAL_ERROR();
						}

						const auto data = reinterpret_cast<Buffer*>(buffer.info.buffer)->getData(buffer.info.offset + offset, sizeof(glm::mat4x4));
						return Variable(data);
					}

				default:
					FATAL_ERROR();
				}
			}

		case Type::BuiltinBuffer:
			return (*builtin.vector)[index];

		default:
			FATAL_ERROR();
		}
	}

	Variable Dereference(SPIRV::SPIRVType* type) const
	{
		if (this->type != Type::Pointer)
		{
			FATAL_ERROR();
		}

		if (type->isTypeMatrix())
		{
			if (type->getMatrixColumnCount() == 4)
			{
				if (type->getMatrixRowCount() == 4)
				{
					if (type->getMatrixComponentType()->isTypeFloat())
					{
						return Variable(*reinterpret_cast<const glm::mat4x4*>(pointer));
					}
					else
					{
						FATAL_ERROR();
					}
				}
				else
				{
					FATAL_ERROR();
				}
			}
			else
			{
				FATAL_ERROR();
			}
		}

		if (type->isTypeVector())
		{
			if (type->getVectorComponentCount() == 4)
			{
				if (type->getVectorComponentType()->isTypeFloat())
				{
					return Variable(*reinterpret_cast<const glm::vec4*>(pointer));
				}
				else
				{
					FATAL_ERROR();
				}
			}
			else
			{
				FATAL_ERROR();
			}
		}

		FATAL_ERROR();
	}

	void SetValue(const Variable& variable)
	{
		if (this->type != Type::Pointer)
		{
			FATAL_ERROR();
		}

		switch (variable.type)
		{
		case Type::Vector4Float:
			*reinterpret_cast<glm::vec4*>(pointer) = variable.f32.v4;
			break;

		default:
			FATAL_ERROR();
		}
	}
};

struct ShaderState
{
	std::vector<Variable> scratch;
	std::vector<Variable> input;
	std::array<std::vector<Variable>, MAX_DESCRIPTOR_SETS> uniforms;
	std::vector<Variable>* output;
	std::vector<Variable>* builtins;
};

struct VertexOutput
{
	std::unique_ptr<uint8_t[]> data;
	std::vector<std::vector<Variable>> outputs;
	std::vector<std::vector<Variable>> builtins;
	uint64_t variableStride;
	uint64_t vertexStride;
};

template<typename Size>
static void SetPixel(void* data, const FormatInformation& format, uint32_t x, uint32_t y, uint32_t z, uint32_t width, uint32_t height, uint32_t depth, uint64_t values[4])
{
	static_assert(std::numeric_limits<Size>::is_integer);
	static_assert(!std::numeric_limits<Size>::is_signed);
	
	if (format.ElementSize != sizeof(Size))
	{
		FATAL_ERROR();
	}

	const auto pixelSize = static_cast<uint64_t>(format.TotalSize);
	const auto stride = width * pixelSize;
	const auto pane = height * stride;

	auto pixel = reinterpret_cast<Size*>(reinterpret_cast<uint8_t*>(data) +
		static_cast<uint64_t>(z) * pane +
		static_cast<uint64_t>(y) * stride +
		static_cast<uint64_t>(x) * pixelSize);

	if (format.RedOffset != -1) pixel[format.RedOffset] = static_cast<Size>(values[0]);
	if (format.GreenOffset != -1) pixel[format.GreenOffset] = static_cast<Size>(values[1]);
	if (format.BlueOffset != -1) pixel[format.BlueOffset] = static_cast<Size>(values[2]);
	if (format.AlphaOffset != -1) pixel[format.AlphaOffset] = static_cast<Size>(values[3]);
}

template<typename Size>
static void SetPackedPixel(void* data, const FormatInformation& format, uint32_t x, uint32_t y, uint32_t z, uint32_t width, uint32_t height, uint32_t depth, uint64_t values[4])
{
	static_assert(std::numeric_limits<Size>::is_integer);
	static_assert(!std::numeric_limits<Size>::is_signed);

	const auto pixelSize = static_cast<uint64_t>(format.TotalSize);
	const auto stride = width * pixelSize;
	const auto pane = height * stride;

	auto pixel = reinterpret_cast<Size*>(reinterpret_cast<uint8_t*>(data) +
		static_cast<uint64_t>(z) * pane +
		static_cast<uint64_t>(y) * stride +
		static_cast<uint64_t>(x) * pixelSize);

	*pixel = static_cast<Size>(values[0]);
}

template<typename Size>
static void GetPixel(void* data, const FormatInformation& format, uint32_t x, uint32_t y, uint32_t z, uint32_t width, uint32_t height, uint32_t depth, uint64_t values[4])
{
	static_assert(std::numeric_limits<Size>::is_integer);
	static_assert(!std::numeric_limits<Size>::is_signed);

	if (format.ElementSize != sizeof(Size))
	{
		FATAL_ERROR();
	}

	const auto pixelSize = static_cast<uint64_t>(format.TotalSize);
	const auto stride = width * pixelSize;
	const auto pane = height * stride;

	auto pixel = reinterpret_cast<Size*>(reinterpret_cast<uint8_t*>(data) +
		static_cast<uint64_t>(z) * pane +
		static_cast<uint64_t>(y) * stride +
		static_cast<uint64_t>(x) * pixelSize);

	if (format.RedOffset != -1) values[0] = static_cast<uint64_t>(pixel[format.RedOffset]);
	if (format.GreenOffset != -1) values[1] = static_cast<uint64_t>(pixel[format.GreenOffset]);
	if (format.BlueOffset != -1) values[2] = static_cast<uint64_t>(pixel[format.BlueOffset]);
	if (format.AlphaOffset != -1) values[3] = static_cast<uint64_t>(pixel[format.AlphaOffset]);
}

static void SetPixel(const FormatInformation& format, Image* image, uint32_t x, uint32_t y, uint32_t z, uint64_t values[4])
{
	if (format.BaseType == BaseType::UNorm || format.BaseType == BaseType::SNorm)
	{
		if (format.ElementSize == 0 && format.TotalSize == 1)
		{
			SetPackedPixel<uint8_t>(image->getData(), format, x, y, z, image->getWidth(), image->getHeight(), image->getDepth(), values);
		}
		else if (format.ElementSize == 0 && format.TotalSize == 2)
		{
			SetPackedPixel<uint16_t>(image->getData(), format, x, y, z, image->getWidth(), image->getHeight(), image->getDepth(), values);
		}
		else if (format.ElementSize == 0 && format.TotalSize == 4)
		{
			SetPackedPixel<uint32_t>(image->getData(), format, x, y, z, image->getWidth(), image->getHeight(), image->getDepth(), values);
		}
		else if (format.ElementSize == 1)
		{
			SetPixel<uint8_t>(image->getData(), format, x, y, z, image->getWidth(), image->getHeight(), image->getDepth(), values);
		}
		else if (format.ElementSize == 2)
		{
			SetPixel<uint16_t>(image->getData(), format, x, y, z, image->getWidth(), image->getHeight(), image->getDepth(), values);
		}
		else
		{
			FATAL_ERROR();
		}
	}
	else
	{
		FATAL_ERROR();
	}
}

template<typename T1, typename T2>
static void Convert(const uint64_t source[4], T2 destination[4]);

template<>
static void Convert<uint16_t, float>(const uint64_t source[4], float destination[4])
{
	destination[0] = static_cast<float>(static_cast<uint16_t>(source[0])) / std::numeric_limits<uint16_t>::max();
	destination[1] = static_cast<float>(static_cast<uint16_t>(source[1])) / std::numeric_limits<uint16_t>::max();
	destination[2] = static_cast<float>(static_cast<uint16_t>(source[2])) / std::numeric_limits<uint16_t>::max();
	destination[3] = static_cast<float>(static_cast<uint16_t>(source[3])) / std::numeric_limits<uint16_t>::max();
}

template<typename T>
static void GetPixel(const FormatInformation& format, Image* image, uint32_t x, uint32_t y, uint32_t z, T values[4])
{
	if (format.BaseType == BaseType::UNorm)
	{
		if (format.ElementSize == 2)
		{
			uint64_t rawValues[4];
			GetPixel<uint16_t>(image->getData(), format, x, y, z, image->getWidth(), image->getHeight(), image->getDepth(), rawValues);
			Convert<uint16_t, T>(rawValues, values);
		}
		else
		{
			FATAL_ERROR();
		}
	}
	else
	{
		FATAL_ERROR();
	}
}

static void ClearImage(Image* image, const FormatInformation& format, uint64_t values[4])
{
	for (auto z = 0u; z < image->getDepth(); z++)
	{
		for (auto y = 0u; y < image->getHeight(); y++)
		{
			for (auto x = 0u; x < image->getWidth(); x++)
			{
				SetPixel(format, image, x, y, z, values);
			}
		}
	}
}

static void GetImageColour(uint64_t output[4], const FormatInformation& format, const VkClearColorValue& input)
{
	if (format.BaseType == BaseType::UNorm)
	{
		if (format.ElementSize == 0)
		{
			output[0] = 0;
			if (format.RedPack)
			{
				const auto size = (1 << format.RedPack) - 1;
				const auto value = static_cast<uint64_t>(input.float32[0] * size);
				output[0] |= value << format.RedOffset;
			}
			if (format.GreenPack)
			{
				const auto size = (1 << format.GreenPack) - 1;
				const auto value = static_cast<uint64_t>(input.float32[1] * size);
				output[0] |= value << format.GreenOffset;
			}
			if (format.BluePack)
			{
				const auto size = (1 << format.BluePack) - 1;
				const auto value = static_cast<uint64_t>(input.float32[2] * size);
				output[0] |= value << format.BlueOffset;
			}
			if (format.AlphaPack)
			{
				const auto size = (1 << format.AlphaPack) - 1;
				const auto value = static_cast<uint64_t>(input.float32[3] * size);
				output[0] |= value << format.AlphaOffset;
			}
		}
		else if (format.ElementSize == 1)
		{
			output[0] = static_cast<uint64_t>(std::round(input.float32[0] * std::numeric_limits<uint8_t>::max()));
			output[1] = static_cast<uint64_t>(std::round(input.float32[1] * std::numeric_limits<uint8_t>::max()));
			output[2] = static_cast<uint64_t>(std::round(input.float32[2] * std::numeric_limits<uint8_t>::max()));
			output[3] = static_cast<uint64_t>(std::round(input.float32[3] * std::numeric_limits<uint8_t>::max()));
		}
		else if (format.ElementSize == 2)
		{
			output[0] = static_cast<uint64_t>(std::round(input.float32[0] * std::numeric_limits<uint16_t>::max()));
			output[1] = static_cast<uint64_t>(std::round(input.float32[1] * std::numeric_limits<uint16_t>::max()));
			output[2] = static_cast<uint64_t>(std::round(input.float32[2] * std::numeric_limits<uint16_t>::max()));
			output[3] = static_cast<uint64_t>(std::round(input.float32[3] * std::numeric_limits<uint16_t>::max()));
		}
		else
		{
			FATAL_ERROR();
		}
	}
	else if (format.BaseType == BaseType::SNorm)
	{
		if (format.ElementSize == 0)
		{
			output[0] = 0;
			if (format.RedPack)
			{
				const auto size = (1 << (format.RedPack - 1)) - 1;
				const auto value = static_cast<int64_t>(input.float32[0] * size);
				output[0] |= value << format.RedOffset;
			}
			if (format.GreenPack)
			{
				const auto size = (1 << (format.GreenPack - 1)) - 1;
				const auto value = static_cast<int64_t>(input.float32[1] * size);
				output[0] |= value << format.GreenOffset;
			}
			if (format.BluePack)
			{
				const auto size = (1 << (format.BluePack - 1)) - 1;
				const auto value = static_cast<int64_t>(input.float32[2] * size);
				output[0] |= value << format.BlueOffset;
			}
			if (format.AlphaPack)
			{
				const auto size = (1 << (format.AlphaPack - 1)) - 1;
				const auto value = static_cast<int64_t>(input.float32[3] * size);
				output[0] |= value << format.AlphaOffset;
			}
		}
		else if (format.ElementSize == 1)
		{
			output[0] = static_cast<int64_t>(std::round(input.float32[0] * std::numeric_limits<int8_t>::max()));
			output[1] = static_cast<int64_t>(std::round(input.float32[1] * std::numeric_limits<int8_t>::max()));
			output[2] = static_cast<int64_t>(std::round(input.float32[2] * std::numeric_limits<int8_t>::max()));
			output[3] = static_cast<int64_t>(std::round(input.float32[3] * std::numeric_limits<int8_t>::max()));
		}
		else if (format.ElementSize == 2)
		{
			output[0] = static_cast<int64_t>(std::round(input.float32[0] * std::numeric_limits<int16_t>::max()));
			output[1] = static_cast<int64_t>(std::round(input.float32[1] * std::numeric_limits<int16_t>::max()));
			output[2] = static_cast<int64_t>(std::round(input.float32[2] * std::numeric_limits<int16_t>::max()));
			output[3] = static_cast<int64_t>(std::round(input.float32[3] * std::numeric_limits<int16_t>::max()));
		}
		else
		{
			FATAL_ERROR();
		}
	}
	else
	{
		FATAL_ERROR();
	}
}

static void ClearImage(Image* image, VkFormat format, VkClearColorValue colour)
{
	const auto& information = GetFormatInformation(format);
	uint64_t values[4];
	GetImageColour(values, information, colour);
	ClearImage(image, information, values);
}

static void ClearImage(Image* image, VkFormat format, VkClearDepthStencilValue colour)
{
	const auto& information = GetFormatInformation(format);
	uint64_t data[4];
	switch (format)
	{
	case VK_FORMAT_D16_UNORM:
	case VK_FORMAT_D16_UNORM_S8_UINT:
		data[0] = static_cast<uint64_t>(colour.depth * std::numeric_limits<uint16_t>::max());
		data[1] = colour.stencil;
		break;
	default:
		FATAL_ERROR();
	}

	ClearImage(image, information, data);
}

static Variable LoadValue(ShaderState& state, SPIRV::SPIRVValue* value)
{
	if (value->getOpCode() == OpVariable)
	{
		const auto variable = reinterpret_cast<SPIRV::SPIRVVariable*>(value);
		switch (variable->getStorageClass())
		{
		case StorageClassInput:
			{
				auto locationDecorate = variable->getDecorate(DecorationLocation);
				if (locationDecorate.size() != 1)
				{
					FATAL_ERROR();
				}

				const auto location = *locationDecorate.begin();
				return state.input[location];
			}

		case StorageClassOutput:
			{
				auto locationDecorate = variable->getDecorate(DecorationLocation);
				if (locationDecorate.size() != 1)
				{
					auto builtinDecorate = variable->getDecorate(DecorationBuiltIn);
					if (builtinDecorate.size() != 1)
					{
						auto type = variable->getType();
						if (type->isTypePointer())
						{
							type = type->getPointerElementType();
							if (type->isTypeStruct())
							{
								if (type->hasDecorate(DecorationBlock))
								{
									for (const auto& decorate : type->getMemberDecorates())
									{
										if (decorate.first.second == DecorationBuiltIn)
										{
											return Variable(state.builtins, reinterpret_cast<SPIRV::SPIRVTypeStruct*>(type));
										}
									}
								}
							}
						}
						
						FATAL_ERROR();
					}
					else
					{
						const auto builtin = *builtinDecorate.begin();
						return (*state.builtins)[builtin];
					}
				}

				const auto location = *locationDecorate.begin();
				return (*state.output)[location];
			}
			
		case StorageClassUniform:
			{
				auto bindingDecorate = variable->getDecorate(DecorationBinding);
				if (bindingDecorate.size() != 1)
				{
					FATAL_ERROR();
				}

				auto setDecorate = variable->getDecorate(DecorationDescriptorSet);
				if (setDecorate.size() != 1)
				{
					FATAL_ERROR();
				}

				const auto binding = *bindingDecorate.begin();
				const auto set = *setDecorate.begin();
				return state.uniforms[set][binding];
			}

		default:
			FATAL_ERROR();
		}
	}
	return state.scratch[value->getId()];
}

static Variable LoadConstant(SPIRV::SPIRVValue* value)
{
	switch (value->getOpCode())
	{
	case OpConstant:
		FATAL_ERROR();
		
	case OpConstantComposite:
		{
			const auto constantComposite = reinterpret_cast<SPIRV::SPIRVConstantComposite*>(value);
			const auto type = constantComposite->getType();

			if (type->isTypeMatrix())
			{
				FATAL_ERROR();
			}
			else if (type->isTypeVector())
			{
				if (type->getVectorComponentCount() == 4)
				{
					if (type->getVectorComponentType()->getOpCode() == OpTypeFloat)
					{
						if (constantComposite->getElements().size() != 4)
						{
							FATAL_ERROR();
						}

						return Variable(glm::vec4
							{
								reinterpret_cast<SPIRV::SPIRVConstant*>(constantComposite->getElements()[0])->getFloatValue(),
								reinterpret_cast<SPIRV::SPIRVConstant*>(constantComposite->getElements()[1])->getFloatValue(),
								reinterpret_cast<SPIRV::SPIRVConstant*>(constantComposite->getElements()[2])->getFloatValue(),
								reinterpret_cast<SPIRV::SPIRVConstant*>(constantComposite->getElements()[3])->getFloatValue(),
							});
					}
					else
					{
						FATAL_ERROR();
					}
				}
				else
				{
					FATAL_ERROR();
				}
			}
			else
			{
				FATAL_ERROR();
			}
		}
		
	default:
		FATAL_ERROR();
	}
}

static bool InterpretBlock(ShaderState& state, SPIRV::SPIRVBasicBlock* basicBlock)
{
	for (auto i = 0u; i < basicBlock->getNumInst(); i++)
	{
		const auto instruction = basicBlock->getInst(i);
		switch (instruction->getOpCode())
		{
		case OpLoad:
			{
				const auto load = reinterpret_cast<SPIRV::SPIRVLoad*>(instruction);
				const auto source = load->getSrc();
				state.scratch[instruction->getId()] = LoadValue(state, source).Dereference(load->getType());
				break;
			}

		case OpStore:
			{
				const auto store = reinterpret_cast<SPIRV::SPIRVStore*>(instruction);
				const auto dest = store->getDst();
				auto pointer = LoadValue(state, dest);

				switch (store->getSrc()->getOpCode())
				{
				case OpConstant:
				case OpConstantComposite:
					pointer.SetValue(LoadConstant(store->getSrc()));
					break;
					
				default:
					pointer.SetValue(state.scratch[store->getSrc()->getId()]);
					break;
				}
				
				break;
			}

		case OpAccessChain:
			{
				const auto accessChain = reinterpret_cast<SPIRV::SPIRVAccessChain*>(instruction);
				auto base = LoadValue(state, accessChain->getBase());
				for (const auto& indexValue : accessChain->getIndices())
				{
					const auto index = static_cast<int32_t>(reinterpret_cast<SPIRV::SPIRVConstant*>(indexValue)->getZExtIntValue() & 0xFFFFFFFF);
					base = base.GetChild(index);
				}

				state.scratch[instruction->getId()] = base;
				break;
			}

		case OpMatrixTimesVector:
			{
				const auto matrix = LoadValue(state, reinterpret_cast<SPIRV::SPIRVMatrixTimesVector*>(instruction)->getMatrix());
				const auto vector = LoadValue(state, reinterpret_cast<SPIRV::SPIRVMatrixTimesVector*>(instruction)->getVector());

				if (matrix.type != Variable::Type::Matrix4x4Float)
				{
					FATAL_ERROR();
				}

				if (vector.type != Variable::Type::Vector4Float)
				{
					FATAL_ERROR();
				}

				const auto result = Variable(matrix.f32.m4x4 * vector.f32.v4);
				state.scratch[instruction->getId()] = result;
				break;
			}

		case OpReturn:
			return false;
			
		default:
			FATAL_ERROR();
		}
	}
	
	FATAL_ERROR();
}

static void InterpretShader(ShaderState& state, const SPIRV::SPIRVModule* module, SPIRV::SPIRVExecutionModelKind executionModel, const std::string& entryPoint)
{
	const auto entry = module->getEntryPoint(executionModel, 0);
	if (entry->getName() != entryPoint)
	{
		FATAL_ERROR();
	}

	for (auto i = 0u; i < entry->getNumBasicBlock(); i++)
	{
		if (!InterpretBlock(state, entry->getBasicBlock(i)))
		{
			return;
		}
	}
}

static const VkVertexInputBindingDescription& FindBinding(uint32_t binding, const VertexInputState& vertexInputState)
{
	for (const auto& bindingDescription : vertexInputState.VertexBindingDescriptions)
	{
		if (bindingDescription.binding == binding)
		{
			return bindingDescription;
		}
	}
	FATAL_ERROR();
}

static std::vector<Variable> LoadVertexInput(uint32_t vertex, const VertexInputState& vertexInputState, const Buffer* const vertexBinding[16], const VkDeviceSize vertexBindingOffset[16])
{
	auto maxLocation = 0u;
	for (const auto& attribute : vertexInputState.VertexAttributeDescriptions)
	{
		maxLocation = std::max(maxLocation, attribute.location);
	}

	auto result = std::vector<Variable>(maxLocation + 1);
	for (const auto& attribute : vertexInputState.VertexAttributeDescriptions)
	{
		const auto& binding = FindBinding(attribute.binding, vertexInputState);
		const auto offset = vertexBindingOffset[binding.binding] + binding.stride * vertex + attribute.offset;
		result[attribute.location] = Variable(vertexBinding[binding.binding]->getData(offset, binding.stride));
	}
	return result;
}

static std::array<std::vector<Variable>, MAX_DESCRIPTOR_SETS> LoadUniforms(DeviceState* deviceState, const SPIRV::SPIRVModule* module)
{
	std::array<std::vector<Variable>, MAX_DESCRIPTOR_SETS> results{};
	
	for (auto i = 0u; i < MAX_DESCRIPTOR_SETS; i++)
	{
		const auto descriptorSet = deviceState->descriptorSets[i][0];
		if (descriptorSet)
		{
			auto maxBinding = -1;
			for (const auto& binding : descriptorSet->getBindings())
			{
				if (std::get<0>(binding) == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
				{
					maxBinding = std::max(static_cast<int32_t>(std::get<1>(binding)), maxBinding);
				}
			}

			results[i].resize(maxBinding + 1);
			for (const auto& binding : descriptorSet->getBindings())
			{
				if (std::get<0>(binding) == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
				{
					for (auto j = 0u; j < module->getNumVariables(); j++)
					{
						const auto variable = module->getVariable(j);
						if (variable->getStorageClass() == StorageClassUniform)
						{
							auto bindingDecorate = variable->getDecorate(DecorationBinding);
							if (bindingDecorate.size() != 1)
							{
								FATAL_ERROR();
							}

							auto setDecorate = variable->getDecorate(DecorationDescriptorSet);
							if (setDecorate.size() != 1)
							{
								FATAL_ERROR();
							}

							const auto bindingNmber = *bindingDecorate.begin();
							const auto set = *setDecorate.begin();

							if (set == i && bindingNmber == std::get<1>(binding))
							{
								auto type = variable->getType();
								if (type->getOpCode() != OpTypePointer)
								{
									FATAL_ERROR();
								}
								type = reinterpret_cast<SPIRV::SPIRVTypePointer*>(type)->getElementType();
								switch (type->getOpCode())
								{
								case OpTypeStruct:
									{
										const auto structType = reinterpret_cast<SPIRV::SPIRVTypeStruct*>(type);
										results[i][std::get<1>(binding)] = Variable(std::get<2>(binding).BufferInfo, structType);
										goto found_variable;
									}
									
								default:
									FATAL_ERROR();
								}
							}
						}
					}
				}

			found_variable:
				continue;
			}
		}
	}

	return results;
}

static float EdgeFunction(const glm::vec4& a, const glm::vec4& b, const glm::vec2& c)
{
	return (c.x - a.x) * (b.y - a.y) - (c.y - a.y) * (b.x - a.x);
}

static VertexOutput ProcessVertexShader(DeviceState* deviceState, uint32_t vertexCount)
{
	const auto& shaderStage = deviceState->pipeline[0]->getShaderStage(0);
	const auto& vertexInput = deviceState->pipeline[0]->getVertexInputState();
	
	const auto module = shaderStage->getModule();

	auto maxLocation = -1;
	for (auto i = 0u; i < module->getNumVariables(); i++)
	{
		const auto variable = module->getVariable(i);
		if (variable->getStorageClass() == StorageClassOutput)
		{
			auto locations = variable->getDecorate(DecorationLocation);
			if (locations.empty())
			{
				continue;
			}

			auto location = *locations.begin();
			maxLocation = std::max(maxLocation, int32_t(location));
		}
	}

	VertexOutput output
	{
		std::unique_ptr<uint8_t[]>(new uint8_t[vertexCount * (maxLocation + 1 + 1) * 256]),
		std::vector<std::vector<Variable>>(vertexCount),
		std::vector<std::vector<Variable>>(vertexCount),
		256,
		(maxLocation + 1 + 1) * 256,
	};
	for (auto i = 0u; i < vertexCount; i++)
	{
		auto ptr = output.data.get() + i * output.vertexStride;
		output.outputs[i].resize(maxLocation + 1);
		for (auto j = 0u; j < maxLocation + 1; j++)
		{
			output.outputs[i][j] = Variable(ptr);
			ptr += output.variableStride;
		}
		output.builtins[i].resize(1);
		for (auto j = 0u; j < 1; j++)
		{
			output.builtins[i][j] = Variable(ptr);
			ptr += output.variableStride;
		}
	}
	
	ShaderState state
	{
		std::vector<Variable>(128),
		{},
		LoadUniforms(deviceState, module),
		nullptr,
		nullptr
	};
	for (auto i = 0u; i < vertexCount; i++)
	{
		state.input = LoadVertexInput(i, vertexInput, deviceState->vertexBinding, deviceState->vertexBindingOffset);
		state.output = &output.outputs[i];
		state.builtins = &output.builtins[i];
		InterpretShader(state, shaderStage->getModule(), SPIRV::SPIRVExecutionModelKind::ExecutionModelVertex, shaderStage->getName());
	}

	return output;
}

static bool GetFragmentInput(std::vector<Variable>& fragmentInput, const std::vector<std::vector<Variable>>& vertexOutput, const std::vector<SPIRV::SPIRVType*>& types, std::vector<Variable>& storage,
                             uint32_t p0Index, uint32_t p1Index, uint32_t p2Index,
                             const glm::vec4& p0, const glm::vec4& p1, const glm::vec4& p2, const glm::vec2& p, float& depth)
{
	const auto area = EdgeFunction(p0, p1, p2);
	auto w0 = EdgeFunction(p1, p2, p);
	auto w1 = EdgeFunction(p2, p0, p);
	auto w2 = EdgeFunction(p0, p1, p);

	if (w0 >= 0 && w1 >= 0 && w2 >= 0)
	{
		w0 /= area;
		w1 /= area;
		w2 /= area;

		depth = (p0 * w0 + p1 * w1 + p2 * w2).z;

		for (auto i = 0u; i < fragmentInput.size(); i++)
		{
			const auto v0 = vertexOutput[p0Index][i].Dereference(types[i]);
			const auto v1 = vertexOutput[p1Index][i].Dereference(types[i]);
			const auto v2 = vertexOutput[p2Index][i].Dereference(types[i]);
			switch (v0.type)
			{
			case Variable::Type::Vector4Float:
				storage[i] = v0.f32.v4 * w0 + v1.f32.v4 * w1 + v2.f32.v4 * w2;
				fragmentInput[i] = Variable(reinterpret_cast<uint8_t*>(&storage[i].f32.v4));
				break;
			default:
				FATAL_ERROR();
			}
		}
		
		return true;
	}

	return false;
}

static float GetDepthPixel(const FormatInformation& format, Image* image, uint32_t x, uint32_t y)
{
	float values[4];
	GetPixel(format, image, x, y, 0, values);
	return values[0];
}

static void ProcessFragmentShader(DeviceState* deviceState, const VertexOutput& output)
{
	const auto& inputAssembly = deviceState->pipeline[0]->getInputAssemblyState();

	std::vector<glm::vec4> vertices(output.outputs.size());
	for (auto i = 0u; i < output.outputs.size(); i++)
	{
		const auto ptr = output.data.get() + i * output.vertexStride + (output.vertexStride - output.variableStride);
		vertices[i] = *reinterpret_cast<const glm::vec4*>(ptr);
		vertices[i] = vertices[i] / vertices[i].w;
	}

	if (inputAssembly.Topology != VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
	{
		FATAL_ERROR();
	}

	std::vector<std::pair<VkAttachmentDescription, Image*>> images{MAX_FRAGMENT_ATTACHMENTS};
	std::pair<VkAttachmentDescription, Image*> depthImage;

	for (auto i = 0; i < deviceState->renderPass->getSubpasses()[0].ColourAttachments.size(); i++)
	{
		const auto attachmentIndex = deviceState->renderPass->getSubpasses()[0].ColourAttachments[i].attachment;
		const auto& attachment = deviceState->renderPass->getAttachments()[attachmentIndex];
		images[i] = std::make_pair(attachment, deviceState->framebuffer->getAttachments()[attachmentIndex]->getImage());
	}

	if (deviceState->renderPass->getSubpasses()[0].DepthStencilAttachment.layout != VK_IMAGE_LAYOUT_UNDEFINED)
	{
		const auto attachmentIndex = deviceState->renderPass->getSubpasses()[0].DepthStencilAttachment.attachment;
		const auto& attachment = deviceState->renderPass->getAttachments()[attachmentIndex];
		depthImage = std::make_pair(attachment, deviceState->framebuffer->getAttachments()[attachmentIndex]->getImage());
	}
	const auto& depthFormat = GetFormatInformation(depthImage.first.format);

	const auto image = images[0].second;
	const auto halfPixel = glm::vec2(1.0f / image->getWidth(), 1.0f / image->getHeight()) * 0.5f;

	const auto& shaderStage = deviceState->pipeline[0]->getShaderStage(4);

	const auto module = shaderStage->getModule();

	std::vector<Variable> outputStorage{MAX_FRAGMENT_ATTACHMENTS};
	std::vector<Variable> outputs{MAX_FRAGMENT_ATTACHMENTS};
	for (auto i = 0u; i < MAX_FRAGMENT_ATTACHMENTS; i++)
	{
		outputs[i] = Variable(reinterpret_cast<uint8_t*>(&outputStorage[i].f32.v4));
	}
	std::vector<Variable> builtins{};
	std::vector<SPIRV::SPIRVType*> inputTypes(output.outputs[0].size());

	for (auto i = 0u; i < module->getNumVariables(); i++)
	{
		const auto variable = module->getVariable(i);
		if (variable->getStorageClass() == StorageClassInput)
		{
			const auto locationDecorate = variable->getDecorate(DecorationLocation);
			if (locationDecorate.size() != 1)
			{
				FATAL_ERROR();
			}

			const auto location = *locationDecorate.begin();
			const auto type = variable->getType()->getPointerElementType();
			inputTypes[location] = type;
		}
	}
	
	ShaderState state
	{
		std::vector<Variable>(128),
		std::vector<Variable>(output.outputs[0].size()),
		LoadUniforms(deviceState, module),
		&outputs,
		&builtins
	};

	std::vector<Variable> storage(output.outputs[0].size());
	const auto numberTriangles = vertices.size() / 3;
	for (auto i = 0u; i < numberTriangles; i++)
	{
		const auto& p0 = vertices[i * 3 + 2];
		const auto& p1 = vertices[i * 3 + 1];
		const auto& p2 = vertices[i * 3 + 0];

		for (auto y = 0u; y < image->getHeight(); y++)
		{
			const auto yf = (static_cast<float>(y) / image->getHeight() + halfPixel.y) * 2 - 1;

			for (auto x = 0u; x < image->getWidth(); x++)
			{
				const auto xf = (static_cast<float>(x) / image->getWidth() + halfPixel.x) * 2 - 1;
				const auto p = glm::vec2(xf, yf);

				float depth;
				if (GetFragmentInput(state.input, output.outputs, inputTypes, storage, i * 3 + 0, i * 3 + 1, i * 3 + 2, p0, p1, p2, p, depth))
				{
					if (depthImage.second)
					{
						const auto currentDepth = GetDepthPixel(depthFormat, depthImage.second, x, y);
						if (currentDepth <= depth)
						{
							continue;
						}
					}
					
					InterpretShader(state, shaderStage->getModule(), SPIRV::SPIRVExecutionModelKind::ExecutionModelFragment, shaderStage->getName());
					uint64_t values[4];
					for (auto j = 0; j < MAX_FRAGMENT_ATTACHMENTS; j++)
					{
						if (images[j].second)
						{
							const VkClearColorValue input
							{
								{outputStorage[j].f32.v4.r, outputStorage[j].f32.v4.g, outputStorage[j].f32.v4.b, outputStorage[j].f32.v4.a}
							};
							const auto& format = GetFormatInformation(images[j].first.format);
							GetImageColour(values, format, input);
							SetPixel(format, images[j].second, x, y, 0, values);
						}
					}

					if (depthImage.second)
					{
						const VkClearColorValue input
						{
							{depth, 0, 0, 0}
						};
						GetImageColour(values, depthFormat, input);
						SetPixel(depthFormat, depthImage.second, x, y, 0, values);
					}
				}
			}
		}
	}
}

static void Draw(DeviceState* deviceState, uint32_t vertexCount)
{
	const auto vertexOutput = ProcessVertexShader(deviceState, vertexCount);

	if (deviceState->pipeline[0]->getShaderStage(1) != nullptr)
	{
		FATAL_ERROR();
	}

	if (deviceState->pipeline[0]->getShaderStage(2) != nullptr)
	{
		FATAL_ERROR();
	}

	if (deviceState->pipeline[0]->getShaderStage(3) != nullptr)
	{
		FATAL_ERROR();
	}
	
	ProcessFragmentShader(deviceState, vertexOutput);
}

class Command
{
public:
	virtual ~Command() = default;
	
	virtual void Process(DeviceState* deviceState) = 0;
};

class BindPipelineCommand final : public Command
{
public:
	BindPipelineCommand(int index, Pipeline* pipeline):
		index{index},
		pipeline{pipeline}
	{
	}

	void Process(DeviceState* deviceState) override
	{
		deviceState->pipeline[index] = pipeline;
	}

private:
	int index;
	Pipeline* pipeline;
};

class SetViewportCommand final : public Command
{
public:
	SetViewportCommand(uint32_t firstViewport, std::vector<VkViewport> viewports):
		firstViewport{firstViewport},
		viewports{std::move(viewports)}
	{
	}

	void Process(DeviceState* deviceState) override
	{
		for (auto i = 0u; i < viewports.size(); i++)
		{
			deviceState->viewports[i + firstViewport] = viewports[i];
		}
	}

private:
	uint32_t firstViewport;
	std::vector<VkViewport> viewports;
};

class SetScissorCommand final : public Command
{
public:
	SetScissorCommand(uint32_t firstScissor, std::vector<VkRect2D> scissors):
		firstScissor{firstScissor},
		scissors{std::move(scissors)}
	{
	}

	void Process(DeviceState* deviceState) override
	{
		for (auto i = 0u; i < scissors.size(); i++)
		{
			deviceState->scissors[i + firstScissor] = scissors[i];
		}
	}

private:
	uint32_t firstScissor;
	std::vector<VkRect2D> scissors;
};

class BindDescriptorSetsCommand final : public Command
{
public:
	BindDescriptorSetsCommand(int index, PipelineLayout* pipelineLayout, uint32_t firstSet, std::vector<DescriptorSet*> descriptorSets, std::vector<uint32_t> dynamicOffsets):
		index{index},
		pipelineLayout{pipelineLayout},
		firstSet{firstSet},
		descriptorSets{std::move(descriptorSets)},
		dynamicOffsets{std::move(dynamicOffsets)}
	{
	}

	void Process(DeviceState* deviceState) override
	{
		for (auto i = 0u; i < descriptorSets.size(); i++)
		{
			deviceState->descriptorSets[index][i + firstSet] = descriptorSets[i];
		}

		if (!dynamicOffsets.empty())
		{
			FATAL_ERROR();
		}
	}

private:
	int index;
	PipelineLayout* pipelineLayout;
	uint32_t firstSet;
	std::vector<DescriptorSet*> descriptorSets;
	std::vector<uint32_t> dynamicOffsets;
};

class BindVertexBuffersCommand final : public Command
{
public:
	BindVertexBuffersCommand(uint32_t firstBinding, std::vector<Buffer*> buffers, std::vector<VkDeviceSize> bufferOffsets):
		firstBinding{firstBinding},
		buffers{std::move(buffers)},
		bufferOffsets{std::move(bufferOffsets)}
	{
	}

	void Process(DeviceState* deviceState) override
	{
		for (auto i = 0u; i < buffers.size(); i++)
		{
			deviceState->vertexBinding[i + firstBinding] = buffers[i];
			deviceState->vertexBindingOffset[i + firstBinding] = bufferOffsets[i];
		}
	}

private:
	uint32_t firstBinding;
	std::vector<Buffer*> buffers;
	std::vector<VkDeviceSize> bufferOffsets;
};

class DrawCommand final : public Command
{
public:
	DrawCommand(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance):
		vertexCount{vertexCount},
		instanceCount{instanceCount},
		firstVertex{firstVertex},
		firstInstance{firstInstance}
	{
	}

	void Process(DeviceState* deviceState) override
	{
		if (instanceCount != 1)
		{
			FATAL_ERROR();
		}

		if (firstVertex != 0)
		{
			FATAL_ERROR();
		}

		if (firstInstance != 0)
		{
			FATAL_ERROR();
		}

		Draw(deviceState, vertexCount);
	}

private:
	uint32_t vertexCount;
	uint32_t instanceCount;
	uint32_t firstVertex;
	uint32_t firstInstance;
};

class CopyImageToBufferCommand final : public Command
{
public:
	CopyImageToBufferCommand(Image* srcImage, VkImageLayout srcImageLayout, Buffer* dstBuffer, std::vector<VkBufferImageCopy> regions):
		srcImage{srcImage},
		srcImageLayout{srcImageLayout},
		dstBuffer{dstBuffer},
		regions{std::move(regions)}
	{
	}

	void Process(DeviceState* deviceState) override
	{
		const auto& format = GetFormatInformation(srcImage->getFormat());
		
		for (const auto& region : regions)
		{
			if (region.bufferRowLength)
			{
				FATAL_ERROR();
			}

			if (region.bufferImageHeight)
			{
				FATAL_ERROR();
			}

			if (region.imageSubresource.baseArrayLayer)
			{
				FATAL_ERROR();
			}

			if (region.imageSubresource.mipLevel)
			{
				FATAL_ERROR();
			}

			if (region.imageSubresource.layerCount != 1)
			{
				FATAL_ERROR();
			}

			if (region.imageSubresource.aspectMask != VK_IMAGE_ASPECT_COLOR_BIT)
			{
				FATAL_ERROR();
			}

			auto offset = region.bufferOffset;
			for (auto z = region.imageOffset.z; z < static_cast<int32_t>(region.imageExtent.depth); z++)
			{
				for (auto y = region.imageOffset.y; y < static_cast<int32_t>(region.imageExtent.height); y++)
				{
					for (auto x = region.imageOffset.x; x < static_cast<int32_t>(region.imageExtent.width); x++)
					{
						auto pixel = reinterpret_cast<uint8_t*>(srcImage->getData()) +
							static_cast<uint64_t>(z) * (srcImage->getWidth() * srcImage->getHeight() * format.TotalSize) +
							static_cast<uint64_t>(y) * (srcImage->getWidth() * format.TotalSize) +
							static_cast<uint64_t>(x) * format.TotalSize;
						memcpy(reinterpret_cast<uint8_t*>(dstBuffer->getData(offset, format.TotalSize)), pixel, format.TotalSize);
						offset += format.TotalSize;
					}
				}
			}
		}
	}

private:
	Image* srcImage;
	VkImageLayout srcImageLayout;
	Buffer* dstBuffer;
	std::vector<VkBufferImageCopy> regions;
};

class ClearColourImageCommand final : public Command
{
public:
	ClearColourImageCommand(Image* image, VkImageLayout imageLayout, VkClearColorValue colour, std::vector<VkImageSubresourceRange> ranges):
		image{image},
		imageLayout{imageLayout},
		colour{colour},
		ranges{std::move(ranges)}
	{
	}

	void Process(DeviceState* deviceState) override
	{
		if (ranges.size() != 1)
		{
			FATAL_ERROR();
		}

		if (ranges[0].aspectMask != VK_IMAGE_ASPECT_COLOR_BIT)
		{
			FATAL_ERROR();
		}

		if (ranges[0].baseMipLevel != 0)
		{
			FATAL_ERROR();
		}

		if (ranges[0].baseArrayLayer != 0)
		{
			FATAL_ERROR();
		}

		if (ranges[0].layerCount != 1)
		{
			FATAL_ERROR();
		}

		if (ranges[0].levelCount != 1)
		{
			FATAL_ERROR();
		}
		
		ClearImage(image, image->getFormat(), colour);
	}

private:
	Image* image;
	VkImageLayout imageLayout;
	VkClearColorValue colour;
	std::vector<VkImageSubresourceRange> ranges;
};

class SetEventCommand final : public Command
{
public:
	SetEventCommand(Event* event, VkPipelineStageFlags stageMask):
		event{event},
		stageMask{stageMask}
	{
	}

	void Process(DeviceState*) override
	{
		event->Signal();
	}
	
private:
	Event* event;
	VkPipelineStageFlags stageMask;
};

class ResetEventCommand final : public Command
{
public:
	ResetEventCommand(Event* event, VkPipelineStageFlags stageMask):
		event{event},
		stageMask{stageMask}
	{
	}

	void Process(DeviceState*) override
	{
		event->Reset();
	}
	
private:
	Event* event;
	VkPipelineStageFlags stageMask;
};

class BeginRenderPassCommand final : public Command
{
public:
	BeginRenderPassCommand(RenderPass* renderPass, Framebuffer* framebuffer, VkRect2D renderArea, std::vector<VkClearValue> clearValues):
		renderPass{renderPass},
		framebuffer{framebuffer},
		renderArea{renderArea},
		clearValues{std::move(clearValues)}
	{
	}

	void Process(DeviceState* deviceState) override
	{
		if (renderPass->getSubpasses().size() != 1)
		{
			FATAL_ERROR();
		}

		for (auto i = 0u; i < renderPass->getSubpasses()[0].ColourAttachments.size(); i++)
		{
			auto attachmentReference = renderPass->getSubpasses()[0].ColourAttachments[i];
			if (attachmentReference.attachment != VK_ATTACHMENT_UNUSED)
			{
				auto attachment = renderPass->getAttachments()[attachmentReference.attachment];
				auto imageView = framebuffer->getAttachments()[attachmentReference.attachment];
				ClearImage(imageView->getImage(), attachment.format, clearValues[attachmentReference.attachment].color);
			}
		}

		if (renderPass->getSubpasses()[0].DepthStencilAttachment.attachment != VK_ATTACHMENT_UNUSED)
		{
			auto attachmentReference = renderPass->getSubpasses()[0].DepthStencilAttachment;
			auto attachment = renderPass->getAttachments()[attachmentReference.attachment];
			auto imageView = framebuffer->getAttachments()[attachmentReference.attachment];
			ClearImage(imageView->getImage(), attachment.format, clearValues[attachmentReference.attachment].depthStencil);
		}

		deviceState->renderPass = renderPass;
		deviceState->framebuffer = framebuffer;
		deviceState->renderArea = renderArea;
	}

private:
	RenderPass* renderPass;
	Framebuffer* framebuffer;
	VkRect2D renderArea;
	std::vector<VkClearValue> clearValues;
};

class EndRenderPassCommand final : public Command
{
public:
	void Process(DeviceState* deviceState) override
	{
		// TODO
	}
};

CommandBuffer::CommandBuffer(DeviceState* deviceState, VkCommandBufferLevel level, VkCommandPoolCreateFlags poolFlags):
	deviceState{deviceState},
	level{level},
	poolFlags{poolFlags}
{
}

class ExecuteCommandsCommand final : public Command
{
public:
	ExecuteCommandsCommand(std::vector<CommandBuffer*> commands):
		commands{std::move(commands)}
	{
	}

	void Process(DeviceState* deviceState) override
	{
		for (const auto commandBuffer : commands)
		{
			for (const auto& command : commandBuffer->commands)
			{
				command->Process(deviceState);
			}
		}
	}

private:
	std::vector<CommandBuffer*> commands;
};

CommandBuffer::~CommandBuffer() = default;

VkResult CommandBuffer::Begin(const VkCommandBufferBeginInfo* pBeginInfo)
{
	assert(pBeginInfo->sType == VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO);

	auto next = pBeginInfo->pNext;
	while (next)
	{
		const auto type = static_cast<const VkBaseInStructure*>(next)->sType;
		switch (type)
		{
		case VK_STRUCTURE_TYPE_DEVICE_GROUP_COMMAND_BUFFER_BEGIN_INFO:
			FATAL_ERROR();
		}
		next = static_cast<const VkBaseInStructure*>(next)->pNext;
	}

	if (poolFlags & VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT)
	{
		Reset(0);
	}
	else
	{
		assert(state == State::Initial);
	}

	if (level == VK_COMMAND_BUFFER_LEVEL_SECONDARY)
	{
		assert(pBeginInfo->pInheritanceInfo->sType == VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO);

		next = pBeginInfo->pInheritanceInfo->pNext;
		while (next)
		{
			const auto type = static_cast<const VkBaseInStructure*>(next)->sType;
			switch (type)
			{
			case VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_CONDITIONAL_RENDERING_INFO_EXT:
				FATAL_ERROR();
			}
			next = static_cast<const VkBaseInStructure*>(next)->pNext;
		}
	}

	state = State::Recording;
	return VK_SUCCESS;
}

VkResult CommandBuffer::End()
{
	assert(state == State::Recording);

	if (level == VK_COMMAND_BUFFER_LEVEL_PRIMARY)
	{
		// TODO: If commandBuffer is a primary command buffer, there must not be an active render pass instance
	}
	else
	{
		// TODO: If commandBuffer is a secondary command buffer, there must not be an outstanding vkCmdBeginDebugUtilsLabelEXT command recorded to commandBuffer that has not previously been ended by a call to vkCmdEndDebugUtilsLabelEXT.
		// TODO: If commandBuffer is a secondary command buffer, there must not be an outstanding vkCmdDebugMarkerBeginEXT command recorded to commandBuffer that has not previously been ended by a call to vkCmdDebugMarkerEndEXT.
	}

	// TODO: All queries made active during the recording of commandBuffer must have been made inactive
	// TODO: Conditional rendering must not be active

	state = State::Executable;
	return VK_SUCCESS;
}

VkResult CommandBuffer::Reset(VkCommandBufferResetFlags flags)
{
	if (flags)
	{
		FATAL_ERROR();
	}

	ForceReset();

	return VK_SUCCESS;
}

void CommandBuffer::BindPipeline(VkPipelineBindPoint pipelineBindPoint, VkPipeline pipeline)
{
	assert(state == State::Recording);
	if (pipelineBindPoint != VK_PIPELINE_BIND_POINT_GRAPHICS && pipelineBindPoint != VK_PIPELINE_BIND_POINT_COMPUTE)
	{
		FATAL_ERROR();
	}
	
	commands.push_back(std::make_unique<BindPipelineCommand>(static_cast<int>(pipelineBindPoint), reinterpret_cast<Pipeline*>(pipeline)));
}

void CommandBuffer::SetViewport(uint32_t firstViewport, uint32_t viewportCount, const VkViewport* pViewports)
{
	assert(state == State::Recording);
	std::vector<VkViewport> viewports(viewportCount);
	memcpy(viewports.data(), pViewports, sizeof(VkViewport) * viewportCount);
	commands.push_back(std::make_unique<SetViewportCommand>(firstViewport, viewports));
}

void CommandBuffer::SetScissor(uint32_t firstScissor, uint32_t scissorCount, const VkRect2D* pScissors)
{
	assert(state == State::Recording);
	std::vector<VkRect2D> scissors(scissorCount);
	memcpy(scissors.data(), pScissors, sizeof(VkRect2D) * scissorCount);
	commands.push_back(std::make_unique<SetScissorCommand>(firstScissor, scissors));
}

void CommandBuffer::BindDescriptorSets(VkPipelineBindPoint pipelineBindPoint, VkPipelineLayout layout, uint32_t firstSet, uint32_t descriptorSetCount, const VkDescriptorSet* pDescriptorSets, uint32_t dynamicOffsetCount, const uint32_t* pDynamicOffsets)
{
	assert(state == State::Recording);
	if (pipelineBindPoint != VK_PIPELINE_BIND_POINT_GRAPHICS && pipelineBindPoint != VK_PIPELINE_BIND_POINT_COMPUTE)
	{
		FATAL_ERROR();
	}

	std::vector<DescriptorSet*> descriptorSets(descriptorSetCount);
	for (auto i = 0u; i < descriptorSetCount; i++)
	{
		descriptorSets[i] = reinterpret_cast<DescriptorSet*>(pDescriptorSets[i]);
	}

	std::vector<uint32_t> dynamicOffsets(dynamicOffsetCount);
	memcpy(dynamicOffsets.data(), pDynamicOffsets, sizeof(uint32_t) * dynamicOffsetCount);

	commands.push_back(std::make_unique<BindDescriptorSetsCommand>(static_cast<int>(pipelineBindPoint), reinterpret_cast<PipelineLayout*>(layout), firstSet, descriptorSets, dynamicOffsets));
}

void CommandBuffer::BindVertexBuffers(uint32_t firstBinding, uint32_t bindingCount, const VkBuffer* pBuffers, const VkDeviceSize* pOffsets)
{
	assert(state == State::Recording);
	std::vector<Buffer*> buffers(bindingCount);
	std::vector<VkDeviceSize> bufferOffsets(bindingCount);

	for (auto i = 0u; i < bindingCount; i++)
	{
		buffers[i] = reinterpret_cast<Buffer*>(pBuffers[i]);
		bufferOffsets[i] = pOffsets[i];
	}
	
	commands.push_back(std::make_unique<BindVertexBuffersCommand>(firstBinding, buffers, bufferOffsets));
}

void CommandBuffer::Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
{
	assert(state == State::Recording);
	commands.push_back(std::make_unique<DrawCommand>(vertexCount, instanceCount, firstVertex, firstInstance));
}

void CommandBuffer::CopyImageToBuffer(VkImage srcImage, VkImageLayout srcImageLayout, VkBuffer dstBuffer, uint32_t regionCount, const VkBufferImageCopy* pRegions)
{
	assert(state == State::Recording);
	commands.push_back(std::make_unique<CopyImageToBufferCommand>(reinterpret_cast<Image*>(srcImage), srcImageLayout, reinterpret_cast<Buffer*>(dstBuffer), ArrayToVector(regionCount, pRegions)));
}

void CommandBuffer::ClearColorImage(VkImage image, VkImageLayout imageLayout, const VkClearColorValue* pColor, uint32_t rangeCount, const VkImageSubresourceRange* pRanges)
{
	assert(state == State::Recording);
	commands.push_back(std::make_unique<ClearColourImageCommand>(reinterpret_cast<Image*>(image), imageLayout, *pColor, ArrayToVector(rangeCount, pRanges)));
}

void CommandBuffer::ClearAttachments(uint32_t attachmentCount, const VkClearAttachment* pAttachments, uint32_t rectCount, const VkClearRect* pRects)
{
	// assert(state == State::Recording);
	// commands.push_back(std::make_unique<ClearAttachmentsCommand>(reinterpret_cast<Event*>(event), stageMask));
	FATAL_ERROR();
}

void CommandBuffer::SetEvent(VkEvent event, VkPipelineStageFlags stageMask)
{
	assert(state == State::Recording);
	commands.push_back(std::make_unique<SetEventCommand>(reinterpret_cast<Event*>(event), stageMask));
}

void CommandBuffer::ResetEvent(VkEvent event, VkPipelineStageFlags stageMask)
{
	assert(state == State::Recording);
	commands.push_back(std::make_unique<ResetEventCommand>(reinterpret_cast<Event*>(event), stageMask));
}

void CommandBuffer::PipelineBarrier(VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask,
                                    VkDependencyFlags dependencyFlags, uint32_t memoryBarrierCount, const VkMemoryBarrier* pMemoryBarriers,
                                    uint32_t bufferMemoryBarrierCount, const VkBufferMemoryBarrier* pBufferMemoryBarriers,
                                    uint32_t imageMemoryBarrierCount, const VkImageMemoryBarrier* pImageMemoryBarriers)
{
	assert(state == State::Recording);
	// TODO
}

void CommandBuffer::BeginRenderPass(const VkRenderPassBeginInfo* pRenderPassBegin, VkSubpassContents contents)
{
	assert(state == State::Recording);
	assert(pRenderPassBegin->sType == VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO);

	auto next = pRenderPassBegin->pNext;
	while (next)
	{
		const auto type = static_cast<const VkBaseInStructure*>(next)->sType;
		switch (type)
		{
		case VK_STRUCTURE_TYPE_DEVICE_GROUP_RENDER_PASS_BEGIN_INFO:
			FATAL_ERROR();
			
		case VK_STRUCTURE_TYPE_RENDER_PASS_ATTACHMENT_BEGIN_INFO_KHR:
			FATAL_ERROR();
			
		case VK_STRUCTURE_TYPE_RENDER_PASS_SAMPLE_LOCATIONS_BEGIN_INFO_EXT:
			FATAL_ERROR();
			
		default:
			next = static_cast<const VkBaseInStructure*>(next)->pNext;
			break;
		}
	}

	std::vector<VkClearValue> clearValues(pRenderPassBegin->clearValueCount);
	memcpy(clearValues.data(), pRenderPassBegin->pClearValues, sizeof(VkClearValue) * pRenderPassBegin->clearValueCount);
	commands.push_back(std::make_unique<BeginRenderPassCommand>(reinterpret_cast<RenderPass*>(pRenderPassBegin->renderPass), 
	                                                            reinterpret_cast<Framebuffer*>(pRenderPassBegin->framebuffer),
	                                                            pRenderPassBegin->renderArea,
	                                                            clearValues));
}

void CommandBuffer::EndRenderPass()
{
	assert(state == State::Recording);
	commands.push_back(std::make_unique<EndRenderPassCommand>());
}

void CommandBuffer::ExecuteCommands(uint32_t commandBufferCount, const VkCommandBuffer* pCommandBuffers)
{
	assert(state == State::Recording);
	commands.push_back(std::make_unique<ExecuteCommandsCommand>(ArrayToVector<CommandBuffer*, VkCommandBuffer>(commandBufferCount, pCommandBuffers, [](VkCommandBuffer commandBuffer)
	{
		return reinterpret_cast<CommandBuffer*>(commandBuffer);
	})));
}

void CommandBuffer::ForceReset()
{
	if (state == State::Pending)
	{
		FATAL_ERROR();
	}

	commands.clear();

	state = State::Initial;
}

VkResult CommandBuffer::Submit()
{
	for (const auto& command : commands)
	{
		command->Process(deviceState);
	}
	return VK_SUCCESS;
}
