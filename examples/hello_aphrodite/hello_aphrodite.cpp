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
    : aph::App("hello aphrdite")
{
}

void hello_aphrodite::init()
{
    APH_PROFILER_SCOPE();

    // setup window
    aph::RenderConfig config{
        .maxFrames = 3,
        .width = getOptions().getWindowWidth(),
        .height = getOptions().getWindowHeight(),
    };

    m_renderer = aph::Renderer::Create(config);
    m_pDevice = m_renderer->getDevice();
    m_pSwapChain = m_renderer->getSwapchain();
    m_pResourceLoader = m_renderer->getResourceLoader();
    m_pWindowSystem = m_renderer->getWindowSystem();

    m_pWindowSystem->registerEvent(
        [this](const aph::WindowResizeEvent& e) -> bool
        {
            m_pSwapChain->reCreate();
            return true;
        });

    m_pWindowSystem->registerEvent(
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
        aph::LoadRequest loadRequest = m_pResourceLoader->getLoadRequest();
        // vertex: position, color
        std::vector<VertexData> vertices;
        std::vector<uint32_t> indices;
        createCube(vertices, indices);

        // vertex buffer
        {
            aph::BufferLoadInfo bufferLoadInfo{ .debugName = "cube::vertex_buffer",
                                                .data = vertices.data(),
                                                .createInfo = {
                                                    .size = vertices.size() * sizeof(vertices[0]),
                                                    .usage = aph::BufferUsage::Storage | aph::BufferUsage::Vertex,
                                                    .domain = aph::MemoryDomain::Device,
                                                } };

            loadRequest.add(bufferLoadInfo, &m_pVertexBuffer);
        }

        // index buffer
        {
            aph::BufferLoadInfo bufferLoadInfo{ .debugName = "cube::index_buffer",
                                                .data = indices.data(),
                                                .createInfo = {
                                                    .size = indices.size() * sizeof(indices[0]),
                                                    .usage = aph::BufferUsage::Storage | aph::BufferUsage::Index,
                                                    .domain = aph::MemoryDomain::Device,
                                                } };

            loadRequest.add(bufferLoadInfo, &m_pIndexBuffer);
        }

        // matrix uniform buffer
        {
            m_camera.setLookAt({ 0.0f, 0.0f, 3.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f })
                .setProjection(aph::Perspective{
                    .aspect = static_cast<float>(getOptions().getWindowWidth()) /
                              static_cast<float>(getOptions().getWindowHeight()),
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
                                                    .usage = aph::BufferUsage::Uniform,
                                                    .domain = aph::MemoryDomain::Host,
                                                } };

            loadRequest.add(bufferLoadInfo, &m_pMatrixBffer);
        }

        // image and sampler
        {
            APH_VR(
                m_pDevice->create(aph::vk::SamplerCreateInfo{}.preset(aph::SamplerPreset::LinearClamp), &m_pSampler));

            aph::ImageLoadInfo imageLoadInfo{ .debugName = "container texture",
                                              .data = "texture://container2.png",
                                              .createInfo = {
                                                  .usage = aph::ImageUsage::Sampled,
                                                  .domain = aph::MemoryDomain::Device,
                                                  .imageType = aph::ImageType::e2D,
                                              } };

            loadRequest.add(imageLoadInfo, &m_pImage);
        }

        loadRequest.load();
    }

    // setup pipeline
    {
        aph::LoadRequest loadRequest = m_pResourceLoader->getLoadRequest();
        // geometry shading
        {
            aph::ShaderLoadInfo shaderLoadInfo{ .debugName = "vs + fs",
                                                .data = { "shader_slang://hello_geometry.slang" },
                                                .stageInfo = {
                                                    { aph::ShaderStage::VS, "vertexMain" },
                                                    { aph::ShaderStage::FS, "fragMain" },
                                                } };

            loadRequest.add(shaderLoadInfo, &m_program[ShadingType::Geometry]);
        }

        // mesh shading
        {
            aph::ShaderLoadInfo shaderLoadInfo{ .debugName = "ts + ms + fs",
                                                .data = { "shader_slang://hello_mesh.slang" },
                                                .stageInfo = {
                                                    { aph::ShaderStage::TS, "taskMain" },
                                                    { aph::ShaderStage::MS, "meshMain" },
                                                    { aph::ShaderStage::FS, "fragMain" },
                                                } };

            loadRequest.add(shaderLoadInfo, &m_program[ShadingType::Mesh]);
        }

        // bindless mesh shading
        {
            auto bindless = m_pDevice->getBindlessResource();
            {
                bindless->updateResource(m_pImage, "texture_container");
                bindless->updateResource(m_pSampler, "samp");
                bindless->updateResource(m_pMatrixBffer, "transform_cube");
                bindless->updateResource(m_pVertexBuffer, "vertex_cube");
                bindless->updateResource(m_pIndexBuffer, "index_cube");
            }

            aph::ShaderLoadInfo shaderLoadInfo{ .debugName = "ts + ms + fs (bindless)",
                                                .data = { "shader_slang://hello_mesh_bindless.slang" },
                                                .stageInfo = {
                                                    {  aph::ShaderStage::TS,  "taskMain" },
                                                    {  aph::ShaderStage::MS,  "meshMain" },
                                                    {  aph::ShaderStage::FS,  "fragMain" },
                                                }, .pBindlessResource = bindless};

            loadRequest.add(shaderLoadInfo, &m_program[ShadingType::MeshBindless]);
        }

        loadRequest.load();
    }

    // record graph execution
    {
        for (auto* graph : m_renderer->recordGraph())
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
            drawPass->addUniformBufferIn("matrix ubo", m_pMatrixBffer);

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
                        pCmd->bindVertexBuffers(m_pVertexBuffer);
                        pCmd->bindIndexBuffers(m_pIndexBuffer);
                        pCmd->setResource({ m_pMatrixBffer }, 0, 0);
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
                        pCmd->setResource({ m_pMatrixBffer }, 0, 0);
                        pCmd->setResource({ m_pImage }, 1, 0);
                        pCmd->setResource({ m_pSampler }, 1, 1);
                        pCmd->setResource({ m_pVertexBuffer }, 0, 1);
                        pCmd->setResource({ m_pIndexBuffer }, 0, 2);
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
                        pCmd->draw(aph::DispatchArguments{ 1, 1, 1 });
                        pCmd->endDebugLabel();
                    }
                    break;
                    }
                });
        }
    }
}

void hello_aphrodite::loop()
{
    while (m_pWindowSystem->update())
    {
        APH_PROFILER_FRAME("application loop");
        m_mvp.model = glm::rotate(m_mvp.model, (float)m_renderer->getCPUFrameTime(), { 0.5f, 1.0f, 0.0f });
        m_pResourceLoader->update({ .data = &m_mvp, .range = { 0, sizeof(m_mvp) } }, &m_pMatrixBffer);
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
    m_pDevice->destroy(m_pSampler);
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

    auto result =
        app.getOptions()
            .setVsync(false)
            .addCLICallback("--shading-type", [&app](std::string_view value) { app.switchShadingType(value); })
            .parse(argc, argv);

    APH_VR(result);

    app.run();
}
