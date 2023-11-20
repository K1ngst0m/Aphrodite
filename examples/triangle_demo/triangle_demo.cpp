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

    aph::EventManager::GetInstance().registerEventHandler<aph::WindowResizeEvent>(
        [this](const aph::WindowResizeEvent& e) {
            m_pSwapChain->reCreate();
            return true;
        });

    // setup triangle
    {
        // vertex: position, color
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
        constexpr std::array indexArray{0U, 1U, 2U};

        m_pResourceLoader->loadAsync(
            aph::BufferLoadInfo{
                .debugName  = "triangle::vertexBuffer",
                .data       = vertexArray.data(),
                .createInfo = {.size  = static_cast<uint32_t>(vertexArray.size() * sizeof(vertexArray[0])),
                               .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT}},
            &m_pVB);

        // index buffer
        m_pResourceLoader->loadAsync(
            aph::BufferLoadInfo{.debugName  = "triangle::indexbuffer",
                                .data       = indexArray.data(),
                                .createInfo = {.size = static_cast<uint32_t>(indexArray.size() * sizeof(indexArray[0])),
                                               .usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT}},
            &m_pIB);

        // shader program
        m_pResourceLoader->loadAsync(
            aph::ShaderLoadInfo{.stageInfo =
                                    {
                                        {aph::ShaderStage::VS, {"shader_slang://triangle.slang"}},
                                        {aph::ShaderStage::FS, {"shader_slang://triangle.slang"}},
                                    }},
            &m_pProgram);
        m_pResourceLoader->wait();

        // record graph execution
        m_renderer->recordGraph([this](auto* graph) {
            auto drawPass = graph->createPass("drawing triangle", aph::QueueType::Graphics);

            drawPass->setColorOutput("render target",
                                     {
                                         .extent = {m_pSwapChain->getWidth(), m_pSwapChain->getHeight(), 1},
                                         .format = m_pSwapChain->getFormat(),
                                     });

            drawPass->recordExecute([this](auto* pCmd) {
                pCmd->setProgram(m_pProgram);
                pCmd->bindVertexBuffers(m_pVB);
                pCmd->bindIndexBuffers(m_pIB);
                pCmd->drawIndexed({3, 1, 0, 0, 0});
            });
        });
    }
}

void triangle_demo::run()
{
    while(m_pWSI->update())
    {
        PROFILE_SCOPE("application loop");
        m_renderer->update();
        m_renderer->render("render target");
    }
}

void triangle_demo::finish()
{
    PROFILE_FUNCTION();
    m_pDevice->waitIdle();
    m_pDevice->destroy(m_pVB);
    m_pDevice->destroy(m_pIB);
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
