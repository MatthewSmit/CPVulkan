#include "CommandBuffer.h"
#include "CommandBuffer.Internal.h"

#include "Buffer.h"
#include "DescriptorSet.h"
#include "DeviceState.h"
#include "Formats.h"
#include "Framebuffer.h"
#include "Image.h"
#include "ImageView.h"
#include "Pipeline.h"
#include "RenderPass.h"
#include "Sampler.h"
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
	const auto width = image->getWidth();
	const auto height = image->getHeight();
	const auto depth = image->getDepth();

	assert(x < width);
	assert(y < height);
	assert(z < depth);
	
	switch (format.Base)
	{
	case BaseType::UScaled:
	case BaseType::SScaled:
		FATAL_ERROR();
		
	case BaseType::UFloat:
	case BaseType::SFloat:
		FATAL_ERROR();
		
	case BaseType::SRGB:
		FATAL_ERROR();
	}
	
	if (format.ElementSize == 0 && format.TotalSize == 1)
	{
		SetPackedPixel<uint8_t>(image->getData(), format, x, y, z, width, height, depth, values);
	}
	else if (format.ElementSize == 0 && format.TotalSize == 2)
	{
		SetPackedPixel<uint16_t>(image->getData(), format, x, y, z, width, height, depth, values);
	}
	else if (format.ElementSize == 0 && format.TotalSize == 4)
	{
		SetPackedPixel<uint32_t>(image->getData(), format, x, y, z, width, height, depth, values);
	}
	else if (format.ElementSize == 1)
	{
		SetPixel<uint8_t>(image->getData(), format, x, y, z, width, height, depth, values);
	}
	else if (format.ElementSize == 2)
	{
		SetPixel<uint16_t>(image->getData(), format, x, y, z, width, height, depth, values);
	}
	else if (format.ElementSize == 4)
	{
		SetPixel<uint32_t>(image->getData(), format, x, y, z, width, height, depth, values);
	}
	else
	{
		FATAL_ERROR();
	}
}

template<typename T1, typename T2>
static void Convert(const uint64_t source[4], T2 destination[4]);

template<>
void Convert<uint8_t, float>(const uint64_t source[4], float destination[4])
{
	destination[0] = static_cast<float>(static_cast<uint8_t>(source[0])) / std::numeric_limits<uint8_t>::max();
	destination[1] = static_cast<float>(static_cast<uint8_t>(source[1])) / std::numeric_limits<uint8_t>::max();
	destination[2] = static_cast<float>(static_cast<uint8_t>(source[2])) / std::numeric_limits<uint8_t>::max();
	destination[3] = static_cast<float>(static_cast<uint8_t>(source[3])) / std::numeric_limits<uint8_t>::max();
}

template<>
void Convert<uint16_t, float>(const uint64_t source[4], float destination[4])
{
	destination[0] = static_cast<float>(static_cast<uint16_t>(source[0])) / std::numeric_limits<uint16_t>::max();
	destination[1] = static_cast<float>(static_cast<uint16_t>(source[1])) / std::numeric_limits<uint16_t>::max();
	destination[2] = static_cast<float>(static_cast<uint16_t>(source[2])) / std::numeric_limits<uint16_t>::max();
	destination[3] = static_cast<float>(static_cast<uint16_t>(source[3])) / std::numeric_limits<uint16_t>::max();
}

template<typename T>
static void GetPixel(const FormatInformation& format, Image* image, uint32_t x, uint32_t y, uint32_t z, T values[4])
{
	if (format.Base == BaseType::UNorm)
	{
		if (format.ElementSize == 1)
		{
			uint64_t rawValues[4];
			GetPixel<uint8_t>(image->getData(), format, x, y, z, image->getWidth(), image->getHeight(), image->getDepth(), rawValues);
			Convert<uint8_t, T>(rawValues, values);
		}
		else if (format.ElementSize == 2)
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

static glm::vec4 SampleExplicitLod(ImageView* imageView, Sampler* sampler, const glm::vec2& vec, const float lod)
{
	if (imageView->getViewType() != VK_IMAGE_VIEW_TYPE_2D)
	{
		FATAL_ERROR();
	}

	if (imageView->getComponents().r != VK_COMPONENT_SWIZZLE_R && imageView->getComponents().r != VK_COMPONENT_SWIZZLE_IDENTITY)
	{
		FATAL_ERROR();
	}

	if (imageView->getComponents().g != VK_COMPONENT_SWIZZLE_G && imageView->getComponents().g != VK_COMPONENT_SWIZZLE_IDENTITY)
	{
		FATAL_ERROR();
	}

	if (imageView->getComponents().b != VK_COMPONENT_SWIZZLE_B && imageView->getComponents().b != VK_COMPONENT_SWIZZLE_IDENTITY)
	{
		FATAL_ERROR();
	}

	if (imageView->getComponents().a != VK_COMPONENT_SWIZZLE_A && imageView->getComponents().a != VK_COMPONENT_SWIZZLE_IDENTITY)
	{
		FATAL_ERROR();
	}

	if (imageView->getSubresourceRange().aspectMask != VK_IMAGE_ASPECT_COLOR_BIT)
	{
		FATAL_ERROR();
	}

	if (imageView->getSubresourceRange().baseMipLevel != 0)
	{
		FATAL_ERROR();
	}

	if (imageView->getSubresourceRange().levelCount != 1)
	{
		FATAL_ERROR();
	}

	if (imageView->getSubresourceRange().baseArrayLayer != 0)
	{
		FATAL_ERROR();
	}

	if (imageView->getSubresourceRange().layerCount != 1)
	{
		FATAL_ERROR();
	}

	if (sampler->getFlags())
	{
		FATAL_ERROR();
	}

	if (sampler->getMagFilter() != VK_FILTER_NEAREST)
	{
		FATAL_ERROR();
	}

	if (sampler->getMinFilter() != VK_FILTER_NEAREST)
	{
		FATAL_ERROR();
	}

	if (sampler->getMipmapMode() != VK_SAMPLER_MIPMAP_MODE_NEAREST)
	{
		FATAL_ERROR();
	}

	if (sampler->getAnisotropyEnable())
	{
		FATAL_ERROR();
	}

	if (sampler->getCompareEnable())
	{
		FATAL_ERROR();
	}

	if (sampler->getUnnormalisedCoordinates())
	{
		FATAL_ERROR();
	}

	if (vec.x < 0 || vec.x > 1 || vec.y < 0 || vec.y > 1)
	{
		FATAL_ERROR();
	}

	const auto& formatInformation = GetFormatInformation(imageView->getFormat());

	const auto x = static_cast<uint32_t>(vec.x * imageView->getImage()->getWidth() + 0.5);
	const auto y = static_cast<uint32_t>(vec.y * imageView->getImage()->getHeight() + 0.5);

	float values[4];
	GetPixel(formatInformation, imageView->getImage(), x, y, 0, values);
	return glm::vec4(values[0], values[1], values[2], values[3]);
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

static void LoadUniforms(DeviceState* deviceState, const SPIRV::SPIRVModule* module, const std::vector<std::tuple<void*, int, int>>& uniformPointers)
{
	for (const auto& pointer : uniformPointers)
	{
		const auto descriptorSet = deviceState->descriptorSets[std::get<1>(pointer)][0];
		for (const auto& binding : descriptorSet->getBindings())
		{
			if (std::get<1>(binding) == std::get<2>(pointer))
			{
				switch (std::get<0>(binding))
				{
				case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
					FATAL_ERROR();

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
			if (type->getVectorComponentCount() == 4)
			{
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

	LoadUniforms(deviceState, module, uniformPointers);
		 
	for (auto i = 0u; i < vertexCount; i++)
	{
		struct
		{
		} builtinInput;

		*static_cast<void**>(builtinInputPointer) = &builtinInput;
		*static_cast<void**>(builtinOutputPointer) = &output.builtinData[i];

		assert(outputPointers.size() == 1);
		*static_cast<void**>(std::get<0>(outputPointers[0])) = output.outputData.get() + i * output.outputStride;

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
			case VK_FORMAT_R32G32B32A32_SFLOAT:
				*reinterpret_cast<glm::vec4*>(output + std::get<1>(vertexOutput)) = *reinterpret_cast<glm::vec4*>(data0) * w0 + 
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

	LoadUniforms(deviceState, module, uniformPointers);
	
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
	
	// std::vector<Variable> outputStorage{MAX_FRAGMENT_ATTACHMENTS};
	// std::vector<Variable> outputs{MAX_FRAGMENT_ATTACHMENTS};
	// for (auto i = 0u; i < MAX_FRAGMENT_ATTACHMENTS; i++)
	// {
	// 	outputs[i] = Variable(reinterpret_cast<uint8_t*>(&outputStorage[i].f32._14));
	// }
	// std::vector<Variable> builtins{};
	// std::vector<SPIRV::SPIRVType*> inputTypes(1);
	// std::vector<std::vector<Variable>> vertexOutputs(output.vertexCount);
	//
	// for (auto i = 0u; i < module->getNumVariables(); i++)
	// {
	// 	const auto variable = module->getVariable(i);
	// 	if (variable->getStorageClass() == StorageClassInput)
	// 	{
	// 		const auto locationDecorate = variable->getDecorate(DecorationLocation);
	// 		if (locationDecorate.empty())
	// 		{
	// 			continue;
	// 		}
	//
	// 		const auto location = *locationDecorate.begin();
	// 		const auto type = variable->getType()->getPointerElementType();
	// 		inputTypes[location] = type;
	//
	// 		for (auto j = 0u; j < output.vertexCount; j++)
	// 		{
	// 			vertexOutputs[j].resize(1);
	// 			vertexOutputs[j][location] = Variable(output.outputData.get() + j * output.outputStride);
	// 		}
	// 	}
	// }
	//
	// ShaderState state
	// {
	// 	std::vector<Variable>(128),
	// 	std::vector<Variable>(inputTypes.size()),
	// 	LoadUniforms(deviceState, module),
	// 	&outputs,
	// 	&builtins
	// };
	//
	// std::vector<Variable> storage(32);

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