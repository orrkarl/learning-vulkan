#include "BoundImage.h"

#include "general.h"

BoundImage::BoundImage(
    vk::Device dev, 
    uint32_t width, uint32_t height, 
    vk::Format format, vk::ImageTiling tiling, 
    vk::ImageUsageFlags usage, 
    vk::MemoryPropertyFlags imageMemoryProperties, 
    vk::PhysicalDeviceMemoryProperties physicalMemoryProperties)
    : m_image(dev.createImageUnique(
        vk::ImageCreateInfo(
            vk::ImageCreateFlags(), 
            vk::ImageType::e2D, 
            format,
            vk::Extent3D(width, height, 1), 
            1, 
            1,
            vk::SampleCountFlagBits::e1, 
            tiling, 
            usage, 
            vk::SharingMode::eExclusive))),
      m_memory(createMemory(dev, dev.getImageMemoryRequirements(*m_image), physicalMemoryProperties, imageMemoryProperties)) {
    
    dev.bindImageMemory(*m_image, *m_memory, 0);
}

const vk::Image& BoundImage::image() const {
    return *m_image;
}

const vk::DeviceMemory& BoundImage::memory() const {
    return *m_memory;
}
