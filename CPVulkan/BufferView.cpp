#include "BufferView.h"

#include "Device.h"

VkResult BufferView::Create(gsl::not_null<const VkBufferViewCreateInfo*> pCreateInfo, const VkAllocationCallbacks* pAllocator, VkBufferView* pView)
{
	assert(pCreateInfo->sType == VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO);

	auto bufferView = Allocate<BufferView>(pAllocator, VK_SYSTEM_ALLOCATION_SCOPE_DEVICE);

	auto next = pCreateInfo->pNext;
	while (next)
	{
		const auto type = static_cast<const VkBaseInStructure*>(next)->sType;
		switch (type)
		{
		}
		next = static_cast<const VkBaseInStructure*>(next)->pNext;
	}

	if (pCreateInfo->flags)
	{
		FATAL_ERROR();
	}

	bufferView->buffer = UnwrapVulkan<Buffer>(pCreateInfo->buffer);
	bufferView->format = pCreateInfo->format;
	bufferView->offset = pCreateInfo->offset;
	bufferView->range = pCreateInfo->range;

	WrapVulkan(bufferView, pView);
	return VK_SUCCESS;
}

VkResult Device::CreateBufferView(const VkBufferViewCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkBufferView* pView)
{
	return BufferView::Create(pCreateInfo, pAllocator, pView);
}

void Device::DestroyBufferView(VkBufferView bufferView, const VkAllocationCallbacks* pAllocator) noexcept
{
	Free(UnwrapVulkan<BufferView>(bufferView), pAllocator);
}
