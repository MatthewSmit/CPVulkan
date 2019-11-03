#pragma once
#include "Converter.h"

struct FormatInformation;

STL_DLL_EXPORT FunctionPointer CompileGetPixelDepth(SpirvJit* jit, const FormatInformation* information);
STL_DLL_EXPORT FunctionPointer CompileSetPixelDepthStencil(SpirvJit* jit, const FormatInformation* information);
STL_DLL_EXPORT FunctionPointer CompileSetPixelFloat(SpirvJit* jit, const FormatInformation* information);
STL_DLL_EXPORT FunctionPointer CompileSetPixelInt32(SpirvJit* jit, const FormatInformation* information);
STL_DLL_EXPORT FunctionPointer CompileSetPixelUInt32(SpirvJit* jit, const FormatInformation* information);