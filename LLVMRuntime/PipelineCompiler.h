#pragma once
#include "Jit.h"

CP_DLL_EXPORT CompiledModule* CompileVertexPipeline(CPJit* jit, const SPIRV::SPIRVModule* vertexShader, const std::vector<const std::vector<VkDescriptorSetLayoutBinding>*>& layoutBindings, std::function<void*(const std::string&)> getFunction);