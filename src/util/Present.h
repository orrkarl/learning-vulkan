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

    Present(const Present& other) = delete;

    Present(Present&& other);

    ~Present();

    Present& operator=(const Present& other) = delete;

    Present& operator=(Present&& other);

    vk::Extent2D extent() const;

    vk::Format format() const;

    const uint32_t imageCount() const;

    const vk::ImageView& view(const uint32_t idx) const;

    vk::Result present(const vk::Semaphore& signal, const uint32_t& imageIndex);

    vk::Result acquireNextImage(const vk::Semaphore& wait, uint32_t& index);

    void await();

    void reset();

    void release();

private:
    vk::Device                  m_device;
    vk::Queue                   m_queue;
    vk::SwapchainKHR            m_swapChain;
    vk::Extent2D                m_swapChainExtent;
    vk::Format                  m_swapChainImageFormat;
    std::vector<vk::ImageView>  m_swapChainImageViews;
};
