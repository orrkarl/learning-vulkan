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

vk::Fence copyBuffer(const vk::Device& device, const vk::Queue& queue, const vk::CommandPool& pool, const vk::Buffer& src, const vk::Buffer& dest, const vk::DeviceSize& size);
