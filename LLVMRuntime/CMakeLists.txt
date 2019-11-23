cmake_minimum_required(VERSION 3.8)

find_package(glm CONFIG REQUIRED)
find_package(LLVM CONFIG REQUIRED)

add_library(LLVMRuntime SHARED
	"Converter.cpp"
	"Converter.h"

	"ImageCompiler.cpp"
	"ImageCompiler.h"

	"LlvmHelper.cpp"

	"SpirvFunctions.cpp"
	"SpirvFunctions.h"

	"SpirvJit.cpp"
	)

target_link_libraries(LLVMRuntime PRIVATE CPVulkanBase SPIRVParser glm LLVMBitWriter LLVMCore LLVMOrcJIT LLVMX86AsmParser LLVMX86CodeGen)
target_include_directories(LLVMRuntime PRIVATE ${LLVM_INCLUDE_DIR})
set_target_properties(LLVMRuntime PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR})