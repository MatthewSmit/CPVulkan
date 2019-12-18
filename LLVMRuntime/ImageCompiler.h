#pragma once
#include <llvm-c/Types.h>

class CompiledModuleBuilder;

struct FormatInformation;

LLVMValueRef EmitGetPixel(CompiledModuleBuilder* moduleBuilder, LLVMValueRef sourcePointer, LLVMTypeRef resultType, const FormatInformation* information);
