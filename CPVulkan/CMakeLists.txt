cmake_minimum_required(VERSION 3.8)

find_package(glm CONFIG REQUIRED)
find_package(Threads REQUIRED)

set(FILES
		"Base.h"

		"Buffer.cpp"
		"Buffer.h"

		"BufferView.cpp"
		"BufferView.h"

		"CommandBuffer.cpp"
		"CommandBuffer.h"

		"CommandPool.cpp"
		"CommandPool.h"

		"CPVulkan.cpp"

		"DescriptorPool.cpp"
		"DescriptorPool.h"

		"DescriptorSet.cpp"
		"DescriptorSet.h"

		"DescriptorSetLayout.cpp"
		"DescriptorSetLayout.h"

		"Device.cpp"
		"Device.h"

		"Event.cpp"
		"Event.h"

		"Extensions.cpp"
		"Extensions.h"

		"Fence.cpp"
		"Fence.h"

		"Formats.cpp"
		"Formats.h"

		"Framebuffer.cpp"
		"Framebuffer.h"

		"Image.cpp"
		"Image.h"

		"ImageView.cpp"
		"ImageView.h"

		"Instance.cpp"
		"Instance.h"

		"PhysicalDevice.cpp"
		"PhysicalDevice.h"

		"Pipeline.cpp"
		"Pipeline.h"

		"PipelineCache.cpp"
		"PipelineCache.h"

		"PipelineLayout.cpp"
		"PipelineLayout.h"

		"Platform.h"

		"QueryPool.cpp"
		"QueryPool.h"

		"Queue.cpp"
		"Queue.h"

		"RenderPass.cpp"
		"RenderPass.h"

		"Sampler.cpp"
		"Sampler.h"

		"Semaphore.cpp"
		"Semaphore.h"

		"ShaderModule.cpp"
		"ShaderModule.h"

		"Swapchain.cpp"
		"Swapchain.h"

		"Trampoline.cpp"
		"Trampoline.h"

		"Util.h"

		"VulkanFunctions.h"
		)

set(DEFINES VK_NO_PROTOTYPES)
set(INCLUDES)
set(LINKS glm SPIRVParser Threads::Threads)

if(WIN32)
	list(APPEND FILES
			"PlatformWin32.cpp"
			"SwapchainWin32.cpp")
	list(APPEND DEFINES VK_USE_PLATFORM_WIN32_KHR WIN32_LEAN_AND_MEAN)
	list(APPEND INCLUDES "C:\\VulkanSDK\\1.1.121.0\\Include\\")
else()
	list(APPEND FILES
			"PlatformLinux.cpp"
			"SwapchainLinux.cpp")
	list(APPEND DEFINES VK_USE_PLATFORM_XCB_KHR VK_USE_PLATFORM_XLIB_KHR VK_USE_PLATFORM_XLIB_XRANDR_EXT)
	list(APPEND INCLUDES "/home/programmdude/1.1.121.0/x86_64/include/")
	list(APPEND LINKS xcb xcb-image X11)
endif()

add_library(CPVulkan SHARED	${FILES})
target_compile_definitions(CPVulkan PRIVATE ${DEFINES})
target_include_directories(CPVulkan PRIVATE ${INCLUDES})
target_link_libraries(CPVulkan PRIVATE ${LINKS})