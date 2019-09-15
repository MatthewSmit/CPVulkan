cmake_minimum_required(VERSION 3.8)

add_compile_options("/std:c++latest")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "C:\\Work\\CPVulkan\\out\\build\\x64-Debug\\CPVulkan\\")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "C:\\Work\\CPVulkan\\out\\build\\x64-Debug\\CPVulkan\\")

project("CPVulkan")

add_subdirectory("SPIRVParser")
add_subdirectory("SPIRVToLLVM")
add_subdirectory("CPVulkan")
add_subdirectory("Samples")
