#pragma once
#include "Converter.h"

struct FormatInformation;

STL_DLL_EXPORT FunctionPointer CompileGetPixelDepth(SpirvJit* jit, const FormatInformation* information);
STL_DLL_EXPORT FunctionPointer CompileGetPixelStencil(SpirvJit* jit, const FormatInformation* information);
STL_DLL_EXPORT FunctionPointer CompileGetPixel(SpirvJit* jit, const FormatInformation* information);
STL_DLL_EXPORT FunctionPointer CompileGetPixelF32(SpirvJit* jit, const FormatInformation* information);
STL_DLL_EXPORT FunctionPointer CompileGetPixelI32(SpirvJit* jit, const FormatInformation* information);
STL_DLL_EXPORT FunctionPointer CompileGetPixelU32(SpirvJit* jit, const FormatInformation* information);

STL_DLL_EXPORT FunctionPointer CompileSetPixelDepthStencil(SpirvJit* jit, const FormatInformation* information);
STL_DLL_EXPORT FunctionPointer CompileSetPixelF32(SpirvJit* jit, const FormatInformation* information);
STL_DLL_EXPORT FunctionPointer CompileSetPixelI32(SpirvJit* jit, const FormatInformation* information);
STL_DLL_EXPORT FunctionPointer CompileSetPixelU32(SpirvJit* jit, const FormatInformation* information);