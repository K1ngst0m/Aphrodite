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
        struct VertexData
        {
            glm::vec3 pos;
            glm::vec3 color;
        };
        // vertex buffer
        {
            // vertex: position, color
            std::vector<VertexData> vertexArray{
                {.pos = {0.0f, -0.5f, 1.0f}, .color = {1.0f, 0.0f, 0.0f}},
                {.pos = {0.5f, 0.5f, 1.0f}, .color = {0.0f, 1.0f, 0.0f}},
                {.pos = {-0.5f, 0.5f, 1.0f}, .color = {0.0f, 0.0f, 1.0f}},
            };

            aph::vk::BufferCreateInfo vertexBufferCreateInfo{
                .size      = static_cast<uint32_t>(vertexArray.size() * sizeof(vertexArray[0])),
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
            const aph::vk::VertexInput vdesc = {
                .attributes =
                    {
                        {.location = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(VertexData, pos)},
                        {.location = 1, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(VertexData, color)},
                    },
                .inputBindings = {{.stride = sizeof(VertexData)}},
            };

            auto shaderDir = aph::asset::GetShaderDir(aph::asset::ShaderType::GLSL) / "default";
            m_pDevice->createShaderProgram(&m_pShaderProgram, m_renderer->getShaders(shaderDir / "triangle.vert"),
                                           m_renderer->getShaders(shaderDir / "triangle.frag"));
            aph::vk::GraphicsPipelineCreateInfo createInfo{
                .vertexInput = vdesc,
                .pProgram    = m_pShaderProgram,
                .color       = {{.format = m_renderer->getSwapChain()->getFormat()}},
            };

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
        aph::vk::CommandBuffer* cb = m_renderer->acquireFrameCommandBuffer();

        VkExtent2D extent{
            .width  = m_renderer->getWindowWidth(),
            .height = m_renderer->getWindowHeight(),
        };

        aph::vk::Image* presentImage = m_renderer->getSwapChain()->getImage();

        cb->begin();
        cb->setViewport(extent);
        cb->setScissor(extent);
        cb->bindVertexBuffers(m_pVB);
        cb->bindIndexBuffers(m_pIB);
        cb->bindPipeline(m_pPipeline);
        cb->beginRendering({.offset = {0, 0}, .extent = {extent}}, {presentImage});
        cb->insertDebugLabel({
            .name  = "draw a triangle",
            .color = {1.0f, 0.0f, 0.0f, 1.0f},
        });
        cb->drawIndexed(3, 1, 0, 0, 0);
        cb->endRendering();
        cb->end();

        aph::vk::QueueSubmitInfo submitInfo{};
        submitInfo.commandBuffers.push_back(cb);
        submitInfo.waitSemaphores.push_back(m_renderer->getRenderSemaphore());
        submitInfo.signalSemaphores.push_back(m_renderer->getPresentSemaphore());
        queue->submit({submitInfo}, m_renderer->getFrameFence());

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
