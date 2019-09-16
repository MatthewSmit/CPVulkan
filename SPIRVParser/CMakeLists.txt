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

set(DEFINES SPIRV_INTERNAL)
if(WIN32)
	list(APPEND DEFINES OS_WINDOWS)
endif()
target_compile_definitions(SPIRVParser PRIVATE ${DEFINES})

set_target_properties(SPIRVParser PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR})