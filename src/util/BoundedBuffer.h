#pragma once

#include <vector>

#include <vulkan/vulkan.hpp>

class BoundedBuffer
{
public:
    BoundedBuffer(
        const vk::PhysicalDevice& physicalDevice, const vk::Device& dev, 
        const vk::DeviceSize size, const vk::BufferUsageFlags& usage, 
        const vk::MemoryPropertyFlags& properties
    );

    BoundedBuffer(
        const vk::PhysicalDevice& physicalDevice, const vk::Device& dev, 
        const vk::DeviceSize& size, const void* data, const vk::BufferUsageFlags& usage, 
        const vk::MemoryPropertyFlags& properties
    );

    template <class Container>
    BoundedBuffer(
        const vk::PhysicalDevice& physicalDevice, const vk::Device& dev, 
        const Container& hostBuffer, const vk::BufferUsageFlags& usage, 
        const vk::MemoryPropertyFlags& properties)
        : BoundedBuffer(physicalDevice, dev, hostBuffer.size() * sizeof(hostBuffer[0]), static_cast<const void*>(hostBuffer.data()), usage, properties)
    {
    }

    BoundedBuffer();

    const vk::Buffer& buffer() const;

    const vk::DeviceMemory& memory() const;

    void reset();

private:
    vk::UniqueBuffer m_buffer;
    vk::UniqueDeviceMemory m_memory;
};
