#include "hello_aphrodite.h"

#include "api/vulkan/bindless.h"

struct VertexData
{
    glm::vec4 pos;
    glm::vec2 uv;
    glm::vec2 padding;
};

void createCube(std::vector<VertexData>& outVertices, std::vector<uint32_t>& outIndices)
{
    // Each face is defined in a counter-clockwise (CCW) order
    // when viewed from the outside of the cube.

    // Front face
    // z = +0.5, from top-left -> top-right -> bottom-right -> bottom-left
    VertexData f0 = { { -0.5f, 0.5f, 0.5f, 1.0f }, { 0.0f, 0.0f } };
    VertexData f1 = { { 0.5f, 0.5f, 0.5f, 1.0f }, { 1.0f, 0.0f } };
    VertexData f2 = { { 0.5f, -0.5f, 0.5f, 1.0f }, { 1.0f, 1.0f } };
    VertexData f3 = { { -0.5f, -0.5f, 0.5f, 1.0f }, { 0.0f, 1.0f } };

    // Back face
    // z = -0.5, from top-left -> top-right -> bottom-right -> bottom-left
    VertexData b0 = { { 0.5f, 0.5f, -0.5f, 1.0f }, { 0.0f, 0.0f } };
    VertexData b1 = { { -0.5f, 0.5f, -0.5f, 1.0f }, { 1.0f, 0.0f } };
    VertexData b2 = { { -0.5f, -0.5f, -0.5f, 1.0f }, { 1.0f, 1.0f } };
    VertexData b3 = { { 0.5f, -0.5f, -0.5f, 1.0f }, { 0.0f, 1.0f } };

    // Left face
    // x = -0.5, from top-left -> top-right -> bottom-right -> bottom-left
    VertexData l0 = { { -0.5f, 0.5f, -0.5f, 1.0f }, { 0.0f, 0.0f } };
    VertexData l1 = { { -0.5f, 0.5f, 0.5f, 1.0f }, { 1.0f, 0.0f } };
    VertexData l2 = { { -0.5f, -0.5f, 0.5f, 1.0f }, { 1.0f, 1.0f } };
    VertexData l3 = { { -0.5f, -0.5f, -0.5f, 1.0f }, { 0.0f, 1.0f } };

    // Right face
    // x = +0.5, from top-left -> top-right -> bottom-right -> bottom-left
    VertexData r0 = { { 0.5f, 0.5f, 0.5f, 1.0f }, { 0.0f, 0.0f } };
    VertexData r1 = { { 0.5f, 0.5f, -0.5f, 1.0f }, { 1.0f, 0.0f } };
    VertexData r2 = { { 0.5f, -0.5f, -0.5f, 1.0f }, { 1.0f, 1.0f } };
    VertexData r3 = { { 0.5f, -0.5f, 0.5f, 1.0f }, { 0.0f, 1.0f } };

    // Top face
    // y = +0.5, from front-left -> front-right -> back-right -> back-left
    VertexData t0 = { { -0.5f, 0.5f, 0.5f, 1.0f }, { 0.0f, 0.0f } };
    VertexData t1 = { { 0.5f, 0.5f, 0.5f, 1.0f }, { 1.0f, 0.0f } };
    VertexData t2 = { { 0.5f, 0.5f, -0.5f, 1.0f }, { 1.0f, 1.0f } };
    VertexData t3 = { { -0.5f, 0.5f, -0.5f, 1.0f }, { 0.0f, 1.0f } };

    // Bottom face
    // y = -0.5, from front-left -> front-right -> back-right -> back-left
    VertexData bo0 = { { -0.5f, -0.5f, 0.5f, 1.0f }, { 0.0f, 0.0f } };
    VertexData bo1 = { { 0.5f, -0.5f, 0.5f, 1.0f }, { 1.0f, 0.0f } };
    VertexData bo2 = { { 0.5f, -0.5f, -0.5f, 1.0f }, { 1.0f, 1.0f } };
    VertexData bo3 = { { -0.5f, -0.5f, -0.5f, 1.0f }, { 0.0f, 1.0f } };

    // Collect all 24 vertices in a single array
    outVertices = { // Front
                    f0, f1, f2, f3,
                    // Back
                    b0, b1, b2, b3,
                    // Left
                    l0, l1, l2, l3,
                    // Right
                    r0, r1, r2, r3,
                    // Top
                    t0, t1, t2, t3,
                    // Bottom
                    bo0, bo1, bo2, bo3
    };

    // For each face, the two triangles are formed by these index patterns:
    //   (0, 1, 2) and (2, 3, 0)
    // We repeat the same pattern for each face block of 4 vertices.

    // Indices for each face block of 4:
    //  - Face 0 (front)  : 0..3
    //  - Face 1 (back)   : 4..7
    //  - Face 2 (left)   : 8..11
    //  - Face 3 (right)  : 12..15
    //  - Face 4 (top)    : 16..19
    //  - Face 5 (bottom) : 20..23
    outIndices = { // front
                   0, 1, 2, 2, 3, 0,
                   // back
                   4, 5, 6, 6, 7, 4,
                   // left
                   8, 9, 10, 10, 11, 8,
                   // right
                   12, 13, 14, 14, 15, 12,
                   // top
                   16, 17, 18, 18, 19, 16,
                   // bottom
                   20, 21, 22, 22, 23, 20
    };
}

hello_aphrodite::hello_aphrodite()
    : aph::BaseApp<hello_aphrodite>("base_texture")
{
}

void hello_aphrodite::init()
{
    APH_PROFILER_SCOPE();

    // setup window
    aph::RenderConfig config{
        .maxFrames = 3,
        .width = getOptions().windowWidth,
        .height = getOptions().windowHeight,
    };

    m_renderer = aph::Renderer::Create(config);
    m_pDevice = m_renderer->getDevice();
    m_pSwapChain = m_renderer->getSwapchain();
    m_pResourceLoader = m_renderer->getResourceLoader();
    m_pWindowSystem = m_renderer->getWindowSystem();

    m_pWindowSystem->registerEvent<aph::WindowResizeEvent>(
        [this](const aph::WindowResizeEvent& e)
        {
            m_pSwapChain->reCreate();
            return true;
        });

    m_pWindowSystem->registerEvent<aph::KeyboardEvent>(
        [this](const aph::KeyboardEvent& e)
        {
            APP_LOG_INFO("key pressed.");
            if (e.m_key == aph::Key::Space && e.m_state == aph::KeyState::Pressed)
            {
                switch (m_shadingType)
                {
                case ShadingType::Geometry:
                    m_shadingType = ShadingType::Mesh;
                    break;
                case ShadingType::Mesh:
                    m_shadingType = ShadingType::MeshBindless;
                    break;
                case ShadingType::MeshBindless:
                    m_shadingType = ShadingType::Geometry;
                    break;
                }
                APH_VR(m_pDevice->waitIdle());
                switchShadingType(m_shadingType);
            }
            return true;
        });

    // setup cube
    {
        // vertex: position, color
        std::vector<VertexData> vertices;
        std::vector<uint32_t> indices;
        createCube(vertices, indices);

        // vertex buffer
        {
            aph::BufferLoadInfo bufferLoadInfo{ .debugName = "cube::vertex_buffer",
                                                .data = vertices.data(),
                                                .createInfo = {
                                                    .size =
                                                        static_cast<uint32_t>(vertices.size() * sizeof(vertices[0])),
                                                    .usage = ::vk::BufferUsageFlagBits::eStorageBuffer |
                                                             ::vk::BufferUsageFlagBits::eVertexBuffer,
                                                    .domain = aph::BufferDomain::Device,
                                                } };

            m_pResourceLoader->loadAsync(bufferLoadInfo, &m_pVB);
        }

        // index buffer
        {
            aph::BufferLoadInfo bufferLoadInfo{ .debugName = "cube::index_buffer",
                                                .data = indices.data(),
                                                .createInfo = {
                                                    .size = static_cast<uint32_t>(indices.size() * sizeof(indices[0])),
                                                    .usage = ::vk::BufferUsageFlagBits::eStorageBuffer |
                                                             ::vk::BufferUsageFlagBits::eIndexBuffer,
                                                    .domain = aph::BufferDomain::Device,
                                                } };

            m_pResourceLoader->loadAsync(bufferLoadInfo, &m_pIB);
        }

        // matrix uniform buffer
        {
            m_camera.setLookAt({ 0.0f, 0.0f, 3.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f })
                .setProjection(aph::Perspective{
                    .aspect =
                        static_cast<float>(getOptions().windowWidth) / static_cast<float>(getOptions().windowHeight),
                    .fov = 90.0f,
                    .znear = 0.1f,
                    .zfar = 100.0f,
                });

            m_mvp.view = m_camera.getView();
            m_mvp.proj = m_camera.getProjection();

            aph::BufferLoadInfo bufferLoadInfo{ .debugName = "matrix data",
                                                .data = &m_mvp,
                                                .createInfo = {
                                                    .size = sizeof(m_mvp),
                                                    .usage = ::vk::BufferUsageFlagBits::eUniformBuffer,
                                                    .domain = aph::BufferDomain::LinkedDeviceHost,
                                                } };

            m_pResourceLoader->loadAsync(bufferLoadInfo, &m_pMatBuffer);
        }

        // image and sampler
        {
            APH_VR(
                m_pDevice->create(aph::vk::SamplerCreateInfo{}.preset(aph::SamplerPreset::LinearClamp), &m_pSampler));
            aph::ImageLoadInfo imageLoadInfo{ .debugName = "container texture",
                                              .data = "texture://container2.png",
                                              .createInfo = {
                                                  .usage = ::vk::ImageUsageFlagBits::eSampled,
                                                  .domain = aph::ImageDomain::Device,
                                                  .imageType = aph::ImageType::e2D,
                                              } };

            m_pResourceLoader->loadAsync(imageLoadInfo, &m_pImage);
        }

        // pipeline
        // mesh shading
        {
            aph::ShaderLoadInfo
                shaderLoadInfo{ .stageInfo = {
                                    { aph::ShaderStage::TS,
                                      { .data = "shader_slang://hello_mesh.slang", .entryPoint = "taskMain" } },
                                    { aph::ShaderStage::MS,
                                      { .data = "shader_slang://hello_mesh.slang", .entryPoint = "meshMain" } },
                                    { aph::ShaderStage::FS,
                                      { .data = "shader_slang://hello_mesh.slang", .entryPoint = "fragMain" } },
                                } };

            auto future = m_pResourceLoader->loadAsync(shaderLoadInfo, &m_program[ShadingType::Mesh]);

            APH_VR(future.get());
        }

        // bindless mesh shading
        {
            aph::ShaderLoadInfo shaderLoadInfo{ .stageInfo = {
                                                    { aph::ShaderStage::TS,
                                                      { .data = "shader_slang://hello_mesh_bindless.slang",
                                                        .entryPoint = "taskMain" } },
                                                    { aph::ShaderStage::MS,
                                                      { .data = "shader_slang://hello_mesh_bindless.slang",
                                                        .entryPoint = "meshMain" } },
                                                    { aph::ShaderStage::FS,
                                                      { .data = "shader_slang://hello_mesh_bindless.slang",
                                                        .entryPoint = "fragMain" } },
                                                } };

            auto& program = m_program[ShadingType::MeshBindless];
            APH_VR(m_pResourceLoader->load(shaderLoadInfo, &program));
            auto bindlessResource = m_pDevice->getBindlessResource(program);
            auto textureHandle = bindlessResource->updateResource(m_pImage, ::vk::ImageUsageFlagBits::eSampled);
            m_handleOffset = bindlessResource->addRange(textureHandle);
        }

        // geometry shading
        {
            aph::ShaderLoadInfo shaderLoadInfo{ .stageInfo = {
                                                    { aph::ShaderStage::VS,
                                                      { .data = "shader_slang://hello_geometry.slang",
                                                        .entryPoint = "vertexMain" } },
                                                    { aph::ShaderStage::FS,
                                                      { .data = "shader_slang://hello_geometry.slang",
                                                        .entryPoint = "fragMain" } },
                                                } };

            m_pResourceLoader->loadAsync(shaderLoadInfo, &m_program[ShadingType::Geometry]);
        }

        m_pResourceLoader->wait();

        // record graph execution
        m_renderer->recordGraph(
            [this](auto* graph)
            {
                auto drawPass = graph->createPass("drawing cube", aph::QueueType::Graphics);
                drawPass->setColorOut("render output",
                                      {
                                          .extent = { m_pSwapChain->getWidth(), m_pSwapChain->getHeight(), 1 },
                                          .format = m_pSwapChain->getFormat(),
                                      });
                drawPass->setDepthStencilOut("depth buffer",
                                             {
                                                 .extent = { m_pSwapChain->getWidth(), m_pSwapChain->getHeight(), 1 },
                                                 .format = aph::Format::D32,
                                             });
                drawPass->addTextureIn("container texture", m_pImage);
                drawPass->addUniformBufferIn("matrix ubo", m_pMatBuffer);

                graph->setBackBuffer("render output");

                drawPass->recordExecute(
                    [this](auto* pCmd)
                    {
                        pCmd->setDepthState({
                            .enable = true,
                            .write = true,
                            .compareOp = aph::CompareOp::Less,
                        });

                        switch (m_shadingType)
                        {
                        case ShadingType::Geometry:
                        {
                            pCmd->beginDebugLabel({
                                .name = "geometry shading path",
                                .color = { 0.5f, 0.3f, 0.2f, 1.0f },
                            });
                            pCmd->setProgram(m_program[ShadingType::Geometry]);
                            pCmd->bindVertexBuffers(m_pVB);
                            pCmd->bindIndexBuffers(m_pIB);
                            pCmd->setResource({ m_pMatBuffer }, 0, 0);
                            pCmd->setResource({ m_pImage }, 1, 0);
                            pCmd->setResource({ m_pSampler }, 1, 1);
                            pCmd->drawIndexed({ 36, 1, 0, 0, 0 });
                            pCmd->endDebugLabel();
                        }
                        break;
                        case ShadingType::Mesh:
                        {
                            pCmd->beginDebugLabel({
                                .name = "mesh shading path",
                                .color = { 0.5f, 0.3f, 0.2f, 1.0f },
                            });
                            pCmd->setProgram(m_program[ShadingType::Mesh]);
                            pCmd->setResource({ m_pMatBuffer }, 0, 0);
                            pCmd->setResource({ m_pImage }, 1, 0);
                            pCmd->setResource({ m_pSampler }, 1, 1);
                            pCmd->setResource({ m_pVB }, 0, 1);
                            pCmd->setResource({ m_pIB }, 0, 2);
                            pCmd->draw(aph::DispatchArguments{ 1, 1, 1 });
                            pCmd->endDebugLabel();
                        }
                        break;
                        case ShadingType::MeshBindless:
                        {
                            pCmd->beginDebugLabel({
                                .name = "mesh shading path (bindless)",
                                .color = { 0.5f, 0.3f, 0.2f, 1.0f },
                            });
                            pCmd->setProgram(m_program[ShadingType::MeshBindless]);
                            pCmd->setResource({ m_pMatBuffer }, 2, 0);
                            pCmd->setResource({ m_pVB }, 2, 1);
                            pCmd->setResource({ m_pIB }, 2, 2);
                            pCmd->setResource({ m_pSampler }, 2, 3);

                            pCmd->draw(aph::DispatchArguments{ 1, 1, 1 }, { m_handleOffset });
                            pCmd->endDebugLabel();
                        }
                        break;
                        }
                    });
            });
    }
}

void hello_aphrodite::loop()
{
    while (m_pWindowSystem->update())
    {
        APH_PROFILER_SCOPE_NAME("application loop");
        m_mvp.model = glm::rotate(m_mvp.model, (float)m_renderer->getCPUFrameTime(), { 0.5f, 1.0f, 0.0f });
        m_pResourceLoader->update({ .data = &m_mvp, .range = { 0, sizeof(m_mvp) } }, &m_pMatBuffer);
        m_renderer->update();
        m_renderer->render();
    }
}

void hello_aphrodite::load()
{
    APH_PROFILER_SCOPE();
    m_renderer->load();
}
void hello_aphrodite::unload()
{
    APH_PROFILER_SCOPE();
    m_renderer->unload();
}

void hello_aphrodite::finish()
{
    APH_PROFILER_SCOPE();
    APH_VR(m_pDevice->waitIdle());
    m_pDevice->destroy(m_pVB);
    m_pDevice->destroy(m_pIB);
    m_pDevice->destroy(m_pMatBuffer);
    m_pDevice->destroy(m_pImage);
    m_pDevice->destroy(m_pSampler);
    for (auto [_, program] : m_program)
    {
        m_pDevice->destroy(program);
    }
}

void hello_aphrodite::switchShadingType(ShadingType type)
{
    switch (type)
    {
    case ShadingType::Geometry:
        APP_LOG_INFO("Switch to geometry shading.");
    case ShadingType::Mesh:
        APP_LOG_INFO("Switch to mesh shading.");
        break;
    case ShadingType::MeshBindless:
        APP_LOG_INFO("Switch to mesh shading (bindless).");
        break;
    }
    m_shadingType = type;
}

void hello_aphrodite::switchShadingType(std::string_view value)
{
    ShadingType type = ShadingType::Geometry;
    if (value == "geometry")
    {
        type = ShadingType::Geometry;
    }
    else if (value == "mesh")
    {
        type = ShadingType::Mesh;
    }
    else if (value == "mesh_bindless")
    {
        type = ShadingType::MeshBindless;
    }
    else
    {
        APP_LOG_WARN("Invalid Shading type [%s].", value);
    }

    switchShadingType(type);
}

int main(int argc, char** argv)
{
    hello_aphrodite app{};

    app.setVsync(false)
        .addCLIOption("--shading-type", [&app](auto& parser) { app.switchShadingType(parser.nextString()); })
        .loadConfig(argc, argv)
        .run();
}
