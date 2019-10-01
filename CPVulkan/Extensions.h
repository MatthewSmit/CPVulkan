#pragma once
#include "Base.h"

#include <vector>

struct EntryPoint
{
	const char* Name;
	PFN_vkVoidFunction Function;
	bool Device;
};

struct Extension
{
	const char* Name;
	uint32_t Version;
	bool Device;
	std::vector<EntryPoint> EntryPoints{};
};

class ExtensionGroup
{
public:
	ExtensionGroup();
	explicit ExtensionGroup(std::vector<Extension>&& extensions);

	VkResult MakeInstanceCopy(int vulkanVersion, const std::vector<const char*>& extensionNames, ExtensionGroup* extensionGroup) const;
	VkResult MakeDeviceCopy(const ExtensionGroup& extraExtensions, const std::vector<const char*>& extensionNames, ExtensionGroup* extensionGroup) const;
	void FillExtensionProperties(VkExtensionProperties* pProperties, uint32_t toCopy, bool device) const;
	Extension FindExtension(const char* extensionName) const;

	PFN_vkVoidFunction getFunction(const char* name, bool device) const;
	uint32_t getNumberInstanceExtensions() const;
	uint32_t getNumberDeviceExtensions() const;

private:
	mutable uint32_t numberInstanceExtensions{0xFFFFFFFF};
	mutable uint32_t numberDeviceExtensions{0xFFFFFFFF};
	std::vector<Extension> extensions{};
};

ExtensionGroup& GetInitialExtensions();