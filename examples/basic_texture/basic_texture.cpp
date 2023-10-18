#include "basic_texture.h"

basic_texture::basic_texture() : aph::BaseApp("base_texture")
{
}

void basic_texture::init()
{
    // setup window
    m_wsi = aph::WSI::Create(m_options.windowWidth, m_options.windowHeight);

    aph::RenderConfig config{
        .flags     = aph::RENDER_CFG_ALL,
        .maxFrames = 1,
    };

    m_renderer = aph::IRenderer::Create<aph::vk::Renderer>(m_wsi.get(), config);
    m_pDevice  = m_renderer->m_pDevice.get();

    m_wsi->registerEventHandler<aph::WindowResizeEvent>([this](const aph::WindowResizeEvent& e) {
        m_renderer->m_pSwapChain->reCreate();
        return true;
    });

    // setup quad
    {
        struct VertexData
        {
            glm::vec3 pos;
            glm::vec2 uv;
        };
        // vertex buffer
        {
            // vertex: position, color
            std::vector<VertexData> vertices = {{.pos = {-0.5f, -0.5f, 0.0f}, .uv = {0.0f, 0.0f}},
                                                {.pos = {0.5f, -0.5f, 0.0f}, .uv = {1.0f, 0.0f}},
                                                {.pos = {0.5f, 0.5f, 0.0f}, .uv = {1.0f, 1.0f}},
                                                {.pos = {-0.5f, 0.5f, 0.0f}, .uv = {0.0f, 1.0f}}};

            aph::BufferLoadInfo loadInfo{
                .data       = vertices.data(),
                .createInfo = {.size  = static_cast<uint32_t>(vertices.size() * sizeof(vertices[0])),
                               .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT}};

            m_renderer->m_pResourceLoader->load(loadInfo, &m_pVB);
        }

        // index buffer
        {
            std::vector<uint32_t> indices = {
                0, 1, 2,  // First triangle
                2, 3, 0   // Second triangle
            };

            aph::BufferLoadInfo loadInfo{
                .data       = indices.data(),
                .createInfo = {.size  = static_cast<uint32_t>(indices.size() * sizeof(indices[0])),
                               .usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT}};
            m_renderer->m_pResourceLoader->load(loadInfo, &m_pIB);
        }

        // image and sampler
        {
            m_pDevice->create(aph::vk::init::samplerCreateInfo2(aph::SamplerPreset::Linear), &m_pSampler);

            aph::vk::ImageCreateInfo imageCI{
                .alignment = 0,
                .arraySize = 1,
                .usage     = VK_IMAGE_USAGE_SAMPLED_BIT,
                .domain    = aph::ImageDomain::Device,
                .imageType = VK_IMAGE_TYPE_2D,
            };

            aph::ImageLoadInfo loadInfo{.data        = aph::asset::GetTextureDir() / "container2.png",
                                        .pCreateInfo = &imageCI};

            m_renderer->m_pResourceLoader->load(loadInfo, &m_pImage);

            m_pDevice->executeSingleCommands(aph::QueueType::GRAPHICS, [&](aph::vk::CommandBuffer* cmd) {
                aph::vk::ImageBarrier barrier{
                    .pImage       = m_pImage,
                    .currentState = aph::RESOURCE_STATE_UNDEFINED,
                    .newState     = aph::RESOURCE_STATE_SHADER_RESOURCE,
                };
                cmd->insertBarrier({barrier});
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

            auto shaderDir = aph::asset::GetShaderDir(aph::asset::ShaderType::GLSL) / "default";

            aph::vk::Shader* pVS = {};
            aph::vk::Shader* pFS = {};

            m_renderer->m_pResourceLoader->load({.data = shaderDir / "texture.vert"}, &pVS);
            m_renderer->m_pResourceLoader->load({.data = shaderDir / "texture.frag"}, &pFS);

            aph::vk::GraphicsPipelineCreateInfo createInfo{
                .vertexInput = vdesc,
                .pVertex     = pVS,
                .pFragment   = pFS,
                .color       = {{.format = m_renderer->m_pSwapChain->getFormat()}},
            };

            VK_CHECK_RESULT(m_pDevice->create(createInfo, &m_pPipeline));
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
    while(m_wsi->update())
    {
        static float deltaTime = {};
        auto         timer     = aph::Timer(deltaTime);

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
        cb->bindDescriptorSet({m_pTextureSet});
        cb->beginRendering({.offset = {0, 0}, .extent = {extent}}, {presentImage});
        cb->insertDebugLabel({
            .name  = "draw a quad with texture",
            .color = {0.5f, 0.3f, 0.2f, 1.0f},
        });
        cb->drawIndexed(6, 1, 0, 0, 0);
        cb->endRendering();
        cb->end();

        m_renderer->submit(queue, {.commandBuffers = {cb}}, presentImage);

        m_renderer->endFrame();
    }
}

void basic_texture::finish()
{
    m_renderer->m_pDevice->waitIdle();
    m_pDevice->destroy(m_pVB);
    m_pDevice->destroy(m_pIB);
    m_pDevice->destroy(m_pPipeline);
    m_pDevice->destroy(m_pImage);
    m_pDevice->destroy(m_pSampler);
}

int main(int argc, char** argv)
{
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
    app.run();
    app.finish();
}
