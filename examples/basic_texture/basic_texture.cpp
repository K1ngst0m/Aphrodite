#include "basic_texture.h"

#include "glm/glm.hpp"

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
        const std::vector<uint32_t>   indices  = {
            0, 1, 2,  // First triangle
            2, 3, 0   // Second triangle
        };

        // vertex buffer
        {
            aph::BufferLoadInfo loadInfo{
                .debugName = "quad::vertexBuffer",
                .data = vertices.data(),
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
            m_pResourceLoader->load(loadInfo, &m_pIB);
        }

        // image and sampler
        {
            APH_CHECK_RESULT(
                m_pDevice->create(aph::vk::init::samplerCreateInfo2(aph::SamplerPreset::LinearClamp), &m_pSampler));

            aph::vk::ImageCreateInfo imageCI{
                .alignment = 0,
                .arraySize = 1,
                .usage     = VK_IMAGE_USAGE_SAMPLED_BIT,
                .domain    = aph::ImageDomain::Device,
                .imageType = VK_IMAGE_TYPE_2D,
            };

            aph::ImageLoadInfo loadInfo{.data = "texture://container2.png", .pCreateInfo = &imageCI};

            m_pResourceLoader->load(loadInfo, &m_pImage);

            m_pDevice->executeSingleCommands(m_pDevice->getQueue(aph::QueueType::Graphics),
                                             [&](aph::vk::CommandBuffer* cmd) {
                                                 aph::vk::ImageBarrier barrier{
                                                     .pImage       = m_pImage,
                                                     .currentState = aph::RESOURCE_STATE_UNDEFINED,
                                                     .newState     = aph::RESOURCE_STATE_SHADER_RESOURCE,
                                                 };
                                                 cmd->insertBarrier({barrier});
                                             });
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
                        {.location = 0, .format = aph::Format::RGB_F32, .offset = offsetof(VertexData, pos)},
                        {.location = 1, .format = aph::Format::RG_F32, .offset = offsetof(VertexData, uv)},
                    },
                .bindings = {{.stride = sizeof(VertexData)}},
            };

            aph::ShaderLoadInfo shaderLoadInfo{.stageInfo = {
                                                   {aph::ShaderStage::VS, {"shader_glsl://default/texture.vert"}},
                                                   {aph::ShaderStage::FS, {"shader_glsl://default/texture.frag"}},
                                               }};
            m_pResourceLoader->loadAsync(shaderLoadInfo, &m_pProgram);
            m_pResourceLoader->wait();

            aph::vk::GraphicsPipelineCreateInfo createInfo{
                .vertexInput = vdesc,
                .pProgram    = m_pProgram,
                .color       = {{.format = m_pSwapChain->getFormat()}},
            };

            APH_CHECK_RESULT(m_pDevice->create(createInfo, &m_pPipeline));
        }

        // descriptor set
        {
            m_pTextureSet = m_pPipeline->acquireSet(0);
            m_pTextureSet->update({
                .binding     = 0,
                .arrayOffset = 0,
                .images      = {m_pImage},
                .samplers    = {m_pSampler},
            });
        }
    }
}

void basic_texture::run()
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
        auto drawPass = graph->createPass("drawing quad with texture", aph::QueueType::Graphics);
        drawPass->addColorOutput(m_pRenderTarget);

        drawPass->recordExecute([this](aph::vk::CommandBuffer* pCmd) {
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

        graph->execute(m_pRenderTarget, m_pSwapChain);

        timer.set(TIMELINE_LOOP_END);
        deltaTime = timer.interval(TIMELINE_LOOP_BEGIN, TIMELINE_LOOP_END);
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
    m_pDevice->destroy(m_pPipeline);
    m_pDevice->destroy(m_pRenderTarget);
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
