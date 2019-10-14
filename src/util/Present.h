#pragma once

#include <iostream>
#include <vector>

#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>

class Present
{
public:
    friend std::ostream& operator<<(std::ostream& os, const Present& self);

    Present();

    Present(const vk::Device& dev, const vk::PhysicalDevice& physicalDevice, const vk::SurfaceKHR& surface, GLFWwindow* window);

    vk::Extent2D extent() const;

    vk::Format format() const;

    const uint32_t imageCount() const;

    const vk::ImageView& view(const uint32_t idx);

    vk::Result present(const vk::Semaphore& signal, const uint32_t& imageIndex);

    vk::Result acquireNextImage(const vk::Semaphore& wait, uint32_t& index);

    void await();

private:
    vk::UniqueSwapchainKHR  			m_swapChain;
    std::vector<vk::UniqueImageView> 	m_swapChainImageViews;
    vk::Queue 	 						m_queue;
    vk::Extent2D 						m_swapChainExtent;
    vk::Format   						m_swapChainImageFormat;
    vk::Device                          m_device;
};
