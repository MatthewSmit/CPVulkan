cmake_minimum_required(VERSION 3.8)

find_package(glm CONFIG REQUIRED)
find_package(LLVM CONFIG REQUIRED)

add_library(SPIRVToLLVM SHARED
	"Converter.cpp"
	
	"FunctionDescriptor.cpp"
	"Mangler.cpp"
	"ManglingUtils.cpp"
	"OCLUtil.cpp"
	"OGLUtil.cpp"
	"ParameterType.cpp"

	"SPIRVReader.cpp"
	"SPIRVToLLVMDbgTran.cpp"
	"SPIRVToOCL.cpp"
	"SPIRVToOCL12.cpp"
	"SPIRVToOCL20.cpp"
	"SPIRVUtil.cpp"
	)

target_link_libraries(SPIRVToLLVM PRIVATE SPIRVParser glm LLVMBitWriter LLVMCore)
set_target_properties(SPIRVToLLVM PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR})