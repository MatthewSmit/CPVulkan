cmake_minimum_required(VERSION 3.8)

find_package(glm CONFIG REQUIRED)
find_package(LLVM CONFIG REQUIRED)

add_library(SPIRVToLLVM SHARED
	"Converter.cpp"
	"Converter.h"

	"GlslFunctions.cpp"
	"GlslFunctions.h"

	"SpirvJit.cpp"
	)

target_link_libraries(SPIRVToLLVM PRIVATE SPIRVParser glm LLVMBitWriter LLVMCore LLVMOrcJIT LLVMX86AsmParser LLVMX86CodeGen)
set_target_properties(SPIRVToLLVM PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR})