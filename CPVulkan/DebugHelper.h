#pragma once
#include "Base.h"

#if CV_DEBUG_LEVEL >= CV_DEBUG_LOG
std::ostream& operator<<(std::ostream& stream, VkIndexType value);
std::ostream& operator<<(std::ostream& stream, VkPipelineBindPoint value);
std::ostream& operator<<(std::ostream& stream, const VkBufferCopy& value);
std::ostream& operator<<(std::ostream& stream, const VkBufferImageCopy& value);
std::ostream& operator<<(std::ostream& stream, const VkClearAttachment& value);
std::ostream& operator<<(std::ostream& stream, const VkClearDepthStencilValue& value);
std::ostream& operator<<(std::ostream& stream, const VkClearRect& value);
std::ostream& operator<<(std::ostream& stream, const VkClearValue& value);
std::ostream& operator<<(std::ostream& stream, const VkExtent3D& value);
std::ostream& operator<<(std::ostream& stream, const VkImageBlit& value);
std::ostream& operator<<(std::ostream& stream, const VkImageCopy& value);
std::ostream& operator<<(std::ostream& stream, const VkImageSubresourceLayers& value);
std::ostream& operator<<(std::ostream& stream, const VkImageSubresourceRange& value);
std::ostream& operator<<(std::ostream& stream, const VkOffset3D& value);
std::ostream& operator<<(std::ostream& stream, const VkRect2D& value);
std::ostream& operator<<(std::ostream& stream, const VkViewport& value);
std::ostream& operator<<(std::ostream& stream, Buffer* value);
std::ostream& operator<<(std::ostream& stream, DescriptorSet* value);
std::ostream& operator<<(std::ostream& stream, Pipeline* value);
std::ostream& operator<<(std::ostream& stream, PipelineLayout* value);

std::string DebugPrint(Image* image, VkClearColorValue value);

template<typename T>
std::ostream& operator<<(std::ostream& stream, const std::vector<T>& vector)
{
	stream << "{";
	auto first = true;
	for (const auto& value : vector)
	{
		if (first)
		{
			first = false;
		}
		else
		{
			stream << ", ";
		}
		stream << value;
	}
	return stream << "}";
}
#endif

#if CV_DEBUG_LEVEL >= CV_DEBUG_IMAGE
void DumpImage(const std::string& fileName, Image* image, ImageView* imageView);
#endif