#pragma once
#define VULKAN_H_ 1
#include <vulkan/vk_platform.h>
#include <vulkan/vulkan_core.h>

#if defined(VK_USE_PLATFORM_WIN32_KHR)
// This is to avoid including Windows.h
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

#include "Config.h"

#include <vulkan/vk_icd.h>

#include <gsl/gsl>

#if !defined(_MSC_VER)
#define __debugbreak() __asm__("int3")

inline void strcpy_s(char* destination, const char* source)
{
	strcpy(destination, source);
}
#endif
#define TODO_ERROR() if (1) { __debugbreak(); abort(); } else (void)0
#define FATAL_ERROR() if (1) { __debugbreak(); abort(); } else (void)0

#if defined(_MSC_VER)
#define CP_DLL_EXPORT __declspec(dllexport)
#else
#define CP_DLL_EXPORT
#endif

#pragma warning(push)
#pragma warning(disable: 26490 26474 26408 26409)

struct DeviceMemory;

class Buffer;
class BufferView;
class CommandBuffer;
class CommandPool;
class ComputePipeline;
class DescriptorPool;
class DescriptorSet;
class DescriptorSetLayout;
class Device;
class Event;
class Fence;
class Framebuffer;
class GraphicsPipeline;
class Image;
class ImageView;
class Instance;
class PhysicalDevice;
class Pipeline;
class PipelineCache;
class PipelineLayout;
class QueryPool;
class Queue;
class RenderPass;
class Sampler;
class Semaphore;
class ShaderModule;

#if defined(VK_NV_ray_tracing)
class RayTracingPipeline;
#endif

template<typename LocalType>
struct VulkanTypeHelper;

#define VK_TYPE_HELPER(VulkanType, LocalType, NonDispatchable) template<> struct VulkanTypeHelper<LocalType> { static constexpr auto IsNonDispatchable = NonDispatchable; using Type = VulkanType; }

VK_TYPE_HELPER(VkInstance, Instance, false);

VK_TYPE_HELPER(VkPhysicalDevice, PhysicalDevice, false);

VK_TYPE_HELPER(VkDevice, Device, false);

VK_TYPE_HELPER(VkQueue, Queue, false);

VK_TYPE_HELPER(VkSemaphore, Semaphore, true);

VK_TYPE_HELPER(VkCommandBuffer, CommandBuffer, false);

VK_TYPE_HELPER(VkFence, Fence, true);

VK_TYPE_HELPER(VkDeviceMemory, DeviceMemory, true);

VK_TYPE_HELPER(VkBuffer, Buffer, true);

VK_TYPE_HELPER(VkImage, Image, true);

VK_TYPE_HELPER(VkEvent, Event, true);

VK_TYPE_HELPER(VkQueryPool, QueryPool, true);

VK_TYPE_HELPER(VkBufferView, BufferView, true);

VK_TYPE_HELPER(VkImageView, ImageView, true);

VK_TYPE_HELPER(VkShaderModule, ShaderModule, true);

VK_TYPE_HELPER(VkPipelineCache, PipelineCache, true);

VK_TYPE_HELPER(VkPipelineLayout, PipelineLayout, true);

VK_TYPE_HELPER(VkRenderPass, RenderPass, true);

VK_TYPE_HELPER(VkPipeline, Pipeline, true);

VK_TYPE_HELPER(VkPipeline, ComputePipeline, true);

VK_TYPE_HELPER(VkPipeline, GraphicsPipeline, true);

#if defined(VK_NV_ray_tracing)
VK_TYPE_HELPER(VkPipeline, RayTracingPipeline, true);
#endif

VK_TYPE_HELPER(VkDescriptorSetLayout, DescriptorSetLayout, true);

VK_TYPE_HELPER(VkSampler, Sampler, true);

VK_TYPE_HELPER(VkDescriptorPool, DescriptorPool, true);

VK_TYPE_HELPER(VkDescriptorSet, DescriptorSet, true);

VK_TYPE_HELPER(VkFramebuffer, Framebuffer, true);

VK_TYPE_HELPER(VkCommandPool, CommandPool, true);

#if defined(VK_VERSION_1_1)
class DescriptorUpdateTemplate;
class SamplerYcbcrConversion;

VK_TYPE_HELPER(VkSamplerYcbcrConversion, SamplerYcbcrConversion, true);

VK_TYPE_HELPER(VkDescriptorUpdateTemplate, DescriptorUpdateTemplate, true);
#endif

#if defined(VK_KHR_surface)
VK_TYPE_HELPER(VkSurfaceKHR, VkIcdSurfaceBase, true);

#if defined(VK_USE_PLATFORM_WIN32_KHR)
VK_TYPE_HELPER(VkSurfaceKHR, VkIcdSurfaceWin32, true);
#endif

#if defined(VK_USE_PLATFORM_XCB_KHR)
VK_TYPE_HELPER(VkSurfaceKHR, VkIcdSurfaceXcb, true);
#endif

#if defined(VK_USE_PLATFORM_XLIB_KHR)
VK_TYPE_HELPER(VkSurfaceKHR, VkIcdSurfaceXlib, true);
#endif
#endif

#if defined(VK_KHR_swapchain)
class Swapchain;
VK_TYPE_HELPER(VkSwapchainKHR, Swapchain, true);
#endif

#if defined(VK_KHR_display)
class VDisplay;
class DisplayMode;

VK_TYPE_HELPER(VkDisplayKHR, VDisplay, true);
VK_TYPE_HELPER(VkDisplayModeKHR, DisplayMode, true);
#endif

#if defined(VK_EXT_debug_report)
class DebugReportCallback;
VK_TYPE_HELPER(VkDebugReportCallbackEXT, DebugReportCallback, true);
#endif

#if defined(VK_NVX_device_generated_commands)
class IndirectCommandsLayout;
class ObjectTable;

VK_TYPE_HELPER(VkObjectTableNVX, ObjectTable, true);

VK_TYPE_HELPER(VkIndirectCommandsLayoutNVX, IndirectCommandsLayout, true);
#endif

#if defined(VK_EXT_debug_utils)
class DebugUtilsMessenger;
VK_TYPE_HELPER(VkDebugUtilsMessengerEXT, DebugUtilsMessenger, true);
#endif

#if defined(VK_EXT_validation_cache)
class ValidationCache;
VK_TYPE_HELPER(VkValidationCacheEXT, ValidationCache, true);
#endif

#if defined(VK_NV_ray_tracing)
class AccelerationStructure;
VK_TYPE_HELPER(VkAccelerationStructureNV, AccelerationStructure, true);
#endif

#if defined(VK_INTEL_performance_query)
class PerformanceConfiguration;
VK_TYPE_HELPER(VkPerformanceConfigurationINTEL, PerformanceConfiguration, true);
#endif

#undef VK_TYPE_HELPER

template<typename T, class... Types>
T* Allocate(const VkAllocationCallbacks* pAllocator, VkSystemAllocationScope allocationScope, Types&& ... args)
{
	constexpr auto size = VulkanTypeHelper<T>::IsNonDispatchable ? sizeof(T) : sizeof(T) + 16;
	const auto data = pAllocator
		                  ? static_cast<uint8_t*>(pAllocator->pfnAllocation(pAllocator->pUserData, size, 16, allocationScope))
		                  : static_cast<uint8_t*>(malloc(size));
	
	if (!data)
	{
		return nullptr;
	}

	if (VulkanTypeHelper<T>::IsNonDispatchable)
	{
		return new(data) T(std::forward<Types>(args)...);
	}

	*static_cast<uintptr_t*>(static_cast<void*>(data)) = ICD_LOADER_MAGIC;
	return new(data + 16) T(std::forward<Types>(args)...);
}

template<typename T>
void Free(T* value, const VkAllocationCallbacks* pAllocator) noexcept
{
	value->OnDelete(pAllocator);
	value->~T();
	const auto data = static_cast<uint8_t*>(static_cast<void*>(value)) - (VulkanTypeHelper<T>::IsNonDispatchable ? 0 : 16);
	
	if (pAllocator)
	{
		pAllocator->pfnFree(pAllocator->pUserData, data);
	}
	else
	{
		free(data);
	}
}

#if defined(VK_USE_PLATFORM_WIN32_KHR)
template<>
inline void Free(VkIcdSurfaceWin32* value, const VkAllocationCallbacks* pAllocator) noexcept
{
	if (pAllocator)
	{
		pAllocator->pfnFree(pAllocator->pUserData, value);
	}
	else
	{
		free(value);
	}
}
#endif

#if defined(VK_USE_PLATFORM_XCB_KHR)
template<>
inline void Free(VkIcdSurfaceXcb* value, const VkAllocationCallbacks* pAllocator) noexcept
{
	if (pAllocator)
	{
		pAllocator->pfnFree(pAllocator->pUserData, value);
	}
	else
	{
		free(value);
	}
}
#endif

#if defined(VK_USE_PLATFORM_XLIB_KHR)
template<>
inline void Free(VkIcdSurfaceXlib* value, const VkAllocationCallbacks* pAllocator) noexcept
{
	if (pAllocator)
	{
		pAllocator->pfnFree(pAllocator->pUserData, value);
	}
	else
	{
		free(value);
	}
}
#endif

template<bool IsNonDispatchable>
void WrapVulkan(void* local, uint64_t* vulkan) noexcept;

template<bool IsNonDispatchable>
void* UnwrapVulkan(uint64_t vulkanValue) noexcept;

template<>
inline void WrapVulkan<false>(void* local, uint64_t* vulkan) noexcept
{
	assert(local);
	assert(vulkan);
	const auto data = static_cast<uint8_t*>(local);
	*vulkan = reinterpret_cast<uint64_t>(data - 16);
}

template<>
inline void* UnwrapVulkan<false>(uint64_t vulkanValue) noexcept
{
	assert(vulkanValue);
	return reinterpret_cast<void*>(vulkanValue + 16);
}

template<>
inline void WrapVulkan<true>(void* local, uint64_t* vulkan) noexcept
{
	assert(local);
	assert(vulkan);
	*vulkan = reinterpret_cast<uint64_t>(local);
}

template<>
inline void* UnwrapVulkan<true>(uint64_t vulkanValue) noexcept
{
	assert(vulkanValue);
	return reinterpret_cast<void*>(vulkanValue);
}

template<typename LocalType>
void WrapVulkan(LocalType* local, typename VulkanTypeHelper<LocalType>::Type* vulkan) noexcept
{
	WrapVulkan<VulkanTypeHelper<LocalType>::IsNonDispatchable>(local, reinterpret_cast<uint64_t*>(vulkan));
}

template<typename LocalType>
LocalType* UnwrapVulkan(typename VulkanTypeHelper<LocalType>::Type vulkanValue) noexcept
{
	return reinterpret_cast<LocalType*>(UnwrapVulkan<VulkanTypeHelper<LocalType>::IsNonDispatchable>(reinterpret_cast<uint64_t>(vulkanValue)));
}
#pragma warning(pop)

constexpr auto PI = 3.14159265358979323846;