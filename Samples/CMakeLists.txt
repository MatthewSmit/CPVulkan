cmake_minimum_required(VERSION 3.8)

find_package(glm CONFIG REQUIRED)
find_package(glslang CONFIG REQUIRED)
find_package(Vulkan REQUIRED)
find_package(Threads REQUIRED)

set(DEFINES)
set(LINKS CPVulkan glm glslang::HLSL glslang::SPIRV glslang::glslang glslang::OGLCompiler Vulkan::Vulkan)

if(WIN32)
	list(APPEND DEFINES VK_USE_PLATFORM_WIN32_KHR WIN32_LEAN_AND_MEAN)
else()
	list(APPEND DEFINES VK_USE_PLATFORM_XCB_KHR)
	list(APPEND LINKS xcb)
endif()

add_executable(CPVulkanSamples
	#"01-init_instance/01-init_instance.cpp"
	#"02-enumerate_devices/02-enumerate_devices.cpp"
	#"03-init_device/03-init_device.cpp"
	#"04-init_command_buffer/04-init_command_buffer.cpp"
	#"05-init_swapchain/05-init_swapchain.cpp"
	#"06-init_depth_buffer/06-init_depth_buffer.cpp"
	#"07-init_uniform_buffer/07-init_uniform_buffer.cpp"
	#"08-init_pipeline_layout/08-init_pipeline_layout.cpp"
	#"09-init_descriptor_set/09-init_descriptor_set.cpp"
	#"10-init_render_pass/10-init_render_pass.cpp"
	#"11-init_shaders/11-init_shaders.cpp"
	#"12-init_frame_buffers/12-init_frame_buffers.cpp"
	#"13-init_vertex_buffer/13-init_vertex_buffer.cpp"
	#"14-init_pipeline/14-init_pipeline.cpp"
	"15-draw_cube/15-draw_cube.cpp"
	#"16-vulkan_1_1/16-vulkan_1_1.cpp"
	#"copy_blit_image/copy_blit_image.cpp"
	#"create_debug_report_callback/create_debug_report_callback.cpp"
	##"draw_subpasses/draw_subpasses.cpp"												#SPIRV gl_VertexIndex
	#"draw_textured_cube/draw_textured_cube.cpp"
	##"dynamic_uniform/dynamic_uniform.cpp"												#BindDescriptorSetsCommand - dynamicOffsets
	##"enable_validation_with_callback/enable_validation_with_callback.cpp"				#vkCreateDebugReportCallback
	#"enumerate_devices_adv/enumerate_devices_adv.cpp"
	#"events/events.cpp"
	##"immutable_sampler/immutable_sampler.cpp"											#DescriptorSet Create - pImmutableSamplers
	#"init_texture/init_texture.cpp"
	##"input_attachment/input_attachment.cpp"											#SPIRV OpSMod
	#"instance_extension_properties/instance_extension_properties.cpp"
	#"instance_layer_extension_properties/instance_layer_extension_properties.cpp"
	#"instance_layer_properties/instance_layer_properties.cpp"
	##"memory_barriers/memory_barriers.cpp"												#Multilayer clearing
	##"multiple_sets/multiple_sets.cpp"													#TODO
	##"multithreaded_command_buffers/multithreaded_command_buffers.cpp"					#ClearColourImageCommand - imagelayer > 1
	##"occlusion_query/occlusion_query.cpp"												#vkResetQueryPool
	##"pipeline_cache/pipeline_cache.cpp"												#PipelineCache::GetData
	##"pipeline_derivative/pipeline_derivative.cpp"										#Pipeline::Create flags
	##"push_constants/push_constants.cpp"												#vkCmdPushConstants
	##"push_descriptors/push_descriptors.cpp"											#DescriptorSetLayout::Create flags
	#"secondary_command_buffer/secondary_command_buffer.cpp"
	##"separate_image_sampler/separate_image_sampler.cpp"								#SPIRV Function Variables
	##"spirv_assembly/spirv_assembly.cpp"												#spirv-tools/libspirv.h
	##"spirv_specialization/spirv_specialization.cpp"									#spirv-tools/libspirv.h
	#"template/template.cpp"
	##"texel_buffer/texel_buffer.cpp"													#SPIRV Opcode 100
	##"validation_cache/validation_cache.cpp"											#VK_STRUCTURE_TYPE_SHADER_MODULE_VALIDATION_CACHE_CREATE_INFO_EXT
	#"vulkan_1_1_flexible/vulkan_1_1_flexible.cpp"
	
	"utils/util.cpp"
	"utils/util_init.cpp"
	)
target_include_directories(CPVulkanSamples PRIVATE "data/" "utils/")
target_compile_definitions(CPVulkanSamples PRIVATE VULKAN_SAMPLES_BASE_DIR="/" ${DEFINES})
target_link_libraries(CPVulkanSamples ${LINKS})