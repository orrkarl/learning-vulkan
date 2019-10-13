#pragma once

#include <vector>

#include <vulkan/vulkan.hpp>

#include "BoundedBuffer.h"

class Graphics
{
public:
    vk::UniqueCommandPool 					commandPool;
    std::vector<vk::UniqueSemaphore> 		imageAvailable;
    std::vector<vk::UniqueFence> 			inFlightImages;
    std::vector<vk::UniqueSemaphore>		renderCompleted;
    BoundedBuffer							deviceVertecies;
    BoundedBuffer							deviceIndices;
    std::vector<BoundedBuffer>				uniforms;
    vk::UniqueRenderPass 					renderPass;
    vk::UniquePipelineLayout 				pipelineLayout;
    vk::UniquePipeline 						pipeline;
    std::vector<vk::UniqueCommandBuffer>	commandBuffers;
    std::vector<vk::UniqueFramebuffer>		frameBuffers;
    vk::UniqueDescriptorSetLayout			descriptorSetLayout;
    vk::UniqueDescriptorPool				descriptorPool;
    std::vector<vk::DescriptorSet> 			descriptorSets;
    vk::Queue 								queue;
};
