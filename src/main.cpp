#include <array>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <functional>
#include <iostream>
#include <limits>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "config.h"
#include "util/util.h"

class VkError : public std::runtime_error
{
public:
	VkError(const char *msg, const VkResult errcode)
		: std::runtime_error(msg), status(errcode)
	{
	}

	const VkResult status;
};

class VkLayerNotFoundError : public std::exception
{
public:
	VkLayerNotFoundError(const char *layerName)
		: std::exception(), m_layerName(layerName), m_errorMessage(std::string("Validation layer not found: ") + layerName)
	{
	}

	const char *what() const noexcept override
	{
		return m_errorMessage.c_str();
	}

	const char *layer() const
	{
		return m_layerName;
	}

private:
	const char *m_layerName;
	const std::string m_errorMessage;
};

class VkExtensionNotFoundError : public std::runtime_error
{
public:
	VkExtensionNotFoundError(const char *extensionName)
		: std::runtime_error(std::string("Validation layer not found: ") + extensionName), m_extensionName(extensionName)
	{
	}

	const char *extensions() const
	{
		return m_extensionName;
	}

private:
	const char *m_extensionName;
};

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

const std::array<Vertex, 4> g_vertecies
{
    Vertex{ {-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
	Vertex{ { 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
    Vertex{ { 0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}},
    Vertex{ {-0.5f,  0.5f}, {1.0f, 1.0f, 1.0f}}
};

const std::array<uint16_t, 6> g_indices
{
	0, 1, 2, 2, 3, 0
};

class HelloTriangleApp
{
public:
	void run()
	{
		initWindow();
		initVulkan();
		mainLoop();
	}

	~HelloTriangleApp()
	{
		m_device->waitIdle();

		glfwDestroyWindow(m_window);
		glfwTerminate();
	}

private:
	static void glfwFramebufferResize(GLFWwindow* window, int w, int h)
	{
		auto app = reinterpret_cast<HelloTriangleApp*>(glfwGetWindowUserPointer(window));
		app->m_windowSizeChanged = true;
	}

	void initWindow()
	{
		glfwInit();
		glfwSetErrorCallback(&onGLFWError);

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

		m_window = glfwCreateWindow(config::WIDTH, config::HEIGHT, config::NAME, nullptr, nullptr);
		glfwSetWindowUserPointer(m_window, this);
		glfwSetKeyCallback(m_window, &onKeyPress);
		glfwSetFramebufferSizeCallback(m_window, &glfwFramebufferResize);
	}

	void createInstance()
	{
		vk::ApplicationInfo appInfo(
			config::NAME,
			VK_MAKE_VERSION(1, 0, 0),
			"No Engine",
			VK_MAKE_VERSION(1, 0, 0),
			VK_API_VERSION_1_0
		);

		auto exts = getRequiredExtensions();
		vk::InstanceCreateInfo instInfo(
			vk::InstanceCreateFlags(),
			&appInfo,
			config::VALIDATION_LAYERS.size(), config::VALIDATION_LAYERS.data(),
			exts.size(), exts.data()
		);

		vk::DebugUtilsMessengerCreateInfoEXT debugInfo(
			vk::DebugUtilsMessengerCreateFlagsEXT(),
			vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
			vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
			debugCallback, nullptr
		);
		instInfo.pNext = &debugInfo;
		
		m_instance = vk::createInstanceUnique(instInfo);
		m_dispatchDynamic = vk::DispatchLoaderDynamic(m_instance.get(), vkGetInstanceProcAddr);
	}

	void checkValidationLayerSupport()
	{
		auto layers = vk::enumerateInstanceLayerProperties();

		for (auto layerName : config::VALIDATION_LAYERS)
		{
			bool isFound = false;
			for (const auto &layerProperties : layers)
			{
				if (!strcmp(layerName, layerProperties.layerName))
				{
					isFound = true;
					break;
				}
			}

			if (!isFound)
			{
				throw VkLayerNotFoundError(layerName);
			}
		}
	}

	void setupDebugMessenger()
	{
		vk::DebugUtilsMessengerCreateInfoEXT createInfo(
			vk::DebugUtilsMessengerCreateFlagsEXT(),
			vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
			vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
			debugCallback, nullptr
		);

		m_debugMessenger = m_instance->createDebugUtilsMessengerEXTUnique(createInfo, nullptr, m_dispatchDynamic);
	}

	void createRenderSurface()
	{
		VkSurfaceKHR surface;
		auto status = glfwCreateWindowSurface(
			static_cast<VkInstance>(m_instance.get()), 
			m_window, 
			nullptr, 
			&surface
		);
		if (status != VK_SUCCESS)
		{
			throw VkError("Could not create render surface", status);
		}

		m_renderSurface = vk::UniqueSurfaceKHR(
			vk::SurfaceKHR(surface), 
			vk::ObjectDestroy(m_instance.get(), nullptr, vk::DispatchLoaderStatic())
		);
	}

	void pickPhysicalDevice()
	{
		auto devices = m_instance->enumeratePhysicalDevices();

		auto deviceCount = devices.size();
		if (deviceCount == 0)
		{
			throw std::runtime_error("Failed to find GPUs with Vulkan support!");
		}

		for (const auto &device : devices)
		{
			if (isDeviceSuitable(device, m_renderSurface.get(), config::DEVICE_EXTENSIONS))
			{
				m_physicalDevice = device;
				return;
			}
		}

		throw std::runtime_error("failed to find a suitable GPU!");
	}

	void createLogicalDevice()
	{
		auto indices = QueueFamilyIndices(m_physicalDevice, *m_renderSurface);

		std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;

		std::set<uint32_t> uniqueQueueFamilies = {indices.graphics(), indices.present()};
		float queuePriority = 1.0f;
		vk::DeviceQueueCreateInfo queueCreateInfo(
			vk::DeviceQueueCreateFlags(),
			0, 
			1, &queuePriority
		);
		for (const auto &family : uniqueQueueFamilies)
		{
			queueCreateInfo.queueFamilyIndex = family;
			queueCreateInfos.push_back(queueCreateInfo);
		}

		vk::PhysicalDeviceFeatures deviceFeatures;

		vk::DeviceCreateInfo createInfo(
			vk::DeviceCreateFlags(),
			queueCreateInfos.size(), queueCreateInfos.data(),
			config::VALIDATION_LAYERS.size(), config::VALIDATION_LAYERS.data(),
			config::DEVICE_EXTENSIONS.size(), config::DEVICE_EXTENSIONS.data(),
			&deviceFeatures
		);
		m_device = m_physicalDevice.createDeviceUnique(createInfo);

		m_computeQueue = m_device->getQueue(indices.compute(), 0);
		m_graphics.queue = m_device->getQueue(indices.graphics(), 0);
		m_present.queue = m_device->getQueue(indices.present(), 0);
	}

	void createSwapChain()
	{
		auto support = SwapChainSupportDetails(m_physicalDevice, m_renderSurface.get());

		auto format = support.chooseFormat();
		auto presentationMode = support.choosePresentMode();
		auto extent = support.chooseExtent(m_window);

		auto imageCount = support.chooseImageCount();

		vk::SwapchainCreateInfoKHR chainInfo(
			vk::SwapchainCreateFlagsKHR(),
			m_renderSurface.get(),
			imageCount,
			format.format,
			format.colorSpace,
			extent,
			1,
			vk::ImageUsageFlagBits::eColorAttachment
		);

		QueueFamilyIndices indices = QueueFamilyIndices(m_physicalDevice, *m_renderSurface);
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

		m_present.swapChain = m_device->createSwapchainKHRUnique(chainInfo);
		m_present.swapChainExtent = extent;
		m_present.swapChainImageFormat = format.format;

		m_present.swapChainImages = m_device->getSwapchainImagesKHR(m_present.swapChain.get());
	}

	void createImageViews()
	{
		vk::ImageViewCreateInfo createInfo(
			vk::ImageViewCreateFlags(),
			vk::Image(),
			vk::ImageViewType::e2D,
			m_present.swapChainImageFormat,
			vk::ComponentMapping(),
			vk::ImageSubresourceRange(
				vk::ImageAspectFlagBits::eColor,
				0,
				1,
				0,
				1
			)
		);

		m_present.swapChainImageViews.resize(m_present.swapChainImages.size());
		for (size_t i = 0; i < m_present.swapChainImageViews.size(); i++)
		{
			createInfo.image = m_present.swapChainImages[i];
			m_present.swapChainImageViews[i] = m_device->createImageViewUnique(createInfo);
		}
	}

	vk::UniqueShaderModule createShaderModule(const std::vector<char> &code)
	{
		vk::ShaderModuleCreateInfo shaderInfo(
			vk::ShaderModuleCreateFlags(),
			code.size(), 
			reinterpret_cast<const uint32_t *>(code.data())
		);

		return m_device->createShaderModuleUnique(shaderInfo);
	}

	void createRenderPass()
	{
		vk::AttachmentDescription color(
			vk::AttachmentDescriptionFlags(),
			m_present.swapChainImageFormat,
			vk::SampleCountFlagBits::e1,
			vk::AttachmentLoadOp::eClear,
			vk::AttachmentStoreOp::eStore,
			vk::AttachmentLoadOp::eDontCare,
			vk::AttachmentStoreOp::eDontCare,
			vk::ImageLayout::eUndefined,
			vk::ImageLayout::ePresentSrcKHR
		);

		vk::AttachmentReference colorRef(0, vk::ImageLayout::eColorAttachmentOptimal);

		vk::SubpassDescription subpassDesc;
		subpassDesc.colorAttachmentCount = 1;
		subpassDesc.pColorAttachments = &colorRef;

		vk::SubpassDependency dependency(
			VK_SUBPASS_EXTERNAL, 0,
			vk::PipelineStageFlags(vk::PipelineStageFlagBits::eColorAttachmentOutput), vk::PipelineStageFlags(vk::PipelineStageFlagBits::eColorAttachmentOutput),
			vk::AccessFlags(), vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite
		);

		vk::RenderPassCreateInfo renderPassInfo(
			vk::RenderPassCreateFlags(),
			1, &color,
			1, &subpassDesc,
			1, &dependency
		);

		m_graphics.renderPass = m_device->createRenderPassUnique(renderPassInfo);
	}

	void createDescriptorSetLayout()
	{
		vk::DescriptorSetLayoutBinding mvpLayoutBinding(
			0, 
			vk::DescriptorType::eUniformBuffer, 
			1, 
			vk::ShaderStageFlagBits::eVertex
		);

		m_graphics.descriptorSetLayout = m_device->createDescriptorSetLayoutUnique(
			vk::DescriptorSetLayoutCreateInfo(
				vk::DescriptorSetLayoutCreateFlags(), 
				1, &mvpLayoutBinding
			)
		);
	}

	void createGraphicsPipeline()
	{
		auto vertShaderCode = readFile("vert.spv");
		auto fragShaderCode = readFile("frag.spv");

		auto fragShader = createShaderModule(fragShaderCode);
		auto vertShader = createShaderModule(vertShaderCode);

		vk::PipelineShaderStageCreateInfo vertInfo(
			vk::PipelineShaderStageCreateFlags(), 
			vk::ShaderStageFlagBits::eVertex, 
			vertShader.get(), 
			"main"
		);

		vk::PipelineShaderStageCreateInfo fragInfo(
			vk::PipelineShaderStageCreateFlags(), 
			vk::ShaderStageFlagBits::eFragment, 
			fragShader.get(), 
			"main"
		);

		vk::PipelineShaderStageCreateInfo shaderStages[] = {vertInfo, fragInfo};

		auto bindingDesc = Vertex::getBindingDescription();
		auto attributeDesc = Vertex::getAttributeDescription();

		vk::PipelineVertexInputStateCreateInfo vertexInput(
			vk::PipelineVertexInputStateCreateFlags(),
			1, &bindingDesc,
			attributeDesc.size(), attributeDesc.data()
		);

		vk::PipelineInputAssemblyStateCreateInfo inputAssembly(
			vk::PipelineInputAssemblyStateCreateFlags(),
			vk::PrimitiveTopology::eTriangleList,
			VK_FALSE
		);

		vk::Viewport viewport(
			0, 0, 
			static_cast<float>(m_present.swapChainExtent.width), static_cast<float>(m_present.swapChainExtent.height),
			0.0f, 1.0f
		);

		vk::Rect2D scissor(
			vk::Offset2D(0, 0), 
			m_present.swapChainExtent
		);

		vk::PipelineViewportStateCreateInfo viewportState(
			vk::PipelineViewportStateCreateFlags(), 
			1, &viewport, 
			1, &scissor
		);

		vk::PipelineRasterizationStateCreateInfo rasterizerState(
			vk::PipelineRasterizationStateCreateFlags(),
			VK_FALSE,
			VK_FALSE,
			vk::PolygonMode::eFill,
			vk::CullModeFlagBits::eBack,
			vk::FrontFace::eCounterClockwise,
			VK_FALSE,
			0.0f, 0.0f, 0.0f,
			1.0f
		);

		vk::PipelineMultisampleStateCreateInfo multisamplingState;

		vk::PipelineColorBlendAttachmentState colorBlendAttachment;
		colorBlendAttachment.setColorWriteMask(
			vk::ColorComponentFlagBits::eR | 
			vk::ColorComponentFlagBits::eG | 
			vk::ColorComponentFlagBits::eB | 
			vk::ColorComponentFlagBits::eA  
		);

		vk::PipelineColorBlendStateCreateInfo colorBlending;
		colorBlending.setAttachmentCount(1);
		colorBlending.setPAttachments(&colorBlendAttachment);

		vk::PipelineLayoutCreateInfo pipelineLayoutInfo;
		pipelineLayoutInfo.setSetLayoutCount(1);
		pipelineLayoutInfo.setPSetLayouts(&m_graphics.descriptorSetLayout.get());

		m_graphics.pipelineLayout = m_device->createPipelineLayoutUnique(pipelineLayoutInfo);

		vk::GraphicsPipelineCreateInfo graphicsInfo(
			vk::PipelineCreateFlags(),
			2, shaderStages,
			&vertexInput,
			&inputAssembly,
			nullptr,
			&viewportState,
			&rasterizerState,
			&multisamplingState,
			nullptr,
			&colorBlending,
			nullptr,
			m_graphics.pipelineLayout.get(),
			m_graphics.renderPass.get()
		);

		m_graphics.pipeline = m_device->createGraphicsPipelineUnique(vk::PipelineCache(), graphicsInfo);
	}

	void createFramebuffers()
	{
		m_graphics.frameBuffers.resize(m_present.swapChainImages.size());

		vk::FramebufferCreateInfo framebufferInfo(
			vk::FramebufferCreateFlags(), 
			m_graphics.renderPass.get(), 
			1, nullptr, 
			m_present.swapChainExtent.width, m_present.swapChainExtent.height, 
			1
		);

		for (auto i = 0u; i < m_present.swapChainImageViews.size(); ++i)
		{
			framebufferInfo.pAttachments = &m_present.swapChainImageViews[i].get();
			m_graphics.frameBuffers[i] = m_device->createFramebufferUnique(framebufferInfo);
		}
	}

	void createCommandPool()
	{
		auto indices = QueueFamilyIndices(m_physicalDevice, *m_renderSurface);
		vk::CommandPoolCreateInfo commandPoolInfo(vk::CommandPoolCreateFlags(), indices.graphics());
		m_graphics.commandPool = m_device->createCommandPoolUnique(commandPoolInfo);
	}

	void createCommandBuffers()
	{
		vk::CommandBufferAllocateInfo allocInfo(
			m_graphics.commandPool.get(), 
			vk::CommandBufferLevel::ePrimary, 
			static_cast<uint32_t>(m_present.swapChainImageViews.size())
		);

		m_graphics.commandBuffers = m_device->allocateCommandBuffersUnique(allocInfo);

		vk::CommandBufferBeginInfo commandBufferBegin{};

		vk::Buffer vertexBuffers[] = { m_graphics.deviceVertecies.buffer() };
		vk::DeviceSize vertexOffsets[] = { 0 };

		vk::RenderPassBeginInfo renderPassBegin;
		renderPassBegin.renderPass = m_graphics.renderPass.get();
		renderPassBegin.renderArea.offset = vk::Offset2D(0, 0);
		renderPassBegin.renderArea.extent = m_present.swapChainExtent;
		vk::ClearValue clearColor(vk::ClearColorValue(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f}));
		renderPassBegin.clearValueCount = 1;
		renderPassBegin.pClearValues = &clearColor;

		for (auto i = 0u; i < m_present.swapChainImageViews.size(); ++i)
		{
			renderPassBegin.framebuffer = m_graphics.frameBuffers[i].get();

			m_graphics.commandBuffers[i]->begin(commandBufferBegin);
				m_graphics.commandBuffers[i]->beginRenderPass(renderPassBegin, vk::SubpassContents::eInline);
				m_graphics.commandBuffers[i]->bindPipeline(vk::PipelineBindPoint::eGraphics, m_graphics.pipeline.get());
				m_graphics.commandBuffers[i]->bindVertexBuffers(0, 1, vertexBuffers, vertexOffsets);
				m_graphics.commandBuffers[i]->bindIndexBuffer(m_graphics.deviceIndices.buffer(), 0, vk::IndexType::eUint16);
				m_graphics.commandBuffers[i]->bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *m_graphics.pipelineLayout, 0, {m_graphics.descriptorSets[i]}, {});
				m_graphics.commandBuffers[i]->drawIndexed(static_cast<uint32_t>(g_indices.size()), 1, 0, 0, 0);
				m_graphics.commandBuffers[i]->endRenderPass();
			m_graphics.commandBuffers[i]->end();
		}
	}

	void createSyncObjects()
	{
		auto size = m_present.swapChainImages.size();

		imageAvailable.resize(size);
		renderCompleted.resize(size);
		inFlightImages.resize(size);

		auto semaphoreInfo = vk::SemaphoreCreateInfo();
		vk::FenceCreateInfo fenceInfo(vk::FenceCreateFlags(vk::FenceCreateFlagBits::eSignaled));

		for (auto i = 0u; i < size; ++i)
		{
			imageAvailable[i] = m_device->createSemaphoreUnique(semaphoreInfo);
			renderCompleted[i] = m_device->createSemaphoreUnique(semaphoreInfo);
			inFlightImages[i] = m_device->createFenceUnique(fenceInfo);
		}
	}

	void recreateSwapchain()
	{
		int w = 0, h = 0;
		while (w == 0 and h == 0)
		{
			glfwGetFramebufferSize(m_window, &w, &h);
			glfwWaitEvents();
		}

		m_device->waitIdle();

		m_graphics.commandBuffers.clear();
		m_graphics.descriptorPool.reset();
		m_graphics.uniforms.clear();
		m_graphics.frameBuffers.clear();
		m_graphics.pipeline.reset();
		m_graphics.renderPass.reset();
		m_present.swapChainImageViews.clear();
		m_present.swapChain.reset();

		createSwapChain();
		createImageViews();
		createRenderPass();
		createGraphicsPipeline();
		createFramebuffers();
		createUniformBuffers();
		createDescriptorPool();
		createDescriptorSets();
		createCommandBuffers();
	}

	void copyBuffer(const vk::Buffer& src, const vk::Buffer& dest, const vk::DeviceSize& size)
	{
		auto copyCommand = vk::UniqueCommandBuffer(
			m_device->allocateCommandBuffers(
				vk::CommandBufferAllocateInfo(
					*m_graphics.commandPool,
					vk::CommandBufferLevel::ePrimary,
					1
				)
			)[0],
			vk::PoolFree(
				*m_device, 
				*m_graphics.commandPool, 
				vk::DispatchLoaderStatic()
			)
		);

		copyCommand->begin(
			vk::CommandBufferBeginInfo(
				vk::CommandBufferUsageFlagBits::eOneTimeSubmit
			)
		);
		copyCommand->copyBuffer(src, dest, { vk::BufferCopy(0, 0, size) });
		copyCommand->end();

		auto copyFence = m_device->createFenceUnique(vk::FenceCreateInfo());
		vk::SubmitInfo submitInfo;
		submitInfo.setCommandBufferCount(1);
		submitInfo.setPCommandBuffers(&copyCommand.get());
		m_graphics.queue.submit(1, &submitInfo, *copyFence);

		m_device->waitForFences(1, &copyFence.get(), VK_TRUE, std::numeric_limits<uint64_t>::max());
	}

	template <class Container>
	BoundedBuffer createStagedBuffer(const Container& hostData, const vk::BufferUsageFlags& usage, const vk::MemoryPropertyFlags& properties)
	{
		auto size = sizeof(hostData[0]) * hostData.size();

		auto stagingBuffer = BoundedBuffer(
			m_physicalDevice, *m_device, 
			hostData, vk::BufferUsageFlagBits::eTransferSrc, 
			vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
		);

		auto ret = BoundedBuffer(
			m_physicalDevice, *m_device,
			size, usage | vk::BufferUsageFlagBits::eTransferDst,
			properties 
		);

		copyBuffer(stagingBuffer.buffer(), ret.buffer(), size);

		return ret;
	}

	void createVertexBuffers()
	{
		m_graphics.deviceVertecies = createStagedBuffer(g_vertecies, vk::BufferUsageFlagBits::eVertexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal);
		m_graphics.deviceIndices = createStagedBuffer(g_indices, vk::BufferUsageFlagBits::eIndexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal);
	}

	void createUniformBuffers()
	{
		m_graphics.uniforms.resize(m_present.swapChainImages.size());

		auto bufferSize = sizeof(MVPTransform);
		for (auto i = 0; i < m_graphics.uniforms.size(); ++i)
		{
			m_graphics.uniforms[i] = BoundedBuffer(
				m_physicalDevice, *m_device, 
				bufferSize, vk::BufferUsageFlagBits::eUniformBuffer, 
				vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible
			);
		}
	}

	void createDescriptorPool()
	{
		vk::DescriptorPoolSize poolSize(vk::DescriptorType::eUniformBuffer, m_present.swapChainImages.size());
		vk::DescriptorPoolCreateInfo poolInfo(vk::DescriptorPoolCreateFlags(), m_present.swapChainImages.size(), 1, &poolSize);

		m_graphics.descriptorPool = m_device->createDescriptorPoolUnique(poolInfo);
	}

	void createDescriptorSets()
	{
		const std::vector<vk::DescriptorSetLayout> layouts(m_present.swapChainImages.size(), *m_graphics.descriptorSetLayout);
		vk::DescriptorSetAllocateInfo allocInfo(*m_graphics.descriptorPool, layouts.size(), layouts.data());
		m_graphics.descriptorSets = m_device->allocateDescriptorSets(allocInfo);

		vk::DescriptorBufferInfo bufferInfo(vk::Buffer(), 0, sizeof(MVPTransform));
		vk::WriteDescriptorSet descriptorWrite(vk::DescriptorSet(), 0, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr, &bufferInfo);
		for (auto i = 0u; i < m_present.swapChainImages.size(); ++i)
		{
			bufferInfo.setBuffer(m_graphics.uniforms[i].buffer());
			descriptorWrite.setDstSet(m_graphics.descriptorSets[i]);

			m_device->updateDescriptorSets({descriptorWrite}, {});
		}
	}

	void initVulkan()
	{
		createInstance();
		
		checkValidationLayerSupport();
		setupDebugMessenger();
		
		createRenderSurface();
		pickPhysicalDevice();
		createLogicalDevice();
		createSwapChain();
		createImageViews();
		createRenderPass();
		createDescriptorSetLayout();
		createGraphicsPipeline();
		createFramebuffers();
		createCommandPool();
		createVertexBuffers();
		createUniformBuffers();
		createDescriptorPool();
		createDescriptorSets();
		createCommandBuffers();
		createSyncObjects();
	}

	void updateUniformBuffer(const BoundedBuffer& deviceUniform)
	{
		static const auto initTime = std::chrono::high_resolution_clock::now();

		auto currentTime = std::chrono::high_resolution_clock::now();
		float dt = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - initTime).count();

		auto model = glm::rotate(glm::mat4(1.0f), dt * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		auto view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		auto proj = glm::perspective(glm::radians(45.0f), m_present.swapChainExtent.width / static_cast<float>(m_present.swapChainExtent.height), 0.1f, 10.0f);
		proj[1][1] *= -1;

		auto transform = mkTransform(model, view, proj);

		void* data = m_device->mapMemory(deviceUniform.memory(), 0, sizeof(MVPTransform));
		memcpy(data, &transform, sizeof(transform));
		m_device->unmapMemory(deviceUniform.memory());
	}

	void drawFrame()
	{
		m_device->waitForFences(1, &inFlightImages[m_currentFrame].get(), VK_TRUE, std::numeric_limits<uint64_t>::max());
		const vk::Semaphore* waitSemaphore = &imageAvailable[m_currentFrame].get();
		const vk::Semaphore* signalSemaphore = &renderCompleted[m_currentFrame].get();

		uint32_t imageIndex;
		auto status = m_device->acquireNextImageKHR(m_present.swapChain.get(), std::numeric_limits<uint64_t>::max(), imageAvailable[m_currentFrame].get(), vk::Fence(), &imageIndex);
		if (status == vk::Result::eErrorOutOfDateKHR)
		{
			recreateSwapchain();
			return;
		}
		
		else if (status != vk::Result::eSuccess and status != vk::Result::eSuboptimalKHR)
		{
			vk::throwResultException(status, "could not aquire next image");
		}

		const vk::PipelineStageFlags waitStage(vk::PipelineStageFlagBits::eColorAttachmentOutput);
		const vk::SubmitInfo submitInfo(1, waitSemaphore, &waitStage, 1, &m_graphics.commandBuffers[imageIndex].get(), 1, signalSemaphore);

		m_device->resetFences(1, &inFlightImages[m_currentFrame].get());

		updateUniformBuffer(m_graphics.uniforms[imageIndex]);

		m_graphics.queue.submit({ submitInfo }, inFlightImages[m_currentFrame].get());

		vk::PresentInfoKHR presentInfo(1, signalSemaphore, 1, &m_present.swapChain.get(), &imageIndex);

		status = m_present.queue.presentKHR(&presentInfo);
		if (status == vk::Result::eErrorOutOfDateKHR or status == vk::Result::eSuboptimalKHR)
		{
			recreateSwapchain();
		}
		else if (status != vk::Result::eSuccess)
		{
			vk::throwResultException(status, "could not present queue!");
		}

		m_currentFrame = (m_currentFrame + 1) % config::MAX_FRAMES_IN_FLIGHT;
	}

	void mainLoop()
	{
		while (!glfwWindowShouldClose(m_window))
		{
			drawFrame();
			glfwPollEvents();
		}

		m_graphics.queue.waitIdle();
		m_present.queue.waitIdle();
	}

// Order of fields is important for destructors
	vk::UniqueInstance 						m_instance;
	vk::UniqueSurfaceKHR 					m_renderSurface;
	vk::UniqueHandle<
		vk::DebugUtilsMessengerEXT, 
		vk::DispatchLoaderDynamic> 			m_debugMessenger;
	vk::UniqueDevice 						m_device;
	
	Present m_present;
	Graphics m_graphics;

    std::vector<vk::UniqueSemaphore> 		imageAvailable;
    std::vector<vk::UniqueFence> 			inFlightImages;
    std::vector<vk::UniqueSemaphore>		renderCompleted;
	
	vk::Queue								m_computeQueue;
	int 									m_currentFrame;
	vk::DispatchLoaderDynamic 				m_dispatchDynamic;
	vk::PhysicalDevice 						m_physicalDevice;
	GLFWwindow*								m_window;
	bool									m_windowSizeChanged;

};

int main()
{
	auto app = HelloTriangleApp();

	try
	{
		app.run();
	}
	catch (const VkError &ex)
	{
		std::cerr << "Error while running app: " << ex.what() << "(" << ex.status << ")\n";
		return EXIT_FAILURE;
	}
	catch (const std::exception &err)
	{
		std::cerr << err.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
