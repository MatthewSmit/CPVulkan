#pragma once
#include "Base.h"
#include "Platform.h"

#include <functional>
#include <vector>

constexpr auto REQUIRED_ALIGNMENT = 32;

struct DeviceMemory
{
	ptrdiff_t Size;
	uint8_t Data[1];

	gsl::span<uint8_t> getSpan()
	{
		return gsl::span<uint8_t>{Data, gsl::span<uint8_t>::index_type{Size}};
	}
};

static DeviceMemory* AllocateSized(const VkAllocationCallbacks* pAllocator, uint64_t size, VkSystemAllocationScope allocationScope)
{
	const auto realSize = sizeof(DeviceMemory) + size;
	
	assert(realSize <= uint64_t{std::numeric_limits<ptrdiff_t>::max()});

	const auto deviceMemory = static_cast<DeviceMemory*>(pAllocator
		                                                     ? pAllocator->pfnAllocation(pAllocator->pUserData, realSize, REQUIRED_ALIGNMENT, allocationScope)
		                                                     : Platform::AlignedMalloc(realSize, REQUIRED_ALIGNMENT));
	
	if (deviceMemory)
	{
		deviceMemory->Size = size;
	}
	
	return deviceMemory;
}

static void FreeSized(DeviceMemory* deviceMemory, const VkAllocationCallbacks* pAllocator)
{
	if (pAllocator)
	{
		pAllocator->pfnFree(pAllocator->pUserData, deviceMemory);
	}
	else
	{
		Platform::AlignedFree(deviceMemory);
	}
}

template<typename T>
static VkResult HandleEnumeration(gsl::not_null<uint32_t*> count, T* outputValues, const std::vector<T>& inputValues)
{
	if (outputValues == nullptr)
	{
		*count = static_cast<uint32_t>(inputValues.size());
	}
	else
	{
		if (*count < inputValues.size())
		{
			TODO_ERROR();
		}
		else if (*count == inputValues.size())
		{
			for (auto i = 0; i < inputValues.size(); i++)
			{
				outputValues[i] = inputValues[i];
			}
		}
		else
		{
			TODO_ERROR();
		}
	}

	return VK_SUCCESS;
}

template<typename T1, typename T2>
static VkResult HandleEnumeration(gsl::not_null<uint32_t*> count, T1* outputValues, const std::vector<T2>& inputValues, std::function<T1(T2)> conversion)
{
	assert(inputValues.size() >= 0 && inputValues.size() < 0xFFFFFFFF);
	const auto inputValueSize = static_cast<uint32_t>(inputValues.size());
	
	if (outputValues == nullptr)
	{
		*count = inputValueSize;
	}
	else
	{
		if (*count < inputValueSize)
		{
			for (auto i = 0u; i < *count; i++)
			{
				outputValues[i] = conversion(inputValues[i]);
			}
			return VK_INCOMPLETE;
		}

		for (auto i = 0u; i < inputValues.size(); i++)
		{
			outputValues[i] = conversion(inputValues[i]);
		}
		*count = inputValueSize;
		return VK_SUCCESS;
	}

	return VK_SUCCESS;
}

template<typename T>
std::vector<T> ArrayToVector(uint32_t count, const T* data)
{
	std::vector<T> result(count);
	if (data)
	{
		memcpy(result.data(), data, sizeof(T) * count);
	}
	return result;
}

template<typename T1, typename T2>
std::vector<T1> ArrayToVector(uint32_t count, const T2* data, std::function<T1(T2)> conversion)
{
	std::vector<T1> result(count);
	if (data)
	{
		for (auto i = 0u; i < count; i++)
		{
			result.at(i) = conversion(data[i]);
		}
	}
	return result;
}

template<typename T>
T* GetStructure(VkBaseOutStructure* structure, VkStructureType type)
{
	while (structure->pNext)
	{
		structure = reinterpret_cast<VkBaseOutStructure*>(structure->pNext);
		if (structure->sType == type)
		{
			return reinterpret_cast<T*>(structure);
		}
	}

	return nullptr;
}