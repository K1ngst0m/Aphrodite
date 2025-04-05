#include "hello_aphrodite.h"

// Vertex structure for the cube with position and texture coordinates
struct VertexData
{
    aph::Vec4 pos;
    aph::Vec2 uv;
    aph::Vec2 padding;
};

// Creates a 3D cube mesh with position and texture coordinates
void createCube(std::vector<VertexData>& outVertices, std::vector<uint32_t>& outIndices)
{
    // Each face is defined in a counter-clockwise (CCW) order
    // when viewed from the outside of the cube.

    // Front face
    // z = +0.5, from top-left -> top-right -> bottom-right -> bottom-left
    VertexData f0 = {{-0.5f, 0.5f, 0.5f, 1.0f}, {0.0f, 0.0f}};
    VertexData f1 = {{0.5f, 0.5f, 0.5f, 1.0f}, {1.0f, 0.0f}};
    VertexData f2 = {{0.5f, -0.5f, 0.5f, 1.0f}, {1.0f, 1.0f}};
    VertexData f3 = {{-0.5f, -0.5f, 0.5f, 1.0f}, {0.0f, 1.0f}};

    // Back face
    // z = -0.5, from top-left -> top-right -> bottom-right -> bottom-left
    VertexData b0 = {{0.5f, 0.5f, -0.5f, 1.0f}, {0.0f, 0.0f}};
    VertexData b1 = {{-0.5f, 0.5f, -0.5f, 1.0f}, {1.0f, 0.0f}};
    VertexData b2 = {{-0.5f, -0.5f, -0.5f, 1.0f}, {1.0f, 1.0f}};
    VertexData b3 = {{0.5f, -0.5f, -0.5f, 1.0f}, {0.0f, 1.0f}};

    // Left face
    // x = -0.5, from top-left -> top-right -> bottom-right -> bottom-left
    VertexData l0 = {{-0.5f, 0.5f, -0.5f, 1.0f}, {0.0f, 0.0f}};
    VertexData l1 = {{-0.5f, 0.5f, 0.5f, 1.0f}, {1.0f, 0.0f}};
    VertexData l2 = {{-0.5f, -0.5f, 0.5f, 1.0f}, {1.0f, 1.0f}};
    VertexData l3 = {{-0.5f, -0.5f, -0.5f, 1.0f}, {0.0f, 1.0f}};

    // Right face
    // x = +0.5, from top-left -> top-right -> bottom-right -> bottom-left
    VertexData r0 = {{0.5f, 0.5f, 0.5f, 1.0f}, {0.0f, 0.0f}};
    VertexData r1 = {{0.5f, 0.5f, -0.5f, 1.0f}, {1.0f, 0.0f}};
    VertexData r2 = {{0.5f, -0.5f, -0.5f, 1.0f}, {1.0f, 1.0f}};
    VertexData r3 = {{0.5f, -0.5f, 0.5f, 1.0f}, {0.0f, 1.0f}};

    // Top face
    // y = +0.5, from front-left -> front-right -> back-right -> back-left
    VertexData t0 = {{-0.5f, 0.5f, 0.5f, 1.0f}, {0.0f, 0.0f}};
    VertexData t1 = {{0.5f, 0.5f, 0.5f, 1.0f}, {1.0f, 0.0f}};
    VertexData t2 = {{0.5f, 0.5f, -0.5f, 1.0f}, {1.0f, 1.0f}};
    VertexData t3 = {{-0.5f, 0.5f, -0.5f, 1.0f}, {0.0f, 1.0f}};

    // Bottom face
    // y = -0.5, from front-left -> front-right -> back-right -> back-left
    VertexData bo0 = {{-0.5f, -0.5f, 0.5f, 1.0f}, {0.0f, 0.0f}};
    VertexData bo1 = {{0.5f, -0.5f, 0.5f, 1.0f}, {1.0f, 0.0f}};
    VertexData bo2 = {{0.5f, -0.5f, -0.5f, 1.0f}, {1.0f, 1.0f}};
    VertexData bo3 = {{-0.5f, -0.5f, -0.5f, 1.0f}, {0.0f, 1.0f}};

    // Collect all 24 vertices in a single array
    outVertices = {// Front
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
                   bo0, bo1, bo2, bo3};

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
    outIndices = {// front
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
                  20, 21, 22, 22, 23, 20};
}

HelloAphrodite::HelloAphrodite()
    : aph::App("hello aphrdite")
{
}

void HelloAphrodite::init()
{
    APH_PROFILER_SCOPE();

    // Initialize engine and systems
    setupEngine();
    setupEventHandlers();
}

void HelloAphrodite::setupEngine()
{
    // Configure and create the engine
    aph::EngineConfig config;
    config.setMaxFrames(3).setWidth(getOptions().getWindowWidth()).setHeight(getOptions().getWindowHeight());

    m_engine = aph::Engine::Create(config);

    // Get references to core engine components
    m_pDevice = m_engine->getDevice();
    m_pSwapChain = m_engine->getSwapchain();
    m_pResourceLoader = m_engine->getResourceLoader();
    m_pWindowSystem = m_engine->getWindowSystem();
}

void HelloAphrodite::setupEventHandlers()
{
    // Window resize handler
    m_pWindowSystem->registerEvent(
        [this](const aph::WindowResizeEvent& e) -> bool
        {
            m_pSwapChain->reCreate();
            return true;
        });

    // Keyboard input handler - toggle shading mode on spacebar press
    m_pWindowSystem->registerEvent(
        [this](const aph::KeyboardEvent& e)
        {
            if (e.m_key == aph::Key::Space && e.m_state == aph::KeyState::Pressed)
            {
                APP_LOG_INFO("Key pressed: switching shading mode");

                // Cycle through available shading types
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

                APH_VERIFY_RESULT(m_pDevice->waitIdle());
                switchShadingType(m_shadingType);
            }
            return true;
        });
}

void HelloAphrodite::loop()
{
    for (auto frameResource : m_engine->loop())
    {
        APH_PROFILER_FRAME("application loop");

        // Rotate the model
        m_mvp.model = aph::Rotate(m_mvp.model, (float)m_engine->getCPUFrameTime(), {0.5f, 1.0f, 0.0f});

        // Update the transformation matrix buffer
        m_pResourceLoader->update({.data = &m_mvp, .range = {0, sizeof(m_mvp)}}, &m_pMatrixBffer);

        // Build the render graph for this frame
        buildGraph(frameResource.pGraph);
    }
}

void HelloAphrodite::load()
{
    APH_PROFILER_SCOPE();

    loadResources();
    setupRenderGraph();
    m_engine->load();
}

void HelloAphrodite::loadResources()
{
    // -------- Load geometry resources --------
    aph::LoadRequest geometryRequest = m_pResourceLoader->getLoadRequest();

    // Create cube mesh (vertices and indices)
    std::vector<VertexData> vertices;
    std::vector<uint32_t> indices;
    createCube(vertices, indices);

    // Create vertex buffer
    {
        aph::BufferLoadInfo bufferLoadInfo{.debugName = "cube::vertex_buffer",
                                           .data = vertices.data(),
                                           .createInfo = {
                                               .size = vertices.size() * sizeof(vertices[0]),
                                               .usage = aph::BufferUsage::Storage | aph::BufferUsage::Vertex,
                                               .domain = aph::MemoryDomain::Device,
                                           }};
        geometryRequest.add(bufferLoadInfo, &m_pVertexBuffer);
    }

    // Create index buffer
    {
        aph::BufferLoadInfo bufferLoadInfo{.debugName = "cube::index_buffer",
                                           .data = indices.data(),
                                           .createInfo = {
                                               .size = indices.size() * sizeof(indices[0]),
                                               .usage = aph::BufferUsage::Storage | aph::BufferUsage::Index,
                                               .domain = aph::MemoryDomain::Device,
                                           }};
        geometryRequest.add(bufferLoadInfo, &m_pIndexBuffer);
    }

    // Setup camera and create matrix buffer
    {
        // Configure perspective camera
        float aspectRatio =
            static_cast<float>(getOptions().getWindowWidth()) / static_cast<float>(getOptions().getWindowHeight());

        m_camera.setLookAt({0.0f, 0.0f, 3.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f})
            .setProjection(aph::PerspectiveInfo{
                .aspect = aspectRatio,
                .fov = 90.0f,
                .znear = 0.1f,
                .zfar = 100.0f,
            });

        // Initialize the MVP matrices
        m_mvp.view = m_camera.getView();
        m_mvp.proj = m_camera.getProjection();

        // Create uniform buffer for matrices
        aph::BufferLoadInfo bufferLoadInfo{.debugName = "matrix data",
                                           .data = &m_mvp,
                                           .createInfo = {
                                               .size = sizeof(m_mvp),
                                               .usage = aph::BufferUsage::Uniform,
                                               .domain = aph::MemoryDomain::Host,
                                           }};
        geometryRequest.add(bufferLoadInfo, &m_pMatrixBffer);
    }

    // Create sampler and load texture
    {
        // Create a linear clamp sampler
        m_pSampler = m_pDevice->create(aph::vk::SamplerCreateInfo{}.preset(aph::SamplerPreset::LinearClamp));

        // Load container texture
        aph::ImageLoadInfo imageLoadInfo{.debugName = "container texture",
                                         .data = "texture://container2.png",
                                         .createInfo = {
                                             .usage = aph::ImageUsage::Sampled,
                                             .domain = aph::MemoryDomain::Device,
                                             .imageType = aph::ImageType::e2D,
                                         }};
        geometryRequest.add(imageLoadInfo, &m_pImage);
    }

    // Execute all geometry resource loads
    geometryRequest.load();

    // -------- Load shader programs --------
    aph::LoadRequest shaderRequest = m_pResourceLoader->getLoadRequest();

    // Load geometry shading program
    {
        aph::ShaderLoadInfo shaderLoadInfo{.debugName = "vs + fs",
                                           .data = {"shader_slang://hello_geometry.slang"},
                                           .stageInfo = {
                                               {aph::ShaderStage::VS, "vertexMain"},
                                               {aph::ShaderStage::FS, "fragMain"},
                                           }};
        shaderRequest.add(shaderLoadInfo, &m_program[ShadingType::Geometry]);
    }

    // Load mesh shading program
    {
        aph::ShaderLoadInfo shaderLoadInfo{.debugName = "ts + ms + fs",
                                           .data = {"shader_slang://hello_mesh.slang"},
                                           .stageInfo = {
                                               {aph::ShaderStage::TS, "taskMain"},
                                               {aph::ShaderStage::MS, "meshMain"},
                                               {aph::ShaderStage::FS, "fragMain"},
                                           }};
        shaderRequest.add(shaderLoadInfo, &m_program[ShadingType::Mesh]);
    }

    // Load bindless mesh shading program
    {
        // Register resources with the bindless system
        auto bindless = m_pDevice->getBindlessResource();
        bindless->updateResource(m_pImage, "texture_container");
        bindless->updateResource(m_pSampler, "samp");
        bindless->updateResource(m_pMatrixBffer, "transform_cube");
        bindless->updateResource(m_pVertexBuffer, "vertex_cube");
        bindless->updateResource(m_pIndexBuffer, "index_cube");

        // Load shader with bindless resources
        aph::ShaderLoadInfo shaderLoadInfo{.debugName = "ts + ms + fs (bindless)",
                                           .data = {"shader_slang://hello_mesh_bindless.slang"},
                                           .stageInfo =
                                               {
                                                   {aph::ShaderStage::TS, "taskMain"},
                                                   {aph::ShaderStage::MS, "meshMain"},
                                                   {aph::ShaderStage::FS, "fragMain"},
                                               },
                                           .pBindlessResource = bindless};
        shaderRequest.add(shaderLoadInfo, &m_program[ShadingType::MeshBindless]);
    }

    // Execute all shader loads
    shaderRequest.load();
}

void HelloAphrodite::setupRenderGraph()
{
    // Set up the render graph for each frame resource
    for (auto* graph : m_engine->setupGraph())
    {
        // Create descriptions for color and depth attachments
        aph::vk::ImageCreateInfo renderTargetColorInfo{
            .extent = {m_pSwapChain->getWidth(), m_pSwapChain->getHeight(), 1},
            .format = m_pSwapChain->getFormat(),
        };

        aph::vk::ImageCreateInfo renderTargetDepthInfo{
            .extent = {m_pSwapChain->getWidth(), m_pSwapChain->getHeight(), 1},
            .format = aph::Format::D32,
        };

        // Create a render pass group for main rendering
        auto renderGroup = graph->createPassGroup("MainRender");

        // Create and configure drawing pass using the builder pattern
        auto* drawPass = renderGroup.addPass("drawing cube", aph::QueueType::Graphics);
        drawPass->configure()
            .colorOutput("render output", {.createInfo = renderTargetColorInfo})
            .depthOutput("depth buffer", {.createInfo = renderTargetDepthInfo})
            .textureInput("container texture", m_pImage)
            .bufferInput("matrix ubo", m_pMatrixBffer, aph::BufferUsage::Uniform)
            .build();

        // Create UI pass
        auto* uiPass = graph->createPass("drawing ui", aph::QueueType::Graphics);
        uiPass->configure()
            .colorOutput("render output", {.createInfo = renderTargetColorInfo,
                                           .attachmentInfo =
                                               {
                                                   .loadOp = aph::AttachmentLoadOp::DontCare,
                                               }})
            .build();

        // Set the output buffer for display
        graph->setBackBuffer("render output");
    }
}

void HelloAphrodite::buildGraph(aph::RenderGraph* pGraph)
{
    auto drawPass = pGraph->getPass("drawing cube");
    drawPass->recordExecute(
        [this](auto* pCmd)
        {
            // Set common depth test settings
            pCmd->setDepthState({
                .enable = true,
                .write = true,
                .compareOp = aph::CompareOp::Less,
            });

            // Draw with current shading type
            renderWithShadingType(pCmd, m_shadingType);
        });

    auto uiPass = pGraph->getPass("drawing ui");
    // Add conditional execution for the UI pass
    uiPass->setExecutionCondition(
        [this]()
        {
            // Only render UI when in a specific mode, for example
            return m_shadingType != ShadingType::MeshBindless;
        });

    uiPass->recordExecute(
        [this](auto* pCmd)
        {
            // Render UI elements
            auto* ui = m_engine->getUI();
            ui->beginFrame();
            ui->render(pCmd);
            ui->endFrame();
        });

    pGraph->build(m_pSwapChain);
}

void HelloAphrodite::renderWithShadingType(aph::vk::CommandBuffer* pCmd, ShadingType type)
{
    switch (type)
    {
    case ShadingType::Geometry:
    {
        pCmd->beginDebugLabel({
            .name = "geometry shading path",
            .color = {0.5f, 0.3f, 0.2f, 1.0f},
        });

        pCmd->setProgram(m_program[ShadingType::Geometry]);
        pCmd->bindVertexBuffers(m_pVertexBuffer);
        pCmd->bindIndexBuffers(m_pIndexBuffer);
        pCmd->setResource({m_pMatrixBffer}, 0, 0);
        pCmd->setResource({m_pImage}, 1, 0);
        pCmd->setResource({m_pSampler}, 1, 1);
        pCmd->drawIndexed({36, 1, 0, 0, 0});

        pCmd->endDebugLabel();
    }
    break;

    case ShadingType::Mesh:
    {
        pCmd->beginDebugLabel({
            .name = "mesh shading path",
            .color = {0.5f, 0.3f, 0.2f, 1.0f},
        });

        pCmd->setProgram(m_program[ShadingType::Mesh]);
        pCmd->setResource({m_pMatrixBffer}, 0, 0);
        pCmd->setResource({m_pImage}, 1, 0);
        pCmd->setResource({m_pSampler}, 1, 1);
        pCmd->setResource({m_pVertexBuffer}, 0, 1);
        pCmd->setResource({m_pIndexBuffer}, 0, 2);
        pCmd->draw(aph::DispatchArguments{1, 1, 1});

        pCmd->endDebugLabel();
    }
    break;

    case ShadingType::MeshBindless:
    {
        pCmd->beginDebugLabel({
            .name = "mesh shading path (bindless)",
            .color = {0.5f, 0.3f, 0.2f, 1.0f},
        });

        pCmd->setProgram(m_program[ShadingType::MeshBindless]);
        pCmd->draw(aph::DispatchArguments{1, 1, 1});

        pCmd->endDebugLabel();
    }
    break;
    }
}

void HelloAphrodite::switchShadingType(ShadingType type)
{
    m_shadingType = type;

    switch (type)
    {
    case ShadingType::Geometry:
        APP_LOG_INFO("Switch to geometry shading.");
        break;
    case ShadingType::Mesh:
        APP_LOG_INFO("Switch to mesh shading.");
        break;
    case ShadingType::MeshBindless:
        APP_LOG_INFO("Switch to mesh shading (bindless).");
        break;
    }
}

void HelloAphrodite::switchShadingType(std::string_view value)
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

void HelloAphrodite::unload()
{
    APH_PROFILER_SCOPE();
    m_engine->unload();
}

void HelloAphrodite::finish()
{
    APH_PROFILER_SCOPE();
    APH_VERIFY_RESULT(m_pDevice->waitIdle());
    m_pDevice->destroy(m_pSampler);
}

int main(int argc, char** argv)
{
    HelloAphrodite app{};

    auto result =
        app.getOptions()
            .setVsync(false)
            .addCLICallback("--shading-type", [&app](std::string_view value) { app.switchShadingType(value); })
            .parse(argc, argv);

    APH_VERIFY_RESULT(result);

    app.run();
}
