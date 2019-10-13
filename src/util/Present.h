#pragma once

#include <vector>

#include <vulkan/vulkan.hpp>

class Present
{
public:
    vk::UniqueSwapchainKHR  			swapChain;
    std::vector<vk::UniqueImageView> 	swapChainImageViews;
    vk::Queue 	 						queue;
    vk::Extent2D 						swapChainExtent;
    vk::Format   						swapChainImageFormat;
    std::vector<vk::Image>		 		swapChainImages;
};
