#pragma once
#include "Jit.h"

#include <spirv.hpp>

struct FormatInformation;

class CompiledModuleBuilder;
class GraphicsPipelineStateStorage;

namespace SPIRV
{
	class SPIRVFunction;
	class SPIRVModule;
	class SPIRVVariable;
}

CP_DLL_EXPORT std::string MangleName(const SPIRV::SPIRVVariable* variable);
CP_DLL_EXPORT std::string MangleName(const SPIRV::SPIRVFunction* function);

CompiledModule* Compile(CompiledModuleBuilder* moduleBuilder, CPJit* jit, std::function<void*(const std::string&)> getFunction = nullptr);

CP_DLL_EXPORT FunctionPointer CompileGetPixelDepth(CPJit* jit, const FormatInformation* information);
CP_DLL_EXPORT FunctionPointer CompileGetPixelStencil(CPJit* jit, const FormatInformation* information);
CP_DLL_EXPORT FunctionPointer CompileGetPixelF32(CPJit* jit, const FormatInformation* information);
CP_DLL_EXPORT FunctionPointer CompileGetPixelI32(CPJit* jit, const FormatInformation* information);
CP_DLL_EXPORT FunctionPointer CompileGetPixelU32(CPJit* jit, const FormatInformation* information);

CP_DLL_EXPORT FunctionPointer CompileSetPixelDepthStencil(CPJit* jit, const FormatInformation* information);
CP_DLL_EXPORT FunctionPointer CompileSetPixelF32(CPJit* jit, const FormatInformation* information);
CP_DLL_EXPORT FunctionPointer CompileSetPixelI32(CPJit* jit, const FormatInformation* information);
CP_DLL_EXPORT FunctionPointer CompileSetPixelU32(CPJit* jit, const FormatInformation* information);

CP_DLL_EXPORT CompiledModule* CompileVertexPipeline(CPJit* jit,
                                                    const GraphicsPipelineStateStorage* state,
                                                    const SPIRV::SPIRVModule* vertexShader,
                                                    const SPIRV::SPIRVFunction* entryPoint,
                                                    const VkSpecializationInfo* specializationInfo);

CP_DLL_EXPORT CompiledModule* CompileFragmentPipeline(CPJit* jit,
                                                      const GraphicsPipelineStateStorage* state,
                                                      const SPIRV::SPIRVModule* fragmentShader,
                                                      const SPIRV::SPIRVFunction* entryPoint,
                                                      const VkSpecializationInfo* specializationInfo);

CP_DLL_EXPORT CompiledModule* CompileSPIRVModule(CPJit* jit, const SPIRV::SPIRVModule* spirvModule, spv::ExecutionModel executionModel, 
                                                 const SPIRV::SPIRVFunction* entryPoint, const VkSpecializationInfo* specializationInfo);