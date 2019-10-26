#include "CommandBuffer.h"
#include "CommandBuffer.Internal.h"

#include "Buffer.h"
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

#include <Converter.h>
#include <SPIRVFunction.h>
#include <SPIRVInstruction.h>
#include <SPIRVModule.h>

#include <glm/glm.hpp>

constexpr auto MAX_FRAGMENT_ATTACHMENTS = 4;

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

static void ClearImage(Image* image, const FormatInformation& format, uint32_t layer, uint32_t mipLevel, uint64_t values[4])
{
	auto width = image->getWidth();
	auto height = image->getHeight();
	auto depth = image->getDepth();
	const auto layers = image->getArrayLayers();
	GetFormatMipmapOffset(format, width, height, depth, layers, mipLevel);

	for (auto z = 0u; z < depth; z++)
	{
		for (auto y = 0u; y < height; y++)
		{
			for (auto x = 0u; x < width; x++)
			{
				SetPixel(format, image, x, y, z, mipLevel, layer, values);
			}
		}
	}
}

static void GetImageColour(uint64_t output[4], const FormatInformation& format, const VkClearColorValue& input)
{
	switch (format.Base)
	{
	case BaseType::UNorm:
	case BaseType::SNorm:
	case BaseType::UFloat:
	case BaseType::SFloat:
	case BaseType::SRGB:
		ConvertPixelsToTemp(format, input.float32, output);
		break;

	case BaseType::UInt:
	case BaseType::UScaled:
		ConvertPixelsToTemp(format, input.uint32, output);
		break;
		
	case BaseType::SInt:
	case BaseType::SScaled:
		ConvertPixelsToTemp(format, input.int32, output);
		break;
		
	default:
		FATAL_ERROR();
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

static void LoadVertexInput(uint32_t vertex, const VertexInputState& vertexInputState, const Buffer* const vertexBinding[16], const uint64_t vertexBindingOffset[16], const std::vector<VariableInOutData>& vertexInputs)
{
	for (const auto& attribute : vertexInputState.VertexAttributeDescriptions)
	{
		for (const auto& input : vertexInputs)
		{
			if (input.location == attribute.location)
			{
				const auto& binding = FindBinding(attribute.binding, vertexInputState);
				const auto offset = vertexBindingOffset[binding.binding] + static_cast<uint64_t>(binding.stride) * vertex + attribute.offset;
				memcpy(input.pointer, vertexBinding[binding.binding]->getDataPtr(offset, GetFormatInformation(attribute.format).TotalSize), GetFormatInformation(attribute.format).TotalSize);
				goto end;
			}
		}
		FATAL_ERROR();

	end:
		continue;
	}
}

static void LoadUniforms(DeviceState* deviceState, const SPIRV::SPIRVModule* module, const std::vector<VariableUniformData>& uniformData)
{
	for (const auto& data : uniformData)
	{
		const auto descriptorSet = deviceState->descriptorSets[data.set][0];
		for (const auto& binding : descriptorSet->getBindings())
		{
			if (std::get<1>(binding) == data.binding)
			{
				switch (std::get<0>(binding))
				{
				case VK_DESCRIPTOR_TYPE_SAMPLER:
				case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
				case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
				case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
					*static_cast<const void**>(data.pointer) = &std::get<2>(binding).ImageInfo;
					break;
					
				case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
				case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
					*static_cast<const void**>(data.pointer) = UnwrapVulkan<BufferView>(std::get<2>(binding).TexelBufferView);
					break;

				case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
				case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
				case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
				case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
					*static_cast<const void**>(data.pointer) = UnwrapVulkan<Buffer>(std::get<2>(binding).BufferInfo.buffer)->getDataPtr(std::get<2>(binding).BufferInfo.offset, std::get<2>(binding).BufferInfo.range);
					break;

				case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT: FATAL_ERROR();
				case VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK_EXT: FATAL_ERROR();
				case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV: FATAL_ERROR();

				default:
					FATAL_ERROR();
				}
				goto end;
			}
		}
		FATAL_ERROR();

	end:
		continue;
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
	}

	if (type->isTypeInt(32))
	{
		if (static_cast<SPIRV::SPIRVTypeInt*>(type)->isSigned())
		{
			return VK_FORMAT_R32_SINT;
		}
		else
		{
			return VK_FORMAT_R32_UINT;
		}
	}
	
	FATAL_ERROR();
}

static uint32_t GetVariableSize(SPIRV::SPIRVType* type)
{
	if (type->isTypeStruct())
	{
		// TODO: Alignment & stuff
		auto size = 0u;
		for (auto i = 0; i < type->getStructMemberCount(); i++)
		{
			size += GetVariableSize(type->getStructMemberType(i));
		}
		return size;
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
                                const SpirvCompiledModule* llvmModule, 
                                SpirvJit* jit, 
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
				inputData.push_back(VariableInOutData
					{
						jit->getPointer(llvmModule, MangleName(variable)),
						location,
						GetVariableFormat(variable->getType()->getPointerElementType()),
						inputSize
					});
				inputSize += GetVariableSize(variable->getType()->getPointerElementType());
				break;
			}

		case StorageClassUniform:
		case StorageClassUniformConstant:
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
				maxLocation = std::max(maxLocation, int32_t(location));
				outputData.push_back(VariableInOutData
					{
						jit->getPointer(llvmModule, MangleName(variable)),
						location,
						GetVariableFormat(variable->getType()->getPointerElementType()),
						GetVariableSize(variable->getType()->getPointerElementType()),
						outputSize
					});
				outputSize += GetVariableSize(variable->getType()->getPointerElementType());
				break;
			}

		case StorageClassPushConstant:
			assert(std::get<0>(pushConstant) == nullptr);
			pushConstant = std::make_pair(jit->getPointer(llvmModule, MangleName(variable)), GetVariableSize(variable->getType()->getPointerElementType()));
			// TODO: Offset & stuff?
			break;

		default:
			FATAL_ERROR();
		}
	}
}

static std::vector<uint32_t> ProcessInputAssembler(DeviceState* deviceState, uint32_t vertexCount)
{
	// TODO: Calculate offsets here too
	std::vector<uint32_t> results(vertexCount);
	for (auto i = 0u; i < vertexCount; i++)
	{
		results[i] = i;
	}
	return results;
}

static std::vector<uint32_t> ProcessInputAssemblerIndexed(DeviceState* deviceState, uint32_t indexCount)
{
	// TODO: Calculate offsets here too
	std::vector<uint32_t> results(indexCount);

	if (deviceState->pipeline[0]->getInputAssemblyState().PrimitiveRestartEnable)
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

static VertexOutput ProcessVertexShader(DeviceState* deviceState, const std::vector<uint32_t>& assemblerOutput)
{
	const auto& shaderStage = deviceState->pipeline[0]->getShaderStage(0);
	const auto& vertexInput = deviceState->pipeline[0]->getVertexInputState();

	const auto module = shaderStage->getModule();
	const auto llvmModule = shaderStage->getLLVMModule();

	const auto builtinInputPointer = deviceState->jit->getPointer(llvmModule, "_builtinInput");
	const auto builtinOutputPointer = deviceState->jit->getPointer(llvmModule, "_builtinOutput");

	auto outputSize = 0u;
	std::vector<VariableInOutData> inputData{};
	std::vector<VariableUniformData> uniformData{};
	std::vector<VariableInOutData> outputData{};
	std::pair<void*, uint32_t> pushConstant = {};
	GetVariablePointers(module, llvmModule, deviceState->jit, inputData, uniformData, outputData, pushConstant, outputSize);
	
	VertexOutput output
	{
		std::unique_ptr<VertexBuiltinOutput[]>(new VertexBuiltinOutput[assemblerOutput.size()]),
		std::unique_ptr<uint8_t[]>(new uint8_t[assemblerOutput.size() * outputSize]),
		sizeof(VertexBuiltinOutput),
		outputSize,
		assemblerOutput.size(),
	};

	LoadUniforms(deviceState, module, uniformData);

	const auto builtinInput = static_cast<VertexBuiltinInput*>(builtinInputPointer);
	builtinInput->instanceIndex = 0;

	if (pushConstant.first)
	{
		memcpy(pushConstant.first, deviceState->pushConstants, pushConstant.second);
	}

	for (auto i : assemblerOutput)
	{
		builtinInput->vertexIndex = i;

		LoadVertexInput(i, vertexInput, deviceState->vertexBinding, deviceState->vertexBindingOffset, inputData);

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

	return false;
}

static float GetDepthPixel(const FormatInformation& format, Image* image, uint32_t x, uint32_t y)
{
	// TODO: Optimise so doesn't get all components
	// TODO: Optimise so depth checking uses native type
	float values[4];
	GetPixel(format, image, x, y, 0, values);
	return values[0];
}

static void ProcessFragmentShader(DeviceState* deviceState, const VertexOutput& output)
{
	const auto& inputAssembly = deviceState->pipeline[0]->getInputAssemblyState();
	const auto& shaderStage = deviceState->pipeline[0]->getShaderStage(4);

	if (inputAssembly.Topology != VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
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
	std::pair<void*, uint32_t> pushConstant = {};
	GetVariablePointers(module, llvmModule, deviceState->jit, inputData, uniformData, outputData, pushConstant, outputSize);

	LoadUniforms(deviceState, module, uniformData);
	
	std::vector<std::pair<VkAttachmentDescription, Image*>> images{MAX_FRAGMENT_ATTACHMENTS};
	std::pair<VkAttachmentDescription, Image*> depthImage;
	
	for (auto i = 0u; i < deviceState->renderPass->getSubpasses()[0].ColourAttachments.size(); i++)
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

	const auto numberTriangles = output.vertexCount / 3;

	if (pushConstant.first)
	{
		memcpy(pushConstant.first, deviceState->pushConstants, pushConstant.second);
	}

	auto builtinInput = static_cast<FragmentBuiltinInput*>(builtinInputPointer);
	builtinInput->fragCoord = glm::vec4(0, 0, 0, 1);

	for (auto i = 0u; i < numberTriangles; i++)
	{
		const auto p0 = output.builtinData[i * 3 + 0].position / output.builtinData[i * 3 + 0].position.w;
		const auto p1 = output.builtinData[i * 3 + 1].position / output.builtinData[i * 3 + 1].position.w;
		const auto p2 = output.builtinData[i * 3 + 2].position / output.builtinData[i * 3 + 2].position.w;
		
		for (auto y = 0u; y < image->getHeight(); y++)
		{
			const auto yf = (static_cast<float>(y) / image->getHeight() + halfPixel.y) * 2 - 1;
			builtinInput->fragCoord.y = yf;
			
			for (auto x = 0u; x < image->getWidth(); x++)
			{
				const auto xf = (static_cast<float>(x) / image->getWidth() + halfPixel.x) * 2 - 1;
				builtinInput->fragCoord.x = xf;
				const auto p = glm::vec2(xf, yf);
				
				float depth;
				if (GetFragmentInput(inputData, output.outputData.get(), output.outputStride, i * 3 + 2, i * 3 + 1, i * 3 + 0, p2, p1, p0, p, depth))
				{
					if (depthImage.second)
					{
						const auto currentDepth = GetDepthPixel(depthFormat, depthImage.second, x, y);
						if (currentDepth <= depth)
						{
							continue;
						}
					}
					builtinInput->fragCoord.z = depth;

					shaderStage->getEntryPoint()();
					
					uint64_t values[4];
					for (auto j = 0; j < MAX_FRAGMENT_ATTACHMENTS; j++)
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
							
							const auto& format = GetFormatInformation(images[j].first.format);
							GetImageColour(values, format, *reinterpret_cast<VkClearColorValue*>(dataPtr));
							SetPixel(format, images[j].second, x, y, 0, 0, 0, values);
						}
					}
					
					if (depthImage.second)
					{
						const VkClearColorValue input
						{
							{depth, 0, 0, 0}
						};
						GetImageColour(values, depthFormat, input);
						SetPixel(depthFormat, depthImage.second, x, y, 0, 0, 0, values);
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

		const auto assemblerOutput = ProcessInputAssembler(deviceState, vertexCount);
		const auto vertexOutput = ProcessVertexShader(deviceState, assemblerOutput);

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

	void Process(DeviceState* deviceState) override
	{
		if (instanceCount != 1)
		{
			FATAL_ERROR();
		}

		if (firstIndex != 0)
		{
			FATAL_ERROR();
		}

		if (vertexOffset != 0)
		{
			FATAL_ERROR();
		}

		const auto assemblerOutput = ProcessInputAssemblerIndexed(deviceState, indexCount);
		const auto vertexOutput = ProcessVertexShader(deviceState, assemblerOutput);

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

private:
	uint32_t indexCount;
	uint32_t instanceCount;
	uint32_t firstIndex;
	int32_t vertexOffset;
	uint32_t firstInstance;
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

	void Process(DeviceState*) override
	{
		for (const auto& range : ranges)
		{
			auto levels = range.levelCount;
			if (levels == VK_REMAINING_MIP_LEVELS)
			{
				levels = image->getMipLevels() - range.baseMipLevel;
			}
			if (range.aspectMask == VK_IMAGE_ASPECT_COLOR_BIT)
			{
				ClearImage(image, image->getFormat(), range.baseMipLevel, levels, range.baseArrayLayer, range.layerCount, colour);
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

	void Process(DeviceState*) override
	{
		for (const auto& range : ranges)
		{
			auto levels = range.levelCount;
			if (levels == VK_REMAINING_MIP_LEVELS)
			{
				levels = image->getMipLevels() - range.baseMipLevel;
			}

			if ((range.aspectMask & ~(VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT)) != 0)
			{
				FATAL_ERROR();
			}

			ClearImage(image, image->getFormat(), range.baseMipLevel, levels, range.baseArrayLayer, range.layerCount, range.aspectMask, colour);
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

	void Process(DeviceState* deviceState) override
	{
		for (auto& vkAttachment : attachments)
		{
			// TODO:  If any attachment to be cleared in the current subpass is VK_ATTACHMENT_UNUSED, then the clear has no effect on that attachment.

			Image* image;
			VkFormat format;
			uint64_t values[4];
			if (vkAttachment.aspectMask & VK_IMAGE_ASPECT_COLOR_BIT)
			{
				format = deviceState->renderPass->getAttachments()[vkAttachment.colorAttachment].format;
				image = deviceState->framebuffer->getAttachments()[vkAttachment.colorAttachment]->getImage();
			}
			else
			{
				FATAL_ERROR();
			}

			const auto& formatInformation = GetFormatInformation(format);
			
			if (vkAttachment.aspectMask & VK_IMAGE_ASPECT_COLOR_BIT)
			{
				GetImageColour(values, formatInformation, vkAttachment.clearValue.color);
			}
			else
			{
				FATAL_ERROR();
			}
			
			for (auto& rect : rects)
			{
				if (rect.layerCount != 1)
				{
					FATAL_ERROR();
				}

				if (rect.baseArrayLayer != 0)
				{
					FATAL_ERROR();
				}

				for (auto y = rect.rect.offset.y; y < rect.rect.offset.y + rect.rect.extent.height; y++)
				{
					for (auto x = rect.rect.offset.x; x < rect.rect.offset.x + rect.rect.extent.width; x++)
					{
						SetPixel(formatInformation, image, x, y, 0, 0, 0, values);
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

void ClearImage(Image* image, VkFormat format, uint32_t baseMipLevel, uint32_t levelCount, uint32_t baseArrayLayer, uint32_t layerCount, VkClearColorValue colour)
{
	const auto& information = GetFormatInformation(format);
	uint64_t values[4];
	GetImageColour(values, information, colour);

	for (auto layer = 0u; layer < layerCount; layer++)
	{
		for (auto level = 0u; level < levelCount; level++)
		{
			ClearImage(image, information, baseArrayLayer + layer, baseMipLevel + level, values);
		}
	}
}

void ClearImage(Image* image, VkFormat format, uint32_t baseMipLevel, uint32_t levelCount, uint32_t baseArrayLayer, uint32_t layerCount, VkImageAspectFlags aspects, VkClearDepthStencilValue colour)
{
	const auto& information = GetFormatInformation(format);
	uint64_t values[4];
	switch (format)
	{
	case VK_FORMAT_D16_UNORM:
		assert(aspects == VK_IMAGE_ASPECT_DEPTH_BIT);
		values[0] = static_cast<uint64_t>(colour.depth * std::numeric_limits<uint16_t>::max());
		break;
		
	case VK_FORMAT_X8_D24_UNORM_PACK32:
		assert(aspects == VK_IMAGE_ASPECT_DEPTH_BIT);
		values[0] = static_cast<uint64_t>(colour.depth * 0x00FFFFFF);
		break;
		
	case VK_FORMAT_D32_SFLOAT:
		assert(aspects == VK_IMAGE_ASPECT_DEPTH_BIT);
		values[0] = *reinterpret_cast<uint32_t*>(&colour.depth);
		break;
		
	case VK_FORMAT_S8_UINT:
		assert(aspects == VK_IMAGE_ASPECT_STENCIL_BIT);
		values[0] = colour.stencil;
		break;
		
	case VK_FORMAT_D16_UNORM_S8_UINT:
		assert(aspects == VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT);
		values[0] = static_cast<uint64_t>(colour.depth * std::numeric_limits<uint16_t>::max());
		values[1] = colour.stencil;
		break;
		
	case VK_FORMAT_D24_UNORM_S8_UINT:
		assert(aspects == VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT);
		values[0] = static_cast<uint64_t>(colour.depth * 0x00FFFFFF);
		values[1] = colour.stencil;
		break;
		
	case VK_FORMAT_D32_SFLOAT_S8_UINT:
		assert(aspects == VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT);
		values[0] = *reinterpret_cast<uint32_t*>(&colour.depth);
		values[1] = colour.stencil;
		break;
		
	default:
		FATAL_ERROR();
	}

	for (auto layer = 0u; layer < layerCount; layer++)
	{
		for (auto level = 0u; level < levelCount; level++)
		{
			ClearImage(image, information, baseArrayLayer + layer, baseMipLevel + level, values);
		}
	}
}