cmake_minimum_required(VERSION 3.8)

find_package(glm CONFIG REQUIRED)
find_package(Threads REQUIRED)

set(FILES
		"Buffer.cpp"
		"Buffer.h"

		"BufferView.cpp"
		"BufferView.h"

		"CommandBuffer.Binding.cpp"
		"CommandBuffer.Copy.cpp"
		"CommandBuffer.Draw.cpp"
		"CommandBuffer.cpp"
		"CommandBuffer.Query.cpp"
		"CommandBuffer.h"
		"CommandBuffer.Internal.h"

		"CommandPool.cpp"
		"CommandPool.h"

		"CPVulkan.cpp"

		"DebugHelper.cpp"
		"DebugHelper.h"

		"DescriptorPool.cpp"
		"DescriptorPool.h"

		"DescriptorSet.cpp"
		"DescriptorSet.h"

		"DescriptorSetLayout.cpp"
		"DescriptorSetLayout.h"

		"Device.cpp"
		"Device.h"

		"DeviceState.h"

		"Event.cpp"
		"Event.h"

		"Extensions.cpp"
		"Extensions.h"

		"Fence.cpp"
		"Fence.h"

		"Framebuffer.cpp"
		"Framebuffer.h"

		"GlslFunctions.cpp"
		"GlslFunctions.h"

		"Image.cpp"
		"Image.h"

		"ImageSampler.cpp"
		"ImageSampler.h"

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

		"sha3.c"
		"sha3.h"

		"ShaderModule.cpp"
		"ShaderModule.h"

		"Swapchain.cpp"
		"Swapchain.h"

		"Trampoline.cpp"
		"Trampoline.h"

		"Util.h"

		"VulkanFunctions.h"
		)

set(LINKS glm SPIRVParser LLVMRuntime Threads::Threads CPVulkanBase)

if(WIN32)
	list(APPEND FILES
			"PlatformWin32.cpp"
			"SwapchainWin32.cpp")
else()
	list(APPEND FILES
			"PlatformLinux.cpp"
			"SwapchainLinux.cpp"

			"xcb/XcbHelper.cpp"
			"xcb/XcbHelper.h")
	list(APPEND LINKS xcb X11)
endif()

add_library(CPVulkan SHARED	${FILES})
target_link_libraries(CPVulkan PRIVATE ${LINKS})