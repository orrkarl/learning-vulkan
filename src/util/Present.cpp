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

    swapChain = m_device.createSwapchainKHRUnique(chainInfo);
    swapChainExtent = extent;
    swapChainImageFormat = format.format;

    swapChainImages = m_device.getSwapchainImagesKHR(swapChain.get());

    vk::ImageViewCreateInfo createInfo(
        vk::ImageViewCreateFlags(),
        vk::Image(),
        vk::ImageViewType::e2D,
        swapChainImageFormat,
        vk::ComponentMapping(),
        vk::ImageSubresourceRange(
            vk::ImageAspectFlagBits::eColor,
            0,
            1,
            0,
            1
        )
    );

    swapChainImageViews.resize(swapChainImages.size());
    for (size_t i = 0; i < swapChainImageViews.size(); i++)
    {
        createInfo.image = swapChainImages[i];
        swapChainImageViews[i] = m_device.createImageViewUnique(createInfo);
    }

    queue = m_device.getQueue(indices.present(), 0);
}

Present::Present()
{
    
}

