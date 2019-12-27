#pragma once
#include "Base.h"

#include <Formats.h>

struct DeviceState;

template<typename ResultType, typename RangeType, typename CoordinateType>
ResultType GetPixel(DeviceState* deviceState, VkFormat format, gsl::span<uint8_t> data, RangeType range, CoordinateType coordinates, ResultType borderColour);

template<typename ResultType, typename RangeType, typename CoordinateType>
ResultType SampleImage(DeviceState* deviceState, VkFormat format, gsl::span<uint8_t> data, RangeType range, CoordinateType coordinates, VkFilter filter);

template<typename ResultType, typename RangeType, typename CoordinateType>
ResultType SampleImage(DeviceState* deviceState, VkFormat format, gsl::span<uint8_t> data[MAX_MIP_LEVELS], RangeType range[MAX_MIP_LEVELS], 
                       uint32_t baseLevel, uint32_t levels, CoordinateType coordinates, float lod, Sampler* sampler);

float GetDepthPixel(DeviceState* deviceState, VkFormat format, const Image* image, int32_t i, int32_t j, int32_t k, uint32_t mipLevel, uint32_t layer);
uint8_t GetStencilPixel(DeviceState* deviceState, VkFormat format, const Image* image, int32_t i, int32_t j, int32_t k, uint32_t mipLevel, uint32_t layer);

void SetPixel(DeviceState* deviceState, VkFormat format, Image* image, int32_t i, int32_t j, int32_t k, uint32_t mipLevel, uint32_t layer, VkClearDepthStencilValue value);
void SetPixel(DeviceState* deviceState, VkFormat format, Image* image, int32_t i, int32_t j, int32_t k, uint32_t mipLevel, uint32_t layer, VkClearColorValue value);
void SetPixel(DeviceState* deviceState, VkFormat format, Image* image, int32_t i, int32_t j, int32_t k, uint32_t mipLevel, uint32_t layer, const float values[4]);
void SetPixel(DeviceState* deviceState, VkFormat format, Image* image, int32_t i, int32_t j, int32_t k, uint32_t mipLevel, uint32_t layer, const int32_t values[4]);
void SetPixel(DeviceState* deviceState, VkFormat format, Image* image, int32_t i, int32_t j, int32_t k, uint32_t mipLevel, uint32_t layer, const uint32_t values[4]);