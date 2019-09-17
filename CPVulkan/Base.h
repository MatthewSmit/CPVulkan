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
using DWORD = unsigned long; // NOLINT(google-runtime-int)
#endif

// ReSharper disable once CppUnusedIncludeDirective
#include <vulkan/vulkan_win32.h>
#endif

#if defined(VK_USE_PLATFORM_XCB_KHR)
typedef struct xcb_connection_t xcb_connection_t;
typedef uint32_t xcb_window_t;
typedef uint32_t xcb_visualid_t;

// ReSharper disable once CppUnusedIncludeDirective
#include <vulkan/vulkan_xcb.h>
#endif

#if defined(VK_USE_PLATFORM_XLIB_KHR)
typedef struct _XDisplay Display;
typedef unsigned long XID;
typedef XID Window;
typedef unsigned long VisualID;

// ReSharper disable once CppUnusedIncludeDirective
#include <vulkan/vulkan_xlib.h>
#endif

#if defined(VK_USE_PLATFORM_XLIB_XRANDR_EXT)
typedef struct _XDisplay Display;
typedef unsigned long XID;
typedef XID Window;
typedef unsigned long VisualID;
typedef XID RROutput;

// ReSharper disable once CppUnusedIncludeDirective
#include <vulkan/vulkan_xlib_xrandr.h>
#endif

#include <gsl/gsl>

static constexpr auto LATEST_VERSION = VK_API_VERSION_1_1;

#if !defined(_MSC_VER)
#define __debugbreak() __asm__("int3")

inline void strcpy_s(char* destination, const char* source)
{
	strcpy(destination, source);
}
#endif
#define FATAL_ERROR() if (1) { __debugbreak(); abort(); } else (void)0

template<typename T, class... Types>
T* Allocate(const VkAllocationCallbacks* pAllocator, VkSystemAllocationScope allocationScope, Types&& ... args)
{
	uint8_t* data;
	if (pAllocator)
	{
		data = static_cast<uint8_t*>(pAllocator->pfnAllocation(pAllocator->pUserData, sizeof(T), 16, allocationScope));
		if (!data)
		{
			return nullptr;
		}
	}
	else
	{
		data = static_cast<uint8_t*>(malloc(sizeof(T) + 16));
	}
	 
	*static_cast<uintptr_t*>(static_cast<void*>(data)) = 0x01CDC0DE;
	return new(data + 16) T(std::forward<Types>(args)...);
}

template<typename T>
void Free(T* value, const VkAllocationCallbacks* pAllocator) noexcept
{
	value->~T();
	const auto data = static_cast<uint8_t*>(static_cast<void*>(value)) - 16;
	
	if (pAllocator)
	{
		pAllocator->pfnFree(pAllocator->pUserData, data);
	}
	else
	{
		free(data);
	}
}

template<typename LocalType, typename VulkanType>
void WrapVulkan(LocalType* local, VulkanType* vulkan)
{
	const auto data = static_cast<uint8_t*>(static_cast<void*>(local));
	*vulkan = reinterpret_cast<VulkanType>(data - 16);
}

template<typename LocalType, typename VulkanType>
LocalType* UnwrapVulkan(VulkanType vulkanValue)
{
	const auto data = reinterpret_cast<uint8_t*>(vulkanValue);
	return static_cast<LocalType*>(static_cast<void*>(data + 16));
}