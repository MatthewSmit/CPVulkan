#pragma once
#include "Base.h"
#include "Pipeline.h"

struct Hash
{
	// 256 bits
	union
	{
		uint64_t values[4];
		uint8_t bytes[32];
	};
};

inline bool operator==(const Hash& lhs, const Hash& rhs)
{
	return lhs.values[0] == rhs.values[0] &&
		lhs.values[1] == rhs.values[1] &&
		lhs.values[2] == rhs.values[2] &&
		lhs.values[3] == rhs.values[3];
}

namespace std
{
	template<>
	struct hash<Hash>
	{
		std::size_t operator()(const Hash& hash) const noexcept
		{
			return hash.values[0] ^ hash.values[1] ^ hash.values[2] ^ hash.values[3];
		}
	};
}

class PipelineCache final
{
public:
	PipelineCache() = default;
	PipelineCache(const PipelineCache&) = delete;
	PipelineCache(PipelineCache&&) = delete;
	~PipelineCache() = default;

	PipelineCache& operator=(const PipelineCache&) = delete;
	PipelineCache&& operator=(const PipelineCache&&) = delete;

	void OnDelete(const VkAllocationCallbacks*)
	{
	}

	void LoadData(size_t initialDataSize, const void* initialData);
	VkResult GetData(size_t* pDataSize, void* pData);
	
	CompiledModule* FindModule(const Hash& hash, CPJit* jit, std::function<void*(const std::string&)> getFunction);
	void AddModule(const Hash& hash, CompiledModule* module);

	void Merge(const PipelineCache* cache);

	static VkResult Create(const VkPipelineCacheCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkPipelineCache* pPipelineCache);

private:
	std::unordered_map<Hash, std::vector<uint8_t>> cached{};
};
