#pragma once

#include <vector>

#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>

class Present
{
public:
    Present();

    Present(const vk::Device& dev, const vk::PhysicalDevice& physicalDevice, const vk::SurfaceKHR& surface, GLFWwindow* window);

    vk::UniqueSwapchainKHR  			swapChain;
    std::vector<vk::UniqueImageView> 	swapChainImageViews;
    vk::Queue 	 						queue;
    vk::Extent2D 						swapChainExtent;
    vk::Format   						swapChainImageFormat;
    std::vector<vk::Image>		 		swapChainImages;
    vk::Device                          m_device;
};
