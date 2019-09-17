#pragma once
#include "Base.h"

class CommandBuffer;
class Device;
class Instance;
class PhysicalDevice;
class Queue;

#define VULKAN_FUNCTION(method, returnType, clazz, ...) VKAPI_ATTR returnType VKAPI_PTR __##clazz##_##method(void*, ##__VA_ARGS__);
#include "VulkanFunctions.h"
#undef VULKAN_FUNCTION

VKAPI_ATTR void VKAPI_PTR xxx();

#define GET_TRAMPOLINE(clazz, method) (PFN_vkVoidFunction)(__##clazz##_##method)