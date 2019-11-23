#pragma once
#include "Jit.h"

#include <unordered_map>

std::unordered_map<std::string, FunctionPointer>& getSpirvFunctions();
