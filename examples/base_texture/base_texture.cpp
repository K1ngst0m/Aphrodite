#include "base_texture.h"

base_texture::base_texture() : aph::BaseApp("base_texture")
{
}

void base_texture::init()
{
    // setup window
    m_wsi = aph::WSI::Create(m_options.windowWidth, m_options.windowHeight);

    aph::RenderConfig config{
        .flags     = aph::RENDER_CFG_ALL,
        .maxFrames = 1,
    };

    m_renderer = aph::IRenderer::Create<aph::vk::Renderer>(m_wsi.get(), config);
    m_pDevice  = m_renderer->m_pDevice.get();

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
                               .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT},
                .ppBuffer   = &m_pVB};

            m_renderer->m_pResourceLoader->loadBuffers(loadInfo);
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
                               .usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT},
                .ppBuffer   = &m_pIB};
            m_renderer->m_pResourceLoader->loadBuffers(loadInfo);
        }

        // image and sampler
        {
            m_pDevice->createSampler(aph::SamplerPreset::Linear, &m_pSampler, false);

            aph::vk::ImageCreateInfo imageCI{
                .alignment = 0,
                .arraySize = 1,
                .usage     = VK_IMAGE_USAGE_SAMPLED_BIT,
                .domain    = aph::ImageDomain::Device,
                .imageType = VK_IMAGE_TYPE_2D,
                .tiling    = VK_IMAGE_TILING_OPTIMAL,
            };

            aph::ImageLoadInfo loadInfo{
                .data          = aph::asset::GetTextureDir() / "container2.png",
                .containerType = aph::ImageContainerType::Png,
                .pCreateInfo   = &imageCI,
                .ppImage       = &m_pImage,
            };

            m_renderer->m_pResourceLoader->loadImages(loadInfo);

            m_pDevice->executeSingleCommands(aph::vk::QueueType::GRAPHICS, [&](aph::vk::CommandBuffer* cmd) {
                cmd->transitionImageLayout(m_pImage, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            });
        }

        // pipeline
        {
            const aph::vk::VertexInput vdesc = {
                .attributes =
                    {
                        {.location = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(VertexData, pos)},
                        {.location = 1, .format = VK_FORMAT_R32G32_SFLOAT, .offset = offsetof(VertexData, uv)},
                    },
                .inputBindings = {{.stride = sizeof(VertexData)}},
            };

            auto shaderDir = aph::asset::GetShaderDir(aph::asset::ShaderType::GLSL) / "default";
            auto vs        = m_renderer->getShaders(shaderDir / "texture.vert");
            auto fs        = m_renderer->getShaders(shaderDir / "texture.frag");
            m_pDevice->createShaderProgram(&m_pShaderProgram, vs, fs);
            aph::vk::GraphicsPipelineCreateInfo createInfo{
                .vertexInput = vdesc,
                .pProgram    = m_pShaderProgram,
                .color       = {{.format = m_renderer->m_pSwapChain->getFormat()}},
            };

            VK_CHECK_RESULT(m_pDevice->createGraphicsPipeline(createInfo, &m_pPipeline));
        }

        // descriptor set
        {
            VkDescriptorImageInfo textureInfo{
                .sampler     = m_pSampler->getHandle(),
                .imageView   = m_pImage->getView()->getHandle(),
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            };
            m_textureSet = m_pPipeline->getProgram()->getSetLayout(0)->allocateSet();
            std::vector<VkWriteDescriptorSet> writes{
                aph::vk::init::writeDescriptorSet(m_textureSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0,
                                                  &textureInfo),
            };

            vkUpdateDescriptorSets(m_pDevice->getHandle(), writes.size(), writes.data(), 0, nullptr);
        }
    }
}

void base_texture::run()
{
    while(m_wsi->update())
    {
        static float deltaTime = {};
        auto         timer     = aph::Timer(deltaTime);

        auto* queue = m_renderer->getDefaultQueue(aph::vk::QueueType::GRAPHICS);

        // draw and submit
        m_renderer->beginFrame();
        aph::vk::CommandBuffer* cb = m_renderer->acquireFrameCommandBuffer(queue);

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
        cb->bindDescriptorSet({m_textureSet});
        cb->beginRendering({.offset = {0, 0}, .extent = {extent}}, {presentImage});
        cb->insertDebugLabel({
            .name  = "draw a quad with texture",
            .color = {0.5f, 0.3f, 0.2f, 1.0f},
        });
        cb->drawIndexed(6, 1, 0, 0, 0);
        cb->endRendering();
        cb->end();

        aph::vk::QueueSubmitInfo submitInfo{};
        submitInfo.commandBuffers.push_back(cb);
        submitInfo.waitSemaphores.push_back(m_renderer->getRenderSemaphore());
        submitInfo.signalSemaphores.push_back(m_renderer->getPresentSemaphore());
        queue->submit({submitInfo}, m_renderer->getFrameFence());

        m_renderer->endFrame();
    }
}

void base_texture::finish()
{
    m_renderer->m_pDevice->waitIdle();
    m_pDevice->destroyBuffer(m_pVB);
    m_pDevice->destroyBuffer(m_pIB);
    m_pDevice->destroyPipeline(m_pPipeline);
    m_pDevice->destroyShaderProgram(m_pShaderProgram);
    m_pDevice->destroyImage(m_pImage);
    m_pDevice->destroySampler(m_pSampler);
}

int main()
{
    base_texture app;

    app.init();
    app.run();
    app.finish();
}
