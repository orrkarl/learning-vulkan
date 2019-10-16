#pragma once

#include <chrono>
#include <ostream>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vulkan/vulkan.hpp>

#include "BoundedBuffer.h"
#include "general.h"
#include "MVPTransform.h"
#include "Present.h"

struct Vertex
{
	glm::vec2 pos;
	glm::vec3 color;

	static vk::VertexInputBindingDescription getBindingDescription()
	{
		return vk::VertexInputBindingDescription(0, sizeof(Vertex));
	}

	static std::array<vk::VertexInputAttributeDescription, 2> getAttributeDescription()
	{
		return {
			vk::VertexInputAttributeDescription(0, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, pos)),
			vk::VertexInputAttributeDescription(1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, color))
		};
	}
};


class Graphics
{
public:

    Graphics(
        const vk::Device& dev,
        const Present& present,
        const uint32_t graphicsFamilyIndex,
        const vk::PhysicalDevice& physicalDevice
    );

    Graphics();

    vk::UniqueCommandPool 					commandPool;
    vk::UniqueRenderPass 					renderPass;
    vk::UniquePipelineLayout 				pipelineLayout;
    vk::UniquePipeline 						pipeline;
    std::vector<vk::UniqueCommandBuffer>	commandBuffers;
    std::vector<vk::UniqueFramebuffer>		frameBuffers;
    vk::UniqueDescriptorSetLayout			descriptorSetLayout;
    vk::UniqueDescriptorPool				descriptorPool;
    std::vector<vk::DescriptorSet> 			descriptorSets;
    BoundedBuffer						    deviceVertecies;
    BoundedBuffer						    deviceIndices;
    std::vector<BoundedBuffer>			    uniforms;
    vk::Queue 								queue;

    void render(const vk::Semaphore& wait, const vk::Semaphore& signal, const vk::Fence& hostNotify, const uint32_t& imageIndex);

    void notifySwapchainUpdated(const vk::Extent2D& extent)
    {
        m_projection = glm::perspective(glm::radians(45.0f), extent.width / static_cast<float>(extent.height), 0.1f, 10.0f);
		m_projection[1][1] *= -1;
    }

    void update(const Present& present);

    void setDevice(const vk::Device& dev)
    {
        m_device = dev;
    }

private:
	void updateData(const uint32_t& imageIndex);

    template <class Container>
    BoundedBuffer createStagedBuffer(const vk::PhysicalDevice& physicalDevice, const Container& hostData, const vk::BufferUsageFlags& usage, const vk::MemoryPropertyFlags& properties)
    {
        auto size = sizeof(hostData[0]) * hostData.size();

        auto stagingBuffer = BoundedBuffer(
            physicalDevice, m_device, 
            hostData, vk::BufferUsageFlagBits::eTransferSrc, 
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
        );

        auto ret = BoundedBuffer(
            physicalDevice, m_device,
            size, usage | vk::BufferUsageFlagBits::eTransferDst,
            properties 
        );

        copyBuffer(m_device, queue, *commandPool, stagingBuffer.buffer(), ret.buffer(), size);

        return ret;
    }

	void createRenderPass(const Present& present);

    void createGraphicsPipeline(const Present& present);

    void createFramebuffers(const Present& present);

    void createUniformBuffers(const Present& present);

    void createDescriptorPool(const Present& present);

    void createDescriptorSets(const Present& present);

    void createCommandBuffers(const Present& present);
    
    glm::mat4 m_projection;
    vk::Device m_device;
    vk::PhysicalDevice m_physicalDevice;
};
