#include "Present.h"

#include <iostream>

#include "query.h"
#include "QueueFamilyIndices.h"

Present::Present(const vk::Device& dev, const vk::PhysicalDevice& physicalDevice, const vk::SurfaceKHR& surface, GLFWwindow* window)
    : m_device(dev)
{
    auto support = SwapChainSupportDetails(physicalDevice, surface);

    auto format = support.chooseFormat();
    auto presentationMode = support.choosePresentMode();
    auto extent = support.chooseExtent(window);

    auto imageCount = support.chooseImageCount();

    vk::SwapchainCreateInfoKHR chainInfo(
        vk::SwapchainCreateFlagsKHR(),
        surface,
        imageCount,
        format.format,
        format.colorSpace,
        extent,
        1,
        vk::ImageUsageFlagBits::eColorAttachment
    );

    QueueFamilyIndices indices = QueueFamilyIndices(physicalDevice, surface);
    uint32_t queueFamilyIndices[] = {indices.graphics(), indices.present()};
    if (indices.graphics() != indices.present())
    {
        chainInfo.imageSharingMode = vk::SharingMode::eConcurrent;
        chainInfo.queueFamilyIndexCount = 2;
        chainInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else
    {
        chainInfo.imageSharingMode = vk::SharingMode::eExclusive;
        chainInfo.queueFamilyIndexCount = 0;	 // Optional
        chainInfo.pQueueFamilyIndices = nullptr; // Optional
    }

    chainInfo.preTransform = support.getCurrentTransform();
    chainInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
    chainInfo.presentMode = presentationMode;
    chainInfo.clipped = VK_TRUE;
    chainInfo.oldSwapchain = vk::SwapchainKHR();

    m_swapChain = dev.createSwapchainKHR(chainInfo);
    m_swapChainExtent = extent;
    m_swapChainImageFormat = format.format;

    auto m_swapChainImages = dev.getSwapchainImagesKHR(m_swapChain);

    vk::ImageViewCreateInfo createInfo(
        vk::ImageViewCreateFlags(),
        vk::Image(),
        vk::ImageViewType::e2D,
        m_swapChainImageFormat,
        vk::ComponentMapping(),
        vk::ImageSubresourceRange(
            vk::ImageAspectFlagBits::eColor,
            0,
            1,
            0,
            1
        )
    );

    m_swapChainImageViews.resize(m_swapChainImages.size());
    for (size_t i = 0; i < m_swapChainImageViews.size(); i++)
    {
        createInfo.image = m_swapChainImages[i];
        m_swapChainImageViews[i] = dev.createImageView(createInfo);
    }

    m_queue = dev.getQueue(indices.present(), 0);
}

Present::Present()
{
    
}

Present::Present(Present&& other)
{
    m_swapChain = other.m_swapChain;
    m_swapChainImageViews = other.m_swapChainImageViews;
    m_queue = other.m_queue;
    m_swapChainExtent = other.m_swapChainExtent;
    m_swapChainImageFormat = other.m_swapChainImageFormat;
    m_device = other.m_device;

    other.reset();
}

Present::~Present()
{
    release();
    reset();
}

Present& Present::operator=(Present&& other)
{
    release();
    
    m_swapChain = other.m_swapChain;
    m_swapChainImageViews = other.m_swapChainImageViews;
    m_queue = other.m_queue;
    m_swapChainExtent = other.m_swapChainExtent;
    m_swapChainImageFormat = other.m_swapChainImageFormat;
    m_device = other.m_device;

    other.reset();
}

std::ostream& operator<<(std::ostream& os, const vk::Extent2D& extent)
{
    return os << '(' << extent.width << ", " << extent.height << ')';
}

std::ostream& operator<<(std::ostream& os, const Present& self)
{
    os << "Present: {";
        
    os << "Chain: " << self.m_swapChain << " -> " << self.m_swapChainExtent;
        
    os << ", Image Views: [ ";
    for (const auto& view : self.m_swapChainImageViews)
    {
        os << view << " ";
    }
    os << ']';

    os << ", Queue: " << self.m_queue;

    return os;
}

vk::Extent2D Present::extent() const
{
    return m_swapChainExtent;
}

vk::Format Present::format() const
{
    return m_swapChainImageFormat;
}

const uint32_t Present::imageCount() const
{
    return m_swapChainImageViews.size();
}

const vk::ImageView& Present::view(const uint32_t idx) const
{
    return m_swapChainImageViews[idx];
}

vk::Result Present::present(const vk::Semaphore& signal, const uint32_t& imageIndex)
{
    vk::PresentInfoKHR presentInfo(1, &signal, 1, &m_swapChain, &imageIndex);
	return m_queue.presentKHR(&presentInfo);
}

vk::Result Present::acquireNextImage(const vk::Semaphore& wait, uint32_t& index)
{
	return m_device.acquireNextImageKHR(m_swapChain, std::numeric_limits<uint64_t>::max(), wait, vk::Fence(), &index);
}

void Present::await()
{
    m_queue.waitIdle();
}

void Present::reset()
{
    m_swapChain = vk::SwapchainKHR();
    m_swapChainImageViews.clear();
    m_queue = vk::Queue();
    m_swapChainExtent = vk::Extent2D();
    m_swapChainImageFormat = vk::Format();
    m_device = vk::Device();
}

void Present::release()
{
    for (const auto& view : m_swapChainImageViews)
        if (view)
            m_device.destroyImageView(view);
    
    if (m_swapChain) 
        m_device.destroySwapchainKHR(m_swapChain);
}
