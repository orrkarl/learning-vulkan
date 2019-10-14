#pragma once

#include <chrono>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vulkan/vulkan.hpp>

#include "BoundedBuffer.h"
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

    void render(const vk::Semaphore& wait, const vk::Semaphore& signal, const vk::Fence& hostNotify, const uint32_t& imageIndex)
    {
        update(imageIndex);

        const vk::PipelineStageFlags waitStage(vk::PipelineStageFlagBits::eColorAttachmentOutput);
		const vk::SubmitInfo submitInfo(1, &wait, &waitStage, 1, &commandBuffers[imageIndex].get(), 1, &signal);

		queue.submit({ submitInfo }, hostNotify);
    }

    void notifySwapchainUpdated(const vk::Extent2D& extent)
    {
        m_projection = glm::perspective(glm::radians(45.0f), extent.width / static_cast<float>(extent.height), 0.1f, 10.0f);
		m_projection[1][1] *= -1;
    }

    void setDevice(const vk::Device& dev)
    {
        m_device = dev;
    }

private:
	void update(const uint32_t& imageIndex)
	{
		static const auto initTime = std::chrono::high_resolution_clock::now();

		auto currentTime = std::chrono::high_resolution_clock::now();
		float dt = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - initTime).count();

		auto model = glm::rotate(glm::mat4(1.0f), dt * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		auto view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));

		auto transform = mkTransform(model, view, m_projection);

		void* data = m_device.mapMemory(uniforms[imageIndex].memory(), 0, sizeof(MVPTransform));
		memcpy(data, &transform, sizeof(transform));
		m_device.unmapMemory(uniforms[imageIndex].memory());
	}

    glm::mat4 m_projection;
    vk::Device m_device;
};
