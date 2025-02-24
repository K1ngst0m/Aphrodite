#include "mesh_shading.h"

mesh_shading::mesh_shading() : aph::BaseApp("mesh_shading")
{
}

void mesh_shading::init()
{
    APH_PROFILER_SCOPE();

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

    {
        // shader program
        APH_VR(m_pResourceLoader->loadAsync(
            aph::ShaderLoadInfo{.stageInfo =
                                    {
                                        // {aph::ShaderStage::TS, {"shader_slang://mesh_shading.slang"}},
                                        {aph::ShaderStage::MS, {"shader_slang://mesh_shading.slang"}},
                                        {aph::ShaderStage::FS, {"shader_slang://mesh_shading.slang"}},
                                    }},
            &m_pProgram));
        m_pResourceLoader->wait();

        // record graph execution
        m_renderer->recordGraph([this](auto* graph) {
            auto drawPass = graph->createPass("drawing triangle", aph::QueueType::Graphics);

            drawPass->setColorOutput("render target",
                                     {
                                         .extent = {m_pSwapChain->getWidth(), m_pSwapChain->getHeight(), 1},
                                         .format = m_pSwapChain->getFormat(),
                                     });

            graph->setBackBuffer("render target");

            drawPass->recordExecute([this](auto* pCmd) {
                pCmd->setProgram(m_pProgram);
                pCmd->draw(aph::DispatchArguments{1, 1, 1});
            });
        });
    }
}

void mesh_shading::run()
{
    while(m_pWSI->update())
    {
        APH_PROFILER_SCOPE_NAME("application loop");
        m_renderer->update();
        m_renderer->render();
    }
}

void mesh_shading::finish()
{
    APH_PROFILER_SCOPE();
    m_pDevice->waitIdle();
    m_pDevice->destroy(m_pProgram);
}

void mesh_shading::load()
{
    APH_PROFILER_SCOPE();
    m_renderer->load();
}

void mesh_shading::unload()
{
    APH_PROFILER_SCOPE();
    m_renderer->unload();
}

int main(int argc, char** argv)
{
    mesh_shading app;
    app.loadConfig(argc, argv);

    app.init();
    app.load();
    app.run();
    app.unload();
    app.finish();
}
