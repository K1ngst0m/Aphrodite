#include "triangle_demo.h"

triangle_demo::triangle_demo() : aph::BaseApp("triangle_demo")
{
}

void triangle_demo::init()
{
    PROFILE_FUNCTION();
    // setup window
    m_wsi = aph::WSI::Create({m_options.windowWidth, m_options.windowHeight, false});

    aph::RenderConfig config{
        .flags     = aph::RENDER_CFG_WITHOUT_UI,
        .maxFrames = 1,
    };

    m_renderer = aph::IRenderer::Create<aph::vk::Renderer>(m_wsi.get(), config);
    m_pDevice  = m_renderer->m_pDevice.get();

    aph::EventManager::GetInstance().registerEventHandler<aph::WindowResizeEvent>(
        [this](const aph::WindowResizeEvent& e) {
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
        const std::vector<VertexData> vertexArray{
            {.pos = {0.0f, -0.5f, 1.0f}, .color = {1.0f, 0.0f, 0.0f}},
            {.pos = {0.5f, 0.5f, 1.0f}, .color = {0.0f, 1.0f, 0.0f}},
            {.pos = {-0.5f, 0.5f, 1.0f}, .color = {0.0f, 0.0f, 1.0f}},
        };

        auto& timer = aph::Timer::GetInstance();
        timer.set("load begin");
        {
            // vertex: position, color

            aph::BufferLoadInfo loadInfo{
                .debugName  = "triangle::vertexBuffer",
                .data       = vertexArray.data(),
                .createInfo = {.size  = static_cast<uint32_t>(vertexArray.size() * sizeof(vertexArray[0])),
                               .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT}};

            m_renderer->m_pResourceLoader->loadAsync(loadInfo, &m_pVB);
        }

        constexpr std::array indexArray{0U, 1U, 2U};
        // index buffer
        {
            aph::BufferLoadInfo loadInfo{
                .debugName  = "triangle::indexbuffer",
                .data       = indexArray.data(),
                .createInfo = {.size  = static_cast<uint32_t>(indexArray.size() * sizeof(indexArray[0])),
                               .usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT}};
            m_renderer->m_pResourceLoader->loadAsync(loadInfo, &m_pIB);
        }

        // render target
        {
            aph::vk::ImageCreateInfo createInfo{
                .extent    = {m_wsi->getWidth(), m_wsi->getHeight(), 1},
                .usage     = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                .domain    = aph::ImageDomain::Device,
                .imageType = VK_IMAGE_TYPE_2D,
                .format    = aph::Format::BGRA_UN8,
            };

            APH_CHECK_RESULT(m_pDevice->create(createInfo, &m_pRenderTarget));
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

            aph::vk::Shader* pVS = {};
            aph::vk::Shader* pFS = {};

            // m_renderer->m_pResourceLoader->load({.data = "triangle.slang"}, &pVS);
            m_renderer->m_pResourceLoader->loadAsync(aph::ShaderLoadInfo{.data = "shader_glsl://default/triangle.vert"},
                                                     &pVS);
            m_renderer->m_pResourceLoader->loadAsync(aph::ShaderLoadInfo{.data = "shader_glsl://default/triangle.frag"},
                                                     &pFS);

            m_renderer->m_pResourceLoader->wait();
            aph::vk::GraphicsPipelineCreateInfo createInfo{
                .vertexInput = vdesc,
                .pVertex     = pVS,
                .pFragment   = pFS,
                .color       = {{.format = m_renderer->m_pSwapChain->getFormat()}},
            };

            APH_CHECK_RESULT(m_pDevice->create(createInfo, &m_pPipeline, "pipeline::render"));
        }
        timer.set("load end");
        CM_LOG_DEBUG("load time : %lf", timer.interval("load begin", "load end"));

        // command pool
        {
            m_pCmdPool = m_pDevice->acquireCommandPool({m_renderer->getDefaultQueue(aph::QueueType::Graphics)});
        }
    }
}

void triangle_demo::run()
{
    while(m_wsi->update())
    {
        PROFILE_SCOPE("application loop");
        static double deltaTime = {};
        auto&         timer     = aph::Timer::GetInstance();
        timer.set("loop begin");

        m_renderer->update(deltaTime);

        auto* queue = m_renderer->getDefaultQueue(aph::QueueType::Graphics);

        // draw and submit
        m_renderer->beginFrame();
        aph::vk::CommandBuffer* cb = {};
        APH_CHECK_RESULT(m_pCmdPool->allocate(1, &cb));

        cb->begin();
        cb->bindVertexBuffers(m_pVB);
        cb->bindIndexBuffers(m_pIB);
        cb->bindPipeline(m_pPipeline);
        cb->beginRendering({m_pRenderTarget});
        cb->drawIndexed({3, 1, 0, 0, 0});
        cb->endRendering();

        cb->end();

        m_renderer->submit(queue, {.commandBuffers = {cb}}, m_pRenderTarget);

        m_renderer->endFrame();

        timer.set("loop end");
        deltaTime = timer.interval("loop begin", "loop end");
    }
}

void triangle_demo::finish()
{
    PROFILE_FUNCTION();
    m_renderer->m_pDevice->waitIdle();
    m_pDevice->destroy(m_pVB, m_pIB, m_pPipeline, m_pRenderTarget);
}

void triangle_demo::load()
{
    PROFILE_FUNCTION();
    m_renderer->load();
}

void triangle_demo::unload()
{
    PROFILE_FUNCTION();
    m_renderer->unload();
}

int main(int argc, char** argv)
{
    LOG_SETUP_LEVEL_INFO();

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
