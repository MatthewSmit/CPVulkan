#pragma once
#define VULKAN_H_ 1
#include <vulkan/vk_platform.h>
#include <vulkan/vulkan_core.h>

#if defined(VK_USE_PLATFORM_WIN32_KHR)
// #include <Windows.h>
#ifndef DECLARE_HANDLE
using HANDLE = void*;

#define DECLARE_HANDLE(name) struct name##__; typedef struct name##__ *name
DECLARE_HANDLE(HINSTANCE);
DECLARE_HANDLE(HMONITOR);
DECLARE_HANDLE(HWND);
#undef DECLARE_HANDLE

struct _SECURITY_ATTRIBUTES;
using SECURITY_ATTRIBUTES = _SECURITY_ATTRIBUTES;

using LPCWSTR = const wchar_t*;
using PCWSTR = const wchar_t*;
using DWORD = unsigned long;
#endif

#include <vulkan/vulkan_win32.h>
#endif

#if defined(VK_USE_PLATFORM_XCB_KHR)
typedef struct xcb_connection_t xcb_connection_t;
typedef uint32_t xcb_window_t;
typedef uint32_t xcb_visualid_t;
#include <vulkan/vulkan_xcb.h>
#endif

#if defined(VK_USE_PLATFORM_XLIB_KHR)
typedef struct _XDisplay Display;
typedef unsigned long XID;
typedef XID Window;
typedef unsigned long VisualID;
#include <vulkan/vulkan_xlib.h>
#endif

#if defined(VK_USE_PLATFORM_XLIB_XRANDR_EXT)
typedef struct _XDisplay Display;
typedef unsigned long XID;
typedef XID Window;
typedef unsigned long VisualID;
typedef XID RROutput;
#include <vulkan/vulkan_xlib_xrandr.h>
#endif

#include <gsl/gsl>

#include <functional>
#include <vector>

#include "Platform.h"

static constexpr auto LATEST_VERSION = VK_API_VERSION_1_1;

#if !defined(_MSC_VER)
#define __debugbreak() __asm__("int3")
#define strcpy_s strcpy // TODO: Functionify
#endif
#define FATAL_ERROR() if (1) { __debugbreak(); abort(); } else (void)0

struct DeviceMemory
{
	uint64_t Size;
	uint8_t Data[1];
};

class VulkanBase
{
public:
	VulkanBase() = default;
	VulkanBase(const VulkanBase&) = delete;
	VulkanBase(VulkanBase&&) = delete;
	virtual ~VulkanBase() = default;

	VulkanBase& operator=(const VulkanBase&) = delete;
	VulkanBase&& operator=(const VulkanBase&&) = delete;

	template <typename T>
	static T* Allocate(const VkAllocationCallbacks* pAllocator, VkSystemAllocationScope allocationScope)
	{
		// if (pAllocator)
		// {
		// 	const auto data = pAllocator->pfnAllocation(pAllocator->pUserData, sizeof(T), 16, allocationScope);
		// 	if (!data)
		// 	{
		// 		return nullptr;
		// 	}
		// 	
		// 	return new(data) T();
		// }
		
		return new T();
	}

	static DeviceMemory* AllocateSized(const VkAllocationCallbacks* pAllocator, uint64_t size, VkSystemAllocationScope allocationScope)
	{
		if (pAllocator)
		{
			return static_cast<DeviceMemory*>(pAllocator->pfnAllocation(pAllocator->pUserData, sizeof(DeviceMemory) + size, 4096, allocationScope));
		}
		
		return static_cast<DeviceMemory*>(Platform::AlignedMalloc(sizeof(DeviceMemory) + size, 4096));
	}

	template <typename T>
	static void Free(T* value, const VkAllocationCallbacks* pAllocator) noexcept
	{
		// if (pAllocator)
		// {
		// 	value->~T();
		// 	pAllocator->pfnFree(pAllocator->pUserData, value);
		// }
		// else
		{
			delete value;
		}
	}

	static void FreeSized(DeviceMemory* deviceMemory, const VkAllocationCallbacks* pAllocator) noexcept
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
};

template <typename T>
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
			FATAL_ERROR();
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
			FATAL_ERROR();
		}
	}

	return VK_SUCCESS;
}

template <typename T1, typename T2>
static VkResult HandleEnumeration(gsl::not_null<uint32_t*> count, T1* outputValues, const std::vector<T2>& inputValues, std::function<T1(T2)> conversion)
{
	if (outputValues == nullptr)
	{
		*count = static_cast<uint32_t>(inputValues.size());
	}
	else
	{
		if (*count < inputValues.size())
		{
			FATAL_ERROR();
		}
		else if (*count == inputValues.size())
		{
			for (auto i = 0; i < inputValues.size(); i++)
			{
				outputValues[i] = conversion(inputValues[i]);
			}
		}
		else
		{
			FATAL_ERROR();
		}
	}

	return VK_SUCCESS;
}

template <typename T>
std::vector<T> ArrayToVector(uint32_t count, const T* data)
{
	std::vector<T> result(count);
	if (data)
	{
		memcpy(result.data(), data, sizeof(T) * count);
	}
	return result;
}

template <typename T1, typename T2>
std::vector<T1> ArrayToVector(uint32_t count, const T2* data, std::function<T1(T2)> conversion)
{
	std::vector<T1> result(count);
	if (data)
	{
		for (auto i = 0u; i < count; i++)
		{
			result[i] = conversion(data[i]);
		}
	}
	return result;
}