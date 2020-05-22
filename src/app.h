#pragma once

#include <vulkan/vulkan.hpp>

#include "config.h"
#include "util/util.h"

class VkError : public std::runtime_error
{
public:
	VkError(const char *msg, const VkResult errcode);

	const VkResult status;
};

class VkLayerNotFoundError : public std::exception
{
public:
	VkLayerNotFoundError(const char *layerName);

	const char *what() const noexcept override;

	const char *layer() const;

private:
	const char *m_layerName;
	const std::string m_errorMessage;
};

class VkExtensionNotFoundError : public std::runtime_error
{
public:
	VkExtensionNotFoundError(const char *extensionName);

	const char *extensions() const;

private:
	const char *m_extensionName;
};

struct Vertex
{
	glm::vec2 pos;
	glm::vec3 color;

	static vk::VertexInputBindingDescription getBindingDescription();

	static std::array<vk::VertexInputAttributeDescription, 2> getAttributeDescription();
};

class HelloTriangleApp
{
public:
	void run();

	~HelloTriangleApp();

private:
	static void glfwFramebufferResize(GLFWwindow* window, int w, int h);

	void initWindow();

	void createInstance();

	void checkValidationLayerSupport();

	void setupDebugMessenger();

	void createRenderSurface();

	void pickPhysicalDevice();

	void createLogicalDevice();

	void createSwapChain();

	vk::UniqueImageView createImageView(vk::Image image, vk::Format format);

	void createImageViews();

	vk::UniqueShaderModule createShaderModule(const std::vector<char> &code);

	void createRenderPass();

	void createDescriptorSetLayout();

	void createGraphicsPipeline();

	void createFramebuffers();

	void createCommandPool();

	void createCommandBuffers();

	void createSyncObjects();

	void recreateSwapchain();

	void copyBuffer(const vk::Buffer& src, const vk::Buffer& dest, const vk::DeviceSize& size);

	template <class Container>
	BoundedBuffer createStagedBuffer(const Container& hostData, const vk::BufferUsageFlags& usage, const vk::MemoryPropertyFlags& properties);

	void createVertexBuffers();

	void createUniformBuffers();

	void createDescriptorPool();

	void createDescriptorSets();

	vk::UniqueCommandBuffer beginSingleTimeCommand();

    void applyGraphicsCmd(vk::CommandBuffer cmdBuffer);

	void transitionImageLayout(vk::Image image, vk::Format format, vk::ImageLayout oldLayout, vk::ImageLayout newLayout);

	void copyBufferToImage(vk::Buffer srcBuffer, vk::Image dstImage, uint32_t imageWidth, uint32_t imageHeight);

    void createTextureImage();

	void createTextureView();

	void initVulkan();

	void updateUniformBuffer(const BoundedBuffer& deviceUniform);

	void drawFrame();

	void mainLoop();

// Order of fields is important for destructors
	vk::UniqueInstance 						m_instance;
	vk::UniqueSurfaceKHR 					m_renderSurface;
	vk::UniqueHandle<
		vk::DebugUtilsMessengerEXT, 
		vk::DispatchLoaderDynamic> 			m_debugMessenger;
	vk::UniqueDevice 						m_device;
	vk::UniqueSwapchainKHR  				m_swapChain;
	std::vector<vk::UniqueImageView> 		m_swapChainImageViews;
	vk::UniqueCommandPool 					m_commandPool;
	std::vector<vk::UniqueSemaphore> 		m_imageAvailable;
	std::vector<vk::UniqueFence> 			m_inFlightImages;
	std::vector<vk::UniqueSemaphore>		m_renderCompleted;
	BoundedBuffer							m_deviceVertecies;
	BoundedBuffer							m_deviceIndices;
    BoundImage		                        m_statueTexture;
	vk::UniqueImageView						m_statueTextureView;
	std::vector<BoundedBuffer>				m_uniforms;
	vk::UniqueRenderPass 					m_renderPass;
	vk::UniquePipelineLayout 				m_pipelineLayout;
	vk::UniquePipeline 						m_pipeline;
	std::vector<vk::UniqueCommandBuffer>	m_commandBuffers;
	std::vector<vk::UniqueFramebuffer>		m_frameBuffers;
	vk::UniqueDescriptorSetLayout			m_descriptorSetLayout;
	vk::UniqueDescriptorPool				m_descriptorPool;
	std::vector<vk::DescriptorSet> 			m_descriptorSets;

	vk::Queue								m_computeQueue;
	int 									m_currentFrame;
	vk::DispatchLoaderDynamic 				m_dispatchDynamic;
	vk::Queue 								m_graphicsQueue;
	vk::PhysicalDevice 						m_physicalDevice;
	vk::Queue 	 							m_presentQueue;
	vk::Extent2D 							m_swapChainExtent;
	vk::Format   							m_swapChainImageFormat;
	std::vector<vk::Image>		 			m_swapChainImages;
	GLFWwindow*								m_window;
	bool									m_windowSizeChanged;
};

#include "app.inl"