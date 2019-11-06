#pragma once
#define VULKAN_H_ 1
#include <vulkan/vulkan.h>

#if defined(VK_USE_PLATFORM_WIN32_KHR)
#undef VK_KHR_external_memory_fd
#undef VK_EXT_external_memory_dma_buf
#undef VK_KHR_external_semaphore_fd
#undef VK_KHR_external_fence_fd
#endif

#undef VK_EXT_pci_bus_info
#undef VK_KHR_display
#undef VK_KHR_display_swapchain
#undef VK_EXT_direct_mode_display
#undef VK_EXT_acquire_xlib_display
#undef VK_EXT_display_surface_counter
#undef VK_EXT_display_control
#undef VK_KHR_get_display_properties2

constexpr auto ROBUST_BUFFER_ACCESS = true; // Must be true for Vulkan 1.0+
constexpr auto FULL_DRAW_INDEX_UINT32 = true;
constexpr auto IMAGE_CUBE_ARRAY = true;
constexpr auto INDEPENDENT_BLEND = true;
constexpr auto GEOMETRY_SHADER = false; // TODO
constexpr auto TESSELLATION_SHADER = false; // TODO
constexpr auto SAMPLE_RATE_SHADING = true;
constexpr auto DUAL_SOURCE_BLEND = true;
constexpr auto LOGIC_OPERATION = true;
constexpr auto MULTI_DRAW_INDIRECT = true;
constexpr auto DRAW_INDIRECT_FIRST_INSTANCE = true;
constexpr auto DEPTH_CLAMP = true;
constexpr auto DEPTH_BIAS_CLAMP = true;
constexpr auto FILL_MODE_NON_SOLID = true;
constexpr auto DEPTH_BOUNDS = true;
constexpr auto WIDE_LINES = true;
constexpr auto LARGE_POINTS = true;
constexpr auto ALPHA_TO_ONE = true;
constexpr auto MULTI_VIEWPORT = false; // Requires GEOMETRY_SHADER
constexpr auto SAMPLER_ANISOTROPY = true;
constexpr auto TEXTURE_COMPRESSION_ETC2 = true;
constexpr auto TEXTURE_COMPRESSION_ASTC_LDR = true;
constexpr auto TEXTURE_COMPRESSION_BC = true;
constexpr auto OCCLUSION_QUERY_PRECISE = true;
constexpr auto PIPELINE_STATISTICS_QUERY = true;
constexpr auto VERTEX_PIPELINE_STORES_AND_ATOMICS = true;
constexpr auto FRAGMENT_STORES_AND_ATOMICS = true;
constexpr auto SHADER_TESSELLATION_AND_GEOMETRY_POINT_SIZE = true;
constexpr auto SHADER_IMAGE_GATHER_EXTENDED = true;
constexpr auto SHADER_STORAGE_IMAGE_EXTENDED_FORMATS = true;
constexpr auto SHADER_STORAGE_IMAGE_MULTISAMPLE = true;
constexpr auto SHADER_STORAGE_IMAGE_READ_WITHOUT_FORMAT = true;
constexpr auto SHADER_STORAGE_IMAGE_WRITE_WITHOUT_FORMAT = true;
constexpr auto SHADER_UNIFORM_BUFFER_ARRAY_DYNAMIC_INDEXING = true;
constexpr auto SHADER_SAMPLED_IMAGE_ARRAY_DYNAMIC_INDEXING = true;
constexpr auto SHADER_STORAGE_BUFFER_ARRAY_DYNAMIC_INDEXING = true;
constexpr auto SHADER_STORAGE_IMAGE_ARRAY_DYNAMIC_INDEXING = true;
constexpr auto SHADER_CLIP_DISTANCE = true;
constexpr auto SHADER_CULL_DISTANCE = true;
constexpr auto SHADER_FLOAT64 = true;
constexpr auto SHADER_INT64 = true;
constexpr auto SHADER_INT16 = true;
constexpr auto SHADER_RESOURCE_RESIDENCY = true;
constexpr auto SHADER_RESOURCE_MIN_LOD = true;
constexpr auto SPARSE_BINDING = true;
constexpr auto VARIABLE_MULTISAMPLE_RATE = true;
constexpr auto INHERITED_QUERIES = true;

static_assert(GEOMETRY_SHADER || !MULTI_VIEWPORT);

constexpr auto VENDOR_ID = 0x10000;
constexpr auto DEVICE_ID = 0;
constexpr auto DEVICE_TYPE = VK_PHYSICAL_DEVICE_TYPE_CPU;
constexpr auto DEVICE_NAME = "CPVulkan";
// memset(pProperties->pipelineCacheUUID, 0, VK_UUID_SIZE);
// pProperties->limits.maxImageDimension1D = 4096;
// pProperties->limits.maxImageDimension2D = 4096;
// pProperties->limits.maxImageDimension3D = 256;
// pProperties->limits.maxImageDimensionCube = 4096;
// pProperties->limits.maxImageArrayLayers = 256;
// pProperties->limits.maxTexelBufferElements = 65536;
// pProperties->limits.maxUniformBufferRange = 16384;
// pProperties->limits.maxStorageBufferRange = 134217728;
constexpr auto MAX_PUSH_CONSTANTS_SIZE = 128;
// pProperties->limits.maxMemoryAllocationCount = 4096;
// pProperties->limits.maxSamplerAllocationCount = 4000;
// pProperties->limits.bufferImageGranularity = 1;
// pProperties->limits.sparseAddressSpaceSize = std::numeric_limits<size_t>::max();
// pProperties->limits.maxBoundDescriptorSets = 4;
constexpr auto MAX_BOUND_DESCRIPTOR_SETS = 4;
// pProperties->limits.maxPerStageDescriptorSamplers = 32;
// pProperties->limits.maxPerStageDescriptorUniformBuffers = 32;
// pProperties->limits.maxPerStageDescriptorStorageBuffers = 32;
// pProperties->limits.maxPerStageDescriptorSampledImages = 32;
// pProperties->limits.maxPerStageDescriptorStorageImages = 32;
// pProperties->limits.maxPerStageDescriptorInputAttachments = 32;
// pProperties->limits.maxPerStageResources = 128;
// pProperties->limits.maxDescriptorSetSamplers = pProperties->limits.maxPerStageDescriptorSamplers * 8;
// pProperties->limits.maxDescriptorSetUniformBuffers = pProperties->limits.maxPerStageDescriptorUniformBuffers * 8;
// pProperties->limits.maxDescriptorSetUniformBuffersDynamic = 8;
// pProperties->limits.maxDescriptorSetStorageBuffers = pProperties->limits.maxPerStageDescriptorStorageBuffers * 8;
// pProperties->limits.maxDescriptorSetStorageBuffersDynamic = 4;
// pProperties->limits.maxDescriptorSetSampledImages = pProperties->limits.maxPerStageDescriptorSampledImages * 8;
// pProperties->limits.maxDescriptorSetStorageImages = pProperties->limits.maxPerStageDescriptorStorageImages * 8;
// pProperties->limits.maxDescriptorSetInputAttachments = pProperties->limits.maxPerStageDescriptorInputAttachments;
// pProperties->limits.maxVertexInputAttributes = 16;
constexpr auto MAX_VERTEX_INPUT_BINDINGS = 16;
// pProperties->limits.maxVertexInputAttributeOffset = 2047;
// pProperties->limits.maxVertexInputBindingStride = 2048;
// pProperties->limits.maxVertexOutputComponents = 64;
// pProperties->limits.maxTessellationGenerationLevel = 64;
// pProperties->limits.maxTessellationPatchSize = 32;
// pProperties->limits.maxTessellationControlPerVertexInputComponents = 64;
// pProperties->limits.maxTessellationControlPerVertexOutputComponents = 64;
// pProperties->limits.maxTessellationControlPerPatchOutputComponents = 120;
// pProperties->limits.maxTessellationControlTotalOutputComponents = 2048;
// pProperties->limits.maxTessellationEvaluationInputComponents = 64;
// pProperties->limits.maxTessellationEvaluationOutputComponents = 64;
// pProperties->limits.maxGeometryShaderInvocations = 32;
// pProperties->limits.maxGeometryInputComponents = 64;
// pProperties->limits.maxGeometryOutputComponents = 64;
// pProperties->limits.maxGeometryOutputVertices = 256;
// pProperties->limits.maxGeometryTotalOutputComponents = 1024;
// pProperties->limits.maxFragmentInputComponents = 64;
// pProperties->limits.maxFragmentOutputAttachments = 4;
// pProperties->limits.maxFragmentDualSrcAttachments = 1;
// pProperties->limits.maxFragmentCombinedOutputResources = 4;
// pProperties->limits.maxComputeSharedMemorySize = 16384;
// pProperties->limits.maxComputeWorkGroupCount[0] = 65535;
// pProperties->limits.maxComputeWorkGroupCount[1] = 65535;
// pProperties->limits.maxComputeWorkGroupCount[2] = 65535;
// pProperties->limits.maxComputeWorkGroupInvocations = 128;
// pProperties->limits.maxComputeWorkGroupSize[0] = 128;
// pProperties->limits.maxComputeWorkGroupSize[1] = 128;
// pProperties->limits.maxComputeWorkGroupSize[2] = 64;
// pProperties->limits.subPixelPrecisionBits = 4;
// pProperties->limits.subTexelPrecisionBits = 4;
// pProperties->limits.mipmapPrecisionBits = 4;
// pProperties->limits.maxDrawIndexedIndexValue = 0xFFFFFFFF;
// pProperties->limits.maxDrawIndirectCount = 0xFFFF;
// pProperties->limits.maxSamplerLodBias = 2;
// pProperties->limits.maxSamplerAnisotropy = 16;
constexpr auto MAX_VIEWPORTS = 16;
// pProperties->limits.maxViewportDimensions[0] = 4096;
// pProperties->limits.maxViewportDimensions[1] = 4096;
// pProperties->limits.viewportBoundsRange[0] = -8192;
// pProperties->limits.viewportBoundsRange[1] = 8192;
// pProperties->limits.viewportSubPixelBits = 0;
// pProperties->limits.minMemoryMapAlignment = 64;
// pProperties->limits.minTexelBufferOffsetAlignment = 256; // TODO: Lower to 16?
// pProperties->limits.minUniformBufferOffsetAlignment = 256;
// pProperties->limits.minStorageBufferOffsetAlignment = 256;
// pProperties->limits.minTexelOffset = -8;
// pProperties->limits.maxTexelOffset = 7;
// pProperties->limits.minTexelGatherOffset = -8;
// pProperties->limits.maxTexelGatherOffset = 7;
// pProperties->limits.minInterpolationOffset = -0.5;
// pProperties->limits.maxInterpolationOffset = 0.5f - ULP;
// pProperties->limits.subPixelInterpolationOffsetBits = 4;
// pProperties->limits.maxFramebufferWidth = 4096;
// pProperties->limits.maxFramebufferHeight = 4096;
// pProperties->limits.maxFramebufferLayers = 256;
// pProperties->limits.framebufferColorSampleCounts = VK_SAMPLE_COUNT_1_BIT | VK_SAMPLE_COUNT_2_BIT | VK_SAMPLE_COUNT_4_BIT | VK_SAMPLE_COUNT_8_BIT | VK_SAMPLE_COUNT_16_BIT | VK_SAMPLE_COUNT_32_BIT | VK_SAMPLE_COUNT_64_BIT;
// pProperties->limits.framebufferDepthSampleCounts = VK_SAMPLE_COUNT_1_BIT | VK_SAMPLE_COUNT_2_BIT | VK_SAMPLE_COUNT_4_BIT | VK_SAMPLE_COUNT_8_BIT | VK_SAMPLE_COUNT_16_BIT | VK_SAMPLE_COUNT_32_BIT | VK_SAMPLE_COUNT_64_BIT;
// pProperties->limits.framebufferStencilSampleCounts = VK_SAMPLE_COUNT_1_BIT | VK_SAMPLE_COUNT_2_BIT | VK_SAMPLE_COUNT_4_BIT | VK_SAMPLE_COUNT_8_BIT | VK_SAMPLE_COUNT_16_BIT | VK_SAMPLE_COUNT_32_BIT | VK_SAMPLE_COUNT_64_BIT;
// pProperties->limits.framebufferNoAttachmentsSampleCounts = VK_SAMPLE_COUNT_1_BIT | VK_SAMPLE_COUNT_2_BIT | VK_SAMPLE_COUNT_4_BIT | VK_SAMPLE_COUNT_8_BIT | VK_SAMPLE_COUNT_16_BIT | VK_SAMPLE_COUNT_32_BIT | VK_SAMPLE_COUNT_64_BIT;
constexpr auto MAX_COLOUR_ATTACHMENTS = 4;
// pProperties->limits.sampledImageColorSampleCounts = VK_SAMPLE_COUNT_1_BIT | VK_SAMPLE_COUNT_2_BIT | VK_SAMPLE_COUNT_4_BIT | VK_SAMPLE_COUNT_8_BIT | VK_SAMPLE_COUNT_16_BIT | VK_SAMPLE_COUNT_32_BIT | VK_SAMPLE_COUNT_64_BIT;
// pProperties->limits.sampledImageIntegerSampleCounts = VK_SAMPLE_COUNT_1_BIT | VK_SAMPLE_COUNT_2_BIT | VK_SAMPLE_COUNT_4_BIT | VK_SAMPLE_COUNT_8_BIT | VK_SAMPLE_COUNT_16_BIT | VK_SAMPLE_COUNT_32_BIT | VK_SAMPLE_COUNT_64_BIT;
// pProperties->limits.sampledImageDepthSampleCounts = VK_SAMPLE_COUNT_1_BIT | VK_SAMPLE_COUNT_2_BIT | VK_SAMPLE_COUNT_4_BIT | VK_SAMPLE_COUNT_8_BIT | VK_SAMPLE_COUNT_16_BIT | VK_SAMPLE_COUNT_32_BIT | VK_SAMPLE_COUNT_64_BIT;
// pProperties->limits.sampledImageStencilSampleCounts = VK_SAMPLE_COUNT_1_BIT | VK_SAMPLE_COUNT_2_BIT | VK_SAMPLE_COUNT_4_BIT | VK_SAMPLE_COUNT_8_BIT | VK_SAMPLE_COUNT_16_BIT | VK_SAMPLE_COUNT_32_BIT | VK_SAMPLE_COUNT_64_BIT;
// pProperties->limits.storageImageSampleCounts = VK_SAMPLE_COUNT_1_BIT | VK_SAMPLE_COUNT_2_BIT | VK_SAMPLE_COUNT_4_BIT | VK_SAMPLE_COUNT_8_BIT | VK_SAMPLE_COUNT_16_BIT | VK_SAMPLE_COUNT_32_BIT | VK_SAMPLE_COUNT_64_BIT;
// pProperties->limits.maxSampleMaskWords = 1;
// pProperties->limits.timestampComputeAndGraphics = true;
// pProperties->limits.timestampPeriod = 1;
// pProperties->limits.maxClipDistances = 8;
// pProperties->limits.maxCullDistances = 8;
// pProperties->limits.maxCombinedClipAndCullDistances = 8;
// pProperties->limits.discreteQueuePriorities = 2;
// pProperties->limits.pointSizeRange[0] = 1.0;
// pProperties->limits.pointSizeRange[1] = 64.0f - ULP;
// pProperties->limits.lineWidthRange[0] = 1;
// pProperties->limits.lineWidthRange[1] = 8.0f - ULP;
constexpr auto POINT_SIZE_GRANULARITY = LARGE_POINTS ? 1 : 0;
constexpr auto LINE_WIDTH_GRANULARITY = WIDE_LINES ? 1 : 0;
// pProperties->limits.strictLines = true;
// pProperties->limits.standardSampleLocations = true;
// pProperties->limits.optimalBufferCopyOffsetAlignment = 16;
// pProperties->limits.optimalBufferCopyRowPitchAlignment = 16;
// pProperties->limits.nonCoherentAtomSize = 256;
// pProperties->sparseProperties.residencyStandard2DBlockShape = false;
// pProperties->sparseProperties.residencyStandard2DMultisampleBlockShape = false;
// pProperties->sparseProperties.residencyStandard3DBlockShape = false;
// pProperties->sparseProperties.residencyAlignedMipSize = false;
// pProperties->sparseProperties.residencyNonResidentStrict = false;

#if defined(VK_NV_ray_tracing)
constexpr auto MAX_PIPELINES = 3;
#else
constexpr auto MAX_PIPELINES = 2;
#endif