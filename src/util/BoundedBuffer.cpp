#include "BoundedBuffer.h"

#include <stdexcept>

#include "general.h"

BoundedBuffer::BoundedBuffer(
    const vk::PhysicalDevice& physicalDevice, const vk::Device& dev, 
    const vk::DeviceSize size, const vk::BufferUsageFlags& usage, 
    const vk::MemoryPropertyFlags& properties)
    :   m_buffer(dev.createBufferUnique(vk::BufferCreateInfo(vk::BufferCreateFlags(), size, usage))),
        m_memory(createMemory(dev, dev.getBufferMemoryRequirements(*m_buffer), physicalDevice.getMemoryProperties(), properties))
{
    dev.bindBufferMemory(*m_buffer, *m_memory, 0);
}

BoundedBuffer::BoundedBuffer(
    const vk::PhysicalDevice& physicalDevice, const vk::Device& dev, 
    const vk::DeviceSize& size, const void* data, const vk::BufferUsageFlags& usage, 
    const vk::MemoryPropertyFlags& properties)
    :   BoundedBuffer(physicalDevice, dev, size, usage, properties)
{
    void* d_data = dev.mapMemory(*m_memory, 0, size);		
	std::memcpy(d_data, data, static_cast<size_t>(size));
	dev.unmapMemory(*m_memory);
}

BoundedBuffer::BoundedBuffer()
{
}

const vk::Buffer& BoundedBuffer::buffer() const
{
    return *m_buffer;
}

const vk::DeviceMemory& BoundedBuffer::memory() const
{
    return *m_memory; 
}

void BoundedBuffer::reset()
{
    m_buffer.reset();
    m_memory.reset();
}
