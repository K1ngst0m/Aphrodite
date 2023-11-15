#include "basic_texture.h"

basic_texture::basic_texture() : aph::BaseApp("base_texture")
{
}

void basic_texture::init()
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

    // setup quad
    {
        struct VertexData
        {
            aph::vec3 pos;
            aph::vec2 uv;
        };
        // vertex: position, color
        const std::vector<VertexData> vertices = {{.pos = {-0.5f, -0.5f, 0.0f}, .uv = {0.0f, 0.0f}},
                                                  {.pos = {0.5f, -0.5f, 0.0f}, .uv = {1.0f, 0.0f}},
                                                  {.pos = {0.5f, 0.5f, 0.0f}, .uv = {1.0f, 1.0f}},
                                                  {.pos = {-0.5f, 0.5f, 0.0f}, .uv = {0.0f, 1.0f}}};
        const std::vector<uint32_t>   indices  = {
            0, 1, 2,  // First triangle
            2, 3, 0   // Second triangle
        };

        // vertex buffer
        {
            aph::BufferLoadInfo loadInfo{
                .debugName  = "quad::vertexBuffer",
                .data       = vertices.data(),
                .createInfo = {.size  = static_cast<uint32_t>(vertices.size() * sizeof(vertices[0])),
                               .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT}};

            m_pResourceLoader->loadAsync(loadInfo, &m_pVB);
        }

        // index buffer
        {
            aph::BufferLoadInfo loadInfo{
                .data       = indices.data(),
                .createInfo = {.size  = static_cast<uint32_t>(indices.size() * sizeof(indices[0])),
                               .usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT}};
            m_pResourceLoader->loadAsync(loadInfo, &m_pIB);
        }

        // image and sampler
        {
            APH_CHECK_RESULT(
                m_pDevice->create(aph::vk::init::samplerCreateInfo2(aph::SamplerPreset::LinearClamp), &m_pSampler));

            aph::ImageLoadInfo loadInfo{.data       = "texture://container2.png",
                                        .createInfo = {
                                            .alignment = 0,
                                            .arraySize = 1,
                                            .usage     = VK_IMAGE_USAGE_SAMPLED_BIT,
                                            .domain    = aph::ImageDomain::Device,
                                            .imageType = VK_IMAGE_TYPE_2D,
                                        }};
            m_pResourceLoader->loadAsync(loadInfo, &m_pImage);
        }

        // pipeline
        {
            const aph::VertexInput vdesc = {
                .attributes =
                    {
                        {.location = 0, .format = aph::Format::RGB32_FLOAT, .offset = offsetof(VertexData, pos)},
                        {.location = 1, .format = aph::Format::RG32_FLOAT, .offset = offsetof(VertexData, uv)},
                    },
                .bindings = {{.stride = sizeof(VertexData)}},
            };

            aph::ShaderLoadInfo shaderLoadInfo{.stageInfo = {
                                                   {aph::ShaderStage::VS, {"shader_slang://texture.slang"}},
                                                   {aph::ShaderStage::FS, {"shader_slang://texture.slang"}},
                                               }};
            m_pResourceLoader->loadAsync(shaderLoadInfo, &m_pProgram);
            m_pResourceLoader->wait();

            aph::vk::GraphicsPipelineCreateInfo createInfo{
                .vertexInput = vdesc,
                .pProgram    = m_pProgram,
                .color       = {{.format = m_pSwapChain->getFormat()}},
            };

            m_pPipeline = m_pDevice->acquirePipeline(createInfo);
        }

        // descriptor set
        {
            m_pTextureSet = m_pPipeline->acquireSet(0);
            m_pTextureSet->update({
                .binding     = 0,
                .arrayOffset = 0,
                .images      = {m_pImage},
                .samplers    = {},
            });
            m_pTextureSet->update({
                .binding     = 1,
                .arrayOffset = 0,
                .images      = {},
                .samplers    = {m_pSampler},
            });
        }

        // record graph execution
        m_renderer->recordGraph([this](auto* graph) {
            auto drawPass = graph->createPass("drawing quad with texture", aph::QueueType::Graphics);
            drawPass->setColorOutput("render target",
                                     {
                                         .extent = {m_pSwapChain->getWidth(), m_pSwapChain->getHeight(), 1},
                                         .format = m_pSwapChain->getFormat(),
                                     });
            drawPass->addTextureInput("container texture", m_pImage);

            drawPass->recordExecute([this](auto* pCmd) {
                pCmd->bindVertexBuffers(m_pVB);
                pCmd->bindIndexBuffers(m_pIB);
                pCmd->bindPipeline(m_pPipeline);
                pCmd->bindDescriptorSet({m_pTextureSet});
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
        PROFILE_SCOPE("application loop");
        m_renderer->update();
        m_renderer->render("render target");
    }
}

void basic_texture::load()
{
    PROFILE_FUNCTION();
    m_renderer->load();
}
void basic_texture::unload()
{
    PROFILE_FUNCTION();
    m_renderer->unload();
}

void basic_texture::finish()
{
    PROFILE_FUNCTION();
    m_pDevice->waitIdle();
    m_pDevice->destroy(m_pVB);
    m_pDevice->destroy(m_pIB);
    m_pDevice->destroy(m_pProgram);
    m_pDevice->destroy(m_pImage);
    m_pDevice->destroy(m_pSampler);
}

int main(int argc, char** argv)
{
    LOG_SETUP_LEVEL_INFO();

    basic_texture app;

    // parse command
    {
        int               exitCode;
        aph::CLICallbacks cbs;
        cbs.add("--width", [&](aph::CLIParser& parser) { app.m_options.windowWidth = parser.nextUint(); });
        cbs.add("--height", [&](aph::CLIParser& parser) { app.m_options.windowHeight = parser.nextUint(); });
        cbs.m_errorHandler = [&]() { CM_LOG_ERR("Failed to parse CLI arguments."); };
        if(!aph::parseCliFiltered(std::move(cbs), argc, argv, exitCode))
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
