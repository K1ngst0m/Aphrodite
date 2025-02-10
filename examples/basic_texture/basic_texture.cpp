#include "basic_texture.h"

basic_texture::basic_texture() : aph::BaseApp("base_texture")
{
}

void basic_texture::init()
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

    // setup quad
    {
        struct VertexData
        {
            glm::vec3 pos;
            glm::vec2 uv;
        };

        // vertex: position, color
        const std::vector<VertexData> vertices = {{.pos = {-0.5f, -0.5f, 0.0f}, .uv = {0.0f, 0.0f}},
                                                  {.pos = {0.5f, -0.5f, 0.0f}, .uv = {1.0f, 0.0f}},
                                                  {.pos = {0.5f, 0.5f, 0.0f}, .uv = {1.0f, 1.0f}},
                                                  {.pos = {-0.5f, 0.5f, 0.0f}, .uv = {0.0f, 1.0f}}};

        const std::vector<uint32_t> indices = {
            0, 1, 2,  // First triangle
            2, 3, 0   // Second triangle
        };

        // vertex buffer
        m_pResourceLoader->loadAsync(
            aph::BufferLoadInfo{.debugName  = "quad::vertexBuffer",
                                .data       = vertices.data(),
                                .createInfo = {.size  = static_cast<uint32_t>(vertices.size() * sizeof(vertices[0])),
                                               .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT}},
            &m_pVB);

        // index buffer
        m_pResourceLoader->loadAsync(
            aph::BufferLoadInfo{.data       = indices.data(),
                                .createInfo = {.size  = static_cast<uint32_t>(indices.size() * sizeof(indices[0])),
                                               .usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT}},
            &m_pIB);

        // matrix uniform buffer
        m_pResourceLoader->load(aph::BufferLoadInfo{.debugName = "matrix data",
                                                    .data      = &m_modelMatrix,
                                                    .createInfo =
                                                        {
                                                            .size   = sizeof(glm::mat4),
                                                            .usage  = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                                            .domain = aph::BufferDomain::LinkedDeviceHost,
                                                        }},
                                &m_pMatBuffer);

        // image and sampler
        APH_VR(m_pDevice->create(aph::vk::init::samplerCreateInfo2(aph::SamplerPreset::LinearClamp), &m_pSampler));
        m_pResourceLoader->loadAsync(aph::ImageLoadInfo{.data = "texture://container2.png",
                                                        .createInfo =
                                                            {
                                                                .usage     = VK_IMAGE_USAGE_SAMPLED_BIT,
                                                                .domain    = aph::ImageDomain::Device,
                                                                .imageType = VK_IMAGE_TYPE_2D,
                                                            }},
                                     &m_pImage);

        // pipeline
        m_pResourceLoader->loadAsync(
            aph::ShaderLoadInfo{.stageInfo =
                                    {
                                        {aph::ShaderStage::VS, {"shader_slang://texture.slang"}},
                                        {aph::ShaderStage::FS, {"shader_slang://texture.slang"}},
                                    }},
            &m_pProgram);
        m_pResourceLoader->wait();

        // record graph execution
        m_renderer->recordGraph([this](auto* graph) {
            auto drawPass = graph->createPass("drawing quad with texture", aph::QueueType::Graphics);
            drawPass->setColorOutput("render target",
                                     {
                                         .extent = {m_pSwapChain->getWidth(), m_pSwapChain->getHeight(), 1},
                                         .format = m_pSwapChain->getFormat(),
                                     });
            drawPass->addTextureInput("container texture", m_pImage);
            drawPass->addUniformBufferInput("matrix ubo", m_pMatBuffer);

            graph->setBackBuffer("render target");

            drawPass->recordExecute([this](auto* pCmd) {
                pCmd->bindVertexBuffers(m_pVB);
                pCmd->bindIndexBuffers(m_pIB);
                pCmd->setResource({m_pMatBuffer}, 0, 0);
                pCmd->setResource({m_pImage}, 1, 0);
                pCmd->setResource({m_pSampler}, 1, 1);
                pCmd->setProgram(m_pProgram);
                pCmd->insertDebugLabel({
                    .name  = "draw a quad with texture",
                    .color = {0.5f, 0.3f, 0.2f, 1.0f},
                });
                pCmd->drawIndexed({6, 1, 0, 0, 0});
            });
        });
    }
}

void basic_texture::run()
{
    while(m_pWSI->update())
    {
        APH_PROFILER_SCOPE_NAME("application loop");
        m_modelMatrix = glm::rotate(m_modelMatrix, glm::radians(0.1f), {.0, .0, 1.0f});
        m_pResourceLoader->update({.data = &m_modelMatrix, .range = {0, sizeof(glm::mat4)}}, &m_pMatBuffer);

        m_renderer->update();
        m_renderer->render();
    }
}

void basic_texture::load()
{
    APH_PROFILER_SCOPE();
    m_renderer->load();
}
void basic_texture::unload()
{
    APH_PROFILER_SCOPE();
    m_renderer->unload();
}

void basic_texture::finish()
{
    APH_PROFILER_SCOPE();
    m_pDevice->waitIdle();
    m_pDevice->destroy(m_pVB);
    m_pDevice->destroy(m_pIB);
    m_pDevice->destroy(m_pMatBuffer);
    m_pDevice->destroy(m_pProgram);
    m_pDevice->destroy(m_pImage);
    m_pDevice->destroy(m_pSampler);
}

int main(int argc, char** argv)
{
    LOG_SETUP_LEVEL_INFO();

    basic_texture app;
    app.loadConfig(argc, argv);

    app.init();
    app.load();
    app.run();
    app.unload();
    app.finish();
}
