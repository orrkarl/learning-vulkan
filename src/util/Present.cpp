#include "Present.h"

#include <iostream>

#include "query.h"
#include "QueueFamilyIndices.h"

Present::Present(const vk::Device& dev, const vk::PhysicalDevice& physicalDevice, const vk::SurfaceKHR& surface, GLFWwindow* window)
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

    m_swapChain = dev.createSwapchainKHRUnique(chainInfo);
    m_swapChainExtent = extent;
    m_swapChainImageFormat = format.format;

    auto m_swapChainImages = dev.getSwapchainImagesKHR(m_swapChain.get());

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
        m_swapChainImageViews[i] = dev.createImageViewUnique(createInfo);
    }

    m_queue = dev.getQueue(indices.present(), 0);
}

Present::Present()
{
    
}


std::ostream& operator<<(std::ostream& os, const vk::Extent2D& extent)
{
    return os << '(' << extent.width << ", " << extent.height << ')';
}

std::ostream& operator<<(std::ostream& os, const Present& self)
{
    os << "Present: {";
        
    os << "Chain: " << *self.m_swapChain << " -> " << self.m_swapChainExtent;
        
    os << ", Image Views: [ ";
    for (const auto& view : self.m_swapChainImageViews)
    {
        os << *view << " ";
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

const vk::Queue& Present::queue() const
{
    return m_queue;
}

const uint32_t Present::imageCount() const
{
    return m_swapChainImageViews.size();
}

const vk::ImageView& Present::view(const uint32_t idx)
{
    return m_swapChainImageViews[idx].get();
}

const vk::SwapchainKHR& Present::chain() const
{
    return *m_swapChain;
}
