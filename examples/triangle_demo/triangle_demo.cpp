#include "triangle_demo.h"

triangle_demo::triangle_demo() : aph::BaseApp("triangle_demo")
{
}

void triangle_demo::init()
{
    PROFILE_FUNCTION();

    // setup window
    aph::RenderConfig config{
        .flags     = aph::RENDER_CFG_WITHOUT_UI,
        .maxFrames = 3,
        .width     = m_options.windowWidth,
        .height    = m_options.windowHeight,
    };

    m_renderer        = aph::vk::Renderer::Create(config);
    m_pDevice         = m_renderer->getDevice();
    m_pSwapChain      = m_renderer->getSwapchain();
    m_pResourceLoader = m_renderer->getResourceLoader();
    m_pWSI            = m_renderer->getWSI();

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

            m_pResourceLoader->loadAsync(loadInfo, &m_pVB);
        }

        constexpr std::array indexArray{0U, 1U, 2U};
        // index buffer
        {
            aph::BufferLoadInfo loadInfo{
                .debugName  = "triangle::indexbuffer",
                .data       = indexArray.data(),
                .createInfo = {.size  = static_cast<uint32_t>(indexArray.size() * sizeof(indexArray[0])),
                               .usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT}};
            m_pResourceLoader->loadAsync(loadInfo, &m_pIB);
        }

        // render target
        {
            aph::vk::ImageCreateInfo createInfo{
                .extent    = {m_pWSI->getWidth(), m_pWSI->getHeight(), 1},
                .usage     = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                .domain    = aph::ImageDomain::Device,
                .imageType = VK_IMAGE_TYPE_2D,
                .format    = m_pSwapChain->getFormat(),
            };

            APH_CHECK_RESULT(m_pDevice->create(createInfo, &m_pRenderTarget));

            aph::EventManager::GetInstance().registerEventHandler<aph::WindowResizeEvent>(
                [createInfo, this](const aph::WindowResizeEvent& e) {
                    m_pSwapChain->reCreate();
                    m_pDevice->destroy(m_pRenderTarget);
                    auto newCreateInfo   = createInfo;
                    newCreateInfo.extent = {m_pWSI->getWidth(), m_pWSI->getHeight(), 1};
                    APH_CHECK_RESULT(m_pDevice->create(newCreateInfo, &m_pRenderTarget));
                    return true;
                });
        }

        // pipeline
        {
            const aph::VertexInput vdesc = {
                .attributes =
                    {
                        {.location = 0, .format = aph::Format::RGB32_FLOAT, .offset = offsetof(VertexData, pos)},
                        {.location = 1, .format = aph::Format::RGB32_FLOAT, .offset = offsetof(VertexData, color)},
                    },
                .bindings = {{.stride = sizeof(VertexData)}},
            };

            aph::ShaderLoadInfo shaderLoadInfo{.stageInfo = {
                                                   {aph::ShaderStage::VS, {"shader_slang://triangle.slang"}},
                                                   {aph::ShaderStage::FS, {"shader_slang://triangle.slang"}},
                                                   // {aph::ShaderStage::VS, {"shader_glsl://default/triangle.vert"}},
                                                   // {aph::ShaderStage::FS, {"shader_glsl://default/triangle.frag"}},
                                               }};

            m_pResourceLoader->loadAsync(shaderLoadInfo, &m_pProgram);

            m_pResourceLoader->wait();
            aph::vk::GraphicsPipelineCreateInfo createInfo{
                .vertexInput = vdesc,
                .pProgram    = m_pProgram,
                .color       = {{.format = m_pSwapChain->getFormat()}},
            };

            APH_CHECK_RESULT(m_pDevice->create(createInfo, &m_pPipeline, "pipeline::render"));
        }
        timer.set("load end");
        CM_LOG_DEBUG("load time : %lf", timer.interval("load begin", "load end"));
    }
}

void triangle_demo::run()
{
    while(m_pWSI->update())
    {
        PROFILE_SCOPE("application loop");
        static double deltaTime = {};
        enum : uint32_t
        {
            TIMELINE_LOOP_BEGIN,
            TIMELINE_LOOP_END,
        };
        auto& timer = aph::Timer::GetInstance();
        timer.set(TIMELINE_LOOP_BEGIN);

        m_renderer->update(deltaTime);

        // draw and submit
        m_renderer->nextFrame();

        auto graph    = m_renderer->getGraph();
        auto drawPass = graph->createPass("drawing triangle", aph::QueueType::Graphics);
        drawPass->addColorOutput(m_pRenderTarget);

        drawPass->recordExecute([this](aph::vk::CommandBuffer* pCmd) {
            pCmd->bindVertexBuffers(m_pVB);
            pCmd->bindIndexBuffers(m_pIB);
            pCmd->bindPipeline(m_pPipeline);
            pCmd->drawIndexed({3, 1, 0, 0, 0});
        });

        graph->execute(m_pRenderTarget, m_pSwapChain);

        timer.set(TIMELINE_LOOP_END);
        deltaTime = timer.interval(TIMELINE_LOOP_BEGIN, TIMELINE_LOOP_END);
    }
}

void triangle_demo::finish()
{
    PROFILE_FUNCTION();
    m_pDevice->waitIdle();
    m_pDevice->destroy(m_pVB);
    m_pDevice->destroy(m_pIB);
    m_pDevice->destroy(m_pPipeline);
    m_pDevice->destroy(m_pRenderTarget);
    m_pDevice->destroy(m_pProgram);
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
