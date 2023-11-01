#include "triangle_demo.h"

triangle_demo::triangle_demo() : aph::BaseApp("triangle_demo")
{
}

void triangle_demo::init()
{
    PROFILE_FUNCTION();
    // setup window
    m_wsi = aph::WSI::Create(m_options.windowWidth, m_options.windowHeight);

    aph::RenderConfig config{
        .flags     = aph::RENDER_CFG_ALL,
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
        std::vector<VertexData> vertexArray{
            {.pos = {0.0f, -0.5f, 1.0f}, .color = {1.0f, 0.0f, 0.0f}},
            {.pos = {0.5f, 0.5f, 1.0f}, .color = {0.0f, 1.0f, 0.0f}},
            {.pos = {-0.5f, 0.5f, 1.0f}, .color = {0.0f, 0.0f, 1.0f}},
        };
        std::array indexArray{0U, 1U, 2U};

        auto & timer = aph::Timer::GetInstance();
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

        // index buffer
        {
            aph::BufferLoadInfo loadInfo{
                .debugName  = "triangle::indexbuffer",
                .data       = indexArray.data(),
                .createInfo = {.size  = static_cast<uint32_t>(indexArray.size() * sizeof(indexArray[0])),
                               .usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT}};
            m_renderer->m_pResourceLoader->loadAsync(loadInfo, &m_pIB);
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
            m_renderer->m_pResourceLoader->loadAsync(aph::ShaderLoadInfo{.data = "shader_glsl://default/triangle.vert"}, &pVS);
            m_renderer->m_pResourceLoader->loadAsync(aph::ShaderLoadInfo{.data = "shader_glsl://default/triangle.frag"}, &pFS);

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
        CM_LOG_INFO("load time : %lf", timer.interval("load begin", "load end"));


        // command pool
        {
            APH_CHECK_RESULT(m_pDevice->create(
                aph::vk::CommandPoolCreateInfo{m_renderer->getDefaultQueue(aph::QueueType::Graphics)}, &m_pCmdPool));
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

        m_renderer->update(deltaTime);

        auto* queue = m_renderer->getDefaultQueue(aph::QueueType::Graphics);

        // draw and submit
        timer.set("frame begin");
        m_renderer->beginFrame();
        aph::vk::CommandBuffer* cb = {};
        APH_CHECK_RESULT(m_pCmdPool->allocate(1, &cb));

        VkExtent2D extent{
            .width  = m_renderer->getWindowWidth(),
            .height = m_renderer->getWindowHeight(),
        };

        aph::vk::Image* presentImage = m_renderer->m_pSwapChain->getImage();

        auto pool = m_renderer->getFrameQueryPool();

        enum
        {
            TIMESTAMP_BEGIN_DRAW = 0,
            TIMESTAMP_END_DRAW   = 1,
        };

        cb->begin();
        cb->setDebugName("triangle drawing command");
        cb->resetQueryPool(pool, 0, 2);
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
        cb->writeTimeStamp(VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, pool, TIMESTAMP_BEGIN_DRAW);
        cb->drawIndexed({3, 1, 0, 0, 0});
        cb->writeTimeStamp(VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, pool, TIMESTAMP_END_DRAW);
        m_renderer->pUI->draw(cb);
        cb->endRendering();

        cb->end();

        m_renderer->submit(queue, {.commandBuffers = {cb}}, presentImage);

        m_renderer->endFrame();
        timer.set("frame end");

        deltaTime          = timer.interval("frame begin", "frame end");
        auto timeInSeconds = m_pDevice->getTimeQueryResults(pool, 0, 1, aph::vk::TimeUnit::Seconds);

        CM_LOG_DEBUG("draw time: %lfs", timeInSeconds);
        CM_LOG_DEBUG("Fps: %.0f", 1 / deltaTime);
    }
}

void triangle_demo::finish()
{
    PROFILE_FUNCTION();
    m_renderer->m_pDevice->waitIdle();
    m_pDevice->destroy(m_pVB, m_pIB, m_pPipeline, m_pCmdPool);
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
