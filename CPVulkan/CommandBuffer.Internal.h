#pragma once
#include "Base.h"

class Command
{
public:
	virtual ~Command() = default;

#if CV_DEBUG_LEVEL > 0
	virtual void DebugOutput(DeviceState* deviceState) = 0;
#endif

	virtual void Process(DeviceState* deviceState) = 0;
};

class FunctionCommand final : public Command
{
public:
	explicit FunctionCommand(std::function<void(DeviceState*)> action) :
		action{std::move(action)}
	{
	}

	~FunctionCommand() override = default;

#if CV_DEBUG_LEVEL > 0
	void DebugOutput(DeviceState*) override
	{
	}
#endif

	void Process(DeviceState* deviceState) override
	{
		action(deviceState);
	}

private:
	std::function<void(DeviceState*)> action;
};

void ClearImage(DeviceState* deviceState, Image* image, VkFormat format, uint32_t baseMipLevel, uint32_t levelCount, uint32_t baseArrayLayer, uint32_t layerCount, VkClearColorValue colour);
void ClearImage(DeviceState* deviceState, Image* image, VkFormat format, uint32_t baseMipLevel, uint32_t levelCount, uint32_t baseArrayLayer, uint32_t layerCount, VkImageAspectFlags aspects, VkClearDepthStencilValue colour);
