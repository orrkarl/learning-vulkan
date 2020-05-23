#pragma once

#include <algorithm>
#include <string>
#include <vector>
#include <vulkan/vulkan.hpp>

std::vector<char> readFile(const std::string &path);

template <typename T>
T clamp(const T &min, const T &value, const T &max)
{
	return std::max(min, std::min(value, max));
}

vk::UniqueDeviceMemory createMemory(
        const vk::Device& dev,
        const vk::MemoryRequirements& requirements,
        const vk::PhysicalDeviceMemoryProperties& physicalProperties,
        const vk::MemoryPropertyFlags& properties);

uint32_t findMemoryType(const vk::PhysicalDeviceMemoryProperties& properties, const uint32_t typeFilter, vk::MemoryPropertyFlags propertyFlags);

vk::Format findSupportedFormat(
        vk::PhysicalDevice dev, 
        const std::vector<vk::Format>& candidates, 
        vk::ImageTiling requiredTiling, 
        vk::FormatFeatureFlags requiredFormatFeatures);

bool hasStencilAttachment(vk::Format format);
