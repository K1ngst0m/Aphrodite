#include "triangle_demo.h"

#include "glm/glm.hpp"

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
    m_pDevice  = m_renderer->m_pDevice.get();

    aph::EventManager::GetInstance().registerEventHandler<aph::WindowResizeEvent>([this](const aph::WindowResizeEvent& e) {
        m_renderer->m_pSwapChain->reCreate();
        return true;
    });

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

            aph::BufferLoadInfo loadInfo{
                .data       = vertexArray.data(),
                .createInfo = {.size  = static_cast<uint32_t>(vertexArray.size() * sizeof(vertexArray[0])),
                               .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT}};

            m_renderer->m_pResourceLoader->load(loadInfo, &m_pVB);
        }

        // index buffer
        {
            std::array          indexArray{0U, 1U, 2U};
            aph::BufferLoadInfo loadInfo{
                .data       = indexArray.data(),
                .createInfo = {.size  = static_cast<uint32_t>(indexArray.size() * sizeof(indexArray[0])),
                               .usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT}};
            m_renderer->m_pResourceLoader->load(loadInfo, &m_pIB);
        }

        // pipeline
        {
            const aph::VertexInput vdesc = {
                .attributes =
                    {
                        {.location = 0, .format = aph::Format::RGB_F32, .offset = offsetof(VertexData, pos)},
                        {.location = 1, .format = aph::Format::RGB_F32, .offset = offsetof(VertexData, color)},
                    },
                .bindings = {{.stride = sizeof(VertexData)}},
            };

            auto shaderDir = aph::asset::GetShaderDir(aph::asset::ShaderType::GLSL) / "default";

            aph::vk::Shader* pVS = {};
            aph::vk::Shader* pFS = {};

            m_renderer->m_pResourceLoader->load({.data = "triangle.slang"}, &pVS);
            // m_renderer->m_pResourceLoader->load({.data = shaderDir / "triangle.vert"}, &pVS);
            m_renderer->m_pResourceLoader->load({.data = shaderDir / "triangle.frag"}, &pFS);

            aph::vk::GraphicsPipelineCreateInfo createInfo{
                .vertexInput = vdesc,
                .pVertex     = pVS,
                .pFragment   = pFS,
                .color       = {{.format = m_renderer->m_pSwapChain->getFormat()}},
            };

            APH_CHECK_RESULT(m_pDevice->create(createInfo, &m_pPipeline));
        }
    }
}

void triangle_demo::run()
{
    while(m_wsi->update())
    {
        static float deltaTime = {};
        auto         timer     = aph::Timer(deltaTime);

        m_renderer->update(deltaTime);

        auto* queue = m_renderer->getDefaultQueue(aph::QueueType::GRAPHICS);

        // draw and submit
        m_renderer->beginFrame();
        aph::vk::CommandBuffer* cb = m_renderer->acquireCommandBuffer(queue);

        VkExtent2D extent{
            .width  = m_renderer->getWindowWidth(),
            .height = m_renderer->getWindowHeight(),
        };

        aph::vk::Image* presentImage = m_renderer->m_pSwapChain->getImage();

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
        cb->drawIndexed({3, 1, 0, 0, 0});
        m_renderer->pUI->draw(cb);
        cb->endRendering();
        cb->end();

        m_renderer->submit(queue, {.commandBuffers = {cb}}, presentImage);

        m_renderer->endFrame();
    }
}

void triangle_demo::finish()
{
    m_renderer->m_pDevice->waitIdle();
    m_pDevice->destroy(m_pVB, m_pIB, m_pPipeline);
}

void triangle_demo::load()
{
    m_renderer->load();
}

void triangle_demo::unload()
{
    m_renderer->unload();
}

int main(int argc, char** argv)
{
    triangle_demo app;

    // parse command
    {
        int               exitCode;
        aph::CLICallbacks cbs;
        cbs.add("--width", [&](aph::CLIParser& parser) { app.m_options.windowWidth = parser.nextUint(); });
        cbs.add("--height", [&](aph::CLIParser& parser) { app.m_options.windowHeight = parser.nextUint(); });
        cbs.m_errorHandler = [&]() { CM_LOG_ERR("Failed to parse CLI arguments."); };
        if(!aph::parseCliFiltered(cbs, argc, argv, exitCode))
        {
            return exitCode;
        }
    }

    app.init();
    app.load();
    app.run();
    app.unload();
    app.finish();
}
