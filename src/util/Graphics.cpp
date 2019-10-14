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

template <class Container>
BoundedBuffer createStagedBuffer(const vk::PhysicalDevice& physicalDevice, const vk::Device& dev, const Container& hostData, const vk::BufferUsageFlags& usage, const vk::MemoryPropertyFlags& properties)
{
    auto size = sizeof(hostData[0]) * hostData.size();

    auto stagingBuffer = BoundedBuffer(
        physicalDevice, dev, 
        hostData, vk::BufferUsageFlagBits::eTransferSrc, 
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
    );

    auto ret = BoundedBuffer(
        physicalDevice, dev,
        size, usage | vk::BufferUsageFlagBits::eTransferDst,
        properties 
    );

    copyBuffer(dev, physicalDstagingBuffer.buffer(), ret.buffer(), size);

    return ret;
}

Graphics::Graphics(
    const vk::Device& dev,
    const Present& present,
    const uint32_t graphicsFamilyIndex,
    const vk::PhysicalDevice& physicalDevice)
    : m_device(dev)
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

    renderPass = dev.createRenderPassUnique(renderPassInfo);

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

    auto vertShaderCode = readFile("vert.spv");
    auto fragShaderCode = readFile("frag.spv");

    auto fragShader = dev.createShaderModuleUnique(
        vk::ShaderModuleCreateInfo(
            vk::ShaderModuleCreateFlags(),
            fragShaderCode.size(), 
            reinterpret_cast<const uint32_t *>(fragShaderCode.data())
        )
    );
    
    auto vertShader = dev.createShaderModuleUnique(
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

    pipelineLayout = dev.createPipelineLayoutUnique(pipelineLayoutInfo);

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

    pipeline = dev.createGraphicsPipelineUnique(vk::PipelineCache(), graphicsInfo);

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
        frameBuffers[i] = dev.createFramebufferUnique(framebufferInfo);
    }

    vk::CommandPoolCreateInfo commandPoolInfo(vk::CommandPoolCreateFlags(), graphicsFamilyIndex);
    commandPool = dev.createCommandPoolUnique(commandPoolInfo);

    deviceVertecies = createStagedBuffer(g_vertecies, vk::BufferUsageFlagBits::eVertexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal);
    deviceIndices = createStagedBuffer(g_indices, vk::BufferUsageFlagBits::eIndexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal);

    uniforms.resize(present.imageCount());

    auto bufferSize = sizeof(MVPTransform);
    for (auto i = 0; i < uniforms.size(); ++i)
    {
        uniforms[i] = BoundedBuffer(
            physicalDevice, dev, 
            bufferSize, vk::BufferUsageFlagBits::eUniformBuffer, 
            vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible
        );
    }

    vk::DescriptorPoolSize poolSize(vk::DescriptorType::eUniformBuffer, present.imageCount());
    vk::DescriptorPoolCreateInfo poolInfo(vk::DescriptorPoolCreateFlags(), present.imageCount(), 1, &poolSize);

    descriptorPool = dev.createDescriptorPoolUnique(poolInfo);

    const std::vector<vk::DescriptorSetLayout> layouts(present.imageCount(), *descriptorSetLayout);
    vk::DescriptorSetAllocateInfo allocInfo(*descriptorPool, layouts.size(), layouts.data());
    descriptorSets = dev.allocateDescriptorSets(allocInfo);

    vk::DescriptorBufferInfo bufferInfo(vk::Buffer(), 0, sizeof(MVPTransform));
    vk::WriteDescriptorSet descriptorWrite(vk::DescriptorSet(), 0, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr, &bufferInfo);
    for (auto i = 0u; i < present.imageCount(); ++i)
    {
        bufferInfo.setBuffer(uniforms[i].buffer());
        descriptorWrite.setDstSet(descriptorSets[i]);

        dev.updateDescriptorSets({descriptorWrite}, {});
    }

    vk::CommandBufferAllocateInfo allocInfo(
        commandPool.get(), 
        vk::CommandBufferLevel::ePrimary, 
        static_cast<uint32_t>(present.imageCount())
    );

    commandBuffers = dev.allocateCommandBuffersUnique(allocInfo);

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

Graphics::Graphics()
{

}
