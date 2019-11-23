#pragma once
#include "Jit.h"

struct FormatInformation;

CP_DLL_EXPORT FunctionPointer CompileGetPixelDepth(CPJit* jit, const FormatInformation* information);
CP_DLL_EXPORT FunctionPointer CompileGetPixelStencil(CPJit* jit, const FormatInformation* information);
CP_DLL_EXPORT FunctionPointer CompileGetPixelF32(CPJit* jit, const FormatInformation* information);
CP_DLL_EXPORT FunctionPointer CompileGetPixelI32(CPJit* jit, const FormatInformation* information);
CP_DLL_EXPORT FunctionPointer CompileGetPixelU32(CPJit* jit, const FormatInformation* information);

CP_DLL_EXPORT FunctionPointer CompileSetPixelDepthStencil(CPJit* jit, const FormatInformation* information);
CP_DLL_EXPORT FunctionPointer CompileSetPixelF32(CPJit* jit, const FormatInformation* information);
CP_DLL_EXPORT FunctionPointer CompileSetPixelI32(CPJit* jit, const FormatInformation* information);
CP_DLL_EXPORT FunctionPointer CompileSetPixelU32(CPJit* jit, const FormatInformation* information);