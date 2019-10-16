#include "Graphics.h"

#include "general.h"
#include "QueueFamilyIndices.h"

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

Graphics::Graphics(
    const vk::Device& dev,
    const Present& present,
    const uint32_t graphicsFamilyIndex,
    const vk::PhysicalDevice& physicalDevice)
    : m_device(dev), m_physicalDevice(physicalDevice), m_projection(1.0f)
{
    queue = dev.getQueue(graphicsFamilyIndex, 0);

    createRenderPass(present);

    vk::DescriptorSetLayoutBinding mvpLayoutBinding(
        0, 
        vk::DescriptorType::eUniformBuffer, 
        1, 
        vk::ShaderStageFlagBits::eVertex
    );

    descriptorSetLayout = dev.createDescriptorSetLayoutUnique(
        vk::DescriptorSetLayoutCreateInfo(
            vk::DescriptorSetLayoutCreateFlags(), 
            1, &mvpLayoutBinding
        )
    );

    createGraphicsPipeline(present);
    createFramebuffers(present);

    vk::CommandPoolCreateInfo commandPoolInfo(vk::CommandPoolCreateFlags(), graphicsFamilyIndex);
    commandPool = dev.createCommandPoolUnique(commandPoolInfo);

    deviceVertecies = createStagedBuffer(physicalDevice, g_vertecies, vk::BufferUsageFlagBits::eVertexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal);
    deviceIndices = createStagedBuffer(physicalDevice, g_indices, vk::BufferUsageFlagBits::eIndexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal);

    createUniformBuffers(present);
    createDescriptorPool(present);
    createDescriptorSets(present);
    createCommandBuffers(present);

    m_projection = glm::perspective(glm::radians(45.0f), present.extent().width / static_cast<float>(present.extent().height), 0.1f, 10.0f);
    m_projection[1][1] *= -1;   
}

Graphics::Graphics()
{

}

void Graphics::render(const vk::Semaphore& wait, const vk::Semaphore& signal, const vk::Fence& hostNotify, const uint32_t& imageIndex)
{
    updateData(imageIndex);

    const vk::PipelineStageFlags waitStage(vk::PipelineStageFlagBits::eColorAttachmentOutput);
    const vk::SubmitInfo submitInfo(1, &wait, &waitStage, 1, &commandBuffers[imageIndex].get(), 1, &signal);

    queue.submit({ submitInfo }, hostNotify);
}


void Graphics::update(const Present& present)
{
    createRenderPass(present);
    createGraphicsPipeline(present);
    createFramebuffers(present);
    createUniformBuffers(present);
    createDescriptorPool(present);
    createDescriptorSets(present);
    createCommandBuffers(present);

    m_projection = glm::perspective(glm::radians(45.0f), present.extent().width / static_cast<float>(present.extent().height), 0.1f, 10.0f);
	m_projection[1][1] *= -1;
}

void Graphics::updateData(const uint32_t& imageIndex)
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


void Graphics::createRenderPass(const Present& present)
{
    vk::AttachmentDescription color(
        vk::AttachmentDescriptionFlags(),
        present.format(),
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

    renderPass = m_device.createRenderPassUnique(renderPassInfo);
}

void Graphics::createGraphicsPipeline(const Present& present)
{
    auto vertShaderCode = readFile("vert.spv");
    auto fragShaderCode = readFile("frag.spv");

    auto fragShader = m_device.createShaderModuleUnique(
        vk::ShaderModuleCreateInfo(
            vk::ShaderModuleCreateFlags(),
            fragShaderCode.size(), 
            reinterpret_cast<const uint32_t *>(fragShaderCode.data())
        )
    );
    
    auto vertShader = m_device.createShaderModuleUnique(
        vk::ShaderModuleCreateInfo(
            vk::ShaderModuleCreateFlags(),
            vertShaderCode.size(), 
            reinterpret_cast<const uint32_t *>(vertShaderCode.data())
        )
    );

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
        static_cast<float>(present.extent().width), static_cast<float>(present.extent().height),
        0.0f, 1.0f
    );

    vk::Rect2D scissor(
        vk::Offset2D(0, 0), 
        present.extent()
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
    pipelineLayoutInfo.setPSetLayouts(&descriptorSetLayout.get());

    pipelineLayout = m_device.createPipelineLayoutUnique(pipelineLayoutInfo);

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
        pipelineLayout.get(),
        renderPass.get()
    );

    pipeline = m_device.createGraphicsPipelineUnique(vk::PipelineCache(), graphicsInfo);
}

void Graphics::createFramebuffers(const Present& present)
{
    frameBuffers.resize(present.imageCount());

    vk::FramebufferCreateInfo framebufferInfo(
        vk::FramebufferCreateFlags(), 
        renderPass.get(), 
        1, nullptr, 
        present.extent().width, present.extent().height, 
        1
    );

    for (auto i = 0u; i < present.imageCount(); ++i)
    {
        framebufferInfo.pAttachments = &present.view(i);
        frameBuffers[i] = m_device.createFramebufferUnique(framebufferInfo);
    }
}

void Graphics::createUniformBuffers(const Present& present)
{
    uniforms.resize(present.imageCount());

    auto bufferSize = sizeof(MVPTransform);
    for (auto i = 0; i < uniforms.size(); ++i)
    {
        uniforms[i] = BoundedBuffer(
            m_physicalDevice, m_device, 
            bufferSize, vk::BufferUsageFlagBits::eUniformBuffer, 
            vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible
        );
    }
}

void Graphics::createDescriptorPool(const Present& present)
{
    vk::DescriptorPoolSize poolSize(vk::DescriptorType::eUniformBuffer, present.imageCount());
    vk::DescriptorPoolCreateInfo poolInfo(vk::DescriptorPoolCreateFlags(), present.imageCount(), 1, &poolSize);

    descriptorPool = m_device.createDescriptorPoolUnique(poolInfo);
}

void Graphics::createDescriptorSets(const Present& present)
{
    const std::vector<vk::DescriptorSetLayout> layouts(present.imageCount(), *descriptorSetLayout);
    vk::DescriptorSetAllocateInfo allocInfo(*descriptorPool, layouts.size(), layouts.data());
    descriptorSets = m_device.allocateDescriptorSets(allocInfo);

    vk::DescriptorBufferInfo bufferInfo(vk::Buffer(), 0, sizeof(MVPTransform));
    vk::WriteDescriptorSet descriptorWrite(vk::DescriptorSet(), 0, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr, &bufferInfo);
    for (auto i = 0u; i < present.imageCount(); ++i)
    {
        bufferInfo.setBuffer(uniforms[i].buffer());
        descriptorWrite.setDstSet(descriptorSets[i]);

        m_device.updateDescriptorSets({descriptorWrite}, {});
    }
}

void Graphics::createCommandBuffers(const Present& present)
{
    vk::CommandBufferAllocateInfo commandBufferAllocInfo(
        commandPool.get(), 
        vk::CommandBufferLevel::ePrimary, 
        static_cast<uint32_t>(present.imageCount())
    );

    commandBuffers = m_device.allocateCommandBuffersUnique(commandBufferAllocInfo);

    vk::CommandBufferBeginInfo commandBufferBegin{};

    vk::Buffer vertexBuffers[] = { deviceVertecies.buffer() };
    vk::DeviceSize vertexOffsets[] = { 0 };

    vk::RenderPassBeginInfo renderPassBegin;
    renderPassBegin.renderPass = renderPass.get();
    renderPassBegin.renderArea.offset = vk::Offset2D(0, 0);
    renderPassBegin.renderArea.extent = present.extent();
    vk::ClearValue clearColor(vk::ClearColorValue(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f}));
    renderPassBegin.clearValueCount = 1;
    renderPassBegin.pClearValues = &clearColor;

    for (auto i = 0u; i < present.imageCount(); ++i)
    {
        renderPassBegin.framebuffer = frameBuffers[i].get();

        commandBuffers[i]->begin(commandBufferBegin);
            commandBuffers[i]->beginRenderPass(renderPassBegin, vk::SubpassContents::eInline);
            commandBuffers[i]->bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline.get());
            commandBuffers[i]->bindVertexBuffers(0, 1, vertexBuffers, vertexOffsets);
            commandBuffers[i]->bindIndexBuffer(deviceIndices.buffer(), 0, vk::IndexType::eUint16);
            commandBuffers[i]->bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipelineLayout, 0, {descriptorSets[i]}, {});
            commandBuffers[i]->drawIndexed(static_cast<uint32_t>(g_indices.size()), 1, 0, 0, 0);
            commandBuffers[i]->endRenderPass();
        commandBuffers[i]->end();
    }
}


