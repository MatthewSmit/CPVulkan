#pragma once
#include "Converter.h"

#include <unordered_map>

std::unordered_map<std::string, FunctionPointer>& getSpirvFunctions();
