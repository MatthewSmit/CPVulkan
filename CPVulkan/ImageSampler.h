#pragma once
#include "Base.h"

struct FormatInformation;

template<typename InputType, typename OutputType>
void ConvertPixelsFromTemp(const uint64_t input[4], OutputType output[4]);

template<typename OutputType>
void ConvertPixelsFromTemp(const FormatInformation& format, const uint64_t input[4], OutputType output[]);


template<typename InputType, typename OutputType>
void ConvertPixelsToTemp(const InputType input[4], uint64_t output[4]);

template<typename InputType>
void ConvertPixelsToTemp(const FormatInformation& format, const InputType input[4], uint64_t output[4]);


template<typename Size>
void GetPixel(const FormatInformation& format, gsl::span<uint8_t> data, uint32_t i, uint32_t j, uint32_t k, uint32_t width, uint32_t height, uint32_t depth, uint32_t arrayLayers, uint64_t values[4]);

template<typename OutputType>
void GetPixel(const FormatInformation& format, const Image* image, int32_t i, int32_t j, int32_t k, OutputType output[4]);

template<typename OutputType>
OutputType GetPixel(const FormatInformation& format, const Image* image, int32_t i, int32_t j, int32_t k);


template<typename Size>
void SetPixel(const FormatInformation& format, gsl::span<uint8_t> data, uint32_t i, uint32_t j, uint32_t k, uint32_t width, uint32_t height, uint32_t depth, uint32_t arrayLayers, uint32_t mipLevel, uint32_t layer, const uint64_t values[4]);

void SetPixel(const FormatInformation& format, Image* image, uint32_t x, uint32_t y, uint32_t z, uint32_t mipLevel, uint32_t layer, uint64_t values[4]);


using SampleImageType = void(*)(const FormatInformation& format, const Image* image, float u, float v, float w, float q, uint32_t a, VkFilter filter, VkSamplerMipmapMode mipmapMode, VkSamplerAddressMode addressMode, uint64_t output[4]);

template<typename OutputType>
void SampleImage(const FormatInformation& format, const Image* image, float u, float v, float w, float q, uint32_t a, VkFilter filter, VkSamplerMipmapMode mipmapMode, VkSamplerAddressMode addressMode, uint64_t output[4]);

template<typename ReturnType>
ReturnType SampleImage(const Image* image, float u, float v, float w, float q, uint32_t a, VkFilter filter, VkSamplerMipmapMode mipmapMode, VkSamplerAddressMode addressMode);