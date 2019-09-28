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

struct VertexBuiltinOutput
{
	glm::vec4 position;
	float a;
	float b[1];
};

struct VertexOutput
{
	std::unique_ptr<VertexBuiltinOutput[]> builtinData;
	std::unique_ptr<uint8_t[]> outputData;
	uint64_t builtinStride;
	uint64_t outputStride;
	uint32_t vertexCount;
};

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
	switch (format.Base)
	{
	case BaseType::UNorm:
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
		break;
		
	case BaseType::SNorm:
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
		break;
		
	case BaseType::UScaled:
		if (format.ElementSize == 0)
		{
			FATAL_ERROR();
		}
		else
		{
			FATAL_ERROR();
		}
		break;
		
	case BaseType::SScaled:
		if (format.ElementSize == 0)
		{
			FATAL_ERROR();
		}
		else
		{
			FATAL_ERROR();
		}
		break;
		
	case BaseType::UInt:
	case BaseType::SInt:
		if (format.ElementSize == 0)
		{
			FATAL_ERROR();
		}
		else
		{
			output[0] = input.uint32[0];
			output[1] = input.uint32[1];
			output[2] = input.uint32[2];
			output[3] = input.uint32[3];
		}
		break;
		
	case BaseType::UFloat:
		if (format.ElementSize == 0)
		{
			FATAL_ERROR();
		}
		else
		{
			FATAL_ERROR();
		}
		break;
		
	case BaseType::SFloat:
		if (format.ElementSize == 0)
		{
			FATAL_ERROR();
		}
		else
		{
			FATAL_ERROR();
		}
		break;
		
	case BaseType::SRGB:
		if (format.ElementSize == 0)
		{
			FATAL_ERROR();
		}
		else
		{
			FATAL_ERROR();
		}
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

static void LoadVertexInput(uint32_t vertex, const VertexInputState& vertexInputState, const Buffer* const vertexBinding[16], const uint64_t vertexBindingOffset[16], const std::vector<std::tuple<void*, int>>& vertexInputs)
{
	for (const auto& attribute : vertexInputState.VertexAttributeDescriptions)
	{
		for (const auto& input : vertexInputs)
		{
			if (std::get<1>(input) == attribute.location)
			{
				const auto& binding = FindBinding(attribute.binding, vertexInputState);
				const auto offset = vertexBindingOffset[binding.binding] + binding.stride * vertex + attribute.offset;
				*static_cast<void**>(std::get<0>(input)) = vertexBinding[binding.binding]->getDataPtr(offset, GetFormatInformation(attribute.format).TotalSize);
				goto end;
			}
		}
		FATAL_ERROR();

	end:
		continue;
	}
}

static void LoadUniforms(DeviceState* deviceState, const SPIRV::SPIRVModule* module, const std::vector<std::tuple<void*, int, int>>& uniformPointers, std::vector<const void*>& pointers)
{
	for (const auto& pointer : uniformPointers)
	{
		const auto descriptorSet = deviceState->descriptorSets[std::get<2>(pointer)][0];
		for (const auto& binding : descriptorSet->getBindings())
		{
			if (std::get<1>(binding) == std::get<1>(pointer))
			{
				switch (std::get<0>(binding))
				{
				case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
					pointers.push_back(&std::get<2>(binding).ImageInfo);
					*static_cast<const void**>(std::get<0>(pointer)) = &pointers[pointers.size() - 1];
					break;

				case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
					FATAL_ERROR();

				case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
					*static_cast<void**>(std::get<0>(pointer)) = UnwrapVulkan<Buffer>(std::get<2>(binding).BufferInfo.buffer)->getDataPtr(std::get<2>(binding).BufferInfo.offset, std::get<2>(binding).BufferInfo.range);
					break;

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
	
	FATAL_ERROR();
}

static uint32_t GetVariableSize(SPIRV::SPIRVType* type)
{
	if (type->isTypeVector())
	{
		return GetVariableSize(type->getVectorComponentType()) * type->getVectorComponentCount();
	}

	if (type->isTypeFloat())
	{
		return type->getFloatBitWidth() / 8;
	}
	
	FATAL_ERROR();
}

static VertexOutput ProcessVertexShader(DeviceState* deviceState, uint32_t vertexCount)
{
	const auto& shaderStage = deviceState->pipeline[0]->getShaderStage(0);
	const auto& vertexInput = deviceState->pipeline[0]->getVertexInputState();

	const auto module = shaderStage->getModule();
	const auto llvmModule = shaderStage->getLLVMModule();

	std::vector<std::tuple<void*, int>> inputPointers{};
	std::vector<std::tuple<void*, int>> outputPointers{};
	std::vector<std::tuple<void*, int, int>> uniformPointers{};
	const auto builtinInputPointer = deviceState->jit->getPointer(llvmModule, "_builtinInput");
	const auto builtinOutputPointer = deviceState->jit->getPointer(llvmModule, "_builtinOutput");

	auto outputSize = 0u;
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
				inputPointers.push_back(std::make_tuple(deviceState->jit->getPointer(llvmModule, "_input_" + variable->getName()), location));
				break;
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
				uniformPointers.push_back(std::make_tuple(deviceState->jit->getPointer(llvmModule, "_uniform_" + variable->getName()), binding, set));
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
				outputPointers.push_back(std::make_tuple(deviceState->jit->getPointer(llvmModule, "_output_" + variable->getName()), location));
				outputSize += GetVariableSize(variable->getType()->getPointerElementType());
				break;
			}

		default:
			FATAL_ERROR();
		}
	}
	
	VertexOutput output
	{
		std::unique_ptr<VertexBuiltinOutput[]>(new VertexBuiltinOutput[vertexCount]),
		std::unique_ptr<uint8_t[]>(new uint8_t[vertexCount * outputSize]),
		sizeof(VertexBuiltinOutput),
		outputSize,
		vertexCount,
	};

	std::vector<const void*> pointers{};
	LoadUniforms(deviceState, module, uniformPointers, pointers);

	struct
	{
		int32_t vertexIndex;
		int32_t instanceIndex;
	} builtinInput{};
	*static_cast<void**>(builtinInputPointer) = &builtinInput;
	
	for (auto i = 0u; i < vertexCount; i++)
	{
		builtinInput.vertexIndex = i;
		*static_cast<void**>(builtinOutputPointer) = &output.builtinData[i];

		for (auto j = 0; j < outputPointers.size(); j++)
		{
			if (j != 0)
			{
				FATAL_ERROR();
			}
			*static_cast<void**>(std::get<0>(outputPointers[j])) = output.outputData.get() + i * output.outputStride;
		}

		LoadVertexInput(i, vertexInput, deviceState->vertexBinding, deviceState->vertexBindingOffset, inputPointers);

		shaderStage->getEntryPoint()();
	}
	
	return output;
}

static auto GetFragmentInput(const std::vector<std::tuple<VkFormat, int>>& vertexOutputs, uint8_t* vertexData, int vertexStride, uint8_t* output,
                             uint32_t p0Index, uint32_t p1Index, uint32_t p2Index,
                             const glm::vec4& p0, const glm::vec4& p1, const glm::vec4& p2, const glm::vec2& p, float& depth) -> bool
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

		for (auto vertexOutput : vertexOutputs)
		{
			const auto data0 = vertexData + p0Index * vertexStride + std::get<1>(vertexOutput);
			const auto data1 = vertexData + p1Index * vertexStride + std::get<1>(vertexOutput);
			const auto data2 = vertexData + p2Index * vertexStride + std::get<1>(vertexOutput);
			switch (std::get<0>(vertexOutput))
			{
			case VK_FORMAT_R32G32_SFLOAT:
				*reinterpret_cast<glm::vec2*>(output + std::get<1>(vertexOutput)) = 
					*reinterpret_cast<glm::vec2*>(data0) * w0 +
					*reinterpret_cast<glm::vec2*>(data1) * w1 +
					*reinterpret_cast<glm::vec2*>(data2) * w2;
				break;
				
			case VK_FORMAT_R32G32B32_SFLOAT:
				*reinterpret_cast<glm::vec3*>(output + std::get<1>(vertexOutput)) = 
					*reinterpret_cast<glm::vec3*>(data0) * w0 +
					*reinterpret_cast<glm::vec3*>(data1) * w1 +
					*reinterpret_cast<glm::vec3*>(data2) * w2;
				break;

			case VK_FORMAT_R32G32B32A32_SFLOAT:
				*reinterpret_cast<glm::vec4*>(output + std::get<1>(vertexOutput)) =
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

	std::vector<std::tuple<void*, int>> inputPointers{};
	std::vector<std::tuple<void*, int>> outputPointers{};
	std::vector<std::tuple<void*, int, int>> uniformPointers{};
	const auto builtinInputPointer = deviceState->jit->getPointer(llvmModule, "_builtinInput");
	const auto builtinOutputPointer = deviceState->jit->getPointer(llvmModule, "_builtinOutput");
	std::vector<std::tuple<VkFormat, int>> vertexOutput{};

	auto outputSize = 0u;
	auto maxLocation = -1;
	auto inputStride = 0u;
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
				inputPointers.push_back(std::make_tuple(deviceState->jit->getPointer(llvmModule, "_input_" + variable->getName()), location));
				vertexOutput.push_back(std::make_tuple(GetVariableFormat(variable->getType()->getPointerElementType()), inputStride));
				inputStride += GetVariableSize(variable->getType()->getPointerElementType());
				break;
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
				uniformPointers.push_back(std::make_tuple(deviceState->jit->getPointer(llvmModule, "_uniform_" + variable->getName()), binding, set));
				break;
			}

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
				uniformPointers.push_back(std::make_tuple(deviceState->jit->getPointer(llvmModule, "_uniformc_" + variable->getName()), binding, set));
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
				outputPointers.push_back(std::make_tuple(deviceState->jit->getPointer(llvmModule, "_output_" + variable->getName()), location));
				outputSize += GetVariableSize(variable->getType()->getPointerElementType());
				break;
			}

		default:
			FATAL_ERROR();
		}
	}

	std::vector<const void*> pointers{};
	LoadUniforms(deviceState, module, uniformPointers, pointers);
	
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

	auto inputData = std::unique_ptr<uint8_t[]>(new uint8_t[output.outputStride]);
	auto outputData = std::unique_ptr<uint8_t[]>(new uint8_t[MAX_FRAGMENT_ATTACHMENTS * 128]); // TODO: Use real stride
	const auto numberTriangles = output.vertexCount / 3;

	for (auto outputPointer : outputPointers)
	{
		*static_cast<void**>(std::get<0>(outputPointer)) = outputData.get() + 128 * std::get<1>(outputPointer);
	}

	for (auto inputPointer : inputPointers)
	{
		*static_cast<void**>(std::get<0>(inputPointer)) = inputData.get() + std::get<1>(vertexOutput[std::get<1>(inputPointer)]);
	}
	
	for (auto i = 0u; i < numberTriangles; i++)
	{
		const auto p0 = output.builtinData[i * 3 + 0].position / output.builtinData[i * 3 + 0].position.w;
		const auto p1 = output.builtinData[i * 3 + 1].position / output.builtinData[i * 3 + 1].position.w;
		const auto p2 = output.builtinData[i * 3 + 2].position / output.builtinData[i * 3 + 2].position.w;
		
		for (auto y = 0u; y < image->getHeight(); y++)
		{
			const auto yf = (static_cast<float>(y) / image->getHeight() + halfPixel.y) * 2 - 1;
			
			for (auto x = 0u; x < image->getWidth(); x++)
			{
				const auto xf = (static_cast<float>(x) / image->getWidth() + halfPixel.x) * 2 - 1;
				const auto p = glm::vec2(xf, yf);
				
				float depth;
				if (GetFragmentInput(vertexOutput, output.outputData.get(), output.outputStride, inputData.get(), i * 3 + 2, i * 3 + 1, i * 3 + 0, p2, p1, p0, p, depth))
				{
					if (depthImage.second)
					{
						const auto currentDepth = GetDepthPixel(depthFormat, depthImage.second, x, y);
						if (currentDepth <= depth)
						{
							continue;
						}
					}

					struct
					{
					} builtinInput;

					struct
					{
					} builtinOutput;

					*static_cast<void**>(builtinInputPointer) = &builtinInput;
					*static_cast<void**>(builtinOutputPointer) = &builtinOutput;

					shaderStage->getEntryPoint()();
					
					uint64_t values[4];
					for (auto j = 0; j < MAX_FRAGMENT_ATTACHMENTS; j++)
					{
						if (images[j].second)
						{
							const auto& format = GetFormatInformation(images[j].first.format);
							GetImageColour(values, format, *reinterpret_cast<VkClearColorValue*>(outputData.get() + 128 * j));
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
						SetPixel(formatInformation, image, x, y, 0, values);
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

void CommandBuffer::ClearColorImage(VkImage image, VkImageLayout imageLayout, const VkClearColorValue* pColor, uint32_t rangeCount, const VkImageSubresourceRange* pRanges)
{
	assert(state == State::Recording);
	commands.push_back(std::make_unique<ClearColourImageCommand>(UnwrapVulkan<Image>(image), imageLayout, *pColor, ArrayToVector(rangeCount, pRanges)));
}

void CommandBuffer::ClearAttachments(uint32_t attachmentCount, const VkClearAttachment* pAttachments, uint32_t rectCount, const VkClearRect* pRects)
{
	assert(state == State::Recording);
	commands.push_back(std::make_unique<ClearAttachmentsCommand>(ArrayToVector(attachmentCount, pAttachments), ArrayToVector(rectCount, pRects)));
}

void ClearImage(Image* image, VkFormat format, VkClearColorValue colour)
{
	const auto& information = GetFormatInformation(format);
	uint64_t values[4];
	GetImageColour(values, information, colour);
	ClearImage(image, information, values);
}

void ClearImage(Image* image, VkFormat format, VkClearDepthStencilValue colour)
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