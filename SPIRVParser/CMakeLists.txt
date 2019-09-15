cmake_minimum_required(VERSION 3.8)

add_library(SPIRVParser SHARED
	"SPIRVBasicBlock.cpp"
	"SPIRVDebug.cpp"
	"SPIRVDecorate.cpp"
	"SPIRVEntry.cpp"
	"SPIRVFunction.cpp"
	"SPIRVInstruction.cpp"
	"SPIRVModule.cpp"
	"SPIRVStream.cpp"
	"SPIRVType.cpp"
	"SPIRVValue.cpp"
	)

target_compile_definitions(SPIRVParser PRIVATE SPIRV_INTERNAL)

set_target_properties(SPIRVParser PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR})