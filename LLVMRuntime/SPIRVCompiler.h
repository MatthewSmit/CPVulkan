#pragma once
#include <Base.h>

#include "CompiledModuleBuilder.h"

#include <SPIRVInstruction.h>
#include <SPIRVModule.h>

#include <spirv.hpp>

#include <unordered_set>

class SPIRVCompiledModuleBuilder final : public CompiledModuleBuilder
{
public:
	SPIRVCompiledModuleBuilder(const SPIRV::SPIRVModule* spirvModule, spv::ExecutionModel executionModel, const SPIRV::SPIRVFunction* entryPoint,
	                           const VkSpecializationInfo* specializationInfo);

	LLVMTypeRef ConvertType(const SPIRV::SPIRVType* spirvType, bool isClassMember = false);

	LLVMValueRef ConvertValue(const SPIRV::SPIRVValue* spirvValue, LLVMValueRef currentFunction);

	[[nodiscard]] LLVMValueRef getBuiltinInput() const
	{
		return builtinInputVariable;
	}
	
	[[nodiscard]] LLVMValueRef getBuiltinOutput() const
	{
		return builtinOutputVariable;
	}
	
	[[nodiscard]] LLVMValueRef getUserData() const
	{
		return userData;
	}

protected:
	LLVMValueRef CompileMainFunctionImpl() override;

private:
	LLVMBasicBlockRef currentBlock{};
	LLVMDIBuilderRef diBuilder{};

	std::unordered_map<uint32_t, LLVMTypeRef> typeMapping{};
	std::unordered_map<LLVMTypeRef, uint32_t> arrayStrideMultiplier{};
	std::unordered_map<LLVMTypeRef, std::vector<uint32_t>> structIndexMapping{};
	std::unordered_map<uint32_t, LLVMValueRef> valueMapping{};
	std::unordered_set<uint32_t> variablePointers{};
	std::unordered_map<uint32_t, LLVMValueRef> functionMapping{};
	std::unordered_map<std::string, LLVMValueRef> functionCache{};

	LLVMValueRef builtinInputVariable{};
	LLVMValueRef builtinOutputVariable{};
	std::vector<std::pair<spv::BuiltIn, uint32_t>> builtinInputMapping{};
	std::vector<std::pair<spv::BuiltIn, uint32_t>> builtinOutputMapping{};
	LLVMValueRef userData{};

	const SPIRV::SPIRVModule* spirvModule;
	spv::ExecutionModel executionModel;
	const SPIRV::SPIRVFunction* entryPoint;
	const VkSpecializationInfo* specializationInfo;
	
	template<typename T>
	T GetConstant(LLVMValueRef value);

	LLVMTypeRef CreateOpaqueImageType(const std::string& name);
	
	static std::string GetTypeName(const char* baseName, const std::string& postfixes);
	
	static std::string GetTypeName(const SPIRV::SPIRVType* type);

	static std::string GetImageTypeName(const SPIRV::SPIRVTypeImage* imageType);

	void AddDebugInformation(const SPIRV::SPIRVEntry* spirvEntry);

	static LLVMLinkage ConvertLinkage(const SPIRV::SPIRVValue* value);

	static bool IsOpaqueType(SPIRV::SPIRVType* spirvType);

	bool HasSpecOverride(const SPIRV::SPIRVValue* spirvValue);

	template<typename T>
	T GetSpecOverride(const SPIRV::SPIRVValue* spirvValue);

	LLVMValueRef HandleSpecConstantOperation(const SPIRV::SPIRVSpecConstantOp* spirvValue);

	LLVMValueRef ConvertValueNoDecoration(const SPIRV::SPIRVValue* spirvValue, LLVMValueRef currentFunction);

	void ConvertDecoration(LLVMValueRef llvmValue, const SPIRV::SPIRVValue* spirvValue);

	LLVMValueRef ConvertBuiltin(spv::BuiltIn builtin);

	std::vector<LLVMValueRef> ConvertValue(std::vector<SPIRV::SPIRVValue*> spirvValues, LLVMValueRef currentFunction);

	static bool NeedsPointer(const SPIRV::SPIRVType* type);

	LLVMValueRef GetInbuiltFunction(const std::string& functionNamePrefix, SPIRV::SPIRVType* returnType,
	                                const std::vector<std::pair<const char*, const SPIRV::SPIRVType*>>& arguments, bool hasUserData = false);

	LLVMValueRef CallInbuiltFunction(LLVMValueRef function, SPIRV::SPIRVType* returnType,
	                                 const std::vector<std::pair<const SPIRV::SPIRVType*, LLVMValueRef>>& arguments, bool hasUserData = false);

	LLVMValueRef CallInbuiltFunction(SPIRV::SPIRVMatrixTimesScalar* matrixTimesScalar, LLVMValueRef currentFunction);

	LLVMValueRef CallInbuiltFunction(SPIRV::SPIRVVectorTimesMatrix* vectorTimesMatrix, LLVMValueRef currentFunction);

	LLVMValueRef CallInbuiltFunction(SPIRV::SPIRVMatrixTimesVector* matrixTimesVector, LLVMValueRef currentFunction);

	LLVMValueRef CallInbuiltFunction(SPIRV::SPIRVMatrixTimesMatrix* matrixTimesMatrix, LLVMValueRef currentFunction);

	LLVMValueRef CallInbuiltFunction(SPIRV::SPIRVDot* dot, LLVMValueRef currentFunction);

	LLVMValueRef CallInbuiltFunction(SPIRV::SPIRVImageSampleImplicitLod* imageSampleImplicitLod, LLVMValueRef currentFunction);

	LLVMValueRef CallInbuiltFunction(SPIRV::SPIRVImageSampleExplicitLod* imageSampleExplicitLod, LLVMValueRef currentFunction);

	LLVMValueRef CallInbuiltFunction(SPIRV::SPIRVImageFetch* imageFetch, LLVMValueRef currentFunction);

	LLVMValueRef CallInbuiltFunction(SPIRV::SPIRVImageRead* imageRead, LLVMValueRef currentFunction);

	std::vector<LLVMValueRef> MapBuiltin(spv::BuiltIn builtin, const std::vector<std::pair<spv::BuiltIn, uint32_t>>& builtinMapping);

	std::vector<LLVMValueRef> MapBuiltin(const std::vector<LLVMValueRef>& indices, SPIRV::SPIRVType* type,
	                                     const std::vector<std::pair<spv::BuiltIn, uint32_t>>& builtinMapping);

	template<typename LoopInstructionType>
	void SetLLVMLoopMetadata(const LoopInstructionType* loopMerge, LLVMValueRef branchInstruction);

	LLVMValueRef ConvertOCLFromExtensionInstruction(const SPIRV::SPIRVExtInst* extensionInstruction, LLVMValueRef currentFunction);

	LLVMValueRef ConvertOGLFromExtensionInstruction(const SPIRV::SPIRVExtInst* extensionInstruction, LLVMValueRef currentFunction);

	LLVMValueRef ConvertDebugFromExtensionInstruction(const SPIRV::SPIRVExtInst* extensionInstruction, LLVMValueRef currentFunction);

	static LLVMAtomicOrdering ConvertSemantics(uint32_t semantics);

	bool TranslateNonTemporalMetadata(LLVMValueRef instruction);

	LLVMValueRef GetConstantFloatOrVector(LLVMTypeRef type, double value);

	void MoveBuilder(LLVMBasicBlockRef basicBlock, SPIRV::SPIRVInstruction* instruction);

	LLVMValueRef ConvertInstruction(SPIRV::SPIRVInstruction* instruction, LLVMValueRef currentFunction);

	LLVMBasicBlockRef GetBasicBlock(const SPIRV::SPIRVBasicBlock* spirvBasicBlock);

	void CompileBasicBlock(const SPIRV::SPIRVBasicBlock* spirvBasicBlock, LLVMValueRef currentFunction, LLVMBasicBlockRef llvmBasicBlock);

	LLVMValueRef ConvertFunction(const SPIRV::SPIRVFunction* spirvFunction);

	void AddBuiltin();
};
