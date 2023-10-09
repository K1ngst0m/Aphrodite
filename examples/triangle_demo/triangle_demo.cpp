#include "triangle_demo.h"

triangle_demo::triangle_demo() : aph::BaseApp("triangle_demo")
{
}

void triangle_demo::init()
{
    // setup window
    m_wsi = aph::WSI::Create(m_options.windowWidth, m_options.windowHeight);

    aph::RenderConfig config{
        .flags     = aph::RENDER_CFG_ALL,
        .maxFrames = 1,
    };

    m_renderer = aph::IRenderer::Create<aph::vk::Renderer>(m_wsi.get(), config);
    m_pDevice  = m_renderer->getDevice();

    // setup triangle
    {
        // vertex buffer
        {
            // vertex: position, color
            std::array vertexArray{
                0.0f, -0.5f, 1.0f, 1.0f,  0.0f, 0.0f, 0.5f, 0.5f, 1.0f,
                0.0f, 1.0f,  0.0f, -0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f,
            };

            aph::vk::BufferCreateInfo vertexBufferCreateInfo{
                .size      = vertexArray.size() * sizeof(vertexArray[0]),
                .alignment = 0,
                .usage     = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                .domain    = aph::BufferDomain::Device,
            };
            m_pDevice->createDeviceLocalBuffer(vertexBufferCreateInfo, &m_pVB, vertexArray.data());
        }

        // index buffer
        {
            std::array                indexArray{0U, 1U, 2U};
            aph::vk::BufferCreateInfo indexBufferCreateInfo{
                .size      = indexArray.size() * sizeof(indexArray[0]),
                .alignment = 0,
                .usage     = VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                .domain    = aph::BufferDomain::Device,
            };
            m_pDevice->createDeviceLocalBuffer(indexBufferCreateInfo, &m_pIB, indexArray.data());
        }

        // pipeline
        {
            aph::vk::GraphicsPipelineCreateInfo createInfo{
                {aph::vk::VertexComponent::POSITION, aph::vk::VertexComponent::COLOR}};
            auto                  shaderDir    = aph::asset::GetShaderDir(aph::asset::ShaderType::GLSL) / "default";
            std::vector<VkFormat> colorFormats = {m_renderer->getSwapChain()->getFormat()};
            createInfo.renderingCreateInfo     = {
                    .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
                    .colorAttachmentCount    = static_cast<uint32_t>(colorFormats.size()),
                    .pColorAttachmentFormats = colorFormats.data(),
                    .depthAttachmentFormat   = VK_FORMAT_UNDEFINED,
            };

            createInfo.colorBlendAttachments.resize(1, {.blendEnable = VK_FALSE, .colorWriteMask = 0xf});

            {
                m_pDevice->createShaderProgram(&m_pShaderProgram, m_renderer->getShaders(shaderDir / "triangle.vert"),
                                               m_renderer->getShaders(shaderDir / "triangle.frag"));
                createInfo.pProgram = m_pShaderProgram;
            }

            VK_CHECK_RESULT(m_pDevice->createGraphicsPipeline(createInfo, &m_pPipeline));
        }
    }
}

void triangle_demo::run()
{
    while(m_wsi->update())
    {
        static float deltaTime = {};
        auto         timer     = aph::Timer(deltaTime);

        auto* queue = m_renderer->getGraphicsQueue();

        // draw and submit
        m_renderer->beginFrame();
        aph::vk::CommandBuffer* cb = {};
        m_pDevice->allocateCommandBuffers(1, &cb, queue);

        VkExtent2D extent{
            .width  = m_renderer->getWindowWidth(),
            .height = m_renderer->getWindowHeight(),
        };

        aph::vk::Image* presentImage = m_renderer->getSwapChain()->getImage();

        // dynamic state
        cb->begin();
        cb->setViewport(extent);
        cb->setScissor(extent);
        cb->bindVertexBuffers(m_pVB);
        cb->bindIndexBuffers(m_pIB);
        cb->bindPipeline(m_pPipeline);
        cb->beginRendering({.offset = {0, 0}, .extent = {extent}}, {presentImage});
        cb->drawIndexed(3, 1, 0, 0, 0);
        cb->endRendering();
        cb->transitionImageLayout(presentImage, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
        cb->end();

        VkSemaphore timelineMain = m_renderer->acquireTimelineMain();

        aph::vk::QueueSubmitInfo2 submitInfo{};
        submitInfo.commands.push_back({.commandBuffer = cb->getHandle()});
        submitInfo.waits.push_back({.semaphore = m_renderer->getRenderSemaphore()});
        submitInfo.signals.push_back({.semaphore = timelineMain, .value = UINT64_MAX});
        submitInfo.signals.push_back({.semaphore = m_renderer->getPresentSemaphore()});
        queue->submit({submitInfo});
        m_renderer->endFrame();
    }
}

void triangle_demo::finish()
{
    m_renderer->getDevice()->waitIdle();
    m_pDevice->destroyBuffer(m_pVB);
    m_pDevice->destroyBuffer(m_pIB);
    m_pDevice->destroyPipeline(m_pPipeline);
    m_pDevice->destroyShaderProgram(m_pShaderProgram);
}

int main()
{
    triangle_demo app;

    app.init();
    app.run();
    app.finish();
}
