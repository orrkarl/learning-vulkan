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

    Graphics(const Graphics& other) = delete;

    Graphics(Graphics&& other);

    ~Graphics();

    Graphics& operator=(const Graphics& other) = delete;

    Graphics& operator=(Graphics&& other);

    void render(const vk::Semaphore& wait, const vk::Semaphore& signal, const vk::Fence& hostNotify, const uint32_t& imageIndex);

    void update(const Present& present);

    void await();

    void reset();

    void release();

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

        copyBuffer(m_device, queue, commandPool, stagingBuffer.buffer(), ret.buffer(), size);

        return ret;
    }

	void createRenderPass(const Present& present);

    void createGraphicsPipeline(const Present& present);

    void createFramebuffers(const Present& present);

    void createUniformBuffers(const Present& present);

    void createDescriptorPool(const Present& present);

    void createDescriptorSets(const Present& present);

    void createCommandBuffers(const Present& present);
    
    vk::CommandPool 				commandPool;
    vk::RenderPass 					renderPass;
    vk::PipelineLayout 				pipelineLayout;
    vk::Pipeline 					pipeline;
    std::vector<vk::CommandBuffer>	commandBuffers;
    std::vector<vk::Framebuffer>	frameBuffers;
    vk::DescriptorSetLayout			descriptorSetLayout;
    vk::DescriptorPool				descriptorPool;
    std::vector<vk::DescriptorSet> 	descriptorSets;
    BoundedBuffer					deviceVertecies;
    BoundedBuffer					deviceIndices;
    std::vector<BoundedBuffer>		uniforms;
    vk::Queue 						queue;
    glm::mat4                       m_projection;
    vk::Device                      m_device;
    vk::PhysicalDevice              m_physicalDevice;
};
