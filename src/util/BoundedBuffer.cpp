#include "BoundedBuffer.h"

#include <stdexcept>

uint32_t findMemoryType(const vk::PhysicalDeviceMemoryProperties& properties, const uint32_t typeFilter, vk::MemoryPropertyFlags propertyFlags)
{
	for (auto i = 0u; i < properties.memoryTypeCount; ++i)
	{
		if ((typeFilter & (1 << i)) && ((properties.memoryTypes[i].propertyFlags & propertyFlags) == propertyFlags))
		{
			return i;
		}
	}

	throw std::runtime_error("could not find compatible memory");
}

vk::UniqueDeviceMemory createMemory(
        const vk::Device& dev,
        const vk::MemoryRequirements& requirements,
        const vk::PhysicalDeviceMemoryProperties& physicalProperties,
        const vk::MemoryPropertyFlags& properties)
{
    return dev.allocateMemoryUnique(
        vk::MemoryAllocateInfo(
            requirements.size, 
            findMemoryType(
	    		physicalProperties,
	    		requirements.memoryTypeBits, 
	    		properties
	    	)
        )
    );
}

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
    m_buffer.release();
    m_memory.release();
}

void BoundedBuffer::release()
{
    m_buffer.reset();
    m_memory.reset();
}
