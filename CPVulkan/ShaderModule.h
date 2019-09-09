#pragma once
#include "Base.h"

#include <SPIRV/doc.h>

#include <array>

struct Instruction
{
	spv::Id resultId{};
	spv::Id typeId{};
	spv::Op opCode{};
	std::vector<uint32_t> arguments{};

	Instruction(spv::Id resultId, spv::Id typeId, spv::Op opCode) :
		resultId(resultId),
		typeId(typeId),
		opCode(opCode)
	{
	}
};

struct ShaderEntryPoint
{
	spv::ExecutionModel ExecutionModel;
	uint32_t EntryPoint;
	std::string Name;
	std::vector<uint32_t> Interface;
};

class ShaderModule final : public VulkanBase
{
public:
	~ShaderModule() override = default;

	static VkResult Create(const VkShaderModuleCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkShaderModule* pShaderModule);

private:
	std::array<bool, spv::CapabilityGroupNonUniformQuad + 1> capabilities{};
	spv::AddressingModel addressingModel{};
	spv::MemoryModel memoryModel{};
	std::vector<ShaderEntryPoint> entryPoints{};

	void Disassemble(const std::vector<uint32_t>& data);
	void HandleInitial(const std::vector<Instruction>& instructions, uint32_t& index);
	void HandleTypes(uint32_t maxResults, const std::vector<Instruction>& instructions, uint32_t& index);
	void HandleCapability(const Instruction& instruction);
};
