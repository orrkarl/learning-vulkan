#pragma once

#include <vector>

#include <vulkan/vulkan.hpp>

class BoundImage
{
public:
    BoundImage() = default;
    BoundImage(
        vk::Device dev, 
        uint32_t width, uint32_t height, uint32_t mipLevels,
        vk::Format format, vk::ImageTiling tiling, 
        vk::ImageUsageFlags usage, 
        vk::MemoryPropertyFlags imageMemoryProperties, 
        vk::PhysicalDeviceMemoryProperties physicalMemoryProperties
    );

    const vk::Image& image() const;

    const vk::DeviceMemory& memory() const;

    void reset();

private:
    vk::UniqueImage m_image;
    vk::UniqueDeviceMemory m_memory;
};
