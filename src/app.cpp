#include "app.h"

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

#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>


VkError::VkError(const char *msg, const VkResult errcode)
    : std::runtime_error(msg), status(errcode)
{
}

VkLayerNotFoundError::VkLayerNotFoundError(const char *layerName)
    : std::exception(), m_layerName(layerName), m_errorMessage(std::string("Validation layer not found: ") + layerName)
{
}

const char* VkLayerNotFoundError::what() const noexcept
{
    return m_errorMessage.c_str();
}

const char* VkLayerNotFoundError::layer() const
{
    return m_layerName;
}

VkExtensionNotFoundError::VkExtensionNotFoundError(const char *extensionName)
    : std::runtime_error(std::string("Validation layer not found: ") + extensionName), m_extensionName(extensionName)
{
}

const char* VkExtensionNotFoundError::extensions() const
{
    return m_extensionName;
}

vk::VertexInputBindingDescription Vertex::getBindingDescription()
{
    return vk::VertexInputBindingDescription(0, sizeof(Vertex));
}

std::array<vk::VertexInputAttributeDescription, 2> Vertex::getAttributeDescription()
{
    return {
        vk::VertexInputAttributeDescription(0, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, pos)),
        vk::VertexInputAttributeDescription(1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, color))
    };
}


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

void HelloTriangleApp::run()
{
    initWindow();
    initVulkan();
    mainLoop();
}

HelloTriangleApp::~HelloTriangleApp()
{
    m_device->waitIdle();

    glfwDestroyWindow(m_window);
    glfwTerminate();
}

    void HelloTriangleApp::glfwFramebufferResize(GLFWwindow* window, int w, int h)
{
    auto app = reinterpret_cast<HelloTriangleApp*>(glfwGetWindowUserPointer(window));
    app->m_windowSizeChanged = true;
}

void HelloTriangleApp::initWindow()
{
    glfwInit();
    glfwSetErrorCallback(&onGLFWError);

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    m_window = glfwCreateWindow(config::WIDTH, config::HEIGHT, config::NAME, nullptr, nullptr);
    glfwSetWindowUserPointer(m_window, this);
    glfwSetKeyCallback(m_window, &onKeyPress);
    glfwSetFramebufferSizeCallback(m_window, &glfwFramebufferResize);
}

void HelloTriangleApp::createInstance()
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

void HelloTriangleApp::checkValidationLayerSupport()
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

void HelloTriangleApp::setupDebugMessenger()
{
    vk::DebugUtilsMessengerCreateInfoEXT createInfo(
        vk::DebugUtilsMessengerCreateFlagsEXT(),
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning 
            | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
        vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral 
            | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation 
            | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
        debugCallback, nullptr
    );

    m_debugMessenger = m_instance->createDebugUtilsMessengerEXTUnique(createInfo, nullptr, m_dispatchDynamic);
}

void HelloTriangleApp::createRenderSurface()
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

void HelloTriangleApp::pickPhysicalDevice()
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

void HelloTriangleApp::createLogicalDevice()
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
    deviceFeatures.samplerAnisotropy = VK_TRUE;

    vk::DeviceCreateInfo createInfo(
        vk::DeviceCreateFlags(),
        queueCreateInfos.size(), queueCreateInfos.data(),
        config::VALIDATION_LAYERS.size(), config::VALIDATION_LAYERS.data(),
        config::DEVICE_EXTENSIONS.size(), config::DEVICE_EXTENSIONS.data(),
        &deviceFeatures
    );
    m_device = m_physicalDevice.createDeviceUnique(createInfo);

    m_computeQueue = m_device->getQueue(indices.compute(), 0);
    m_graphicsQueue = m_device->getQueue(indices.graphics(), 0);
    m_presentQueue = m_device->getQueue(indices.present(), 0);
}

void HelloTriangleApp::createSwapChain()
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

    m_swapChain = m_device->createSwapchainKHRUnique(chainInfo);
    m_swapChainExtent = extent;
    m_swapChainImageFormat = format.format;

    m_swapChainImages = m_device->getSwapchainImagesKHR(m_swapChain.get());
}

vk::UniqueImageView HelloTriangleApp::createImageView(vk::Image image, vk::Format format)
{
    vk::ImageViewCreateInfo createInfo(
        vk::ImageViewCreateFlags(),
        image,
        vk::ImageViewType::e2D,
        format,
        vk::ComponentMapping(),
        vk::ImageSubresourceRange(
            vk::ImageAspectFlagBits::eColor,
            0,
            1,
            0,
            1
        )
    );

    return m_device->createImageViewUnique(createInfo);
}

void HelloTriangleApp::createImageViews()
{
    m_swapChainImageViews.resize(m_swapChainImages.size());
 
    for (size_t i = 0; i < m_swapChainImageViews.size(); i++)
    {
        m_swapChainImageViews[i] = createImageView(m_swapChainImages[i], m_swapChainImageFormat);
    }
}

vk::UniqueShaderModule HelloTriangleApp::createShaderModule(const std::vector<char> &code)
{
    vk::ShaderModuleCreateInfo shaderInfo(
        vk::ShaderModuleCreateFlags(),
        code.size(), 
        reinterpret_cast<const uint32_t *>(code.data())
    );

    return m_device->createShaderModuleUnique(shaderInfo);
}

void HelloTriangleApp::createRenderPass()
{
    vk::AttachmentDescription color(
        vk::AttachmentDescriptionFlags(),
        m_swapChainImageFormat,
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

    m_renderPass = m_device->createRenderPassUnique(renderPassInfo);
}

void HelloTriangleApp::createDescriptorSetLayout()
{
    vk::DescriptorSetLayoutBinding mvpLayoutBinding(
        0, 
        vk::DescriptorType::eUniformBuffer, 
        1, 
        vk::ShaderStageFlagBits::eVertex
    );

    m_descriptorSetLayout = m_device->createDescriptorSetLayoutUnique(
        vk::DescriptorSetLayoutCreateInfo(
            vk::DescriptorSetLayoutCreateFlags(), 
            1, &mvpLayoutBinding
        )
    );
}

void HelloTriangleApp::createGraphicsPipeline()
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
        static_cast<float>(m_swapChainExtent.width), static_cast<float>(m_swapChainExtent.height),
        0.0f, 1.0f
    );

    vk::Rect2D scissor(
        vk::Offset2D(0, 0), 
        m_swapChainExtent
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
    pipelineLayoutInfo.setPSetLayouts(&m_descriptorSetLayout.get());

    m_pipelineLayout = m_device->createPipelineLayoutUnique(pipelineLayoutInfo);

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
        m_pipelineLayout.get(),
        m_renderPass.get()
    );

    m_pipeline = m_device->createGraphicsPipelineUnique(vk::PipelineCache(), graphicsInfo);
}

void HelloTriangleApp::createFramebuffers()
{
    m_frameBuffers.resize(m_swapChainImages.size());

    vk::FramebufferCreateInfo framebufferInfo(
        vk::FramebufferCreateFlags(), 
        m_renderPass.get(), 
        1, nullptr, 
        m_swapChainExtent.width, m_swapChainExtent.height, 
        1
    );

    for (auto i = 0u; i < m_swapChainImageViews.size(); ++i)
    {
        framebufferInfo.pAttachments = &m_swapChainImageViews[i].get();
        m_frameBuffers[i] = m_device->createFramebufferUnique(framebufferInfo);
    }
}

void HelloTriangleApp::createCommandPool()
{
    auto indices = QueueFamilyIndices(m_physicalDevice, *m_renderSurface);
    vk::CommandPoolCreateInfo commandPoolInfo(vk::CommandPoolCreateFlags(), indices.graphics());
    m_commandPool = m_device->createCommandPoolUnique(commandPoolInfo);
}

void HelloTriangleApp::createCommandBuffers()
{
    vk::CommandBufferAllocateInfo allocInfo(
        m_commandPool.get(), 
        vk::CommandBufferLevel::ePrimary, 
        static_cast<uint32_t>(m_swapChainImageViews.size())
    );

    m_commandBuffers = m_device->allocateCommandBuffersUnique(allocInfo);

    vk::CommandBufferBeginInfo commandBufferBegin{};

    vk::Buffer vertexBuffers[] = { m_deviceVertecies.buffer() };
    vk::DeviceSize vertexOffsets[] = { 0 };

    vk::RenderPassBeginInfo renderPassBegin;
    renderPassBegin.renderPass = m_renderPass.get();
    renderPassBegin.renderArea.offset = vk::Offset2D(0, 0);
    renderPassBegin.renderArea.extent = m_swapChainExtent;
    vk::ClearValue clearColor(vk::ClearColorValue(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f}));
    renderPassBegin.clearValueCount = 1;
    renderPassBegin.pClearValues = &clearColor;

    for (auto i = 0u; i < m_swapChainImageViews.size(); ++i)
    {
        renderPassBegin.framebuffer = m_frameBuffers[i].get();

        m_commandBuffers[i]->begin(commandBufferBegin);
            m_commandBuffers[i]->beginRenderPass(renderPassBegin, vk::SubpassContents::eInline);
            m_commandBuffers[i]->bindPipeline(vk::PipelineBindPoint::eGraphics, m_pipeline.get());
            m_commandBuffers[i]->bindVertexBuffers(0, 1, vertexBuffers, vertexOffsets);
            m_commandBuffers[i]->bindIndexBuffer(m_deviceIndices.buffer(), 0, vk::IndexType::eUint16);
            m_commandBuffers[i]->bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *m_pipelineLayout, 0, {m_descriptorSets[i]}, {});
            m_commandBuffers[i]->drawIndexed(static_cast<uint32_t>(g_indices.size()), 1, 0, 0, 0);
            m_commandBuffers[i]->endRenderPass();
        m_commandBuffers[i]->end();
    }
}

void HelloTriangleApp::createSyncObjects()
{
    auto size = m_swapChainImages.size();

    m_imageAvailable.resize(size);
    m_renderCompleted.resize(size);
    m_inFlightImages.resize(size);

    auto semaphoreInfo = vk::SemaphoreCreateInfo();
    vk::FenceCreateInfo fenceInfo(vk::FenceCreateFlags(vk::FenceCreateFlagBits::eSignaled));

    for (auto i = 0u; i < size; ++i)
    {
        m_imageAvailable[i] = m_device->createSemaphoreUnique(semaphoreInfo);
        m_renderCompleted[i] = m_device->createSemaphoreUnique(semaphoreInfo);
        m_inFlightImages[i] = m_device->createFenceUnique(fenceInfo);
    }
}

void HelloTriangleApp::recreateSwapchain()
{
    int w = 0, h = 0;
    while (w == 0 && h == 0)
    {
        glfwGetFramebufferSize(m_window, &w, &h);
        glfwWaitEvents();
    }

    m_device->waitIdle();

    m_commandBuffers.clear();
    m_descriptorPool.reset();
    m_uniforms.clear();
    m_frameBuffers.clear();
    m_pipeline.reset();
    m_renderPass.reset();
    m_swapChainImageViews.clear();
    m_swapChain.reset();

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

void HelloTriangleApp::copyBuffer(const vk::Buffer& src, const vk::Buffer& dest, const vk::DeviceSize& size)
{
    auto copyCommand = beginSingleTimeCommand();
    copyCommand->copyBuffer(src, dest, { vk::BufferCopy(0, 0, size) });
    applyGraphicsCmd(*copyCommand);    
}

void HelloTriangleApp::createVertexBuffers()
{
    m_deviceVertecies = createStagedBuffer(g_vertecies, vk::BufferUsageFlagBits::eVertexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal);
    m_deviceIndices = createStagedBuffer(g_indices, vk::BufferUsageFlagBits::eIndexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal);
}

void HelloTriangleApp::createUniformBuffers()
{
    m_uniforms.resize(m_swapChainImages.size());

    auto bufferSize = sizeof(MVPTransform);
    for (auto i = 0; i < m_uniforms.size(); ++i)
    {
        m_uniforms[i] = BoundedBuffer(
            m_physicalDevice, *m_device, 
            bufferSize, vk::BufferUsageFlagBits::eUniformBuffer, 
            vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible
        );
    }
}

void HelloTriangleApp::createDescriptorPool()
{
    vk::DescriptorPoolSize poolSize(vk::DescriptorType::eUniformBuffer, m_swapChainImages.size());
    vk::DescriptorPoolCreateInfo poolInfo(vk::DescriptorPoolCreateFlags(), m_swapChainImages.size(), 1, &poolSize);

    m_descriptorPool = m_device->createDescriptorPoolUnique(poolInfo);
}

void HelloTriangleApp::createDescriptorSets()
{
    const std::vector<vk::DescriptorSetLayout> layouts(m_swapChainImages.size(), *m_descriptorSetLayout);
    vk::DescriptorSetAllocateInfo allocInfo(*m_descriptorPool, layouts.size(), layouts.data());
    m_descriptorSets = m_device->allocateDescriptorSets(allocInfo);

    vk::DescriptorBufferInfo bufferInfo(vk::Buffer(), 0, sizeof(MVPTransform));
    vk::WriteDescriptorSet descriptorWrite(vk::DescriptorSet(), 0, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr, &bufferInfo);
    for (auto i = 0u; i < m_swapChainImages.size(); ++i)
    {
        bufferInfo.setBuffer(m_uniforms[i].buffer());
        descriptorWrite.setDstSet(m_descriptorSets[i]);

        m_device->updateDescriptorSets({descriptorWrite}, {});
    }
}

vk::UniqueCommandBuffer HelloTriangleApp::beginSingleTimeCommand() {
    auto cmdBuffer = vk::UniqueCommandBuffer(
        m_device->allocateCommandBuffers(
            vk::CommandBufferAllocateInfo(
                *m_commandPool,
                vk::CommandBufferLevel::ePrimary,
                1
            )
        )[0],
        vk::PoolFree(
            *m_device, 
            *m_commandPool, 
            vk::DispatchLoaderStatic()
        )
    );

    cmdBuffer->begin(
        vk::CommandBufferBeginInfo(
            vk::CommandBufferUsageFlagBits::eOneTimeSubmit
        )
    );

    return std::move(cmdBuffer);
}

void HelloTriangleApp::applyGraphicsCmd(vk::CommandBuffer cmdBuffer) {
    cmdBuffer.end();

    auto cmdFence = m_device->createFenceUnique(vk::FenceCreateInfo());
    vk::SubmitInfo submitInfo;
    submitInfo.setCommandBufferCount(1);
    submitInfo.setPCommandBuffers(&cmdBuffer);
    m_graphicsQueue.submit(1, &submitInfo, *cmdFence);

    m_device->waitForFences(1, &cmdFence.get(), VK_TRUE, std::numeric_limits<uint64_t>::max());
}

void HelloTriangleApp::transitionImageLayout(vk::Image image, vk::Format format, vk::ImageLayout oldLayout, vk::ImageLayout newLayout) {
    vk::AccessFlags barrierSrc, barrierDst;
    vk::PipelineStageFlags pipelineSrc, pipelineDst;

    if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal) {
        barrierSrc = vk::AccessFlags();
        barrierDst = vk::AccessFlagBits::eTransferWrite;

        pipelineSrc = vk::PipelineStageFlagBits::eTopOfPipe;
        pipelineDst = vk::PipelineStageFlagBits::eTransfer;
    } else if (oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
        barrierSrc = vk::AccessFlagBits::eTransferWrite;
        barrierDst = vk::AccessFlagBits::eShaderRead;

        pipelineSrc = vk::PipelineStageFlagBits::eTransfer;
        pipelineDst = vk::PipelineStageFlagBits::eFragmentShader;
    } else {
        throw std::runtime_error("[" + std::string(__func__) + "] invalid layouts: " + vk::to_string(oldLayout) + " -> " + vk::to_string(newLayout));
    }
    
    auto transitionCmd = beginSingleTimeCommand();
    vk::ImageMemoryBarrier transitionBarrier(
        barrierSrc, barrierDst,
        oldLayout, newLayout,
        VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
        m_statueTexture.image(),
        vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)
    );
    transitionCmd->pipelineBarrier(pipelineSrc, pipelineDst, vk::DependencyFlags(), {}, {}, {transitionBarrier});
    applyGraphicsCmd(*transitionCmd);
}

void HelloTriangleApp::copyBufferToImage(vk::Buffer srcBuffer, vk::Image dstImage, uint32_t imageWidth, uint32_t imageHeight) {
    auto copyCmd = beginSingleTimeCommand();
    
    vk::BufferImageCopy imageCopyInfo(
        0, 0, 0, 
        vk::ImageSubresourceLayers(
            vk::ImageAspectFlagBits::eColor, 
            0, 
            0, 
        1), 
        {0, 0, 0}, 
        {imageWidth, imageHeight, 1}
    );
    copyCmd->copyBufferToImage(srcBuffer, dstImage, vk::ImageLayout::eTransferDstOptimal, {imageCopyInfo});

    applyGraphicsCmd(*copyCmd);    
}

void HelloTriangleApp::createTextureImage() {
    int texWidth, texHeight, texChannelCount;
    auto pixels = stbi_load("../resources/textures/statue.jpg", &texWidth, &texHeight, &texChannelCount, STBI_rgb_alpha);

    if (!pixels) {
        throw std::runtime_error("could not load texture!");
    }

    auto w = static_cast<uint32_t>(texWidth);
    auto h = static_cast<uint32_t>(texHeight);

    VkDeviceSize desiredTextureSize = w * h * 4;
    BoundedBuffer staging(
        m_physicalDevice, 
        *m_device, 
        desiredTextureSize, 
        pixels,
        vk::BufferUsageFlagBits::eTransferSrc, 
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
    );

    stbi_image_free(pixels);

    m_statueTexture = BoundImage(
        *m_device, 
        w, h, 
        vk::Format::eR8G8B8A8Srgb, 
        vk::ImageTiling::eOptimal, 
        vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled, 
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        m_physicalDevice.getMemoryProperties());        

    transitionImageLayout(m_statueTexture.image(), vk::Format::eR8G8B8A8Srgb, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
    copyBufferToImage(staging.buffer(), m_statueTexture.image(), w, h);    
    transitionImageLayout(m_statueTexture.image(), vk::Format::eR8G8B8A8Srgb, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);
}

void HelloTriangleApp::createTextureView() {
    m_statueTextureView = createImageView(m_statueTexture.image(), vk::Format::eR8G8B8A8Srgb); 
}

void HelloTriangleApp::createTextureSampler() {
    vk::SamplerCreateInfo sampleInfo(
        vk::SamplerCreateFlags(),
        vk::Filter::eLinear, vk::Filter::eLinear, 
        vk::SamplerMipmapMode::eLinear, 
        vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eRepeat, 
        0.0f, 
        VK_TRUE, 16.0f, 
        VK_FALSE, vk::CompareOp::eAlways, 
        0.0f, 0.0f, 
        vk::BorderColor::eIntOpaqueBlack, 
        VK_FALSE
    );

    m_statueTextureSampler = m_device->createSamplerUnique(sampleInfo);
}

void HelloTriangleApp::initVulkan()
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
    createTextureImage();
    createTextureView();
    createTextureSampler();
    createVertexBuffers();
    createUniformBuffers();
    createDescriptorPool();
    createDescriptorSets();
    createCommandBuffers();
    createSyncObjects();
}

void HelloTriangleApp::updateUniformBuffer(const BoundedBuffer& deviceUniform)
{
        const auto initTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float dt = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - initTime).count();

    auto model = glm::rotate(glm::mat4(1.0f), dt * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    auto view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    auto proj = glm::perspective(glm::radians(45.0f), m_swapChainExtent.width / static_cast<float>(m_swapChainExtent.height), 0.1f, 10.0f);
    proj[1][1] *= -1;

    auto transform = mkTransform(model, view, proj);

    void* data = m_device->mapMemory(deviceUniform.memory(), 0, sizeof(MVPTransform));
    memcpy(data, &transform, sizeof(transform));
    m_device->unmapMemory(deviceUniform.memory());
}

void HelloTriangleApp::drawFrame()
{
    m_device->waitForFences(1, &m_inFlightImages[m_currentFrame].get(), VK_TRUE, std::numeric_limits<uint64_t>::max());
    const vk::Semaphore* waitSemaphore = &m_imageAvailable[m_currentFrame].get();
    const vk::Semaphore* signalSemaphore = &m_renderCompleted[m_currentFrame].get();

    uint32_t imageIndex;
    auto status = m_device->acquireNextImageKHR(m_swapChain.get(), std::numeric_limits<uint64_t>::max(), m_imageAvailable[m_currentFrame].get(), vk::Fence(), &imageIndex);
    if (status == vk::Result::eErrorOutOfDateKHR)
    {
        recreateSwapchain();
        return;
    }
    
    else if (status != vk::Result::eSuccess && status != vk::Result::eSuboptimalKHR)
    {
        vk::throwResultException(status, "could not aquire next image");
    }

    const vk::PipelineStageFlags waitStage(vk::PipelineStageFlagBits::eColorAttachmentOutput);
    const vk::SubmitInfo submitInfo(1, waitSemaphore, &waitStage, 1, &m_commandBuffers[imageIndex].get(), 1, signalSemaphore);

    m_device->resetFences(1, &m_inFlightImages[m_currentFrame].get());

    updateUniformBuffer(m_uniforms[imageIndex]);

    m_graphicsQueue.submit({ submitInfo }, m_inFlightImages[m_currentFrame].get());

    vk::PresentInfoKHR presentInfo(1, signalSemaphore, 1, &m_swapChain.get(), &imageIndex);

    status = m_presentQueue.presentKHR(&presentInfo);
    if (status == vk::Result::eErrorOutOfDateKHR || status == vk::Result::eSuboptimalKHR)
    {
        recreateSwapchain();
    }
    else if (status != vk::Result::eSuccess)
    {
        vk::throwResultException(status, "could not present queue!");
    }

    m_currentFrame = (m_currentFrame + 1) % config::MAX_FRAMES_IN_FLIGHT;
}

void HelloTriangleApp::mainLoop()
{
    while (!glfwWindowShouldClose(m_window))
    {
        drawFrame();
        glfwPollEvents();
    }

    m_graphicsQueue.waitIdle();
    m_presentQueue.waitIdle();
}
