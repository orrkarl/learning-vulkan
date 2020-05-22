template <class Container>
BoundedBuffer HelloTriangleApp::createStagedBuffer(const Container& hostData, const vk::BufferUsageFlags& usage, const vk::MemoryPropertyFlags& properties)
{
    auto size = sizeof(hostData[0]) * hostData.size();

    auto stagingBuffer = BoundedBuffer(
        m_physicalDevice, *m_device, 
        hostData, vk::BufferUsageFlagBits::eTransferSrc, 
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
    );

    auto ret = BoundedBuffer(
        m_physicalDevice, *m_device,
        size, usage | vk::BufferUsageFlagBits::eTransferDst,
        properties 
    );

    copyBuffer(stagingBuffer.buffer(), ret.buffer(), size);

    return ret;
}