cmake_minimum_required(VERSION 3.8)

if(WIN32)
    add_compile_options("/std:c++latest")
else()
    add_compile_options("-std=c++2a")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--no-undefined")
endif()

if(WIN32)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "C:\\Work\\CPVulkan\\out\\build\\x64-Debug\\CPVulkan\\")
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "C:\\Work\\CPVulkan\\out\\build\\x64-Debug\\CPVulkan\\")
endif()

project("CPVulkan")

add_subdirectory("SPIRVParser")
#add_subdirectory("SPIRVToLLVM")
add_subdirectory("CPVulkan")
add_subdirectory("Samples")
