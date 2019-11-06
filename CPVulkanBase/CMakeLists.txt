cmake_minimum_required(VERSION 3.8)

set(FILES
		"Base.h"
		
		"Config.h"

		"FloatFormat.h"
		
		"Formats.cpp"
		"Formats.h"

		"Half.h"
		)

set(DEFINES VK_NO_PROTOTYPES)
set(INCLUDES "${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/include" ${CMAKE_CURRENT_SOURCE_DIR})

if(WIN32)
	list(APPEND DEFINES VK_USE_PLATFORM_WIN32_KHR WIN32_LEAN_AND_MEAN)
	list(APPEND INCLUDES "C:\\VulkanSDK\\1.1.126.0\\Include\\")
else()
	list(APPEND DEFINES VK_USE_PLATFORM_XCB_KHR VK_USE_PLATFORM_XLIB_KHR VK_USE_PLATFORM_XLIB_XRANDR_EXT)
	list(APPEND INCLUDES "/home/cosc/student/mjs351/vulkan/1.1.121.1/x86_64/include/")
endif()

add_library(CPVulkanBase SHARED	${FILES})
target_compile_definitions(CPVulkanBase PUBLIC ${DEFINES})
target_include_directories(CPVulkanBase PUBLIC ${INCLUDES})