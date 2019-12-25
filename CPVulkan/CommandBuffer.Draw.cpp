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

#include <CompiledModule.h>
#include <PipelineData.h>
#include <SPIRVCompiler.h>
#include <SPIRVFunction.h>
#include <SPIRVInstruction.h>
#include <SPIRVModule.h>

#include <glm/glm.hpp>
#include <glm/gtx/vec_swizzle.hpp>

#include <fstream>

class PipelineLayout;

enum class PrimitiveType
{
	Point,
	Line,
	Triangle,
};

struct VertexInput
{
	uint32_t rawId;
	uint32_t vertexId;
};

struct Primitive
{
	uint32_t provokingVertex;
	uint32_t vertex[3];
};

struct AssemblerOutput
{
	PrimitiveType primitiveType;
	std::vector<VertexInput> vertices{};
	std::vector<Primitive> primitives{};
};

struct VertexBuiltinInput
{
	int32_t vertexIndex;
	int32_t instanceIndex;
};

struct FragmentBuiltinInput
{
	glm::vec4 fragCoord;
};

struct FragmentBuiltinOutput
{
	uint32_t stencilReference;
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
	uint64_t builtinStride;
	uint64_t outputStride;
	uint32_t vertexCount;
};

enum class InterpolationType
{
	Perspective,
	Linear,
	Flat,
};

struct VariableInOutData
{
	void* pointer;
	uint32_t location;
	VkFormat format;
	SPIRV::SPIRVType* type;
	InterpolationType interpolation;
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
	const auto& mip = gsl::at(image->getImageSize().Level, mipLevel);

	// TODO: Change to SetPixels when implemented
	for (auto z = 0u; z < mip.Depth; z++)
	{
		for (auto y = 0u; y < mip.Height; y++)
		{
			for (auto x = 0u; x < mip.Width; x++)
			{
				SetPixel(deviceState, format, image, x, y, z, mipLevel, layer, colour);
			}
		}
	}
}

static void ClearImage(DeviceState* deviceState, Image* image, uint32_t layer, uint32_t mipLevel, VkFormat format, VkClearDepthStencilValue colour)
{
	const auto& mip = gsl::at(image->getImageSize().Level, mipLevel);

	// TODO: Change to SetPixels when implemented
	for (auto z = 0u; z < mip.Depth; z++)
	{
		for (auto y = 0u; y < mip.Height; y++)
		{
			for (auto x = 0u; x < mip.Width; x++)
			{
				SetPixel(deviceState, format, image, x, y, z, mipLevel, layer, colour);
			}
		}
	}
}

static VkFormat GetVariableFormat(SPIRV::SPIRVType* type)
{
	if (type->isTypeMatrix())
	{
		return VK_FORMAT_UNDEFINED;
	}

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

	if (type->isTypeFloat(16))
	{
		return VK_FORMAT_R16_SFLOAT;
	}

	if (type->isTypeFloat(32))
	{
		return VK_FORMAT_R32_SFLOAT;
	}

	if (type->isTypeFloat(64))
	{
		return VK_FORMAT_R64_SFLOAT;
	}

	if (type->isTypeInt(8) && static_cast<SPIRV::SPIRVTypeInt*>(type)->isSigned())
	{
		return VK_FORMAT_R8_SINT;
	}

	if (type->isTypeInt(16) && static_cast<SPIRV::SPIRVTypeInt*>(type)->isSigned())
	{
		return VK_FORMAT_R16_SINT;
	}

	if (type->isTypeInt(32) && static_cast<SPIRV::SPIRVTypeInt*>(type)->isSigned())
	{
		return VK_FORMAT_R32_SINT;
	}

	if (type->isTypeInt(64) && static_cast<SPIRV::SPIRVTypeInt*>(type)->isSigned())
	{
		return VK_FORMAT_R64_SINT;
	}

	if (type->isTypeInt(8) && !static_cast<SPIRV::SPIRVTypeInt*>(type)->isSigned())
	{
		return VK_FORMAT_R8_UINT;
	}

	if (type->isTypeInt(16) && !static_cast<SPIRV::SPIRVTypeInt*>(type)->isSigned())
	{
		return VK_FORMAT_R16_UINT;
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
		const auto size = GetVariableSize(type->getArrayElementType());
		if (type->hasDecorate(DecorationArrayStride))
		{
			const auto stride = *type->getDecorate(DecorationArrayStride).begin();
			if (stride != size)
			{
				TODO_ERROR();
			}
		}

		return size * type->getArrayLength();
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
					TODO_ERROR();
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
		return GetVariableSize(type->getScalarType()) * type->getMatrixColumnCount() * type->getMatrixColumnType()->getVectorComponentCount();
	}

	if (type->isTypeVector())
	{
		return GetVariableSize(type->getScalarType()) * type->getVectorComponentCount();
	}

	if (type->isTypeFloat() || type->isTypeInt())
	{
		return type->getBitWidth() / 8;
	}

	FATAL_ERROR();
}

static void LoadUniforms(DeviceState* deviceState, const std::vector<VariableUniformData>& uniformData, int pipelineIndex)
{
	for (const auto& data : uniformData)
	{
		const auto descriptorSet = deviceState->pipelineState[pipelineIndex].descriptorSets[data.set];
		VkDescriptorType descriptorType;
		const Descriptor* value;
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
			for (auto j = 0u; j < value->count; j++)
			{
				const auto& bufferInfo = value->values[j].Buffer;
				static_cast<const void**>(data.pointer)[j] = UnwrapVulkan<Buffer>(bufferInfo.buffer)->getDataPtr(bufferInfo.offset, bufferInfo.range);
			}
			break;

		case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
		case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
			for (auto j = 0u; j < value->count; j++)
			{
				const auto dynamicOffset = deviceState->pipelineState[pipelineIndex].descriptorSetDynamicOffset[data.set][data.binding];
				const auto& bufferInfo = value->values[j].Buffer;
				static_cast<const void**>(data.pointer)[j] = UnwrapVulkan<Buffer>(bufferInfo.buffer)->getDataPtr(bufferInfo.offset + dynamicOffset, bufferInfo.range);
			}
			break;
			
		case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
			TODO_ERROR();
			
		case VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK_EXT:
			TODO_ERROR();
			
		case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV:
			TODO_ERROR();
			
		default:
			FATAL_ERROR();
		}
	}
}

static float EdgeFunction(const glm::vec2& a, const glm::vec2& b, const glm::vec2& c)
{
	return (c.x - a.x) * (b.y - a.y) - (c.y - a.y) * (b.x - a.x);
}

static float EdgeFunction(const glm::vec4& a, const glm::vec4& b, const glm::vec2& c)
{
	return (c.x - a.x) * (b.y - a.y) - (c.y - a.y) * (b.x - a.x);
}

static void GetVariablePointers(const SPIRV::SPIRVModule* module,
                                const CompiledModule* llvmModule,
                                std::vector<VariableInOutData>& inputData,
                                std::vector<VariableUniformData>& uniformData,
                                std::vector<VariableInOutData>& outputData,
                                std::pair<void*, uint32_t>& pushConstant,
                                uint32_t& inputSize,
                                uint32_t& outputSize)
{
	auto maxLocation = -1;
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
				auto interpolationType = InterpolationType::Perspective;

				if (variable->hasDecorate(DecorationNoPerspective))
				{
					interpolationType = InterpolationType::Linear;
				}

				if (variable->hasDecorate(DecorationFlat))
				{
					interpolationType = InterpolationType::Flat;
				}
				
				inputData.push_back(VariableInOutData
					{
						llvmModule->getPointer(MangleName(variable)),
						location,
						GetVariableFormat(variable->getType()->getPointerElementType()),
						variable->getType()->getPointerElementType(),
						interpolationType,
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
						llvmModule->getPointer(MangleName(variable)),
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

				const auto ptr = llvmModule->getPointer(MangleName(variable));
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
								type,
								InterpolationType::Linear,
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
							type,
							InterpolationType::Linear,
							GetVariableSize(type),
							outputSize
						});
					outputSize += GetVariableSize(type);
				}
				break;
			}

		case StorageClassPushConstant:
			assert(std::get<0>(pushConstant) == nullptr);
			pushConstant = std::make_pair(llvmModule->getPointer(MangleName(variable)), GetVariableSize(variable->getType()->getPointerElementType()));
			break;

		case StorageClassWorkgroup:
		case StorageClassCrossWorkgroup:
		case StorageClassPrivate:
		case StorageClassGeneric:
			break;

		default:
			FATAL_ERROR();
		}
	}
}

void CalculatePrimitives(DeviceState* deviceState, AssemblerOutput& assemblerOutput)
{
	const auto vertexCount = assemblerOutput.vertices.size();
	switch (deviceState->pipelineState[PIPELINE_GRAPHICS].pipeline->getInputAssemblyState().Topology)
	{
	case VK_PRIMITIVE_TOPOLOGY_POINT_LIST:
		{
			assemblerOutput.primitiveType = PrimitiveType::Point;
			assemblerOutput.primitives.resize(vertexCount);
			for (auto i = 0u; i < vertexCount; i++)
			{
				assemblerOutput.primitives[i].provokingVertex = i;
				assemblerOutput.primitives[i].vertex[0] = i;
			}
			break;
		}

	case VK_PRIMITIVE_TOPOLOGY_LINE_LIST:
		{
			assemblerOutput.primitiveType = PrimitiveType::Line;
			const auto primitiveCount = vertexCount / 2;
			assemblerOutput.primitives.resize(primitiveCount);
			for (auto i = 0u; i < primitiveCount; i++)
			{
				assemblerOutput.primitives[i].provokingVertex = i * 2 + 0;
				assemblerOutput.primitives[i].vertex[0] = i * 2 + 0;
				assemblerOutput.primitives[i].vertex[1] = i * 2 + 1;
			}
			break;
		}

	case VK_PRIMITIVE_TOPOLOGY_LINE_STRIP:
		{
			assemblerOutput.primitiveType = PrimitiveType::Line;
			if (vertexCount > 1)
			{
				assemblerOutput.primitives.resize(vertexCount - 1);
				for (auto i = 0u; i < vertexCount - 1; i++)
				{
					assemblerOutput.primitives[i].provokingVertex = i;
					assemblerOutput.primitives[i].vertex[0] = i + 0;
					assemblerOutput.primitives[i].vertex[1] = i + 1;
				}
			}
			break;
		}

	case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST:
		{
			assemblerOutput.primitiveType = PrimitiveType::Triangle;
			const auto primitiveCount = vertexCount / 3;
			assemblerOutput.primitives.resize(primitiveCount);
			for (auto i = 0u; i < primitiveCount; i++)
			{
				assemblerOutput.primitives[i].provokingVertex = i * 3 + 0;
				assemblerOutput.primitives[i].vertex[0] = i * 3 + 0;
				assemblerOutput.primitives[i].vertex[1] = i * 3 + 1;
				assemblerOutput.primitives[i].vertex[2] = i * 3 + 2;
			}
			break;
		}

	case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP:
		{
			assemblerOutput.primitiveType = PrimitiveType::Triangle;
			if (vertexCount > 2)
			{
				assemblerOutput.primitives.resize(vertexCount - 2);
				for (auto i = 0u; i < vertexCount - 2; i++)
				{
					assemblerOutput.primitives[i].provokingVertex = i + 0;
					assemblerOutput.primitives[i].vertex[0] = i + 0;
					assemblerOutput.primitives[i].vertex[1] = i + 1;
					assemblerOutput.primitives[i].vertex[2] = i + 2;
				}
			}
			break;
		}

	case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN:
		{
			assemblerOutput.primitiveType = PrimitiveType::Triangle;
			if (vertexCount > 2)
			{
				assemblerOutput.primitives.resize(vertexCount - 2);
				for (auto i = 0u; i < vertexCount - 2; i++)
				{
					assemblerOutput.primitives[i].provokingVertex = i + 1;
					assemblerOutput.primitives[i].vertex[0] = 0;
					assemblerOutput.primitives[i].vertex[1] = i + 1;
					assemblerOutput.primitives[i].vertex[2] = i + 2;
				}
			}
			break;
		}

	case VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY:
	case VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY:
	case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY:
	case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY:
	case VK_PRIMITIVE_TOPOLOGY_PATCH_LIST:
		TODO_ERROR();

	default:
		FATAL_ERROR();
	}
}

static AssemblerOutput ProcessInputAssembler(DeviceState* deviceState, uint32_t firstVertex, uint32_t vertexCount)
{
	AssemblerOutput assemblerOutput{};
	assemblerOutput.vertices.resize(vertexCount);
	for (auto i = 0u; i < vertexCount; i++)
	{
		assemblerOutput.vertices[i].rawId = i;
		assemblerOutput.vertices[i].vertexId = firstVertex + i;
	}

	CalculatePrimitives(deviceState, assemblerOutput);
	
	return assemblerOutput;
}

template<typename T>
static void ProcessIndexedVertices(AssemblerOutput& assemblerOutput, uint32_t indexCount, uint32_t vertexOffset, uint32_t firstIndex, uint32_t primitiveSize, const T* indexData)
{
	auto currentVertex = 0u;
	for (auto i = 0u; i < indexCount; i++)
	{
		const auto index = indexData[firstIndex + i];
		if (primitiveSize > 0 && index == std::numeric_limits<T>::max())
		{
			TODO_ERROR();
		}
		
		assemblerOutput.vertices[currentVertex].rawId = i;
		assemblerOutput.vertices[currentVertex].vertexId = vertexOffset + index;
		currentVertex++;
	}
	
	if (assemblerOutput.vertices.size() != currentVertex)
	{
		assemblerOutput.vertices.resize(currentVertex);
	}
}

static AssemblerOutput ProcessInputAssemblerIndexed(DeviceState* deviceState, uint32_t firstIndex, uint32_t indexCount, uint32_t vertexOffset)
{
	AssemblerOutput assemblerOutput{};
	assemblerOutput.vertices.resize(indexCount);

	uint32_t primitiveSize = 0;
	if (deviceState->pipelineState[PIPELINE_GRAPHICS].pipeline->getInputAssemblyState().PrimitiveRestartEnable)
	{
		switch (deviceState->pipelineState[PIPELINE_GRAPHICS].pipeline->getInputAssemblyState().Topology)
		{
		case VK_PRIMITIVE_TOPOLOGY_LINE_STRIP:
			primitiveSize = 2;
			break;

		case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP:
		case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN:
			primitiveSize = 3;
			break;

		default:
			FATAL_ERROR();
		}
	}

	const auto indexData = deviceState->indexBinding->getDataPtr(deviceState->indexBindingOffset, static_cast<uint64_t>(indexCount) * deviceState->indexBindingStride);

	switch (deviceState->indexBindingStride)
	{
	case 1:
		ProcessIndexedVertices(assemblerOutput, indexCount, vertexOffset, firstIndex, primitiveSize, indexData);
		break;
		
	case 2:
		ProcessIndexedVertices(assemblerOutput, indexCount, vertexOffset, firstIndex, primitiveSize, reinterpret_cast<uint16_t*>(indexData));
		break;

	case 4:
		ProcessIndexedVertices(assemblerOutput, indexCount, vertexOffset, firstIndex, primitiveSize, reinterpret_cast<uint32_t*>(indexData));
		break;
		
	default:
		FATAL_ERROR();
	}

	CalculatePrimitives(deviceState, assemblerOutput);

	return assemblerOutput;
}

static bool EnsureVertexMemoryStorage(DeviceState* deviceState, uint64_t numberVertices, uint64_t vertexStorageStride)
{
	auto hasChanged = false;

	// TODO: Will probably replace with heap functions eventually, so we can resize without worrying about copying over old data;
	if (deviceState->vertexOutputStorage.size() < numberVertices * vertexStorageStride)
	{
		deviceState->vertexOutputStorage.resize(numberVertices * vertexStorageStride);
		hasChanged = true;
	}
	
	return !hasChanged;
}

static VertexOutput ProcessVertexShader(DeviceState* deviceState, uint32_t instance, const AssemblerOutput& assemblerOutput)
{
	assert(assemblerOutput.vertices.size() <= 0xFFFFFFFF);

	const auto& shaderStage = deviceState->pipelineState[PIPELINE_GRAPHICS].pipeline->getShaderStage(0);

	const auto spirvModule = shaderStage->getSPIRVModule();
	const auto llvmModule = shaderStage->getLLVMModule();

	auto inputSize = 0u;
	uint32_t vertexStorageStride = sizeof(VertexBuiltinOutput);
	std::vector<VariableInOutData> inputData{};
	std::vector<VariableUniformData> uniformData{};
	std::vector<VariableInOutData> outputData{};
	std::pair<void*, uint32_t> pushConstant{};
	GetVariablePointers(spirvModule, llvmModule, inputData, uniformData, outputData, pushConstant, inputSize, vertexStorageStride);

	EnsureVertexMemoryStorage(deviceState, assemblerOutput.vertices.size(), vertexStorageStride);
	const auto outputStorage = deviceState->pipelineState[PIPELINE_GRAPHICS].pipeline->getVertexModule()->getPointer("@outputStorage");
	*static_cast<void**>(outputStorage) = deviceState->vertexOutputStorage.data();

	const VertexOutput output
	{
		sizeof(VertexBuiltinOutput),
		vertexStorageStride,
		static_cast<uint32_t>(assemblerOutput.vertices.size()),
	};

	LoadUniforms(deviceState, uniformData, PIPELINE_GRAPHICS);

	if (pushConstant.first)
	{
		memcpy(pushConstant.first, deviceState->pushConstants, pushConstant.second);
	}

	reinterpret_cast<void(*)(const VertexInput*, uint32_t, uint32_t)>(deviceState->pipelineState[PIPELINE_GRAPHICS].pipeline->getVertexEntryPoint())(assemblerOutput.vertices.data(), static_cast<uint32_t>(assemblerOutput.vertices.size()), instance);
	
	return output;
}

template<bool Perspective, int size, typename T>
T SetDatum(const float points[size], const T values[size], const float weights[size])
{
	if (Perspective)
	{
		auto numerator = 0.0f;
		auto denominator = 0.0f;
		for (auto i = 0; i < size; i++)
		{
			numerator += weights[i] * values[i] / points[i];
			denominator += weights[i] / points[i];
		}
		return numerator / denominator;
	}

	auto result = 0.0f;
	for (auto i = 0; i < size; i++)
	{
		result += weights[i] * values[i];
	}
	return result;
}

template<bool Perspective, int size>
void SetDatum(const VariableInOutData& input, float points[size], const void* data[size], float weights[size])
{
	const auto information = GetFormatInformation(input.format);
	const auto numberElements = information.TotalSize / information.ElementSize;
	for (auto i = 0u; i < numberElements; i++)
	{
		switch (information.Base)
		{
		case BaseType::SFloat:
			switch (information.ElementSize)
			{
			case 4:
				{
					float values[size];
					for (auto j = 0; j < size; j++)
					{
						values[j] = static_cast<const float*>(data[j])[i];
					}

					static_cast<float*>(input.pointer)[i] = SetDatum<Perspective, size>(points, values, weights);
					break;
				}

			default:
				FATAL_ERROR();
			}
			break;

		default:
			FATAL_ERROR();
		}
	}
}

static bool GetFragmentInput(const std::vector<VariableInOutData>& inputData, const uint8_t* vertexData, uint64_t vertexStride,
                             uint32_t provokingVertex, uint32_t p0Index, uint32_t p1Index, uint32_t p2Index,
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

	depth = p0.z * w0 + p1.z * w1 + p2.z * w2;

	for (auto input : inputData)
	{
		float points[]
		{
			p0.w,
			p1.w,
			p2.w,
		};

		const void* data[]
		{
			vertexData + p0Index * vertexStride + input.offset,
			vertexData + p1Index * vertexStride + input.offset,
			vertexData + p2Index * vertexStride + input.offset,
		};

		float weights[]
		{
			w0,
			w1,
			w2,
		};

		switch (input.interpolation)
		{
		case InterpolationType::Perspective:
			SetDatum<true, 3>(input, points, data, weights);
			break;
			
		case InterpolationType::Linear:
			SetDatum<false, 3>(input, points, data, weights);
			break;
			
		case InterpolationType::Flat:
			memcpy(input.pointer, vertexData + provokingVertex * vertexStride + input.offset, input.size);
			break;
			
		default:
			FATAL_ERROR();
		}
	}

	return true;
}

template<typename T>
bool CompareTest(const T reference, const T value, const VkCompareOp compare)
{
	switch (compare)
	{
	case VK_COMPARE_OP_NEVER: return false;
	case VK_COMPARE_OP_LESS: return reference < value;
	case VK_COMPARE_OP_EQUAL: return reference == value;
	case VK_COMPARE_OP_LESS_OR_EQUAL: return reference <= value;
	case VK_COMPARE_OP_GREATER: return reference > value;
	case VK_COMPARE_OP_NOT_EQUAL: return reference != value;
	case VK_COMPARE_OP_GREATER_OR_EQUAL: return reference >= value;
	case VK_COMPARE_OP_ALWAYS: return true;
		
	default:
		FATAL_ERROR();
	}
}

template<typename T, bool IsSource>
static T ApplyBlendFactor(T source, T destination, T constant, const VkPipelineColorBlendAttachmentState& blendState)
{
	const auto colourBlendFactor = IsSource ? blendState.srcColorBlendFactor : blendState.dstColorBlendFactor;
	const auto alphaBlendFactor = IsSource ? blendState.srcAlphaBlendFactor : blendState.dstAlphaBlendFactor;
	T value{};
	switch (colourBlendFactor)
	{
	case VK_BLEND_FACTOR_ZERO:
		break;
		
	case VK_BLEND_FACTOR_ONE:
		value = T(1);
		break;
		
	case VK_BLEND_FACTOR_SRC_COLOR:
		value = source;
		break;
		
	case VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR:
		value = T(1) - source;
		break;
		
	case VK_BLEND_FACTOR_DST_COLOR:
		value = destination;
		break;
		
	case VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR:
		value = T(1) - destination;
		break;

	case VK_BLEND_FACTOR_SRC_ALPHA:
		value = T(source.a);
		break;

	case VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA:
		value = T(1 - source.a);
		break;

	case VK_BLEND_FACTOR_DST_ALPHA:
		value = T(destination.a);
		break;

	case VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA:
		value = T(1 - destination.a);
		break;

	case VK_BLEND_FACTOR_CONSTANT_COLOR:
		value = constant;
		break;
		
	case VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR:
		value = T(1) - constant;
		break;

	case VK_BLEND_FACTOR_CONSTANT_ALPHA:
		value = T(constant.a);
		break;

	case VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA:
		value = T(1 - constant.a);
		break;
		
	case VK_BLEND_FACTOR_SRC_ALPHA_SATURATE:
		{
			auto factor = std::min(source.a, 1 - destination.a);
			value = T(factor, factor, factor, 1);
			break;
		}
		
	case VK_BLEND_FACTOR_SRC1_COLOR:
		TODO_ERROR();
		
	case VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR: 
		TODO_ERROR();
		
	case VK_BLEND_FACTOR_SRC1_ALPHA:
		TODO_ERROR();
		
	case VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA: 
		TODO_ERROR();
		
	default:
		FATAL_ERROR();
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
			
		case VK_BLEND_FACTOR_SRC1_COLOR:
			TODO_ERROR();
			
		case VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR:
			TODO_ERROR();
			
		case VK_BLEND_FACTOR_SRC1_ALPHA:
			TODO_ERROR();
			
		case VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA:
			TODO_ERROR();
			
		default:
			FATAL_ERROR();
		}
	}

	return value;
}

template<typename T>
static T ApplyBlend(T source, T destination, T constant, const VkPipelineColorBlendAttachmentState& blendState)
{
	const auto sourceBlendFactor = ApplyBlendFactor<T, true>(source, destination, constant, blendState);
	const auto destinationBlendFactor = ApplyBlendFactor<T, false>(source, destination, constant, blendState);

	T value{};
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
		
	case VK_BLEND_OP_ZERO_EXT: TODO_ERROR();
	case VK_BLEND_OP_SRC_EXT: TODO_ERROR();
	case VK_BLEND_OP_DST_EXT: TODO_ERROR();
	case VK_BLEND_OP_SRC_OVER_EXT: TODO_ERROR();
	case VK_BLEND_OP_DST_OVER_EXT: TODO_ERROR();
	case VK_BLEND_OP_SRC_IN_EXT: TODO_ERROR();
	case VK_BLEND_OP_DST_IN_EXT: TODO_ERROR();
	case VK_BLEND_OP_SRC_OUT_EXT: TODO_ERROR();
	case VK_BLEND_OP_DST_OUT_EXT: TODO_ERROR();
	case VK_BLEND_OP_SRC_ATOP_EXT: TODO_ERROR();
	case VK_BLEND_OP_DST_ATOP_EXT: TODO_ERROR();
	case VK_BLEND_OP_XOR_EXT: TODO_ERROR();
	case VK_BLEND_OP_MULTIPLY_EXT: TODO_ERROR();
	case VK_BLEND_OP_SCREEN_EXT: TODO_ERROR();
	case VK_BLEND_OP_OVERLAY_EXT: TODO_ERROR();
	case VK_BLEND_OP_DARKEN_EXT: TODO_ERROR();
	case VK_BLEND_OP_LIGHTEN_EXT: TODO_ERROR();
	case VK_BLEND_OP_COLORDODGE_EXT: TODO_ERROR();
	case VK_BLEND_OP_COLORBURN_EXT: TODO_ERROR();
	case VK_BLEND_OP_HARDLIGHT_EXT: TODO_ERROR();
	case VK_BLEND_OP_SOFTLIGHT_EXT: TODO_ERROR();
	case VK_BLEND_OP_DIFFERENCE_EXT: TODO_ERROR();
	case VK_BLEND_OP_EXCLUSION_EXT: TODO_ERROR();
	case VK_BLEND_OP_INVERT_EXT: TODO_ERROR();
	case VK_BLEND_OP_INVERT_RGB_EXT: TODO_ERROR();
	case VK_BLEND_OP_LINEARDODGE_EXT: TODO_ERROR();
	case VK_BLEND_OP_LINEARBURN_EXT: TODO_ERROR();
	case VK_BLEND_OP_VIVIDLIGHT_EXT: TODO_ERROR();
	case VK_BLEND_OP_LINEARLIGHT_EXT: TODO_ERROR();
	case VK_BLEND_OP_PINLIGHT_EXT: TODO_ERROR();
	case VK_BLEND_OP_HARDMIX_EXT: TODO_ERROR();
	case VK_BLEND_OP_HSL_HUE_EXT: TODO_ERROR();
	case VK_BLEND_OP_HSL_SATURATION_EXT: TODO_ERROR();
	case VK_BLEND_OP_HSL_COLOR_EXT: TODO_ERROR();
	case VK_BLEND_OP_HSL_LUMINOSITY_EXT: TODO_ERROR();
	case VK_BLEND_OP_PLUS_EXT: TODO_ERROR();
	case VK_BLEND_OP_PLUS_CLAMPED_EXT: TODO_ERROR();
	case VK_BLEND_OP_PLUS_CLAMPED_ALPHA_EXT: TODO_ERROR();
	case VK_BLEND_OP_PLUS_DARKER_EXT: TODO_ERROR();
	case VK_BLEND_OP_MINUS_EXT: TODO_ERROR();
	case VK_BLEND_OP_MINUS_CLAMPED_EXT: TODO_ERROR();
	case VK_BLEND_OP_CONTRAST_EXT: TODO_ERROR();
	case VK_BLEND_OP_INVERT_OVG_EXT: TODO_ERROR();
	case VK_BLEND_OP_RED_EXT: TODO_ERROR();
	case VK_BLEND_OP_GREEN_EXT: TODO_ERROR();
	case VK_BLEND_OP_BLUE_EXT: TODO_ERROR();
		
	default:
		FATAL_ERROR();
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
			
		case VK_BLEND_OP_ZERO_EXT: TODO_ERROR();
		case VK_BLEND_OP_SRC_EXT: TODO_ERROR();
		case VK_BLEND_OP_DST_EXT: TODO_ERROR();
		case VK_BLEND_OP_SRC_OVER_EXT: TODO_ERROR();
		case VK_BLEND_OP_DST_OVER_EXT: TODO_ERROR();
		case VK_BLEND_OP_SRC_IN_EXT: TODO_ERROR();
		case VK_BLEND_OP_DST_IN_EXT: TODO_ERROR();
		case VK_BLEND_OP_SRC_OUT_EXT: TODO_ERROR();
		case VK_BLEND_OP_DST_OUT_EXT: TODO_ERROR();
		case VK_BLEND_OP_SRC_ATOP_EXT: TODO_ERROR();
		case VK_BLEND_OP_DST_ATOP_EXT: TODO_ERROR();
		case VK_BLEND_OP_XOR_EXT: TODO_ERROR();
		case VK_BLEND_OP_MULTIPLY_EXT: TODO_ERROR();
		case VK_BLEND_OP_SCREEN_EXT: TODO_ERROR();
		case VK_BLEND_OP_OVERLAY_EXT: TODO_ERROR();
		case VK_BLEND_OP_DARKEN_EXT: TODO_ERROR();
		case VK_BLEND_OP_LIGHTEN_EXT: TODO_ERROR();
		case VK_BLEND_OP_COLORDODGE_EXT: TODO_ERROR();
		case VK_BLEND_OP_COLORBURN_EXT: TODO_ERROR();
		case VK_BLEND_OP_HARDLIGHT_EXT: TODO_ERROR();
		case VK_BLEND_OP_SOFTLIGHT_EXT: TODO_ERROR();
		case VK_BLEND_OP_DIFFERENCE_EXT: TODO_ERROR();
		case VK_BLEND_OP_EXCLUSION_EXT: TODO_ERROR();
		case VK_BLEND_OP_INVERT_EXT: TODO_ERROR();
		case VK_BLEND_OP_INVERT_RGB_EXT: TODO_ERROR();
		case VK_BLEND_OP_LINEARDODGE_EXT: TODO_ERROR();
		case VK_BLEND_OP_LINEARBURN_EXT: TODO_ERROR();
		case VK_BLEND_OP_VIVIDLIGHT_EXT: TODO_ERROR();
		case VK_BLEND_OP_LINEARLIGHT_EXT: TODO_ERROR();
		case VK_BLEND_OP_PINLIGHT_EXT: TODO_ERROR();
		case VK_BLEND_OP_HARDMIX_EXT: TODO_ERROR();
		case VK_BLEND_OP_HSL_HUE_EXT: TODO_ERROR();
		case VK_BLEND_OP_HSL_SATURATION_EXT: TODO_ERROR();
		case VK_BLEND_OP_HSL_COLOR_EXT: TODO_ERROR();
		case VK_BLEND_OP_HSL_LUMINOSITY_EXT: TODO_ERROR();
		case VK_BLEND_OP_PLUS_EXT: TODO_ERROR();
		case VK_BLEND_OP_PLUS_CLAMPED_EXT: TODO_ERROR();
		case VK_BLEND_OP_PLUS_CLAMPED_ALPHA_EXT: TODO_ERROR();
		case VK_BLEND_OP_PLUS_DARKER_EXT: TODO_ERROR();
		case VK_BLEND_OP_MINUS_EXT: TODO_ERROR();
		case VK_BLEND_OP_MINUS_CLAMPED_EXT: TODO_ERROR();
		case VK_BLEND_OP_CONTRAST_EXT: TODO_ERROR();
		case VK_BLEND_OP_INVERT_OVG_EXT: TODO_ERROR();
		case VK_BLEND_OP_RED_EXT: TODO_ERROR();
		case VK_BLEND_OP_GREEN_EXT: TODO_ERROR();
		case VK_BLEND_OP_BLUE_EXT: TODO_ERROR();
			
		default:
			FATAL_ERROR();
		}
	}

	return value;
}

// TODO: Merge glslfunctions version, probably move to imagesampler
static void GetFormatOffset(ImageView* imageView, uint64_t& offset, uint64_t& size)
{
	const auto& imageSize = imageView->getImage()->getImageSize();

	offset = imageSize.Level[imageView->getSubresourceRange().baseMipLevel].Offset + imageView->getSubresourceRange().baseArrayLayer * imageSize.LayerSize;
	size = imageSize.Level[imageView->getSubresourceRange().baseMipLevel].LevelSize;
}

template<int length>
glm::vec<length, uint32_t> GetImageRange(ImageView* imageView);

template<>
glm::uvec2 GetImageRange<2>(ImageView* imageView)
{
	const auto& size = imageView->getImage()->getImageSize().Level[imageView->getSubresourceRange().baseMipLevel];
	return glm::uvec2{size.Width, size.Height};
}

template<typename ReturnType, typename CoordinateType>
static ReturnType ImageFetch(DeviceState* deviceState, VkFormat format, ImageView* imageView, CoordinateType coordinates)
{
	uint64_t offset;
	uint64_t size;
	GetFormatOffset(imageView, offset, size);
	auto data = imageView->getImage()->getData(offset, size);
	auto range = GetImageRange<CoordinateType::length()>(imageView);

	return GetPixel<ReturnType>(deviceState,
	                            format,
	                            data,
	                            range,
	                            coordinates,
	                            ReturnType{});
}

static void DrawPixel(DeviceState* deviceState, FragmentBuiltinInput* builtinInput, FragmentBuiltinOutput* builtinOutput, const ShaderFunction* shaderStage, bool front, float depth,
                      std::pair<AttachmentDescription, ImageView*> depthImage, std::pair<AttachmentDescription, ImageView*> stencilImage,
                      std::vector<std::pair<AttachmentDescription, ImageView*>>& images, std::vector<VariableInOutData>& outputData, 
                      uint32_t x, uint32_t y)
{
	builtinInput->fragCoord.z = depth;
	
	const auto viewport = deviceState->pipelineState[PIPELINE_GRAPHICS].pipeline->getDynamicState().DynamicViewport
		                      ? deviceState->dynamicPipelineState.viewports[0]
		                      : deviceState->pipelineState[PIPELINE_GRAPHICS].pipeline->getViewportState().Viewports[0];
	depth = (viewport.maxDepth - viewport.minDepth) * depth + viewport.minDepth;

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

	const auto discard = reinterpret_cast<bool(*)()>(shaderStage->getEntryPoint())();
	if (discard)
	{
		return;
	}

	// TODO: 27.8. Mixed attachment samples
	// TODO: 27.9. Multisample Coverage
	// TODO: 27.10. Depth and Stencil Operations
	 
	// 27.11. Depth Bounds Test
	const auto currentDepth = depthImage.second
		                          ? GetDepthPixel(deviceState, depthImage.first.format, depthImage.second->getImage(), 
		                                          x, y, 0, 
		                                          depthImage.second->getSubresourceRange().baseMipLevel, depthImage.second->getSubresourceRange().baseArrayLayer)
		                          : 0;
	if (deviceState->pipelineState[PIPELINE_GRAPHICS].pipeline->getDepthStencilState().DepthBoundsTestEnable)
	{
		if (deviceState->pipelineState[PIPELINE_GRAPHICS].pipeline->getDynamicState().DynamicDepthBounds)
		{
			if (currentDepth < deviceState->dynamicPipelineState.minDepthBounds ||
				currentDepth > deviceState->dynamicPipelineState.maxDepthBounds)
			{
				return;
			}
		}
		else
		{
			if (currentDepth < deviceState->pipelineState[PIPELINE_GRAPHICS].pipeline->getDepthStencilState().MinDepthBounds ||
				currentDepth > deviceState->pipelineState[PIPELINE_GRAPHICS].pipeline->getDepthStencilState().MaxDepthBounds)
			{
				return;
			}
		}
	}

	// 27.12. Stencil Test
	auto stencilResult = true;
	auto& stencilOpState = front
		                       ? deviceState->pipelineState[PIPELINE_GRAPHICS].pipeline->getDepthStencilState().Front
		                       : deviceState->pipelineState[PIPELINE_GRAPHICS].pipeline->getDepthStencilState().Back;

	uint8_t stencilReference = stencilOpState.reference;
	uint8_t currentStencil = 0;
	if (deviceState->pipelineState[PIPELINE_GRAPHICS].pipeline->getDynamicState().DynamicStencilReference)
	{
		TODO_ERROR();
	}

	if (shaderStage->getFragmentStencilExport())
	{
		stencilReference = builtinOutput->stencilReference;
	}

	if (deviceState->pipelineState[PIPELINE_GRAPHICS].pipeline->getDepthStencilState().StencilTestEnable)
	{
		auto compareMask = stencilOpState.compareMask;
		if (deviceState->pipelineState[PIPELINE_GRAPHICS].pipeline->getDynamicState().DynamicStencilCompareMask)
		{
			TODO_ERROR();
		}

		currentStencil = GetStencilPixel(deviceState, stencilImage.first.format, stencilImage.second->getImage(), 
		                                 x, y, 0, 
		                                 stencilImage.second->getSubresourceRange().baseMipLevel, stencilImage.second->getSubresourceRange().baseArrayLayer); 
		stencilResult = CompareTest(stencilReference & compareMask, currentStencil & compareMask, stencilOpState.compareOp);
	}

	// 27.13. Depth Test
	auto depthResult = true;
	// TODO: Optimise so depth checking uses native type
	if (deviceState->pipelineState[PIPELINE_GRAPHICS].pipeline->getDepthStencilState().DepthTestEnable && depthImage.second)
	{
		if (deviceState->pipelineState[PIPELINE_GRAPHICS].pipeline->getRasterizationState().DepthClampEnable)
		{
			depth = std::clamp(depth, viewport.minDepth, viewport.maxDepth);
		}

		if (depthImage.first.format != VK_FORMAT_D32_SFLOAT && depthImage.first.format != VK_FORMAT_D32_SFLOAT_S8_UINT)
		{
			depth = std::clamp(depth, 0.0f, 1.0f);
		}

		depthResult = CompareTest(depth, currentDepth, deviceState->pipelineState[PIPELINE_GRAPHICS].pipeline->getDepthStencilState().DepthCompareOp);
	}
	else
	{
		depth = std::clamp(depth, 0.0f, 1.0f);
	}
	
	auto depthWrite = depthResult && 
		deviceState->pipelineState[PIPELINE_GRAPHICS].pipeline->getDepthStencilState().DepthTestEnable &&
		deviceState->pipelineState[PIPELINE_GRAPHICS].pipeline->getDepthStencilState().DepthWriteEnable &&
		depthImage.second;

	// Stencil & Depth Write
	if (deviceState->pipelineState[PIPELINE_GRAPHICS].pipeline->getDepthStencilState().StencilTestEnable && stencilImage.second)
	{
		const auto stencilOperation = !stencilResult ? stencilOpState.failOp : (!depthResult ? stencilOpState.depthFailOp : stencilOpState.passOp);
		uint8_t writeValue;
		switch (stencilOperation)
		{
		case VK_STENCIL_OP_KEEP:
			writeValue = currentStencil;
			break;

		case VK_STENCIL_OP_ZERO:
			writeValue = 0;
			break;

		case VK_STENCIL_OP_REPLACE:
			writeValue = stencilReference;
			break;

		case VK_STENCIL_OP_INCREMENT_AND_CLAMP:
			writeValue = currentStencil < 0xFF ? currentStencil + 1 : 0xFF;
			break;

		case VK_STENCIL_OP_DECREMENT_AND_CLAMP:
			writeValue = currentStencil > 0 ? currentStencil - 1 : 0;
			break;

		case VK_STENCIL_OP_INVERT:
			writeValue = ~currentStencil;
			break;

		case VK_STENCIL_OP_INCREMENT_AND_WRAP:
			writeValue = currentStencil + 1;
			break;

		case VK_STENCIL_OP_DECREMENT_AND_WRAP:
			writeValue = currentStencil - 1;
			break;

		default:
			FATAL_ERROR();
		}

		auto writeMask = stencilOpState.writeMask;
		if (deviceState->pipelineState[PIPELINE_GRAPHICS].pipeline->getDynamicState().DynamicStencilWriteMask)
		{
			TODO_ERROR();
		}

		writeValue = (writeValue & writeMask) | (currentStencil & ~writeMask);

		if (depthWrite)
		{
			const VkClearDepthStencilValue input
			{
				depth,
				writeValue,
			};

			// TODO: No clamp if float format?
			SetPixel(deviceState, stencilImage.first.format, stencilImage.second->getImage(),
			         x, y, 0,
			         stencilImage.second->getSubresourceRange().baseMipLevel, stencilImage.second->getSubresourceRange().baseArrayLayer,
			         input);
		}
		else
		{
			const VkClearDepthStencilValue input
			{
				currentDepth,
				writeValue,
			};

			// TODO: No clamp if float format?
			SetPixel(deviceState, stencilImage.first.format, stencilImage.second->getImage(),
			         x, y, 0,
			         stencilImage.second->getSubresourceRange().baseMipLevel, stencilImage.second->getSubresourceRange().baseArrayLayer,
			         input);
		}
	}
	else if (depthWrite)
	{
		const VkClearDepthStencilValue input
		{
			depth,
			stencilImage.second == nullptr
				? 0u
				: GetStencilPixel(deviceState, depthImage.first.format, depthImage.second->getImage(), 
				                  x, y, 0, 
				                  depthImage.second->getSubresourceRange().baseMipLevel, depthImage.second->getSubresourceRange().baseArrayLayer),
		};

		// TODO: No clamp if float format?
		SetPixel(deviceState, depthImage.first.format, depthImage.second->getImage(),
		         x, y, 0,
		         depthImage.second->getSubresourceRange().baseMipLevel, depthImage.second->getSubresourceRange().baseArrayLayer,
		         input);
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

				switch (GetFormatInformation(images[j].first.format).Base)
				{
				case BaseType::UNorm:
				case BaseType::SNorm:
				case BaseType::UScaled:
				case BaseType::SScaled:
				case BaseType::UFloat:
				case BaseType::SFloat:
				case BaseType::SRGB:
					{
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
							TODO_ERROR();
						}

						// 28.3. Color Write Mask
						colour.r = (blend.colorWriteMask & VK_COLOR_COMPONENT_R_BIT) ? colour.r : destination.r;
						colour.g = (blend.colorWriteMask & VK_COLOR_COMPONENT_G_BIT) ? colour.g : destination.g;
						colour.b = (blend.colorWriteMask & VK_COLOR_COMPONENT_B_BIT) ? colour.b : destination.b;
						colour.a = (blend.colorWriteMask & VK_COLOR_COMPONENT_A_BIT) ? colour.a : destination.a;

						SetPixel(deviceState, images[j].first.format, images[j].second->getImage(),
						         x, y, 0,
						         images[j].second->getSubresourceRange().baseMipLevel, images[j].second->getSubresourceRange().baseArrayLayer,
						         *reinterpret_cast<VkClearColorValue*>(&colour));
						break;
					}

				case BaseType::UInt:
					{
						auto colour = *static_cast<glm::uvec4*>(dataPtr);

						// 28.1. Blending
						const auto& blend = deviceState->pipelineState[PIPELINE_GRAPHICS].pipeline->getColourBlendState().Attachments[j];
						const auto destination = ImageFetch<glm::uvec4, glm::ivec2>(deviceState, images[j].first.format, images[j].second, glm::ivec2(x, y));
						if (blend.blendEnable)
						{
							auto constant = *reinterpret_cast<const glm::uvec4*>(deviceState->pipelineState[PIPELINE_GRAPHICS].pipeline->getColourBlendState().BlendConstants);
							colour = ApplyBlend(colour, destination, constant, blend);
						}

						// 28.2. Logical Operations
						if (deviceState->pipelineState[PIPELINE_GRAPHICS].pipeline->getColourBlendState().LogicOpEnable)
						{
							TODO_ERROR();
						}

						// 28.3. Color Write Mask
						colour.r = (blend.colorWriteMask & VK_COLOR_COMPONENT_R_BIT) ? colour.r : destination.r;
						colour.g = (blend.colorWriteMask & VK_COLOR_COMPONENT_G_BIT) ? colour.g : destination.g;
						colour.b = (blend.colorWriteMask & VK_COLOR_COMPONENT_B_BIT) ? colour.b : destination.b;
						colour.a = (blend.colorWriteMask & VK_COLOR_COMPONENT_A_BIT) ? colour.a : destination.a;
						
						SetPixel(deviceState, images[j].first.format, images[j].second->getImage(),
						         x, y, 0,
						         images[j].second->getSubresourceRange().baseMipLevel, images[j].second->getSubresourceRange().baseArrayLayer,
						         *reinterpret_cast<VkClearColorValue*>(&colour));
						break;
					}
					
				case BaseType::SInt:
					{
						auto colour = *static_cast<glm::ivec4*>(dataPtr);

						// 28.1. Blending
						const auto& blend = deviceState->pipelineState[PIPELINE_GRAPHICS].pipeline->getColourBlendState().Attachments[j];
						const auto destination = ImageFetch<glm::ivec4, glm::ivec2>(deviceState, images[j].first.format, images[j].second, glm::ivec2(x, y));
						if (blend.blendEnable)
						{
							auto constant = *reinterpret_cast<const glm::ivec4*>(deviceState->pipelineState[PIPELINE_GRAPHICS].pipeline->getColourBlendState().BlendConstants);
							colour = ApplyBlend(colour, destination, constant, blend);
						}

						// 28.2. Logical Operations
						if (deviceState->pipelineState[PIPELINE_GRAPHICS].pipeline->getColourBlendState().LogicOpEnable)
						{
							TODO_ERROR();
						}

						// 28.3. Color Write Mask
						colour.r = (blend.colorWriteMask & VK_COLOR_COMPONENT_R_BIT) ? colour.r : destination.r;
						colour.g = (blend.colorWriteMask & VK_COLOR_COMPONENT_G_BIT) ? colour.g : destination.g;
						colour.b = (blend.colorWriteMask & VK_COLOR_COMPONENT_B_BIT) ? colour.b : destination.b;
						colour.a = (blend.colorWriteMask & VK_COLOR_COMPONENT_A_BIT) ? colour.a : destination.a;
						
						SetPixel(deviceState, images[j].first.format, images[j].second->getImage(),
						         x, y, 0,
						         images[j].second->getSubresourceRange().baseMipLevel, images[j].second->getSubresourceRange().baseArrayLayer,
						         *reinterpret_cast<VkClearColorValue*>(&colour));
						break;
					}
					
				default:
					FATAL_ERROR();
				}
			}
		}
	}
}

static void ProcessPoints(DeviceState* deviceState, const AssemblerOutput& assemblerOutput, FragmentBuiltinInput* builtinInput, FragmentBuiltinOutput* builtinOutput, const ShaderFunction* shaderStage,
                          std::pair<AttachmentDescription, ImageView*> depthImage, std::pair<AttachmentDescription, ImageView*> stencilImage,
                          std::vector<std::pair<AttachmentDescription, ImageView*>>& images, std::vector<VariableInOutData>& outputData,
                          const VertexOutput& output, const RasterizationState& rasterisationState, std::vector<VariableInOutData>& inputData)
{
	if (output.vertexCount < 1)
	{
		return;
	}

	if (deviceState->pipelineState[PIPELINE_GRAPHICS].pipeline->getViewportState().Viewports.size() != 1)
	{
		TODO_ERROR();
	}

	const auto viewport = deviceState->pipelineState[PIPELINE_GRAPHICS].pipeline->getDynamicState().DynamicViewport
		                      ? deviceState->dynamicPipelineState.viewports[0]
		                      : deviceState->pipelineState[PIPELINE_GRAPHICS].pipeline->getViewportState().Viewports[0];

	for (const auto& primitive : assemblerOutput.primitives)
	{
		auto provokingVertex = primitive.provokingVertex;
		const auto p0Index = primitive.vertex[0];
		
		const auto& builtinData0 = *reinterpret_cast<VertexBuiltinOutput*>(deviceState->vertexOutputStorage.data() + p0Index * output.outputStride);
		
		auto p0 = builtinData0.position / builtinData0.position.w;
		p0.w = builtinData0.position.w;
		const auto pointScreen = glm::ivec2
		{
			static_cast<int32_t>((p0.x + 1) * 0.5f * (viewport.width - 1)),
			static_cast<int32_t>((p0.y + 1) * 0.5f * (viewport.height - 1)),
		};
		const auto pointSize = builtinData0.pointSize;
		const auto intPointSize = static_cast<int32_t>(std::ceil(pointSize / 2));

		const auto startX = std::max(0, pointScreen.x - intPointSize);
		const auto startY = std::max(0, pointScreen.y - intPointSize);
		const auto endX = std::min(static_cast<int32_t>(viewport.width), pointScreen.x + intPointSize + 1);
		const auto endY = std::min(static_cast<int32_t>(viewport.height), pointScreen.y + intPointSize + 1);
		
		for (auto y = startY; y < endY; y++)
		{
			builtinInput->fragCoord.y = y;
		
			for (auto x = startX; x < endX; x++)
			{
				builtinInput->fragCoord.x = shaderStage->getFragmentOriginUpper() ? x : viewport.width - x - 1;

				const auto s = 0.5f + (static_cast<int32_t>(x) - pointScreen.x) / pointSize;
				const auto t = 0.5f + (static_cast<int32_t>(y) - pointScreen.y) / pointSize;
				if (s >= 0 && t >= 0 && s <= 1 && t <= 1)
				{
					for (auto input : inputData)
					{
						const auto data = deviceState->vertexOutputStorage.data() + p0Index * output.outputStride + input.offset;
						memcpy(input.pointer, data, input.size);
					}

					const auto depth = p0.z;
					DrawPixel(deviceState, builtinInput, builtinOutput, shaderStage, true, depth, depthImage, stencilImage, images, outputData, x, y);
				}
			}
		}
	}
}

static void ProcessLines(DeviceState* deviceState, const AssemblerOutput& assemblerOutput, FragmentBuiltinInput* builtinInput, FragmentBuiltinOutput* builtinOutput, const ShaderFunction* shaderStage,
                         std::pair<AttachmentDescription, ImageView*> depthImage, std::pair<AttachmentDescription, ImageView*> stencilImage,
                         std::vector<std::pair<AttachmentDescription, ImageView*>>& images, std::vector<VariableInOutData>& outputData,
                         const VertexOutput& output, const RasterizationState& rasterisationState, std::vector<VariableInOutData>& inputData)
{
	if (output.vertexCount < 2)
	{
		return;
	}

	if (deviceState->pipelineState[PIPELINE_GRAPHICS].pipeline->getViewportState().Viewports.size() != 1)
	{
		TODO_ERROR();
	}

	const auto viewport = deviceState->pipelineState[PIPELINE_GRAPHICS].pipeline->getDynamicState().DynamicViewport
		                      ? deviceState->dynamicPipelineState.viewports[0]
		                      : deviceState->pipelineState[PIPELINE_GRAPHICS].pipeline->getViewportState().Viewports[0];

	const auto halfPixel = glm::vec2(1.0f / viewport.width, 1.0f / viewport.height) * 0.5f;

	for (const auto& primitive : assemblerOutput.primitives)
	{
		auto provokingVertex = primitive.provokingVertex;
		const auto p0Index = primitive.vertex[0];
		const auto p1Index = primitive.vertex[1];
		
		if (rasterisationState.StippledLineEnable)
		{
			TODO_ERROR();
		}

		const auto& builtinData0 = *reinterpret_cast<VertexBuiltinOutput*>(deviceState->vertexOutputStorage.data() + p0Index * output.outputStride);
		const auto& builtinData1 = *reinterpret_cast<VertexBuiltinOutput*>(deviceState->vertexOutputStorage.data() + p1Index * output.outputStride);

		auto p0 = builtinData0.position / builtinData0.position.w;
		auto p1 = builtinData1.position / builtinData1.position.w;
		p0.w = builtinData0.position.w;
		p1.w = builtinData1.position.w;
		const auto lineWidth = rasterisationState.LineWidth / glm::fvec2{viewport.width, viewport.height};

		auto lineDirection = glm::normalize(p1 - p0);
		auto perpendicularLineDirection = glm::fvec2{lineDirection.y, -lineDirection.x};
		auto p00 = glm::xy(p0) + perpendicularLineDirection * lineWidth;
		auto p01 = glm::xy(p0) - perpendicularLineDirection * lineWidth;
		auto p10 = glm::xy(p1) + perpendicularLineDirection * lineWidth;
		auto p11 = glm::xy(p1) - perpendicularLineDirection * lineWidth;
			
		for (auto y = 0u; y < viewport.height; y++)
		{
			const auto yf = (static_cast<float>(y) / viewport.height + halfPixel.y) * 2 - 1;
			builtinInput->fragCoord.y = y;
		
			for (auto x = 0u; x < viewport.width; x++)
			{
				const auto xf = (static_cast<float>(x) / viewport.width + halfPixel.x) * 2 - 1;
				builtinInput->fragCoord.x = shaderStage->getFragmentOriginUpper() ? x : viewport.width - x - 1;
				const auto p = glm::vec2(xf, yf);

				switch (rasterisationState.LineRasterizationMode)
				{
				case VK_LINE_RASTERIZATION_MODE_DEFAULT_EXT:
				case VK_LINE_RASTERIZATION_MODE_RECTANGULAR_EXT:
					{
						// Probably need a better algorithm, too aliased currently
						if (EdgeFunction(p00, p01, p) >= 0 && EdgeFunction(p11, p10, p) >= 0 && EdgeFunction(p10, p00, p) >= 0 && EdgeFunction(p01, p11, p) >= 0)
						{
							auto t = glm::dot(p - glm::xy(p0), glm::xy(p1) - glm::xy(p0)) / (glm::length(glm::xy(p1) - glm::xy(p0)) * glm::length(glm::xy(p1) - glm::xy(p0)));

							for (auto input : inputData)
							{
								float points[]
								{
									p0.w,
									p1.w,
								};

								const void* data[]
								{
									deviceState->vertexOutputStorage.data() + p0Index * output.outputStride + input.offset,
									deviceState->vertexOutputStorage.data() + p1Index * output.outputStride + input.offset,
								};

								float weights[]
								{
									1 - t,
									t,
								};

								switch (input.interpolation)
								{
								case InterpolationType::Perspective:
									SetDatum<true, 2>(input, points, data, weights);
									break;
			
								case InterpolationType::Linear:
									SetDatum<false, 2>(input, points, data, weights);
									break;
			
								case InterpolationType::Flat:
									memcpy(input.pointer, deviceState->vertexOutputStorage.data() + provokingVertex * output.outputStride + input.offset, input.size);
									break;
			
								default:
									FATAL_ERROR();
								}
							}

							const auto depth = p0.z * t + p1.z * (1 - t);
							DrawPixel(deviceState, builtinInput, builtinOutput, shaderStage, true, depth, depthImage, stencilImage, images, outputData, x, y);
						}
						break;
					}

				case VK_LINE_RASTERIZATION_MODE_BRESENHAM_EXT:
					TODO_ERROR();

				case VK_LINE_RASTERIZATION_MODE_RECTANGULAR_SMOOTH_EXT:
					TODO_ERROR();

				default:
					FATAL_ERROR();
				}
			}
		}
	}
}

static void ProcessTriangles(DeviceState* deviceState, const AssemblerOutput& assemblerOutput, FragmentBuiltinInput* builtinInput, FragmentBuiltinOutput* builtinOutput, const ShaderFunction* shaderStage,
                             std::pair<AttachmentDescription, ImageView*> depthImage, std::pair<AttachmentDescription, ImageView*> stencilImage,
                             std::vector<std::pair<AttachmentDescription, ImageView*>>& images, std::vector<VariableInOutData>& outputData, 
                             const VertexOutput& output, const RasterizationState& rasterisationState, std::vector<VariableInOutData>& inputData)
{
	if (deviceState->pipelineState[PIPELINE_GRAPHICS].pipeline->getViewportState().Viewports.size() != 1)
	{
		TODO_ERROR();
	}

	const auto viewport = deviceState->pipelineState[PIPELINE_GRAPHICS].pipeline->getDynamicState().DynamicViewport
		                      ? deviceState->dynamicPipelineState.viewports[0]
		                      : deviceState->pipelineState[PIPELINE_GRAPHICS].pipeline->getViewportState().Viewports[0];

	const auto halfPixel = glm::vec2(1.0f / viewport.width, 1.0f / viewport.height) * 0.5f;

	for (const auto& primitive : assemblerOutput.primitives)
	{
		auto provokingVertex = primitive.provokingVertex;
		auto p0Index = primitive.vertex[0];
		auto p1Index = primitive.vertex[1];
		auto p2Index = primitive.vertex[2];
		if (rasterisationState.FrontFace == VK_FRONT_FACE_CLOCKWISE)
		{
			std::swap(p0Index, p2Index);
		}

		const auto& builtinData0 = *reinterpret_cast<VertexBuiltinOutput*>(deviceState->vertexOutputStorage.data() + p0Index * output.outputStride);
		const auto& builtinData1 = *reinterpret_cast<VertexBuiltinOutput*>(deviceState->vertexOutputStorage.data() + p1Index * output.outputStride);
		const auto& builtinData2 = *reinterpret_cast<VertexBuiltinOutput*>(deviceState->vertexOutputStorage.data() + p2Index * output.outputStride);

		auto p0 = builtinData0.position / builtinData0.position.w;
		auto p1 = builtinData1.position / builtinData1.position.w;
		auto p2 = builtinData2.position / builtinData2.position.w;
		p0.w = builtinData0.position.w;
		p1.w = builtinData1.position.w;
		p2.w = builtinData2.position.w;
		
		const auto p0Screen = glm::ivec2
		{
			static_cast<int32_t>((p0.x + 1) * 0.5f * viewport.width),
			static_cast<int32_t>((p0.y + 1) * 0.5f * viewport.height),
		};
		
		const auto p1Screen = glm::ivec2
		{
			static_cast<int32_t>((p1.x + 1) * 0.5f * viewport.width),
			static_cast<int32_t>((p1.y + 1) * 0.5f * viewport.height),
		};
		
		const auto p2Screen = glm::ivec2
		{
			static_cast<int32_t>((p2.x + 1) * 0.5f * viewport.width),
			static_cast<int32_t>((p2.y + 1) * 0.5f * viewport.height),
		};

		const auto startX = std::max(0, std::min({p0Screen.x, p1Screen.x, p2Screen.x}));
		const auto startY = std::max(0, std::min({p0Screen.y, p1Screen.y, p2Screen.y}));
		const auto endX = std::min(static_cast<int32_t>(viewport.width), std::max({p0Screen.x, p1Screen.x, p2Screen.x}) + 1);
		const auto endY = std::min(static_cast<int32_t>(viewport.height), std::max({p0Screen.y, p1Screen.y, p2Screen.y}) + 1);

		for (auto y = startY; y < endY; y++)
		{
			const auto yf = (static_cast<float>(y) / viewport.height + halfPixel.y) * 2 - 1;
			builtinInput->fragCoord.y = y;

			for (auto x = startX; x < endX; x++)
			{
				const auto xf = (static_cast<float>(x) / viewport.width + halfPixel.x) * 2 - 1;
				builtinInput->fragCoord.x = shaderStage->getFragmentOriginUpper() ? x : viewport.width - x - 1;
				const auto p = glm::vec2(xf, yf);

				float depth;
				bool front;
				if (GetFragmentInput(inputData, deviceState->vertexOutputStorage.data(), output.outputStride,
				                     provokingVertex, p0Index, p1Index, p2Index,
				                     p0, p1, p2, p,
				                     rasterisationState.CullMode, depth, front))
				{
					DrawPixel(deviceState, builtinInput, builtinOutput, shaderStage, front, depth, depthImage, stencilImage, images, outputData, x, y);
				}
			}
		}
	}
}

static void ProcessFragmentShader(DeviceState* deviceState, const AssemblerOutput& assemblerOutput, const VertexOutput& output)
{
	const auto& inputAssembly = deviceState->pipelineState[PIPELINE_GRAPHICS].pipeline->getInputAssemblyState();
	const auto& shaderStage = deviceState->pipelineState[PIPELINE_GRAPHICS].pipeline->getShaderStage(4);
	const auto& rasterisationState = deviceState->pipelineState[PIPELINE_GRAPHICS].pipeline->getRasterizationState();

	if (rasterisationState.RasterizerDiscardEnable)
	{
		TODO_ERROR();
	}

	const auto spirvModule = shaderStage->getSPIRVModule();
	const auto llvmModule = shaderStage->getLLVMModule();

	const auto builtinInputPointer = llvmModule->getPointer("_builtinInput");
	const auto builtinOutputPointer = llvmModule->getPointer("_builtinOutput");
	
	uint32_t inputSize = sizeof(VertexBuiltinOutput);
	auto outputSize = 0u;
	std::vector<VariableInOutData> inputData{};
	std::vector<VariableUniformData> uniformData{};
	std::vector<VariableInOutData> outputData{};
	std::pair<void*, uint32_t> pushConstant{};
	GetVariablePointers(spirvModule, llvmModule, inputData, uniformData, outputData, pushConstant, inputSize, outputSize);
	
	LoadUniforms(deviceState, uniformData, PIPELINE_GRAPHICS);
	
	std::vector<std::pair<AttachmentDescription, ImageView*>> images{MAX_FRAGMENT_OUTPUT_ATTACHMENTS};
	std::pair<AttachmentDescription, ImageView*> depthImage{};
	std::pair<AttachmentDescription, ImageView*> stencilImage{};

	for (auto i = 0u; i < deviceState->currentSubpass->colourAttachments.size(); i++)
	{
		const auto attachmentIndex = deviceState->currentSubpass->colourAttachments[i].attachment;
		if (attachmentIndex != VK_ATTACHMENT_UNUSED)
		{
			const auto& attachment = deviceState->currentRenderPass->getAttachments()[attachmentIndex];
			images[i] = std::make_pair(attachment, deviceState->currentFramebuffer->getAttachments()[attachmentIndex]);
		}
	}
	
	if (deviceState->currentSubpass->depthStencilAttachment.layout != VK_IMAGE_LAYOUT_UNDEFINED)
	{
		const auto attachmentIndex = deviceState->currentSubpass->depthStencilAttachment.attachment;
		if (attachmentIndex != VK_ATTACHMENT_UNUSED)
		{
			const auto& attachment = deviceState->currentRenderPass->getAttachments()[attachmentIndex];
			const auto format = deviceState->currentFramebuffer->getAttachments()[attachmentIndex]->getFormat();
			if (GetFormatInformation(format).DepthStencil.DepthOffset != INVALID_OFFSET)
			{
				depthImage = std::make_pair(attachment, deviceState->currentFramebuffer->getAttachments()[attachmentIndex]);
			}
			if (GetFormatInformation(format).DepthStencil.StencilOffset != INVALID_OFFSET)
			{
				stencilImage = std::make_pair(attachment, deviceState->currentFramebuffer->getAttachments()[attachmentIndex]);
			}
		}
	}
	
	if (pushConstant.first)
	{
		memcpy(pushConstant.first, deviceState->pushConstants, pushConstant.second);
	}
	
	auto builtinInput = static_cast<FragmentBuiltinInput*>(builtinInputPointer);
	auto builtinOutput = static_cast<FragmentBuiltinOutput*>(builtinOutputPointer);
	builtinInput->fragCoord = glm::vec4(0, 0, 0, 1);
	
	if (rasterisationState.PolygonMode != VK_POLYGON_MODE_FILL)
	{
		TODO_ERROR();
	}
	
	switch (assemblerOutput.primitiveType)
	{
	case PrimitiveType::Point:
		ProcessPoints(deviceState, assemblerOutput, builtinInput, builtinOutput, shaderStage, depthImage, stencilImage, images, outputData, output, rasterisationState, inputData);
		break;

	case PrimitiveType::Line:
		ProcessLines(deviceState, assemblerOutput, builtinInput, builtinOutput, shaderStage, depthImage, stencilImage, images, outputData, output, rasterisationState, inputData);
		break;

	case PrimitiveType::Triangle:
		ProcessTriangles(deviceState, assemblerOutput, builtinInput, builtinOutput, shaderStage, depthImage, stencilImage, images, outputData, output, rasterisationState, inputData);
		break;
		
	default:
		FATAL_ERROR();
	}
}

static void ProcessComputeShader(DeviceState* deviceState, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
{
	const auto& shaderStage = deviceState->pipelineState[PIPELINE_COMPUTE].pipeline->getShaderStage(5);
	
	const auto spirvModule = shaderStage->getSPIRVModule();
	const auto llvmModule = shaderStage->getLLVMModule();
	const auto localCount = shaderStage->getComputeLocalSize();
	
	const auto builtinInputPointer = llvmModule->getPointer("_builtinInput");
	
	auto inputSize = 0u;
	auto outputSize = 0u;
	std::vector<VariableInOutData> inputData{};
	std::vector<VariableUniformData> uniformData{};
	std::vector<VariableInOutData> outputData{};
	std::pair<void*, uint32_t> pushConstant{};
	GetVariablePointers(spirvModule, llvmModule, inputData, uniformData, outputData, pushConstant, inputSize, outputSize);
	
	assert(inputData.empty() && outputData.empty());
	
	LoadUniforms(deviceState, uniformData, PIPELINE_COMPUTE);
	
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

	~DrawCommand() override = default;

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
		for (auto i = 0u; i < instanceCount; i++)
		{
			const auto assemblerOutput = ProcessInputAssembler(deviceState, firstVertex, vertexCount);
			const auto vertexOutput = ProcessVertexShader(deviceState, firstInstance + i, assemblerOutput);

			if (deviceState->pipelineState[PIPELINE_GRAPHICS].pipeline->getShaderStage(1) != nullptr)
			{
				TODO_ERROR();
			}

			if (deviceState->pipelineState[PIPELINE_GRAPHICS].pipeline->getShaderStage(2) != nullptr)
			{
				TODO_ERROR();
			}

			if (deviceState->pipelineState[PIPELINE_GRAPHICS].pipeline->getShaderStage(3) != nullptr)
			{
				TODO_ERROR();
			}

			if (deviceState->pipelineState[PIPELINE_GRAPHICS].pipeline->getShaderStage(4) != nullptr)
			{
				ProcessFragmentShader(deviceState, assemblerOutput, vertexOutput);
			}
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

	~DrawIndexedCommand() override = default;

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
		for (auto i = 0u; i < instanceCount; i++)
		{
			const auto assemblerOutput = ProcessInputAssemblerIndexed(deviceState, firstIndex, indexCount, vertexOffset);
			const auto vertexOutput = ProcessVertexShader(deviceState, firstInstance + i, assemblerOutput);

			if (deviceState->pipelineState[PIPELINE_GRAPHICS].pipeline->getShaderStage(1) != nullptr)
			{
				TODO_ERROR();
			}

			if (deviceState->pipelineState[PIPELINE_GRAPHICS].pipeline->getShaderStage(2) != nullptr)
			{
				TODO_ERROR();
			}

			if (deviceState->pipelineState[PIPELINE_GRAPHICS].pipeline->getShaderStage(3) != nullptr)
			{
				TODO_ERROR();
			}

			ProcessFragmentShader(deviceState, assemblerOutput, vertexOutput);
		}
	}

private:
	uint32_t indexCount;
	uint32_t instanceCount;
	uint32_t firstIndex;
	int32_t vertexOffset;
	uint32_t firstInstance;
};

class DrawIndirectCommand final : public Command
{
public:
	DrawIndirectCommand(Buffer* buffer, uint64_t offset, uint32_t drawCount, uint32_t stride) :
		buffer{buffer},
		offset{offset},
		drawCount{drawCount},
		stride{stride}
	{
	}

	~DrawIndirectCommand() override = default;

#if CV_DEBUG_LEVEL > 0
	void DebugOutput(DeviceState* deviceState) override
	{
		*deviceState->debugOutput << "DrawIndirect: drawing" <<
			" from " << buffer <<
			" with offset " << offset <<
			" and stride " << stride << " " <<
			drawCount << " vertices" <<
			std::endl;
	}
#endif

	void Process(DeviceState* deviceState) override
	{
		for (auto j = 0ULL; j < drawCount; j++)
		{
			const auto drawCommand = reinterpret_cast<VkDrawIndirectCommand*>(buffer->getDataPtr(offset + j * stride, sizeof(VkDrawIndirectCommand)));
			for (auto i = 0u; i < drawCommand->instanceCount; i++)
			{
				const auto assemblerOutput = ProcessInputAssembler(deviceState, drawCommand->firstVertex, drawCommand->vertexCount);
				const auto vertexOutput = ProcessVertexShader(deviceState, drawCommand->firstInstance + i, assemblerOutput);

				if (deviceState->pipelineState[PIPELINE_GRAPHICS].pipeline->getShaderStage(1) != nullptr)
				{
					TODO_ERROR();
				}

				if (deviceState->pipelineState[PIPELINE_GRAPHICS].pipeline->getShaderStage(2) != nullptr)
				{
					TODO_ERROR();
				}

				if (deviceState->pipelineState[PIPELINE_GRAPHICS].pipeline->getShaderStage(3) != nullptr)
				{
					TODO_ERROR();
				}

				ProcessFragmentShader(deviceState, assemblerOutput, vertexOutput);
			}
		}
	}

private:
	Buffer* buffer;
	uint64_t offset;
	uint32_t drawCount;
	uint32_t stride;
};

class DrawIndexedIndirectCommand final : public Command
{
public:
	DrawIndexedIndirectCommand(Buffer* buffer, uint64_t offset, uint32_t drawCount, uint32_t stride) :
		buffer{buffer},
		offset{offset},
		drawCount{drawCount},
		stride{stride}
	{
	}

	~DrawIndexedIndirectCommand() override = default;

#if CV_DEBUG_LEVEL > 0
	void DebugOutput(DeviceState* deviceState) override
	{
		*deviceState->debugOutput << "DrawIndexedIndirect: drawing" <<
			" from " << buffer <<
			" with offset " << offset <<
			" and stride " << stride << " " <<
			drawCount << " vertices" <<
			std::endl;
	}
#endif

	void Process(DeviceState* deviceState) override
	{
		for (auto j = 0ULL; j < drawCount; j++)
		{
			const auto drawCommand = reinterpret_cast<VkDrawIndexedIndirectCommand*>(buffer->getDataPtr(offset + j * stride, sizeof(VkDrawIndexedIndirectCommand)));
			for (auto i = 0u; i < drawCommand->instanceCount; i++)
			{
				const auto assemblerOutput = ProcessInputAssemblerIndexed(deviceState, drawCommand->firstIndex, drawCommand->indexCount, drawCommand->vertexOffset);
				const auto vertexOutput = ProcessVertexShader(deviceState, drawCommand->firstInstance + i, assemblerOutput);

				if (deviceState->pipelineState[PIPELINE_GRAPHICS].pipeline->getShaderStage(1) != nullptr)
				{
					TODO_ERROR();
				}

				if (deviceState->pipelineState[PIPELINE_GRAPHICS].pipeline->getShaderStage(2) != nullptr)
				{
					TODO_ERROR();
				}

				if (deviceState->pipelineState[PIPELINE_GRAPHICS].pipeline->getShaderStage(3) != nullptr)
				{
					TODO_ERROR();
				}

				ProcessFragmentShader(deviceState, assemblerOutput, vertexOutput);
			}
		}
	}

private:
	Buffer* buffer;
	uint64_t offset;
	uint32_t drawCount;
	uint32_t stride;
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

	~DispatchCommand() override = default;

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

	~ClearColourImageCommand() override = default;

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
				TODO_ERROR();
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

	~ClearDepthStencilImageCommand() override = default;

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
				TODO_ERROR();
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

	~ClearAttachmentsCommand() override = default;

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
				const auto attachmentReference = deviceState->currentSubpass->depthStencilAttachment.attachment;
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

class DrawIndirectCountCommand final : public Command
{
public:
	DrawIndirectCountCommand(Buffer* buffer, uint64_t offset, Buffer* countBuffer, uint64_t countBufferOffset, uint32_t maxDrawCount, uint32_t stride) :
		buffer{buffer},
		offset{offset},
		countBuffer{countBuffer},
		countBufferOffset{countBufferOffset},
		maxDrawCount{maxDrawCount},
		stride{stride}
	{
	}

	~DrawIndirectCountCommand() override = default;

#if CV_DEBUG_LEVEL > 0
	void DebugOutput(DeviceState* deviceState) override
	{
		*deviceState->debugOutput << "DrawIndirectCount" << std::endl;
	}
#endif

	void Process(DeviceState* deviceState) override
	{
		const auto drawCount = std::min(maxDrawCount, *reinterpret_cast<uint32_t*>(countBuffer->getDataPtr(countBufferOffset, 4)));
		for (auto j = 0ULL; j < drawCount; j++)
		{
			const auto drawCommand = reinterpret_cast<VkDrawIndirectCommand*>(buffer->getDataPtr(offset + j * stride, sizeof(VkDrawIndirectCommand)));
			for (auto i = 0u; i < drawCommand->instanceCount; i++)
			{
				const auto assemblerOutput = ProcessInputAssembler(deviceState, drawCommand->firstVertex, drawCommand->vertexCount);
				const auto vertexOutput = ProcessVertexShader(deviceState, drawCommand->firstInstance + i, assemblerOutput);

				if (deviceState->pipelineState[PIPELINE_GRAPHICS].pipeline->getShaderStage(1) != nullptr)
				{
					TODO_ERROR();
				}

				if (deviceState->pipelineState[PIPELINE_GRAPHICS].pipeline->getShaderStage(2) != nullptr)
				{
					TODO_ERROR();
				}

				if (deviceState->pipelineState[PIPELINE_GRAPHICS].pipeline->getShaderStage(3) != nullptr)
				{
					TODO_ERROR();
				}

				ProcessFragmentShader(deviceState, assemblerOutput, vertexOutput);
			}
		}
	}

private:
	Buffer* buffer;
	uint64_t offset;
	Buffer* countBuffer;
	uint64_t countBufferOffset;
	uint32_t maxDrawCount;
	uint32_t stride;
};

class DrawIndexedIndirectCountCommand final : public Command
{
public:
	DrawIndexedIndirectCountCommand(Buffer* buffer, uint64_t offset, Buffer* countBuffer, uint64_t countBufferOffset, uint32_t maxDrawCount, uint32_t stride) :
		buffer{buffer},
		offset{offset},
		countBuffer{countBuffer},
		countBufferOffset{countBufferOffset},
		maxDrawCount{maxDrawCount},
		stride{stride}
	{
	}

	~DrawIndexedIndirectCountCommand() override = default;

#if CV_DEBUG_LEVEL > 0
	void DebugOutput(DeviceState* deviceState) override
	{
		*deviceState->debugOutput << "DrawIndexedIndirectCount" << std::endl;
	}
#endif

	void Process(DeviceState* deviceState) override
	{
		const auto drawCount = std::min(maxDrawCount, *reinterpret_cast<uint32_t*>(countBuffer->getDataPtr(countBufferOffset, 4)));
		for (auto j = 0ULL; j < drawCount; j++)
		{
			const auto drawCommand = reinterpret_cast<VkDrawIndexedIndirectCommand*>(buffer->getDataPtr(offset + j * stride, sizeof(VkDrawIndexedIndirectCommand)));
			for (auto i = 0u; i < drawCommand->instanceCount; i++)
			{
				const auto assemblerOutput = ProcessInputAssemblerIndexed(deviceState, drawCommand->firstIndex, drawCommand->indexCount, drawCommand->vertexOffset);
				const auto vertexOutput = ProcessVertexShader(deviceState, drawCommand->firstInstance + i, assemblerOutput);

				if (deviceState->pipelineState[PIPELINE_GRAPHICS].pipeline->getShaderStage(1) != nullptr)
				{
					TODO_ERROR();
				}

				if (deviceState->pipelineState[PIPELINE_GRAPHICS].pipeline->getShaderStage(2) != nullptr)
				{
					TODO_ERROR();
				}

				if (deviceState->pipelineState[PIPELINE_GRAPHICS].pipeline->getShaderStage(3) != nullptr)
				{
					TODO_ERROR();
				}

				ProcessFragmentShader(deviceState, assemblerOutput, vertexOutput);
			}
		}
	}

private:
	Buffer* buffer;
	uint64_t offset;
	Buffer* countBuffer;
	uint64_t countBufferOffset;
	uint32_t maxDrawCount;
	uint32_t stride;
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

void CommandBuffer::DrawIndirect(VkBuffer buffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride)
{
	assert(state == State::Recording);
	commands.push_back(std::make_unique<DrawIndirectCommand>(UnwrapVulkan<Buffer>(buffer), offset, drawCount, stride));
}

void CommandBuffer::DrawIndexedIndirect(VkBuffer buffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride)
{
	assert(state == State::Recording);
	commands.push_back(std::make_unique<DrawIndexedIndirectCommand>(UnwrapVulkan<Buffer>(buffer), offset, drawCount, stride));
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

#if defined(VK_KHR_draw_indirect_count) || defined(VK_AMD_draw_indirect_count)
void CommandBuffer::DrawIndirectCount(VkBuffer buffer, VkDeviceSize offset, VkBuffer countBuffer, VkDeviceSize countBufferOffset, uint32_t maxDrawCount, uint32_t stride)
{
	assert(state == State::Recording);
	commands.push_back(std::make_unique<DrawIndirectCountCommand>(UnwrapVulkan<Buffer>(buffer), offset, UnwrapVulkan<Buffer>(countBuffer), countBufferOffset, maxDrawCount, stride));
}

void CommandBuffer::DrawIndexedIndirectCount(VkBuffer buffer, VkDeviceSize offset, VkBuffer countBuffer, VkDeviceSize countBufferOffset, uint32_t maxDrawCount, uint32_t stride)
{
	assert(state == State::Recording);
	commands.push_back(std::make_unique<DrawIndexedIndirectCountCommand>(UnwrapVulkan<Buffer>(buffer), offset, UnwrapVulkan<Buffer>(countBuffer), countBufferOffset, maxDrawCount, stride));
}
#endif