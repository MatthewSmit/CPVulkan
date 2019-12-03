cmake_minimum_required(VERSION 3.8)

add_library(SPIRVParser SHARED
	"LLVMSPIRVLib.h"

	"LLVMSPIRVOpts.h"

	"OpenCL.std.h"

	"OpenGL.std.h"

	"SPIRV.debug.h"

	"spirv.hpp"

	"SPIRVBasicBlock.cpp"
	"SPIRVBasicBlock.h"

	"SPIRVDebug.cpp"
	"SPIRVDebug.h"

	"SPIRVDecorate.cpp"
	"SPIRVDecorate.h"

	"SPIRVEntry.cpp"
	"SPIRVEntry.h"

	"SPIRVEnum.h"

	"SPIRVErrorEnum.h"

	"SPIRVExtInst.h"

	"SPIRVFunction.cpp"
	"SPIRVFunction.h"

	"SPIRVInstruction.cpp"
	"SPIRVInstruction.h"

	"SPIRVIsValidEnum.h"

	"SPIRVModule.cpp"
	"SPIRVModule.h"

	"SPIRVNameMapEnum.h"

	"SPIRVOpCode.h"

	"SPIRVOpCodeEnum.h"

	"SPIRVStream.cpp"
	"SPIRVStream.h"

	"SPIRVType.cpp"
	"SPIRVType.h"

	"SPIRVUtil.h"

	"SPIRVValue.cpp"
	"SPIRVValue.h"
	)

set(DEFINES SPIRV_INTERNAL)
if(WIN32)
	list(APPEND DEFINES OS_WINDOWS)
endif()
target_compile_definitions(SPIRVParser PRIVATE ${DEFINES})

set_target_properties(SPIRVParser PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR})