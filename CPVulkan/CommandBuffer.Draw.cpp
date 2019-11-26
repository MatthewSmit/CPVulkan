#include "CommandBuffer.h"
#include "CommandBuffer.Internal.h"

#include "Buffer.h"
#include "DebugHelper.h"
#include "DescriptorSet.h"
#include "DeviceState.h"
#include "Formats.h"
#include "Framebuffer.h"
#include "Image.h"
#include "ImageSampler.h"
#include "ImageView.h"
#include "Pipeline.h"
#include "RenderPass.h"
#include "Util.h"

#include <ImageCompiler.h>
#include <Jit.h>
#include <SPIRVCompiler.h>
#include <SPIRVFunction.h>
#include <SPIRVInstruction.h>
#include <SPIRVModule.h>

#include <glm/glm.hpp>

#include <fstream>

class PipelineLayout;

struct VertexBuiltinInput
{
	int32_t vertexIndex;
	int32_t instanceIndex;
};

struct VertexBuiltinOutput
{
	glm::vec4 position;
	float a;
	float b[1];
};

struct FragmentBuiltinInput
{
	glm::vec4 fragCoord;
};

struct ComputeBuiltinInput
{
	glm::uvec3 globalInvocationId;
	uint32_t unused0;
	
	glm::uvec3 localInvocationId;
	uint32_t unused1;
	
	glm::uvec3 workgroupInvocationId;
	uint32_t unused2;
};

struct VertexOutput
{
	std::unique_ptr<VertexBuiltinOutput[]> builtinData;
	std::unique_ptr<uint8_t[]> outputData;
	uint64_t builtinStride;
	uint64_t outputStride;
	uint32_t vertexCount;
};

struct VariableInOutData
{
	void* pointer;
	uint32_t location;
	VkFormat format;
	uint32_t size;
	uint32_t offset;
};

struct VariableUniformData
{
	void* pointer;
	uint32_t binding;
	uint32_t set;
};

static void ClearImage(DeviceState* deviceState, Image* image, uint32_t layer, uint32_t mipLevel, VkFormat format, VkClearColorValue colour)
{
	const auto width = image->getImageSize().Level[mipLevel].Width;
	const auto height = image->getImageSize().Level[mipLevel].Height;
	const auto depth = image->getImageSize().Level[mipLevel].Depth;

	// TODO: Change to SetPixels when implemented
	for (auto z = 0u; z < depth; z++)
	{
		for (auto y = 0u; y < height; y++)
		{
			for (auto x = 0u; x < width; x++)
			{
				SetPixel(deviceState, format, image, x, y, z, mipLevel, layer, colour);
			}
		}
	}
}

static void ClearImage(DeviceState* deviceState, Image* image, uint32_t layer, uint32_t mipLevel, VkFormat format, VkClearDepthStencilValue colour)
{
	const auto width = image->getImageSize().Level[mipLevel].Width;
	const auto height = image->getImageSize().Level[mipLevel].Height;
	const auto depth = image->getImageSize().Level[mipLevel].Depth;

	// TODO: Change to SetPixels when implemented
	for (auto z = 0u; z < depth; z++)
	{
		for (auto y = 0u; y < height; y++)
		{
			for (auto x = 0u; x < width; x++)
			{
				SetPixel(deviceState, format, image, x, y, z, mipLevel, layer, colour);
			}
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

static void CopyFormatConversion(DeviceState* deviceState, void* destination, const void* source, VkFormat destinationFormat, VkFormat sourceFormat)
{
	const auto sourceInformation = GetFormatInformation(sourceFormat);
	const auto destinationInformation = GetFormatInformation(destinationFormat);

	switch (sourceInformation.Base)
	{
	case BaseType::UInt:
		{
			assert(GetFormatInformation(sourceFormat).ElementSize <= 4);
			auto& sourceFunctions = deviceState->imageFunctions[sourceFormat];
			auto& destFunctions = deviceState->imageFunctions[destinationFormat];
			
			if (!sourceFunctions.GetPixelU32)
			{
				sourceFunctions.GetPixelU32 = reinterpret_cast<decltype(sourceFunctions.GetPixelU32)>(CompileGetPixelU32(deviceState->jit, &sourceInformation));
			}
			
			if (!destFunctions.SetPixelU32)
			{
				destFunctions.SetPixelU32 = reinterpret_cast<decltype(destFunctions.SetPixelU32)>(CompileSetPixelU32(deviceState->jit, &destinationInformation));
			}

			uint32_t values[4];
			sourceFunctions.GetPixelU32(source, values);
			destFunctions.SetPixelU32(destination, values);
			break;
		}
		
	case BaseType::SInt:
		{
			assert(GetFormatInformation(sourceFormat).ElementSize <= 4);
			auto& sourceFunctions = deviceState->imageFunctions[sourceFormat];
			auto& destFunctions = deviceState->imageFunctions[destinationFormat];
			
			if (!sourceFunctions.GetPixelI32)
			{
				sourceFunctions.GetPixelI32 = reinterpret_cast<decltype(sourceFunctions.GetPixelI32)>(CompileGetPixelI32(deviceState->jit, &sourceInformation));
			}
			
			if (!destFunctions.SetPixelI32)
			{
				destFunctions.SetPixelI32 = reinterpret_cast<decltype(destFunctions.SetPixelI32)>(CompileSetPixelI32(deviceState->jit, &destinationInformation));
			}

			int32_t values[4];
			sourceFunctions.GetPixelI32(source, values);
			destFunctions.SetPixelI32(destination, values);
			break;
		}
		
	case BaseType::UNorm:
	case BaseType::SNorm:
	case BaseType::UScaled:
	case BaseType::SScaled:
	case BaseType::UFloat:
	case BaseType::SFloat:
	case BaseType::SRGB:
		{
			assert(GetFormatInformation(sourceFormat).ElementSize <= 4);
			auto& sourceFunctions = deviceState->imageFunctions[sourceFormat];
			auto& destFunctions = deviceState->imageFunctions[destinationFormat];
			
			if (!sourceFunctions.GetPixelF32)
			{
				sourceFunctions.GetPixelF32 = reinterpret_cast<decltype(sourceFunctions.GetPixelF32)>(CompileGetPixelF32(deviceState->jit, &sourceInformation));
			}
			
			if (!destFunctions.SetPixelF32)
			{
				destFunctions.SetPixelF32 = reinterpret_cast<decltype(destFunctions.SetPixelF32)>(CompileSetPixelF32(deviceState->jit, &destinationInformation));
			}

			float values[4];
			sourceFunctions.GetPixelF32(source, values);
			destFunctions.SetPixelF32(destination, values);
			break;
		}
		
	default: FATAL_ERROR();
	}
}

static void LoadVertexInput(DeviceState* deviceState, uint32_t vertex, uint32_t instance, const VertexInputState& vertexInputState, const Buffer* const vertexBinding[16], const uint64_t vertexBindingOffset[16], const std::vector<VariableInOutData>& vertexInputs)
{
	for (const auto& attribute : vertexInputState.VertexAttributeDescriptions)
	{
		for (const auto& input : vertexInputs)
		{
			if (input.location == attribute.location)
			{
				const auto& binding = FindBinding(attribute.binding, vertexInputState);
				uint64_t offset;
				if (binding.inputRate == VK_VERTEX_INPUT_RATE_VERTEX)
				{
					offset = vertexBindingOffset[binding.binding] + static_cast<uint64_t>(binding.stride) * vertex + attribute.offset;
				}
				else
				{
					offset = vertexBindingOffset[binding.binding] + static_cast<uint64_t>(binding.stride) * instance + attribute.offset;
				}
				const auto size = GetFormatInformation(attribute.format).TotalSize;
				if (input.format != attribute.format)
				{
					CopyFormatConversion(deviceState, input.pointer, vertexBinding[binding.binding]->getDataPtr(offset, size), input.format, attribute.format);
				}
				else
				{
					memcpy(input.pointer, vertexBinding[binding.binding]->getDataPtr(offset, size), size);
				}
				goto end;
			}
		}
		FATAL_ERROR();

	end:
		continue;
	}
}

static void LoadUniforms(DeviceState* deviceState, const SPIRV::SPIRVModule* module, const std::vector<VariableUniformData>& uniformData, int pipelineIndex)
{
	for (const auto& data : uniformData)
	{
		const auto descriptorSet = deviceState->pipelineState[pipelineIndex].descriptorSets[data.set];
		VkDescriptorType descriptorType;
		Descriptor* value;
		descriptorSet->getBinding(data.binding, descriptorType, value);
		switch (descriptorType)
		{
		case VK_DESCRIPTOR_TYPE_SAMPLER:
		case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
		case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
		case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
		case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
		case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
			for (auto j = 0u; j < value->count; j++)
			{
				static_cast<const ImageDescriptor**>(data.pointer)[j] = &value->values[j].Image;
			}
			break;
			
		case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
		case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
		case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
		case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
			for (auto j = 0u; j < value->count; j++)
			{
				const auto& bufferInfo = value->values[j].Buffer;
				static_cast<const void**>(data.pointer)[j] = UnwrapVulkan<Buffer>(bufferInfo.buffer)->getDataPtr(bufferInfo.offset, bufferInfo.range);
			}
			break;
			
		case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT: FATAL_ERROR();
		case VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK_EXT: FATAL_ERROR();
		case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV: FATAL_ERROR();
		default: FATAL_ERROR();
		}
	}
}

static float EdgeFunction(const glm::vec4& a, const glm::vec4& b, const glm::vec2& c)
{
	return (c.x - a.x) * (b.y - a.y) - (c.y - a.y) * (b.x - a.x);
}

static VkFormat GetVariableFormat(SPIRV::SPIRVType* type)
{
	if (type->isTypeVector())
	{
		if (type->getVectorComponentType()->isTypeFloat(32))
		{
			switch (type->getVectorComponentCount())
			{
			case 2:
				return VK_FORMAT_R32G32_SFLOAT;
			case 3:
				return VK_FORMAT_R32G32B32_SFLOAT;
			case 4:
				return VK_FORMAT_R32G32B32A32_SFLOAT;
			}
		}
		
		if (type->getVectorComponentType()->isTypeFloat(64))
		{
			switch (type->getVectorComponentCount())
			{
			case 2:
				return VK_FORMAT_R64G64_SFLOAT;
			case 3:
				return VK_FORMAT_R64G64B64_SFLOAT;
			case 4:
				return VK_FORMAT_R64G64B64A64_SFLOAT;
			}
		}
		
		if (type->getVectorComponentType()->isTypeInt(32) && static_cast<SPIRV::SPIRVTypeInt*>(type->getVectorComponentType())->isSigned())
		{
			switch (type->getVectorComponentCount())
			{
			case 2:
				return VK_FORMAT_R32G32_SINT;
			case 3:
				return VK_FORMAT_R32G32B32_SINT;
			case 4:
				return VK_FORMAT_R32G32B32A32_SINT;
			}
		}
		
		if (type->getVectorComponentType()->isTypeInt(64) && static_cast<SPIRV::SPIRVTypeInt*>(type->getVectorComponentType())->isSigned())
		{
			switch (type->getVectorComponentCount())
			{
			case 2:
				return VK_FORMAT_R64G64_SINT;
			case 3:
				return VK_FORMAT_R64G64B64_SINT;
			case 4:
				return VK_FORMAT_R64G64B64A64_SINT;
			}
		}
		
		if (type->getVectorComponentType()->isTypeInt(32) && !static_cast<SPIRV::SPIRVTypeInt*>(type->getVectorComponentType())->isSigned())
		{
			switch (type->getVectorComponentCount())
			{
			case 2:
				return VK_FORMAT_R32G32_UINT;
			case 3:
				return VK_FORMAT_R32G32B32_UINT;
			case 4:
				return VK_FORMAT_R32G32B32A32_UINT;
			}
		}
		
		if (type->getVectorComponentType()->isTypeInt(64) && !static_cast<SPIRV::SPIRVTypeInt*>(type->getVectorComponentType())->isSigned())
		{
			switch (type->getVectorComponentCount())
			{
			case 2:
				return VK_FORMAT_R64G64_UINT;
			case 3:
				return VK_FORMAT_R64G64B64_UINT;
			case 4:
				return VK_FORMAT_R64G64B64A64_UINT;
			}
		}
	}

	if (type->isTypeFloat(32))
	{
		return VK_FORMAT_R32_SFLOAT;
	}

	if (type->isTypeFloat(64))
	{
		return VK_FORMAT_R64_SFLOAT;
	}

	if (type->isTypeInt(32) && static_cast<SPIRV::SPIRVTypeInt*>(type)->isSigned())
	{
		return VK_FORMAT_R32_SINT;
	}

	if (type->isTypeInt(64) && static_cast<SPIRV::SPIRVTypeInt*>(type)->isSigned())
	{
		return VK_FORMAT_R64_SINT;
	}

	if (type->isTypeInt(32) && !static_cast<SPIRV::SPIRVTypeInt*>(type)->isSigned())
	{
		return VK_FORMAT_R32_UINT;
	}

	if (type->isTypeInt(64) && !static_cast<SPIRV::SPIRVTypeInt*>(type)->isSigned())
	{
		return VK_FORMAT_R64_UINT;
	}
	
	FATAL_ERROR();
}

static uint32_t GetVariableSize(SPIRV::SPIRVType* type)
{
	if (type->isTypeArray())
	{
		// TODO: Array stride & stuff
		return GetVariableSize(type->getArrayElementType()) * type->getArrayLength();
	}
	
	if (type->isTypeStruct())
	{
		auto size = 0u;
		for (auto i = 0u; i < type->getStructMemberCount(); i++)
		{
			const auto decorate = type->getMemberDecorate(i, DecorationOffset);
			if (decorate && decorate->getLiteral(0) != size)
			{
				const auto offset = decorate->getLiteral(0);
				if (offset < size)
				{
					FATAL_ERROR();
				}
				else
				{
					size = offset;
				}
			}
			size += GetVariableSize(type->getStructMemberType(i));
		}
		return size;
	}
	
	if (type->isTypeMatrix())
	{
		// TODO: Matrix stride & so on
		return GetVariableSize(type->getMatrixComponentType()) * type->getMatrixColumnCount() * type->getMatrixRowCount();
	}
	
	if (type->isTypeVector())
	{
		return GetVariableSize(type->getVectorComponentType()) * type->getVectorComponentCount();
	}

	if (type->isTypeFloat() || type->isTypeInt())
	{
		return type->getBitWidth() / 8;
	}
	
	FATAL_ERROR();
}

static void GetVariablePointers(const SPIRV::SPIRVModule* module, 
                                const CompiledModule* llvmModule, 
                                CPJit* jit, 
                                std::vector<VariableInOutData>& inputData,
                                std::vector<VariableUniformData>& uniformData, 
                                std::vector<VariableInOutData>& outputData,
                                std::pair<void*, uint32_t>& pushConstant,
                                uint32_t& outputSize)
{
	auto maxLocation = -1;
	auto inputSize = 0u;
	
	for (auto i = 0u; i < module->getNumVariables(); i++)
	{
		const auto variable = module->getVariable(i);
		switch (variable->getStorageClass())
		{
		case StorageClassInput:
			{
				auto locations = variable->getDecorate(DecorationLocation);
				if (locations.empty())
				{
					continue;
				}

				const auto location = *locations.begin();
				const auto size = GetVariableSize(variable->getType()->getPointerElementType());
				inputData.push_back(VariableInOutData
					{
						jit->getPointer(llvmModule, MangleName(variable)),
						location,
						GetVariableFormat(variable->getType()->getPointerElementType()),
						size,
						inputSize
					});
				inputSize += size;
				break;
			}

		case StorageClassUniform:
		case StorageClassUniformConstant:
		case StorageClassStorageBuffer:
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
				
				uniformData.push_back(VariableUniformData
					{
						jit->getPointer(llvmModule, MangleName(variable)),
						binding,
						set
					});
				break;
			}

		case StorageClassOutput:
			{
				auto locations = variable->getDecorate(DecorationLocation);
				if (locations.empty())
				{
					continue;
				}

				const auto location = *locations.begin();

				const auto ptr = jit->getPointer(llvmModule, MangleName(variable));
				if (variable->getType()->getPointerElementType()->isTypeArray())
				{
					const auto size = variable->getType()->getPointerElementType()->getArrayLength();
					const auto type = variable->getType()->getPointerElementType()->getArrayElementType();
					maxLocation = std::max(maxLocation, int32_t(location + size - 1));
					const auto variableSize = GetVariableSize(type);
					for (auto j = 0U; j < size; j++)
					{
						outputData.push_back(VariableInOutData
							{
								static_cast<uint8_t*>(ptr) + j * variableSize,
								location + j,
								GetVariableFormat(type),
								variableSize,
								outputSize
							});
						outputSize += variableSize;
					}
				}
				else
				{
					const auto type = variable->getType()->getPointerElementType();
					maxLocation = std::max(maxLocation, int32_t(location));
					outputData.push_back(VariableInOutData
						{
							ptr,
							location,
							GetVariableFormat(type),
							GetVariableSize(type),
							outputSize
						});
					outputSize += GetVariableSize(type);
				}
				break;
			}

		case StorageClassPushConstant:
			assert(std::get<0>(pushConstant) == nullptr);
			pushConstant = std::make_pair(jit->getPointer(llvmModule, MangleName(variable)), GetVariableSize(variable->getType()->getPointerElementType()));
			break;

		default:
			FATAL_ERROR();
		}
	}
}

static std::vector<uint32_t> ProcessInputAssembler(DeviceState* deviceState, uint32_t firstVertex, uint32_t vertexCount)
{
	// TODO: Calculate offsets here too
	std::vector<uint32_t> results(vertexCount);
	for (auto i = 0u; i < vertexCount; i++)
	{
		results[i] = firstVertex + i;
	}
	return results;
}

static std::vector<uint32_t> ProcessInputAssemblerIndexed(DeviceState* deviceState, uint32_t indexCount)
{
	// TODO: Calculate offsets here too
	std::vector<uint32_t> results(indexCount);

	if (deviceState->pipelineState[PIPELINE_GRAPHICS].pipeline->getInputAssemblyState().PrimitiveRestartEnable)
	{
		FATAL_ERROR();
	}

	const auto indexData = deviceState->indexBinding->getDataPtr(deviceState->indexBindingOffset, static_cast<uint64_t>(indexCount) * deviceState->indexBindingStride);

	switch (deviceState->indexBindingStride)
	{
	case 1:
		for (auto i = 0u; i < indexCount; i++)
		{
			results[i] = indexData[i];
		}
		break;
		
	case 2:
		for (auto i = 0u; i < indexCount; i++)
		{
			results[i] = reinterpret_cast<uint16_t*>(indexData)[i];
		}
		break;

	case 4:
		for (auto i = 0u; i < indexCount; i++)
		{
			results[i] = reinterpret_cast<uint32_t*>(indexData)[i];
		}
		break;
		
	default:
		FATAL_ERROR();
	}
	
	return results;
}

static VertexOutput ProcessVertexShader(DeviceState* deviceState, uint32_t instance, const std::vector<uint32_t>& assemblerOutput)
{
	assert(assemblerOutput.size() <= 0xFFFFFFFF);
	
	const auto& shaderStage = deviceState->pipelineState[PIPELINE_GRAPHICS].pipeline->getShaderStage(0);
	const auto& vertexInput = deviceState->pipelineState[PIPELINE_GRAPHICS].pipeline->getVertexInputState();

	const auto module = shaderStage->getModule();
	const auto llvmModule = shaderStage->getLLVMModule();

	const auto builtinInputPointer = deviceState->jit->getPointer(llvmModule, "_builtinInput");
	const auto builtinOutputPointer = deviceState->jit->getPointer(llvmModule, "_builtinOutput");

	auto outputSize = 0u;
	std::vector<VariableInOutData> inputData{};
	std::vector<VariableUniformData> uniformData{};
	std::vector<VariableInOutData> outputData{};
	std::pair<void*, uint32_t> pushConstant{};
	GetVariablePointers(module, llvmModule, deviceState->jit, inputData, uniformData, outputData, pushConstant, outputSize);
	
	VertexOutput output
	{
		std::unique_ptr<VertexBuiltinOutput[]>(new VertexBuiltinOutput[assemblerOutput.size()]),
		std::unique_ptr<uint8_t[]>(new uint8_t[assemblerOutput.size() * outputSize]),
		sizeof(VertexBuiltinOutput),
		outputSize,
		static_cast<uint32_t>(assemblerOutput.size()),
	};

	LoadUniforms(deviceState, module, uniformData, PIPELINE_GRAPHICS);

	const auto builtinInput = static_cast<VertexBuiltinInput*>(builtinInputPointer);
	builtinInput->instanceIndex = instance;

	if (pushConstant.first)
	{
		memcpy(pushConstant.first, deviceState->pushConstants, pushConstant.second);
	}

	for (auto i : assemblerOutput)
	{
		builtinInput->vertexIndex = i;

		LoadVertexInput(deviceState, i, instance, vertexInput, deviceState->vertexBinding, deviceState->vertexBindingOffset, inputData);

		shaderStage->getEntryPoint()();
		output.builtinData[i] = *static_cast<VertexBuiltinOutput*>(builtinOutputPointer);

		for (const auto& data : outputData)
		{
			memcpy(output.outputData.get() + i * output.outputStride + data.offset, data.pointer, data.size);
		}
	}
	
	return output;
}

static bool GetFragmentInput(const std::vector<VariableInOutData>& inputData, uint8_t* vertexData, int vertexStride,
                             uint32_t p0Index, uint32_t p1Index, uint32_t p2Index,
                             const glm::vec4& p0, const glm::vec4& p1, const glm::vec4& p2, const glm::vec2& p,
                             VkCullModeFlags cullMode, float& depth, bool& front)
{
	auto area = EdgeFunction(p0, p1, p2);

	float w0;
	float w1;
	float w2;
	if (area < 0)
	{
		area = -area;
		front = false;
		w0 = EdgeFunction(p2, p1, p);
		w1 = EdgeFunction(p0, p2, p);
		w2 = EdgeFunction(p1, p0, p);
	}
	else
	{
		front = true;
		w0 = EdgeFunction(p1, p2, p);
		w1 = EdgeFunction(p2, p0, p);
		w2 = EdgeFunction(p0, p1, p);
	}

	if (w0 < 0 || w1 < 0 || w2 < 0 || ((cullMode & VK_CULL_MODE_BACK_BIT) && !front) || ((cullMode & VK_CULL_MODE_FRONT_BIT) && front))
	{
		return false;
	}

	w0 /= area;
	w1 /= area;
	w2 /= area;

	depth = (p0 * w0 + p1 * w1 + p2 * w2).z;

	for (auto input : inputData)
	{
		const auto data0 = vertexData + p0Index * vertexStride + input.offset;
		const auto data1 = vertexData + p1Index * vertexStride + input.offset;
		const auto data2 = vertexData + p2Index * vertexStride + input.offset;
		switch (input.format)
		{
		case VK_FORMAT_R32G32_SFLOAT:
			*reinterpret_cast<glm::vec2*>(input.pointer) =
				*reinterpret_cast<glm::vec2*>(data0) * w0 +
				*reinterpret_cast<glm::vec2*>(data1) * w1 +
				*reinterpret_cast<glm::vec2*>(data2) * w2;
			break;

		case VK_FORMAT_R32G32B32_SFLOAT:
			*reinterpret_cast<glm::vec3*>(input.pointer) =
				*reinterpret_cast<glm::vec3*>(data0) * w0 +
				*reinterpret_cast<glm::vec3*>(data1) * w1 +
				*reinterpret_cast<glm::vec3*>(data2) * w2;
			break;

		case VK_FORMAT_R32G32B32A32_SFLOAT:
			*reinterpret_cast<glm::vec4*>(input.pointer) =
				*reinterpret_cast<glm::vec4*>(data0) * w0 +
				*reinterpret_cast<glm::vec4*>(data1) * w1 +
				*reinterpret_cast<glm::vec4*>(data2) * w2;
			break;

		default:
			FATAL_ERROR();
		}
	}

	return true;
}

template<typename T>
bool CompareTest(T reference, T value, VkCompareOp compare)
{
	switch (compare)
	{
	case VK_COMPARE_OP_NEVER: return false;
	case VK_COMPARE_OP_LESS: return value < reference;
	case VK_COMPARE_OP_EQUAL: return value = reference;
	case VK_COMPARE_OP_LESS_OR_EQUAL: return value <= reference;
	case VK_COMPARE_OP_GREATER: return value > reference;
	case VK_COMPARE_OP_NOT_EQUAL: return value != reference;
	case VK_COMPARE_OP_GREATER_OR_EQUAL: return value >= reference;
	case VK_COMPARE_OP_ALWAYS: return true;
	default: FATAL_ERROR();
	}
}

template<bool IsSource>
static glm::fvec4 ApplyBlendFactor(glm::fvec4 source, glm::fvec4 destination, glm::fvec4 constant, const VkPipelineColorBlendAttachmentState& blendState)
{
	const auto colourBlendFactor = IsSource ? blendState.srcColorBlendFactor : blendState.dstColorBlendFactor;
	const auto alphaBlendFactor = IsSource ? blendState.srcAlphaBlendFactor : blendState.dstAlphaBlendFactor;
	glm::fvec4 value{};
	switch (colourBlendFactor)
	{
	case VK_BLEND_FACTOR_ZERO:
		break;
		
	case VK_BLEND_FACTOR_ONE:
		value = glm::fvec4(1);
		break;
		
	case VK_BLEND_FACTOR_SRC_COLOR:
		value = source;
		break;
		
	case VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR:
		value = glm::fvec4(1) - source;
		break;
		
	case VK_BLEND_FACTOR_DST_COLOR:
		value = destination;
		break;
		
	case VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR:
		value = glm::fvec4(1) - destination;
		break;

	case VK_BLEND_FACTOR_SRC_ALPHA:
		value = glm::vec4(source.a);
		break;

	case VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA:
		value = glm::vec4(1 - source.a);
		break;

	case VK_BLEND_FACTOR_DST_ALPHA:
		value = glm::vec4(destination.a);
		break;

	case VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA:
		value = glm::vec4(1 - destination.a);
		break;

	case VK_BLEND_FACTOR_CONSTANT_COLOR:
		value = constant;
		break;
		
	case VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR:
		value = glm::fvec4(1) - constant;
		break;

	case VK_BLEND_FACTOR_CONSTANT_ALPHA:
		value = glm::vec4(constant.a);
		break;

	case VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA:
		value = glm::vec4(1 - constant.a);
		break;
		
	case VK_BLEND_FACTOR_SRC_ALPHA_SATURATE:
		{
			auto factor = std::min(source.a, 1 - destination.a);
			value = glm::fvec4(factor, factor, factor, 1);
			break;
		}
		
	case VK_BLEND_FACTOR_SRC1_COLOR: FATAL_ERROR();
	case VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR: FATAL_ERROR();
	case VK_BLEND_FACTOR_SRC1_ALPHA: FATAL_ERROR();
	case VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA: FATAL_ERROR();
	default: FATAL_ERROR();
	}

	if (colourBlendFactor != alphaBlendFactor)
	{
		switch (alphaBlendFactor)
		{
		case VK_BLEND_FACTOR_ZERO:
			value.a = 0;
			break;

		case VK_BLEND_FACTOR_ONE:
		case VK_BLEND_FACTOR_SRC_ALPHA_SATURATE:
			value.a = 1;
			break;

		case VK_BLEND_FACTOR_SRC_COLOR:
		case VK_BLEND_FACTOR_SRC_ALPHA:
			value.a = source.a;
			break;

		case VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR:
		case VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA:
			value.a = 1 - source.a;
			break;

		case VK_BLEND_FACTOR_DST_COLOR:
		case VK_BLEND_FACTOR_DST_ALPHA:
			value.a = destination.a;
			break;

		case VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR:
		case VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA:
			value.a = 1 - destination.a;
			break;
			
		case VK_BLEND_FACTOR_CONSTANT_COLOR:
		case VK_BLEND_FACTOR_CONSTANT_ALPHA:
			value.a = constant.a;
			break;
			
		case VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR:
		case VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA:
			value.a = 1 - constant.a;
			break;
			
		case VK_BLEND_FACTOR_SRC1_COLOR: FATAL_ERROR();
		case VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR: FATAL_ERROR();
		case VK_BLEND_FACTOR_SRC1_ALPHA: FATAL_ERROR();
		case VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA: FATAL_ERROR();
		default: FATAL_ERROR();
		}
	}

	return value;
}

static glm::fvec4 ApplyBlend(glm::fvec4 source, glm::fvec4 destination, glm::fvec4 constant, const VkPipelineColorBlendAttachmentState& blendState)
{
	const auto sourceBlendFactor = ApplyBlendFactor<true>(source, destination, constant, blendState);
	const auto destinationBlendFactor = ApplyBlendFactor<false>(source, destination, constant, blendState);

	glm::fvec4 value{};
	switch (blendState.colorBlendOp)
	{
	case VK_BLEND_OP_ADD:
		value = source * sourceBlendFactor + destination * destinationBlendFactor;
		break;

	case VK_BLEND_OP_SUBTRACT:
		value = source * sourceBlendFactor - destination * destinationBlendFactor;
		break;
		
	case VK_BLEND_OP_REVERSE_SUBTRACT:
		value = destination * destinationBlendFactor - source * sourceBlendFactor;
		break;

	case VK_BLEND_OP_MIN:
		value = glm::min(source, destination);
		break;

	case VK_BLEND_OP_MAX:
		value = glm::max(source, destination);
		break;
		
	case VK_BLEND_OP_ZERO_EXT: FATAL_ERROR();
	case VK_BLEND_OP_SRC_EXT: FATAL_ERROR();
	case VK_BLEND_OP_DST_EXT: FATAL_ERROR();
	case VK_BLEND_OP_SRC_OVER_EXT: FATAL_ERROR();
	case VK_BLEND_OP_DST_OVER_EXT: FATAL_ERROR();
	case VK_BLEND_OP_SRC_IN_EXT: FATAL_ERROR();
	case VK_BLEND_OP_DST_IN_EXT: FATAL_ERROR();
	case VK_BLEND_OP_SRC_OUT_EXT: FATAL_ERROR();
	case VK_BLEND_OP_DST_OUT_EXT: FATAL_ERROR();
	case VK_BLEND_OP_SRC_ATOP_EXT: FATAL_ERROR();
	case VK_BLEND_OP_DST_ATOP_EXT: FATAL_ERROR();
	case VK_BLEND_OP_XOR_EXT: FATAL_ERROR();
	case VK_BLEND_OP_MULTIPLY_EXT: FATAL_ERROR();
	case VK_BLEND_OP_SCREEN_EXT: FATAL_ERROR();
	case VK_BLEND_OP_OVERLAY_EXT: FATAL_ERROR();
	case VK_BLEND_OP_DARKEN_EXT: FATAL_ERROR();
	case VK_BLEND_OP_LIGHTEN_EXT: FATAL_ERROR();
	case VK_BLEND_OP_COLORDODGE_EXT: FATAL_ERROR();
	case VK_BLEND_OP_COLORBURN_EXT: FATAL_ERROR();
	case VK_BLEND_OP_HARDLIGHT_EXT: FATAL_ERROR();
	case VK_BLEND_OP_SOFTLIGHT_EXT: FATAL_ERROR();
	case VK_BLEND_OP_DIFFERENCE_EXT: FATAL_ERROR();
	case VK_BLEND_OP_EXCLUSION_EXT: FATAL_ERROR();
	case VK_BLEND_OP_INVERT_EXT: FATAL_ERROR();
	case VK_BLEND_OP_INVERT_RGB_EXT: FATAL_ERROR();
	case VK_BLEND_OP_LINEARDODGE_EXT: FATAL_ERROR();
	case VK_BLEND_OP_LINEARBURN_EXT: FATAL_ERROR();
	case VK_BLEND_OP_VIVIDLIGHT_EXT: FATAL_ERROR();
	case VK_BLEND_OP_LINEARLIGHT_EXT: FATAL_ERROR();
	case VK_BLEND_OP_PINLIGHT_EXT: FATAL_ERROR();
	case VK_BLEND_OP_HARDMIX_EXT: FATAL_ERROR();
	case VK_BLEND_OP_HSL_HUE_EXT: FATAL_ERROR();
	case VK_BLEND_OP_HSL_SATURATION_EXT: FATAL_ERROR();
	case VK_BLEND_OP_HSL_COLOR_EXT: FATAL_ERROR();
	case VK_BLEND_OP_HSL_LUMINOSITY_EXT: FATAL_ERROR();
	case VK_BLEND_OP_PLUS_EXT: FATAL_ERROR();
	case VK_BLEND_OP_PLUS_CLAMPED_EXT: FATAL_ERROR();
	case VK_BLEND_OP_PLUS_CLAMPED_ALPHA_EXT: FATAL_ERROR();
	case VK_BLEND_OP_PLUS_DARKER_EXT: FATAL_ERROR();
	case VK_BLEND_OP_MINUS_EXT: FATAL_ERROR();
	case VK_BLEND_OP_MINUS_CLAMPED_EXT: FATAL_ERROR();
	case VK_BLEND_OP_CONTRAST_EXT: FATAL_ERROR();
	case VK_BLEND_OP_INVERT_OVG_EXT: FATAL_ERROR();
	case VK_BLEND_OP_RED_EXT: FATAL_ERROR();
	case VK_BLEND_OP_GREEN_EXT: FATAL_ERROR();
	case VK_BLEND_OP_BLUE_EXT: FATAL_ERROR();
	default: FATAL_ERROR();
	}

	if (blendState.colorBlendOp != blendState.alphaBlendOp)
	{
		switch (blendState.alphaBlendOp)
		{
		case VK_BLEND_OP_ADD:
			value.a = source.a * sourceBlendFactor.a + destination.a * destinationBlendFactor.a;
			break;
			
		case VK_BLEND_OP_SUBTRACT:
			value.a = source.a * sourceBlendFactor.a - destination.a * destinationBlendFactor.a;
			break;

		case VK_BLEND_OP_REVERSE_SUBTRACT:
			value.a = destination.a * destinationBlendFactor.a - source.a * sourceBlendFactor.a;
			break;

		case VK_BLEND_OP_MIN:
			value.a = std::min(source.a, destination.a);
			break;
			
		case VK_BLEND_OP_MAX:
			value.a = std::max(source.a, destination.a);
			break;
			
		case VK_BLEND_OP_ZERO_EXT: FATAL_ERROR();
		case VK_BLEND_OP_SRC_EXT: FATAL_ERROR();
		case VK_BLEND_OP_DST_EXT: FATAL_ERROR();
		case VK_BLEND_OP_SRC_OVER_EXT: FATAL_ERROR();
		case VK_BLEND_OP_DST_OVER_EXT: FATAL_ERROR();
		case VK_BLEND_OP_SRC_IN_EXT: FATAL_ERROR();
		case VK_BLEND_OP_DST_IN_EXT: FATAL_ERROR();
		case VK_BLEND_OP_SRC_OUT_EXT: FATAL_ERROR();
		case VK_BLEND_OP_DST_OUT_EXT: FATAL_ERROR();
		case VK_BLEND_OP_SRC_ATOP_EXT: FATAL_ERROR();
		case VK_BLEND_OP_DST_ATOP_EXT: FATAL_ERROR();
		case VK_BLEND_OP_XOR_EXT: FATAL_ERROR();
		case VK_BLEND_OP_MULTIPLY_EXT: FATAL_ERROR();
		case VK_BLEND_OP_SCREEN_EXT: FATAL_ERROR();
		case VK_BLEND_OP_OVERLAY_EXT: FATAL_ERROR();
		case VK_BLEND_OP_DARKEN_EXT: FATAL_ERROR();
		case VK_BLEND_OP_LIGHTEN_EXT: FATAL_ERROR();
		case VK_BLEND_OP_COLORDODGE_EXT: FATAL_ERROR();
		case VK_BLEND_OP_COLORBURN_EXT: FATAL_ERROR();
		case VK_BLEND_OP_HARDLIGHT_EXT: FATAL_ERROR();
		case VK_BLEND_OP_SOFTLIGHT_EXT: FATAL_ERROR();
		case VK_BLEND_OP_DIFFERENCE_EXT: FATAL_ERROR();
		case VK_BLEND_OP_EXCLUSION_EXT: FATAL_ERROR();
		case VK_BLEND_OP_INVERT_EXT: FATAL_ERROR();
		case VK_BLEND_OP_INVERT_RGB_EXT: FATAL_ERROR();
		case VK_BLEND_OP_LINEARDODGE_EXT: FATAL_ERROR();
		case VK_BLEND_OP_LINEARBURN_EXT: FATAL_ERROR();
		case VK_BLEND_OP_VIVIDLIGHT_EXT: FATAL_ERROR();
		case VK_BLEND_OP_LINEARLIGHT_EXT: FATAL_ERROR();
		case VK_BLEND_OP_PINLIGHT_EXT: FATAL_ERROR();
		case VK_BLEND_OP_HARDMIX_EXT: FATAL_ERROR();
		case VK_BLEND_OP_HSL_HUE_EXT: FATAL_ERROR();
		case VK_BLEND_OP_HSL_SATURATION_EXT: FATAL_ERROR();
		case VK_BLEND_OP_HSL_COLOR_EXT: FATAL_ERROR();
		case VK_BLEND_OP_HSL_LUMINOSITY_EXT: FATAL_ERROR();
		case VK_BLEND_OP_PLUS_EXT: FATAL_ERROR();
		case VK_BLEND_OP_PLUS_CLAMPED_EXT: FATAL_ERROR();
		case VK_BLEND_OP_PLUS_CLAMPED_ALPHA_EXT: FATAL_ERROR();
		case VK_BLEND_OP_PLUS_DARKER_EXT: FATAL_ERROR();
		case VK_BLEND_OP_MINUS_EXT: FATAL_ERROR();
		case VK_BLEND_OP_MINUS_CLAMPED_EXT: FATAL_ERROR();
		case VK_BLEND_OP_CONTRAST_EXT: FATAL_ERROR();
		case VK_BLEND_OP_INVERT_OVG_EXT: FATAL_ERROR();
		case VK_BLEND_OP_RED_EXT: FATAL_ERROR();
		case VK_BLEND_OP_GREEN_EXT: FATAL_ERROR();
		case VK_BLEND_OP_BLUE_EXT: FATAL_ERROR();
		default: FATAL_ERROR();
		}
	}

	return value;
}

// TODO: Merge glslfunctions version, probably move to imagesampler
static void GetFormatOffset(Image* image, uint64_t& offset, uint64_t& size)
{
	offset = image->getImageSize().Level[0].Offset;
	size = image->getImageSize().Level[0].LevelSize;
}

template<int length>
glm::vec<length, uint32_t> GetImageRange(Image* image);

template<>
glm::uvec2 GetImageRange<2>(Image* image)
{
	return glm::uvec2{image->getWidth(), image->getHeight()};
}

template<typename ReturnType, typename CoordinateType>
static ReturnType ImageFetch(DeviceState* deviceState, VkFormat format, Image* image, CoordinateType coordinates)
{
	uint64_t offset;
	uint64_t size;
	GetFormatOffset(image, offset, size);
	auto data = image->getData(offset, size);
	auto range = GetImageRange<CoordinateType::length()>(image);

	return GetPixel<ReturnType>(deviceState,
	                            format,
	                            data,
	                            range,
	                            coordinates,
	                            ReturnType{});
}

static void DrawPixel(DeviceState* deviceState, FragmentBuiltinInput* builtinInput, const ShaderFunction* shaderStage, bool front, float depth,
                      std::pair<VkAttachmentDescription, Image*> depthImage, std::pair<VkAttachmentDescription, Image*> stencilImage,
                      std::vector<std::pair<VkAttachmentDescription, Image*>>& images, std::vector<VariableInOutData>& outputData, 
                      uint32_t x, uint32_t y)
{
	builtinInput->fragCoord.z = depth;

	// TODO: 27.2. Discard Rectangles Test
	// TODO: 27.3. Scissor Test
	// TODO: 27.4. Exclusive Scissor Test
	// TODO: 27.5. Sample Mask

	// TODO: If early per-fragment operations are enabled by the fragment shader, these operations are also performed:
	// TODO: 27.11. Depth Bounds Test
	// TODO: 27.12. Stencil Test
	// TODO: 27.13. Depth Test
	// TODO: 27.14. Representative Fragment Test
	// TODO: 27.15. Sample Counting

	shaderStage->getEntryPoint()();

	// TODO: 27.8. Mixed attachment samples
	// TODO: 27.9. Multisample Coverage
	// TODO: 27.10. Depth and Stencil Operations
	// TODO: 27.11. Depth Bounds Test

	// 27.12. Stencil Test
	auto stencilResult = true;
	auto& stencilOpState = front
		                       ? deviceState->pipelineState[PIPELINE_GRAPHICS].pipeline->getDepthStencilState().Front
		                       : deviceState->pipelineState[PIPELINE_GRAPHICS].pipeline->getDepthStencilState().Back;

	uint8_t stencilReference = stencilOpState.reference;
	uint8_t stencil = 0;
	if (deviceState->pipelineState[PIPELINE_GRAPHICS].pipeline->getDynamicState().DynamicStencilReference)
	{
		FATAL_ERROR();
	}

	if (deviceState->pipelineState[PIPELINE_GRAPHICS].pipeline->getDepthStencilState().StencilTestEnable)
	{
		auto compareMask = stencilOpState.compareMask;
		if (deviceState->pipelineState[PIPELINE_GRAPHICS].pipeline->getDynamicState().DynamicStencilCompareMask)
		{
			FATAL_ERROR();
		}

		stencil = GetStencilPixel(deviceState, stencilImage.first.format, stencilImage.second, x, y, 0, 0, 0);
		stencilResult = CompareTest(stencilReference & compareMask, stencil & compareMask, stencilOpState.compareOp);
	}

	// 27.13. Depth Test
	auto depthResult = true;
	// TODO: Optimise so depth checking uses native type
	const auto currentDepth = depthImage.second ? GetDepthPixel(deviceState, depthImage.first.format, depthImage.second, x, y, 0, 0, 0) : 0;
	if (deviceState->pipelineState[PIPELINE_GRAPHICS].pipeline->getDepthStencilState().DepthTestEnable)
	{
		if (depthImage.second)
		{
			if (deviceState->pipelineState[PIPELINE_GRAPHICS].pipeline->getRasterizationState().DepthClampEnable)
			{
				FATAL_ERROR();
			}

			depthResult = CompareTest(currentDepth, depth, deviceState->pipelineState[PIPELINE_GRAPHICS].pipeline->getDepthStencilState().DepthCompareOp);
		}
	}
	auto depthWrite = depthResult && deviceState->pipelineState[PIPELINE_GRAPHICS].pipeline->getDepthStencilState().DepthWriteEnable && depthImage.second;

	// Stencil & Depth Write
	if (deviceState->pipelineState[PIPELINE_GRAPHICS].pipeline->getDepthStencilState().StencilTestEnable && stencilImage.second)
	{
		const auto stencilOperation = !stencilResult ? stencilOpState.failOp : (!depthResult ? stencilOpState.depthFailOp : stencilOpState.passOp);
		uint8_t writeValue;
		switch (stencilOperation)
		{
		case VK_STENCIL_OP_KEEP:
			writeValue = stencil;
			break;

		case VK_STENCIL_OP_ZERO:
			writeValue = 0;
			break;

		case VK_STENCIL_OP_REPLACE:
			writeValue = stencilReference;
			break;

		case VK_STENCIL_OP_INCREMENT_AND_CLAMP:
			writeValue = stencil < 0xFF ? stencil + 1 : 0xFF;
			break;

		case VK_STENCIL_OP_DECREMENT_AND_CLAMP:
			writeValue = stencil > 0 ? stencil - 1 : 0;
			break;

		case VK_STENCIL_OP_INVERT:
			writeValue = ~stencil;
			break;

		case VK_STENCIL_OP_INCREMENT_AND_WRAP:
			writeValue = stencil + 1;
			break;

		case VK_STENCIL_OP_DECREMENT_AND_WRAP:
			writeValue = stencil - 1;
			break;

		default: FATAL_ERROR();
		}

		auto writeMask = stencilOpState.writeMask;
		if (deviceState->pipelineState[PIPELINE_GRAPHICS].pipeline->getDynamicState().DynamicStencilWriteMask)
		{
			FATAL_ERROR();
		}

		writeValue = (writeValue & writeMask) | (stencil & ~writeMask);

		if (depthWrite)
		{
			const VkClearDepthStencilValue input
			{
				depth,
				writeValue,
			};

			// TODO: No clamp if float format?
			SetPixel(deviceState, stencilImage.first.format, stencilImage.second, x, y, 0, 0, 0, input);
		}
		else
		{
			const VkClearDepthStencilValue input
			{
				currentDepth,
				writeValue,
			};

			// TODO: No clamp if float format?
			SetPixel(deviceState, stencilImage.first.format, stencilImage.second, x, y, 0, 0, 0, input);
		}
	}
	else if (depthWrite)
	{
		const VkClearDepthStencilValue input
		{
			depth,
			stencilImage.second == nullptr ? 0u : GetStencilPixel(deviceState, depthImage.first.format, depthImage.second, x, y, 0, 0, 0),
		};

		// TODO: No clamp if float format?
		SetPixel(deviceState, depthImage.first.format, depthImage.second, x, y, 0, 0, 0, input);
	}

	// TODO: 27.14. Representative Fragment Test
	// TODO: 27.15. Sample Counting
	// TODO: 27.16. Fragment Coverage To Color
	// TODO: 27.17. Coverage Reduction

	if (stencilResult && depthResult)
	{
		for (auto j = 0u; j < MAX_FRAGMENT_OUTPUT_ATTACHMENTS; j++)
		{
			if (images[j].second)
			{
				void* dataPtr = nullptr;
				for (auto data : outputData)
				{
					if (data.location == j)
					{
						dataPtr = data.pointer;
						break;
					}
				}
				assert(dataPtr);

				if (GetFormatInformation(images[j].first.format).Base == BaseType::UInt ||
					GetFormatInformation(images[j].first.format).Base == BaseType::SInt)
				{
					FATAL_ERROR();
				}

				auto colour = *static_cast<glm::fvec4*>(dataPtr);

				// 28.1. Blending
				const auto& blend = deviceState->pipelineState[PIPELINE_GRAPHICS].pipeline->getColourBlendState().Attachments[j];
				const auto destination = ImageFetch<glm::fvec4, glm::ivec2>(deviceState, images[j].first.format, images[j].second, glm::ivec2(x, y));
				if (blend.blendEnable)
				{
					auto constant = *reinterpret_cast<const glm::fvec4*>(deviceState->pipelineState[PIPELINE_GRAPHICS].pipeline->getColourBlendState().BlendConstants);
					colour = ApplyBlend(colour, destination, constant, blend);
				}

				// 28.2. Logical Operations
				if (deviceState->pipelineState[PIPELINE_GRAPHICS].pipeline->getColourBlendState().LogicOpEnable)
				{
					FATAL_ERROR();
				}

				// 28.3. Color Write Mask
				colour.r = (blend.colorWriteMask & VK_COLOR_COMPONENT_R_BIT) ? colour.r : destination.r;
				colour.g = (blend.colorWriteMask & VK_COLOR_COMPONENT_G_BIT) ? colour.g : destination.g;
				colour.b = (blend.colorWriteMask & VK_COLOR_COMPONENT_B_BIT) ? colour.b : destination.b;
				colour.a = (blend.colorWriteMask & VK_COLOR_COMPONENT_A_BIT) ? colour.a : destination.a;

				SetPixel(deviceState, images[j].first.format, images[j].second, x, y, 0, 0, 0, *reinterpret_cast<VkClearColorValue*>(&colour));
			}
		}
	}
}

static void ProcessTriangleList(DeviceState* deviceState, FragmentBuiltinInput* builtinInput, const ShaderFunction* shaderStage,
                                std::pair<VkAttachmentDescription, Image*> depthImage, std::pair<VkAttachmentDescription, Image*> stencilImage,
                                std::vector<std::pair<VkAttachmentDescription, Image*>>& images, std::vector<VariableInOutData>& outputData, 
                                uint32_t width, uint32_t height,
                                const VertexOutput& output, const RasterizationState& rasterisationState, std::vector<VariableInOutData>& inputData)
{
	const auto halfPixel = glm::vec2(1.0f / width, 1.0f / height) * 0.5f;
	const auto numberPrimitives = output.vertexCount / 3;

	for (auto i = 0u; i < numberPrimitives; i++)
	{
		auto p0Index = i * 3 + 0;
		auto p1Index = i * 3 + 1;
		auto p2Index = i * 3 + 2;
		if (rasterisationState.FrontFace == VK_FRONT_FACE_CLOCKWISE)
		{
			std::swap(p0Index, p2Index);
		}

		const auto p0 = output.builtinData[p0Index].position / output.builtinData[p0Index].position.w;
		const auto p1 = output.builtinData[p1Index].position / output.builtinData[p1Index].position.w;
		const auto p2 = output.builtinData[p2Index].position / output.builtinData[p2Index].position.w;

		for (auto y = 0u; y < height; y++)
		{
			const auto yf = (static_cast<float>(y) / height + halfPixel.y) * 2 - 1;
			builtinInput->fragCoord.y = y;

			for (auto x = 0u; x < width; x++)
			{
				const auto xf = (static_cast<float>(x) / width + halfPixel.x) * 2 - 1;
				builtinInput->fragCoord.x = shaderStage->getFragmentOriginUpper() ? x : width - x - 1;
				const auto p = glm::vec2(xf, yf);

				float depth;
				bool front;
				if (GetFragmentInput(inputData, output.outputData.get(), output.outputStride,
				                     p0Index, p1Index, p2Index,
				                     p0, p1, p2, p,
				                     rasterisationState.CullMode, depth, front))
				{
					DrawPixel(deviceState, builtinInput, shaderStage, front, depth, depthImage, stencilImage, images, outputData, x, y);
				}
			}
		}
	}
}

static void ProcessTriangleStrip(DeviceState* deviceState, FragmentBuiltinInput* builtinInput, const ShaderFunction* shaderStage,
                                 std::pair<VkAttachmentDescription, Image*> depthImage, std::pair<VkAttachmentDescription, Image*> stencilImage,
                                 std::vector<std::pair<VkAttachmentDescription, Image*>>& images, std::vector<VariableInOutData>& outputData, 
                                 uint32_t width, uint32_t height,
                                 const VertexOutput& output, const RasterizationState& rasterisationState, std::vector<VariableInOutData>& inputData)
{
	const auto halfPixel = glm::vec2(1.0f / width, 1.0f / height) * 0.5f;
	const auto numberPrimitives = output.vertexCount - 2;

	for (auto i = 0u; i < numberPrimitives; i++)
	{
		auto p0Index = i + 0;
		auto p1Index = i + 1;
		auto p2Index = i + 2;
		if (rasterisationState.FrontFace == VK_FRONT_FACE_CLOCKWISE)
		{
			std::swap(p0Index, p2Index);
		}

		const auto p0 = output.builtinData[p0Index].position / output.builtinData[p0Index].position.w;
		const auto p1 = output.builtinData[p1Index].position / output.builtinData[p1Index].position.w;
		const auto p2 = output.builtinData[p2Index].position / output.builtinData[p2Index].position.w;

		for (auto y = 0u; y < height; y++)
		{
			const auto yf = (static_cast<float>(y) / height + halfPixel.y) * 2 - 1;
			builtinInput->fragCoord.y = y;

			for (auto x = 0u; x < width; x++)
			{
				const auto xf = (static_cast<float>(x) / width + halfPixel.x) * 2 - 1;
				builtinInput->fragCoord.x = shaderStage->getFragmentOriginUpper() ? x : width - x - 1;
				const auto p = glm::vec2(xf, yf);

				float depth;
				bool front;
				if (GetFragmentInput(inputData, output.outputData.get(), output.outputStride,
				                     p0Index, p1Index, p2Index,
				                     p0, p1, p2, p,
				                     rasterisationState.CullMode, depth, front))
				{
					DrawPixel(deviceState, builtinInput, shaderStage, front, depth, depthImage, stencilImage, images, outputData, x, y);
				}
			}
		}
	}
}

static void ProcessFragmentShader(DeviceState* deviceState, const VertexOutput& output)
{
	const auto& inputAssembly = deviceState->pipelineState[PIPELINE_GRAPHICS].pipeline->getInputAssemblyState();
	const auto& shaderStage = deviceState->pipelineState[PIPELINE_GRAPHICS].pipeline->getShaderStage(4);
	const auto& rasterisationState = deviceState->pipelineState[PIPELINE_GRAPHICS].pipeline->getRasterizationState();

	if (rasterisationState.RasterizerDiscardEnable)
	{
		FATAL_ERROR();
	}

	const auto module = shaderStage->getModule();
	const auto llvmModule = shaderStage->getLLVMModule();

	const auto builtinInputPointer = deviceState->jit->getPointer(llvmModule, "_builtinInput");
	const auto builtinOutputPointer = deviceState->jit->getPointer(llvmModule, "_builtinOutput");

	auto outputSize = 0u;
	std::vector<VariableInOutData> inputData{};
	std::vector<VariableUniformData> uniformData{};
	std::vector<VariableInOutData> outputData{};
	std::pair<void*, uint32_t> pushConstant{};
	GetVariablePointers(module, llvmModule, deviceState->jit, inputData, uniformData, outputData, pushConstant, outputSize);

	LoadUniforms(deviceState, module, uniformData, PIPELINE_GRAPHICS);
	
	std::vector<std::pair<VkAttachmentDescription, Image*>> images{MAX_FRAGMENT_OUTPUT_ATTACHMENTS};
	std::pair<VkAttachmentDescription, Image*> depthImage{};
	std::pair<VkAttachmentDescription, Image*> stencilImage{};
	
	for (auto i = 0u; i < deviceState->currentSubpass->ColourAttachments.size(); i++)
	{
		const auto attachmentIndex = deviceState->currentSubpass->ColourAttachments[i].attachment;
		const auto& attachment = deviceState->currentRenderPass->getAttachments()[attachmentIndex];
		images[i] = std::make_pair(attachment, deviceState->currentFramebuffer->getAttachments()[attachmentIndex]->getImage());
	}
	
	if (deviceState->currentSubpass->DepthStencilAttachment.layout != VK_IMAGE_LAYOUT_UNDEFINED)
	{
		const auto attachmentIndex = deviceState->currentSubpass->DepthStencilAttachment.attachment;
		const auto& attachment = deviceState->currentRenderPass->getAttachments()[attachmentIndex];
		const auto format = deviceState->currentFramebuffer->getAttachments()[attachmentIndex]->getFormat();
		if (GetFormatInformation(format).DepthStencil.DepthOffset != INVALID_OFFSET)
		{
			depthImage = std::make_pair(attachment, deviceState->currentFramebuffer->getAttachments()[attachmentIndex]->getImage());
		}
		if (GetFormatInformation(format).DepthStencil.StencilOffset != INVALID_OFFSET)
		{
			stencilImage = std::make_pair(attachment, deviceState->currentFramebuffer->getAttachments()[attachmentIndex]->getImage());
		}
	}

	uint32_t width = 0;
	uint32_t height = 0;
	if (depthImage.second)
	{
		width = depthImage.second->getWidth();
		height = depthImage.second->getHeight();
	}
	else if (stencilImage.second)
	{
		width = stencilImage.second->getWidth();
		height = stencilImage.second->getHeight();
	}
	else
	{
		for (auto i = 0u; i < MAX_FRAGMENT_OUTPUT_ATTACHMENTS; i++)
		{
			width = images[0].second->getWidth();
			height = images[0].second->getHeight();
			break;
		}
	}

	if (width == 0 || height == 0)
	{
		FATAL_ERROR();
	}

	if (pushConstant.first)
	{
		memcpy(pushConstant.first, deviceState->pushConstants, pushConstant.second);
	}

	auto builtinInput = static_cast<FragmentBuiltinInput*>(builtinInputPointer);
	builtinInput->fragCoord = glm::vec4(0, 0, 0, 1);

	if (rasterisationState.PolygonMode != VK_POLYGON_MODE_FILL)
	{
		FATAL_ERROR();
	}

	switch (inputAssembly.Topology)
	{
	case VK_PRIMITIVE_TOPOLOGY_POINT_LIST:
		// ProcessTriangleList(deviceState, builtinInput, shaderStage, depthImage, stencilImage, images, outputData, width, height, output, rasterisationState, inputData);
		// break;
		
	case VK_PRIMITIVE_TOPOLOGY_LINE_LIST: FATAL_ERROR();
	case VK_PRIMITIVE_TOPOLOGY_LINE_STRIP: FATAL_ERROR();
		
	case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST:
		ProcessTriangleList(deviceState, builtinInput, shaderStage, depthImage, stencilImage, images, outputData, width, height, output, rasterisationState, inputData);
		break;
		
	case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP:
		ProcessTriangleStrip(deviceState, builtinInput, shaderStage, depthImage, stencilImage, images, outputData, width, height, output, rasterisationState, inputData);
		break;
		
	case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN: FATAL_ERROR();
	case VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY: FATAL_ERROR();
	case VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY: FATAL_ERROR();
	case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY: FATAL_ERROR();
	case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY: FATAL_ERROR();
	case VK_PRIMITIVE_TOPOLOGY_PATCH_LIST: FATAL_ERROR();
	default: FATAL_ERROR();
	}
}

static void ProcessComputeShader(DeviceState* deviceState, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
{
	const auto& shaderStage = deviceState->pipelineState[PIPELINE_COMPUTE].pipeline->getShaderStage(5);
	
	const auto module = shaderStage->getModule();
	const auto llvmModule = shaderStage->getLLVMModule();
	const auto localCount = shaderStage->getComputeLocalSize();
	
	const auto builtinInputPointer = deviceState->jit->getPointer(llvmModule, "_builtinInput");
	const auto builtinOutputPointer = deviceState->jit->getPointer(llvmModule, "_builtinOutput");
	
	auto outputSize = 0u;
	std::vector<VariableInOutData> inputData{};
	std::vector<VariableUniformData> uniformData{};
	std::vector<VariableInOutData> outputData{};
	std::pair<void*, uint32_t> pushConstant{};
	GetVariablePointers(module, llvmModule, deviceState->jit, inputData, uniformData, outputData, pushConstant, outputSize);

	assert(inputData.empty() && outputData.empty());
	
	LoadUniforms(deviceState, module, uniformData, PIPELINE_COMPUTE);
	
	if (pushConstant.first)
	{
		memcpy(pushConstant.first, deviceState->pushConstants, pushConstant.second);
	}
	
	auto builtinInput = static_cast<ComputeBuiltinInput*>(builtinInputPointer);
	builtinInput->globalInvocationId = glm::ivec3();
	builtinInput->localInvocationId = glm::ivec3();
	builtinInput->workgroupInvocationId = glm::ivec3();

	for (builtinInput->workgroupInvocationId.z = 0, builtinInput->globalInvocationId.z = 0; builtinInput->workgroupInvocationId.z < groupCountZ; builtinInput->workgroupInvocationId.z++)
	{
		for (builtinInput->localInvocationId.z = 0u; builtinInput->localInvocationId.z < localCount.z; builtinInput->localInvocationId.z++, builtinInput->globalInvocationId.z++)
		{
			for (builtinInput->workgroupInvocationId.y = 0, builtinInput->globalInvocationId.y = 0; builtinInput->workgroupInvocationId.y < groupCountY; builtinInput->workgroupInvocationId.y++)
			{
				for (builtinInput->localInvocationId.y = 0u; builtinInput->localInvocationId.y < localCount.y; builtinInput->localInvocationId.y++, builtinInput->globalInvocationId.y++)
				{
					for (builtinInput->workgroupInvocationId.x = 0, builtinInput->globalInvocationId.x = 0; builtinInput->workgroupInvocationId.x < groupCountX; builtinInput->workgroupInvocationId.x++)
					{
						for (builtinInput->localInvocationId.x = 0u; builtinInput->localInvocationId.x < localCount.x; builtinInput->localInvocationId.x++, builtinInput->globalInvocationId.x++)
						{
							shaderStage->getEntryPoint()();
						}
					}
				}
			}
		}
	}
}

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

#if CV_DEBUG_LEVEL > 0
	void DebugOutput(DeviceState* deviceState) override
	{
		*deviceState->debugOutput << "Draw: drawing " <<
			vertexCount << " vertices, " <<
			instanceCount << " instances" <<
			" starting at vertex " << firstVertex <<
			" and instance " << firstInstance <<
			std::endl;
	}
#endif

	void Process(DeviceState* deviceState) override
	{
		for (auto i = 0; i < instanceCount; i++)
		{
			const auto assemblerOutput = ProcessInputAssembler(deviceState, firstVertex, vertexCount);
			const auto vertexOutput = ProcessVertexShader(deviceState, firstInstance + i, assemblerOutput);

			if (deviceState->pipelineState[PIPELINE_GRAPHICS].pipeline->getShaderStage(1) != nullptr)
			{
				FATAL_ERROR();
			}

			if (deviceState->pipelineState[PIPELINE_GRAPHICS].pipeline->getShaderStage(2) != nullptr)
			{
				FATAL_ERROR();
			}

			if (deviceState->pipelineState[PIPELINE_GRAPHICS].pipeline->getShaderStage(3) != nullptr)
			{
				FATAL_ERROR();
			}

			ProcessFragmentShader(deviceState, vertexOutput);
		}
	}

private:
	uint32_t vertexCount;
	uint32_t instanceCount;
	uint32_t firstVertex;
	uint32_t firstInstance;
};

class DrawIndexedCommand final : public Command
{
public:
	DrawIndexedCommand(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) :
		indexCount{indexCount},
		instanceCount{instanceCount},
		firstIndex{firstIndex},
		vertexOffset{vertexOffset},
		firstInstance{firstInstance}
	{
	}

#if CV_DEBUG_LEVEL > 0
	void DebugOutput(DeviceState* deviceState) override
	{
		*deviceState->debugOutput << "DrawIndexed: drawing " <<
			indexCount << " indices, " <<
			instanceCount << " instances" <<
			" starting at index " << firstIndex <<
			" with vertex offset " << vertexOffset <<
			" and instance " << firstInstance <<
			std::endl;
	}
#endif

	void Process(DeviceState* deviceState) override
	{
		if (firstIndex != 0)
		{
			FATAL_ERROR();
		}

		if (vertexOffset != 0)
		{
			FATAL_ERROR();
		}

		for (auto i = 0; i < instanceCount; i++)
		{
			const auto assemblerOutput = ProcessInputAssemblerIndexed(deviceState, indexCount);
			const auto vertexOutput = ProcessVertexShader(deviceState, firstInstance + i, assemblerOutput);

			if (deviceState->pipelineState[PIPELINE_GRAPHICS].pipeline->getShaderStage(1) != nullptr)
			{
				FATAL_ERROR();
			}

			if (deviceState->pipelineState[PIPELINE_GRAPHICS].pipeline->getShaderStage(2) != nullptr)
			{
				FATAL_ERROR();
			}

			if (deviceState->pipelineState[PIPELINE_GRAPHICS].pipeline->getShaderStage(3) != nullptr)
			{
				FATAL_ERROR();
			}

			ProcessFragmentShader(deviceState, vertexOutput);
		}
	}

private:
	uint32_t indexCount;
	uint32_t instanceCount;
	uint32_t firstIndex;
	int32_t vertexOffset;
	uint32_t firstInstance;
};

class DispatchCommand final : public Command
{
public:
	DispatchCommand(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) :
		groupCountX{groupCountX},
		groupCountY{groupCountY},
		groupCountZ{groupCountZ}
	{
	}

#if CV_DEBUG_LEVEL > 0
	void DebugOutput(DeviceState* deviceState) override
	{
		*deviceState->debugOutput << "Dispatch: dispatching compute" <<
			" with group count x: " << groupCountX <<
			" with group count y: " << groupCountY <<
			" with group count z: " << groupCountZ <<
			std::endl;
	}
#endif

	void Process(DeviceState* deviceState) override
	{
		ProcessComputeShader(deviceState, groupCountX, groupCountY, groupCountZ);
	}

private:
	uint32_t groupCountX;
	uint32_t groupCountY;
	uint32_t groupCountZ;
};

class ClearColourImageCommand final : public Command
{
public:
	ClearColourImageCommand(Image* image, VkClearColorValue colour, std::vector<VkImageSubresourceRange> ranges):
		image{image},
		colour{colour},
		ranges{std::move(ranges)}
	{
	}

#if CV_DEBUG_LEVEL > 0
	void DebugOutput(DeviceState* deviceState) override
	{
		*deviceState->debugOutput << "ClearColourImage: clearing image" <<
			" on " << image <<
			" to " << DebugPrint(image, colour) <<
			" on ranges " << ranges <<
			std::endl;
	}
#endif

	void Process(DeviceState* deviceState) override
	{
		for (const auto& range : ranges)
		{
			auto levels = range.levelCount;
			if (levels == VK_REMAINING_MIP_LEVELS)
			{
				levels = image->getMipLevels() - range.baseMipLevel;
			}

			auto layers = range.layerCount;
			if (layers == VK_REMAINING_ARRAY_LAYERS)
			{
				layers = image->getArrayLayers() - range.baseArrayLayer;
			}
			
			if (range.aspectMask == VK_IMAGE_ASPECT_COLOR_BIT)
			{
				ClearImage(deviceState, image, image->getFormat(), range.baseMipLevel, levels, range.baseArrayLayer, layers, colour);
			}
			else
			{
				FATAL_ERROR();
			}
		}
	}

private:
	Image* image;
	VkClearColorValue colour;
	std::vector<VkImageSubresourceRange> ranges;
};

class ClearDepthStencilImageCommand final : public Command
{
public:
	ClearDepthStencilImageCommand(Image* image, VkClearDepthStencilValue colour, std::vector<VkImageSubresourceRange> ranges):
		image{image},
		colour{colour},
		ranges{std::move(ranges)}
	{
	}

#if CV_DEBUG_LEVEL > 0
	void DebugOutput(DeviceState* deviceState) override
	{
		*deviceState->debugOutput << "ClearDepthStencilImage: clearing image" <<
			" on " << image <<
			" to " << colour <<
			" on ranges " << ranges <<
			std::endl;
	}
#endif

	void Process(DeviceState* deviceState) override
	{
		for (const auto& range : ranges)
		{
			auto levels = range.levelCount;
			if (levels == VK_REMAINING_MIP_LEVELS)
			{
				levels = image->getMipLevels() - range.baseMipLevel;
			}

			auto layers = range.layerCount;
			if (layers == VK_REMAINING_ARRAY_LAYERS)
			{
				layers = image->getArrayLayers() - range.baseArrayLayer;
			}

			if ((range.aspectMask & ~(VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT)) != 0)
			{
				FATAL_ERROR();
			}

			ClearImage(deviceState, image, image->getFormat(), range.baseMipLevel, levels, range.baseArrayLayer, layers, range.aspectMask, colour);
		}
	}

private:
	Image* image;
	VkClearDepthStencilValue colour;
	std::vector<VkImageSubresourceRange> ranges;
};

class ClearAttachmentsCommand final : public Command
{
public:
	ClearAttachmentsCommand(std::vector<VkClearAttachment> attachments, std::vector<VkClearRect> rects) :
		attachments{std::move(attachments)},
		rects{std::move(rects)}
	{
	}

#if CV_DEBUG_LEVEL > 0
	void DebugOutput(DeviceState* deviceState) override
	{
		*deviceState->debugOutput << "ClearAttachments: clearing attachments" <<
			" on " << attachments <<
			" with values" << rects <<
			std::endl;
	}
#endif

	void Process(DeviceState* deviceState) override
	{
		for (auto& attachment : attachments)
		{
			// TODO:  If any attachment to be cleared in the current subpass is VK_ATTACHMENT_UNUSED, then the clear has no effect on that attachment.

			ImageView* imageView;
			VkFormat format;
			if (attachment.aspectMask & VK_IMAGE_ASPECT_COLOR_BIT)
			{
				format = deviceState->currentRenderPass->getAttachments()[attachment.colorAttachment].format;
				imageView = deviceState->currentFramebuffer->getAttachments()[attachment.colorAttachment];
			}
			else
			{
				assert(attachment.aspectMask & (VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT));
				const auto attachmentReference = deviceState->currentSubpass->DepthStencilAttachment.attachment;
				format = deviceState->currentRenderPass->getAttachments()[attachmentReference].format;
				imageView = deviceState->currentFramebuffer->getAttachments()[attachmentReference];
			}
			
			if (attachment.aspectMask & VK_IMAGE_ASPECT_COLOR_BIT)
			{
				for (auto& rect : rects)
				{
					for (auto layer = 0u; layer < rect.layerCount; layer++)
					{
						for (auto y = rect.rect.offset.y; y < rect.rect.offset.y + rect.rect.extent.height; y++)
						{
							for (auto x = rect.rect.offset.x; x < rect.rect.offset.x + rect.rect.extent.width; x++)
							{
								SetPixel(deviceState, format, imageView->getImage(), x, y, 0, 0, layer + rect.baseArrayLayer + imageView->getSubresourceRange().baseArrayLayer, attachment.clearValue.color);
							}
						}
					}
				}
			}
			
			if (attachment.aspectMask & (VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT))
			{
				// TODO: use aspectMask
				for (auto& rect : rects)
				{
					for (auto layer = 0u; layer < rect.layerCount; layer++)
					{
						for (auto y = rect.rect.offset.y; y < rect.rect.offset.y + rect.rect.extent.height; y++)
						{
							for (auto x = rect.rect.offset.x; x < rect.rect.offset.x + rect.rect.extent.width; x++)
							{
								SetPixel(deviceState, format, imageView->getImage(), x, y, 0, 0, layer + rect.baseArrayLayer + imageView->getSubresourceRange().baseArrayLayer, attachment.clearValue.depthStencil);
							}
						}
					}
				}
			}
		}
	}

private:
	std::vector<VkClearAttachment> attachments;
	std::vector<VkClearRect> rects;
};

void CommandBuffer::Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
{
	assert(state == State::Recording);
	commands.push_back(std::make_unique<DrawCommand>(vertexCount, instanceCount, firstVertex, firstInstance));
}

void CommandBuffer::DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance)
{
	assert(state == State::Recording);
	commands.push_back(std::make_unique<DrawIndexedCommand>(indexCount, instanceCount, firstIndex, vertexOffset, firstInstance));
}

void CommandBuffer::Dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
{
	assert(state == State::Recording);
	commands.push_back(std::make_unique<DispatchCommand>(groupCountX, groupCountY, groupCountZ));
}

void CommandBuffer::ClearColorImage(VkImage image, VkImageLayout, const VkClearColorValue* pColor, uint32_t rangeCount, const VkImageSubresourceRange* pRanges)
{
	assert(state == State::Recording);
	commands.push_back(std::make_unique<ClearColourImageCommand>(UnwrapVulkan<Image>(image), *pColor, ArrayToVector(rangeCount, pRanges)));
}

void CommandBuffer::ClearDepthStencilImage(VkImage image, VkImageLayout, const VkClearDepthStencilValue* pDepthStencil, uint32_t rangeCount, const VkImageSubresourceRange* pRanges)
{
	assert(state == State::Recording);
	commands.push_back(std::make_unique<ClearDepthStencilImageCommand>(UnwrapVulkan<Image>(image), *pDepthStencil, ArrayToVector(rangeCount, pRanges)));
}

void CommandBuffer::ClearAttachments(uint32_t attachmentCount, const VkClearAttachment* pAttachments, uint32_t rectCount, const VkClearRect* pRects)
{
	assert(state == State::Recording);
	commands.push_back(std::make_unique<ClearAttachmentsCommand>(ArrayToVector(attachmentCount, pAttachments), ArrayToVector(rectCount, pRects)));
}

void ClearImage(DeviceState* deviceState, Image* image, VkFormat format, uint32_t baseMipLevel, uint32_t levelCount, uint32_t baseArrayLayer, uint32_t layerCount, VkClearColorValue colour)
{
	for (auto layer = 0u; layer < layerCount; layer++)
	{
		for (auto level = 0u; level < levelCount; level++)
		{
			ClearImage(deviceState, image, baseArrayLayer + layer, baseMipLevel + level, format, colour);
		}
	}
}

void ClearImage(DeviceState* deviceState, Image* image, VkFormat format, uint32_t baseMipLevel, uint32_t levelCount, uint32_t baseArrayLayer, uint32_t layerCount, VkImageAspectFlags aspects, VkClearDepthStencilValue colour)
{
	// TODO: use aspects
	for (auto layer = 0u; layer < layerCount; layer++)
	{
		for (auto level = 0u; level < levelCount; level++)
		{
			ClearImage(deviceState, image, baseArrayLayer + layer, baseMipLevel + level, format, colour);
		}
	}
}