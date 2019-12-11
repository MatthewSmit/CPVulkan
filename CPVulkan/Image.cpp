#include "Image.h"

#include "Device.h"
#include "Formats.h"
#include "Swapchain.h"
#include "Util.h"

#include <cassert>

VkResult Image::BindMemory(VkDeviceMemory memory, uint64_t memoryOffset)
{
	const auto memorySpan = UnwrapVulkan<DeviceMemory>(memory)->getSpan();
	data = memorySpan.subspan(memoryOffset, imageSize.TotalSize);
	return VK_SUCCESS;
}

VkResult Image::BindSwapchainMemory(Swapchain* swapchain, uint32_t imageIndex)
{
	data = swapchain->getImage(imageIndex)->data;
	return VK_SUCCESS;
}

VkResult Device::BindImageMemory(VkImage image, VkDeviceMemory memory, VkDeviceSize memoryOffset)
{
	return UnwrapVulkan<Image>(image)->BindMemory(memory, memoryOffset);
}

void Image::GetMemoryRequirements(VkMemoryRequirements* pMemoryRequirements) const
{
	pMemoryRequirements->size = imageSize.TotalSize;
	pMemoryRequirements->alignment = 16;
	pMemoryRequirements->memoryTypeBits = 1;
}

void Device::GetImageMemoryRequirements(VkImage image, VkMemoryRequirements* pMemoryRequirements)
{
	UnwrapVulkan<Image>(image)->GetMemoryRequirements(pMemoryRequirements);
}

void Device::GetImageSparseMemoryRequirements(VkImage image, uint32_t* pSparseMemoryRequirementCount, VkSparseImageMemoryRequirements* pSparseMemoryRequirements)
{
	TODO_ERROR();
}

void Image::GetSubresourceLayout(const VkImageSubresource* pSubresource, VkSubresourceLayout* pLayout)
{
	// TODO: Use InformationFormat
	if (tiling == VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT)
	{
		TODO_ERROR();
	}
	else if (tiling != VK_IMAGE_TILING_LINEAR)
	{
		TODO_ERROR();
	}

	auto const& formatInformation = GetFormatInformation(format);
	if (formatInformation.Type != FormatType::Normal)
	{
		TODO_ERROR();
	}

	if (pSubresource->aspectMask != VK_IMAGE_ASPECT_COLOR_BIT)
	{
		TODO_ERROR();
	}

	if (pSubresource->arrayLayer != 0)
	{
		TODO_ERROR();
	}

	if (pSubresource->mipLevel != 0)
	{
		TODO_ERROR();
	}

	pLayout->offset = 0;
	pLayout->rowPitch = getWidth() * VkDeviceSize{formatInformation.TotalSize};
	pLayout->depthPitch = getHeight() * pLayout->rowPitch;
	pLayout->arrayPitch = getDepth() * pLayout->depthPitch;
	pLayout->size = arrayLayers * pLayout->arrayPitch;
}

void Device::GetImageSubresourceLayout(VkImage image, const VkImageSubresource* pSubresource, VkSubresourceLayout* pLayout)
{
	UnwrapVulkan<Image>(image)->GetSubresourceLayout(pSubresource, pLayout);
}

VkResult Image::Create(const VkImageCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkImage* pImage)
{
	assert(pCreateInfo->sType == VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO);

	auto image = Allocate<Image>(pAllocator, VK_SYSTEM_ALLOCATION_SCOPE_DEVICE);
	if (!image)
	{
		return VK_ERROR_OUT_OF_HOST_MEMORY;
	}

	auto next = static_cast<const VkBaseInStructure*>(pCreateInfo->pNext);
	while (next)
	{
		const auto type = next->sType;
		switch (type)
		{
		case VK_STRUCTURE_TYPE_DEDICATED_ALLOCATION_IMAGE_CREATE_INFO_NV:
			TODO_ERROR();

		case VK_STRUCTURE_TYPE_EXTERNAL_FORMAT_ANDROID:
			TODO_ERROR();

		case VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO:
			TODO_ERROR();

		case VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO_NV:
			TODO_ERROR();

		case VK_STRUCTURE_TYPE_IMAGE_DRM_FORMAT_MODIFIER_EXPLICIT_CREATE_INFO_EXT:
			TODO_ERROR();

		case VK_STRUCTURE_TYPE_IMAGE_DRM_FORMAT_MODIFIER_LIST_CREATE_INFO_EXT:
			TODO_ERROR();

		case VK_STRUCTURE_TYPE_IMAGE_FORMAT_LIST_CREATE_INFO_KHR:
			TODO_ERROR();

		case VK_STRUCTURE_TYPE_IMAGE_STENCIL_USAGE_CREATE_INFO_EXT:
			TODO_ERROR();

		case VK_STRUCTURE_TYPE_IMAGE_SWAPCHAIN_CREATE_INFO_KHR:
			break;
			
		default:
			break;
		}
		next = next->pNext;
	}

	image->flags = pCreateInfo->flags;
	image->imageType = pCreateInfo->imageType;
	image->format = pCreateInfo->format;
	image->extent = pCreateInfo->extent;
	image->mipLevels = pCreateInfo->mipLevels;
	image->arrayLayers = pCreateInfo->arrayLayers;
	image->samples = pCreateInfo->samples;
	image->tiling = pCreateInfo->tiling;
	image->usage = pCreateInfo->usage;
	image->layout = pCreateInfo->initialLayout;

	image->imageSize = GetImageSize(GetFormatInformation(image->format), image->extent.width, image->extent.height, image->extent.depth, image->arrayLayers, image->mipLevels);
	if (image->imageSize.TotalSize == 0)
	{
		FATAL_ERROR();
	}

	WrapVulkan(image, pImage);
	return VK_SUCCESS;
}

VkResult Device::CreateImage(const VkImageCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkImage* pImage)
{
	return Image::Create(pCreateInfo, pAllocator, pImage);
}

void Device::DestroyImage(VkImage image, const VkAllocationCallbacks* pAllocator)
{
	if (image)
	{
		Free(UnwrapVulkan<Image>(image), pAllocator);
	}
}

VkResult Device::BindImageMemory2(uint32_t bindInfoCount, const VkBindImageMemoryInfo* pBindInfos)
{
	for (auto i = 0u; i < bindInfoCount; i++)
	{
		assert(pBindInfos[i].sType == VK_STRUCTURE_TYPE_BIND_IMAGE_MEMORY_INFO);
		
		VkSwapchainKHR swapchain = VK_NULL_HANDLE;
		uint32_t imageIndex = 0;

		auto next = static_cast<const VkBaseInStructure*>(pBindInfos[i].pNext);
		while (next)
		{
			const auto type = next->sType;
			switch (type)
			{
			case VK_STRUCTURE_TYPE_BIND_IMAGE_MEMORY_DEVICE_GROUP_INFO:
				TODO_ERROR();
				
			case VK_STRUCTURE_TYPE_BIND_IMAGE_MEMORY_SWAPCHAIN_INFO_KHR:
				{
					const auto bindInfo = reinterpret_cast<const VkBindImageMemorySwapchainInfoKHR*>(next);
					swapchain = bindInfo->swapchain;
					imageIndex = bindInfo->imageIndex;
					break;
				}
				
			case VK_STRUCTURE_TYPE_BIND_IMAGE_PLANE_MEMORY_INFO:
				TODO_ERROR();
				
			default:
				break;
			}
			next = next->pNext;
		}

		VkResult result;
		
		if (swapchain == VK_NULL_HANDLE)
		{
			result = UnwrapVulkan<Image>(pBindInfos[i].image)->BindMemory(pBindInfos[i].memory, pBindInfos[i].memoryOffset);
		}
		else
		{
			result = UnwrapVulkan<Image>(pBindInfos[i].image)->BindSwapchainMemory(UnwrapVulkan<Swapchain>(swapchain), imageIndex);
		}
		
		if (result != VK_SUCCESS)
		{
			return result;
		}
	}

	return VK_SUCCESS;
}

void Device::GetImageMemoryRequirements2(const VkImageMemoryRequirementsInfo2* pInfo, VkMemoryRequirements2* pMemoryRequirements)
{
	assert(pInfo->sType == VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2);
	assert(pMemoryRequirements->sType == VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2);

	{
		auto next = reinterpret_cast<const VkBaseInStructure*>(pInfo->pNext);
		while (next)
		{
			const auto type = next->sType;
			switch (type)
			{
			case VK_STRUCTURE_TYPE_IMAGE_PLANE_MEMORY_REQUIREMENTS_INFO:
				TODO_ERROR();

			default:
				break;
			}
			next = next->pNext;
		}
	}

	{
		auto next = static_cast<VkBaseOutStructure*>(pMemoryRequirements->pNext);
		while (next)
		{
			const auto type = next->sType;
			switch (type)
			{
			case VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS:
				{
					const auto memoryRequirements = reinterpret_cast<VkMemoryDedicatedRequirements*>(next);
					memoryRequirements->prefersDedicatedAllocation = false;
					memoryRequirements->requiresDedicatedAllocation = false;
					break;
				}

			default:
				break;
			}
			next = next->pNext;
		}
	}

	UnwrapVulkan<Image>(pInfo->image)->GetMemoryRequirements(&pMemoryRequirements->memoryRequirements);
}

void Device::GetImageSparseMemoryRequirements2(const VkImageSparseMemoryRequirementsInfo2* pInfo, uint32_t* pSparseMemoryRequirementCount, VkSparseImageMemoryRequirements2* pSparseMemoryRequirements)
{
	FATAL_ERROR();
}
