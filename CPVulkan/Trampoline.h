#pragma once
#include "Base.h"

class CommandBuffer;
class Device;
class Instance;
class PhysicalDevice;
class Queue;

#define VULKAN_FUNCTION(method, returnType, clazz, ...) VKAPI_ATTR returnType VKAPI_PTR __##clazz##_##method(__VA_ARGS__) noexcept;
// ReSharper disable once CppUnusedIncludeDirective
#include "VulkanFunctions.h"
#undef VULKAN_FUNCTION

#define GET_TRAMPOLINE(clazz, method) (PFN_vkVoidFunction)(__##clazz##_##method)