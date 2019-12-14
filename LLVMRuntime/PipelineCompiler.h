#pragma once
#include "Jit.h"

namespace SPIRV
{
	class SPIRVModule;
}

CP_DLL_EXPORT CompiledModule* CompileVertexPipeline(CPJit* jit, const SPIRV::SPIRVModule* spirvVertexShader, CompiledModule* llvmVertexShader,
                                                    const std::vector<const std::vector<VkDescriptorSetLayoutBinding>*>& layoutBindings,
                                                    const std::vector<VkVertexInputBindingDescription>& vertexBindingDescriptions,
                                                    const std::vector<VkVertexInputAttributeDescription>& vertexAttributeDescriptions,
                                                    std::function<void*(const std::string&)> getFunction);