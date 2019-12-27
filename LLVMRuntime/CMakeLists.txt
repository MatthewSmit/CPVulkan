cmake_minimum_required(VERSION 3.8)

find_package(glm CONFIG REQUIRED)
find_package(LLVM CONFIG REQUIRED)

add_library(LLVMRuntime SHARED
	"CompiledModule.cpp"
	"CompiledModule.h"
	"CompiledModuleBuilder.h"
	
	"Compilers.h"

	"ImageCompiler.cpp"
	"ImageCompiler.h"

	"Jit.cpp"
	"Jit.h"

	"LLVMHelper.cpp"

	"PipelineCompiler.cpp"
	"PipelineCompiler.h"
	
	"SPIRVCompiler.cpp"
	"SPIRVCompiler.h"

	"SpirvFunctions.cpp"
	"SpirvFunctions.h"
	)

target_link_libraries(LLVMRuntime PRIVATE CPVulkanBase SPIRVParser glm LLVM-C)
target_include_directories(LLVMRuntime PRIVATE ${LLVM_INCLUDE_DIR})
set_target_properties(LLVMRuntime PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR})