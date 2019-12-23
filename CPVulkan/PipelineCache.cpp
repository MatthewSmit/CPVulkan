#include "PipelineCache.h"

#include "Device.h"

#include <CompiledModule.h>

#include <cassert>

struct CacheHeader
{
	uint32_t Size;
	VkPipelineCacheHeaderVersion Version;
	uint32_t VendorId;
	uint32_t DeviceId;
	uint8_t PipelineCacheUUID[VK_UUID_SIZE];
	
	uint32_t NumberEntries;
};

void PipelineCache::LoadData(size_t initialDataSize, const void* initialData)
{
	if (initialDataSize < sizeof(CacheHeader))
	{
		return;
	}
	
	const auto header = static_cast<const CacheHeader*>(initialData);
	if (header->Size > initialDataSize)
	{
		return;
	}

	if (header->Version != VK_PIPELINE_CACHE_HEADER_VERSION_ONE)
	{
		return;
	}

	if (header->VendorId != VENDOR_ID)
	{
		return;
	}

	if (header->DeviceId != DEVICE_ID)
	{
		return;
	}

	if (memcmp(header->PipelineCacheUUID, PIPELINE_CACHE_UUID, VK_UUID_SIZE) != 0)
	{
		return;
	}

	auto ptr = static_cast<const uint8_t*>(initialData) + sizeof(CacheHeader);
	for (auto i = 0u; i < header->NumberEntries; i++)
	{
		const auto hash = *reinterpret_cast<const Hash*>(ptr);
		ptr += sizeof(Hash);

		const auto dataSize = *reinterpret_cast<const uint32_t*>(ptr);
		ptr += 4;

		auto& vector = cached[hash];
		vector.resize(dataSize);
		memcpy(vector.data(), ptr, dataSize);
		ptr += dataSize;
	}
}

VkResult PipelineCache::GetData(size_t* pDataSize, void* pData)
{
	auto size = sizeof(CacheHeader) + 4;
	for (const auto& entry : cached)
	{
		size += sizeof(Hash) + 4 + entry.second.size();
	}

	assert(size <= 0xFFFFFFFF);
	
	if (pData)
	{
		if (*pDataSize < size)
		{
			*pDataSize = 0;
			return VK_INCOMPLETE;
		}

		const auto header = static_cast<CacheHeader*>(pData);
		header->Size = static_cast<uint32_t>(size);
		header->Version = VK_PIPELINE_CACHE_HEADER_VERSION_ONE;
		header->VendorId = VENDOR_ID;
		header->DeviceId = DEVICE_ID;
		memcpy(header->PipelineCacheUUID, PIPELINE_CACHE_UUID, VK_UUID_SIZE);

		assert(cached.size() <= 0xFFFFFFFF);
		header->NumberEntries = static_cast<uint32_t>(cached.size());

		auto ptr = static_cast<uint8_t*>(pData) + sizeof(CacheHeader);
		for (const auto& entry : cached)
		{
			*reinterpret_cast<Hash*>(ptr) = entry.first;
			ptr += sizeof(Hash);
			assert(entry.second.size() <= 0xFFFFFFFF);
			*reinterpret_cast<uint32_t*>(ptr) = static_cast<uint32_t>(entry.second.size());
			ptr += 4;
			memcpy(ptr, entry.second.data(), entry.second.size());
			ptr += entry.second.size();
		}
	}
	else
	{
		*pDataSize = size;
	}
	return VK_SUCCESS;
}

VkResult Device::GetPipelineCacheData(VkPipelineCache pipelineCache, size_t* pDataSize, void* pData)
{
	return UnwrapVulkan<PipelineCache>(pipelineCache)->GetData(pDataSize, pData);
}

CompiledModule* PipelineCache::FindModule(const Hash& hash, CPJit* jit, std::function<void*(const std::string&)> getFunction)
{
	const auto& result = cached.find(hash);
	if (result == cached.end())
	{
		return nullptr;
	}

	return CompiledModule::CreateFromBitcode(jit, getFunction, result->second);
}

void PipelineCache::AddModule(const Hash& hash, CompiledModule* module)
{
	const auto data = module->ExportBitcode();
	cached[hash] = data;
}

VkResult PipelineCache::Create(const VkPipelineCacheCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkPipelineCache* pPipelineCache)
{
	assert(pCreateInfo->sType == VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO);
	assert(pCreateInfo->flags == 0);

	const auto pipelineCache = Allocate<PipelineCache>(pAllocator, VK_SYSTEM_ALLOCATION_SCOPE_DEVICE);
	if (!pipelineCache)
	{
		return VK_ERROR_OUT_OF_HOST_MEMORY;
	}

	if (pCreateInfo->initialDataSize)
	{
		pipelineCache->LoadData(pCreateInfo->initialDataSize, pCreateInfo->pInitialData);
	}

	WrapVulkan(pipelineCache, pPipelineCache);
	return VK_SUCCESS;
}

VkResult Device::CreatePipelineCache(const VkPipelineCacheCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkPipelineCache* pPipelineCache)
{
	return PipelineCache::Create(pCreateInfo, pAllocator, pPipelineCache);
}

void Device::DestroyPipelineCache(VkPipelineCache pipelineCache, const VkAllocationCallbacks* pAllocator)
{
	if (pipelineCache)
	{
		Free(UnwrapVulkan<PipelineCache>(pipelineCache), pAllocator);
	}
}

VkResult Device::MergePipelineCaches(VkPipelineCache dstCache, uint32_t srcCacheCount, const VkPipelineCache* pSrcCaches)
{
	TODO_ERROR();
}
