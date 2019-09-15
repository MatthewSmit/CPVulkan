cmake_minimum_required(VERSION 3.8)

find_package(glm CONFIG REQUIRED)

add_library(CPVulkan SHARED
	"Base.h"

	"Buffer.cpp"
	"Buffer.h"

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
	)
target_compile_definitions(CPVulkan PRIVATE VK_NO_PROTOTYPES VK_USE_PLATFORM_WIN32_KHR WIN32_LEAN_AND_MEAN)
target_include_directories(CPVulkan PRIVATE "C:\\VulkanSDK\\1.1.121.0\\Include\\")
target_link_libraries(CPVulkan PRIVATE glm SPIRVParser)