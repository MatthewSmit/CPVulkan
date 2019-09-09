cmake_minimum_required(VERSION 3.8)

find_package(glm CONFIG REQUIRED)
find_package(glslang CONFIG REQUIRED)
find_package(Vulkan REQUIRED)

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
	##"copy_blit_image/copy_blit_image.cpp"
	#"create_debug_report_callback/create_debug_report_callback.cpp"
	##"draw_subpasses/draw_subpasses.cpp"
	##"draw_textured_cube/draw_textured_cube.cpp"
	##"dynamic_uniform/dynamic_uniform.cpp"
	#"enable_validation_with_callback/enable_validation_with_callback.cpp"
	#"enumerate_devices_adv/enumerate_devices_adv.cpp"
	##"events/events.cpp"
	##"immutable_sampler/immutable_sampler.cpp"
	##"init_texture/init_texture.cpp"
	##"input_attachment/input_attachment.cpp"
	#"instance_extension_properties/instance_extension_properties.cpp"
	#"instance_layer_extension_properties/instance_layer_extension_properties.cpp"
	#"instance_layer_properties/instance_layer_properties.cpp"
	##"memory_barriers/memory_barriers.cpp"
	##"multiple_sets/multiple_sets.cpp"
	##"multithreaded_command_buffers/multithreaded_command_buffers.cpp"
	##"occlusion_query/occlusion_query.cpp"
	##"pipeline_cache/pipeline_cache.cpp"
	##"pipeline_derivative/pipeline_derivative.cpp"
	##"push_constants/push_constants.cpp"
	##"push_descriptors/push_descriptors.cpp"
	##"secondary_command_buffer/secondary_command_buffer.cpp"
	##"separate_image_sampler/separate_image_sampler.cpp"
	##"spirv_assembly/spirv_assembly.cpp"
	##"spirv_specialization/spirv_specialization.cpp"
	##"template/template.cpp"
	##"texel_buffer/texel_buffer.cpp"
	##"validation_cache/validation_cache.cpp"
	#"vulkan_1_1_flexible/vulkan_1_1_flexible.cpp"
	
	"utils/util.cpp"
	"utils/util_init.cpp"
	)
target_include_directories(CPVulkanSamples PRIVATE "data/" "utils/")
target_compile_definitions(CPVulkanSamples PRIVATE VK_USE_PLATFORM_WIN32_KHR VULKAN_SAMPLES_BASE_DIR="/")
target_link_libraries(CPVulkanSamples CPVulkan glm glslang::HLSL glslang::SPIRV glslang::glslang glslang::OGLCompiler Vulkan::Vulkan)