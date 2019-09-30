cmake_minimum_required(VERSION 3.8)

SET(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

#set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address")
#set(CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fsanitize=address -static-libasan -lasan")

#if(__WINDOWS_CLANG)
#elif(MSVC)
#else()
#    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--no-undefined")
#endif()

if(WIN32)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "C:\\Work\\CPVulkan\\out\\build\\x64-Debug\\CPVulkan\\")
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "C:\\Work\\CPVulkan\\out\\build\\x64-Debug\\CPVulkan\\")
endif()

project("CPVulkan")

add_subdirectory("SPIRVParser")
add_subdirectory("SPIRVToLLVM")
add_subdirectory("CPVulkan")
add_subdirectory("Samples")