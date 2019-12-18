#pragma once
#include "Jit.h"

#include <spirv.hpp>

struct FormatInformation;

namespace SPIRV
{
	class SPIRVFunction;
	class SPIRVModule;
}

CP_DLL_EXPORT FunctionPointer CompileGetPixelDepth(CPJit* jit, const FormatInformation* information);
CP_DLL_EXPORT FunctionPointer CompileGetPixelStencil(CPJit* jit, const FormatInformation* information);
CP_DLL_EXPORT FunctionPointer CompileGetPixelF32(CPJit* jit, const FormatInformation* information);
CP_DLL_EXPORT FunctionPointer CompileGetPixelI32(CPJit* jit, const FormatInformation* information);
CP_DLL_EXPORT FunctionPointer CompileGetPixelU32(CPJit* jit, const FormatInformation* information);

CP_DLL_EXPORT FunctionPointer CompileSetPixelDepthStencil(CPJit* jit, const FormatInformation* information);
CP_DLL_EXPORT FunctionPointer CompileSetPixelF32(CPJit* jit, const FormatInformation* information);
CP_DLL_EXPORT FunctionPointer CompileSetPixelI32(CPJit* jit, const FormatInformation* information);
CP_DLL_EXPORT FunctionPointer CompileSetPixelU32(CPJit* jit, const FormatInformation* information);

CP_DLL_EXPORT CompiledModule* CompileVertexPipeline(CPJit* jit, const SPIRV::SPIRVModule* spirvVertexShader, CompiledModule* llvmVertexShader,
                                                    const std::vector<const std::vector<VkDescriptorSetLayoutBinding>*>& layoutBindings,
                                                    const std::vector<VkVertexInputBindingDescription>& vertexBindingDescriptions,
                                                    const std::vector<VkVertexInputAttributeDescription>& vertexAttributeDescriptions,
                                                    std::function<void*(const std::string&)> getFunction);

CP_DLL_EXPORT CompiledModule* CompileSPIRVModule(CPJit* jit, const SPIRV::SPIRVModule* spirvModule, spv::ExecutionModel executionModel, 
                                                 const SPIRV::SPIRVFunction* entryPoint, const VkSpecializationInfo* specializationInfo);