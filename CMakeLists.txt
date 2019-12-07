cmake_minimum_required(VERSION 3.8)

SET(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (MSVC)
    if (NOT (MSVC_VERSION LESS 1910))
      add_compile_options(/permissive-)
    endif()
endif()

if(WIN32)
else()
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--no-undefined")
endif()

if(WIN32)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "C:\\Work\\CPVulkan\\out\\build\\x64-Debug\\CPVulkan\\")
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "C:\\Work\\CPVulkan\\out\\build\\x64-Debug\\CPVulkan\\")
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "C:\\Work\\CPVulkan\\out\\build\\x64-Debug\\CPVulkan\\")
endif()

project("CPVulkan")

add_subdirectory("SPIRVParser")
add_subdirectory("LLVMRuntime")
add_subdirectory("CPVulkanBase")
add_subdirectory("CPVulkan")
add_subdirectory("Samples")