#pragma once
#include <vector>

#include "spirv.hpp"

__declspec(dllexport) void ConvertSpirv(const std::vector<uint32_t>& spirv, spv::ExecutionModel executionModel);
