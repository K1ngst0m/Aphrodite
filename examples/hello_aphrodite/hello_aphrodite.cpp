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
    VertexData f0 = {
        {-0.5f, 0.5f, 0.5f, 1.0f},
        {0.0f, 0.0f}
    };
    VertexData f1 = {
        {0.5f, 0.5f, 0.5f, 1.0f},
        {1.0f, 0.0f}
    };
    VertexData f2 = {
        {0.5f, -0.5f, 0.5f, 1.0f},
        {1.0f, 1.0f}
    };
    VertexData f3 = {
        {-0.5f, -0.5f, 0.5f, 1.0f},
        {0.0f, 1.0f}
    };

    // Back face
    // z = -0.5, from top-left -> top-right -> bottom-right -> bottom-left
    VertexData b0 = {
        {0.5f, 0.5f, -0.5f, 1.0f},
        {0.0f, 0.0f}
    };
    VertexData b1 = {
        {-0.5f, 0.5f, -0.5f, 1.0f},
        {1.0f, 0.0f}
    };
    VertexData b2 = {
        {-0.5f, -0.5f, -0.5f, 1.0f},
        {1.0f, 1.0f}
    };
    VertexData b3 = {
        {0.5f, -0.5f, -0.5f, 1.0f},
        {0.0f, 1.0f}
    };

    // Left face
    // x = -0.5, from top-left -> top-right -> bottom-right -> bottom-left
    VertexData l0 = {
        {-0.5f, 0.5f, -0.5f, 1.0f},
        {0.0f, 0.0f}
    };
    VertexData l1 = {
        {-0.5f, 0.5f, 0.5f, 1.0f},
        {1.0f, 0.0f}
    };
    VertexData l2 = {
        {-0.5f, -0.5f, 0.5f, 1.0f},
        {1.0f, 1.0f}
    };
    VertexData l3 = {
        {-0.5f, -0.5f, -0.5f, 1.0f},
        {0.0f, 1.0f}
    };

    // Right face
    // x = +0.5, from top-left -> top-right -> bottom-right -> bottom-left
    VertexData r0 = {
        {0.5f, 0.5f, 0.5f, 1.0f},
        {0.0f, 0.0f}
    };
    VertexData r1 = {
        {0.5f, 0.5f, -0.5f, 1.0f},
        {1.0f, 0.0f}
    };
    VertexData r2 = {
        {0.5f, -0.5f, -0.5f, 1.0f},
        {1.0f, 1.0f}
    };
    VertexData r3 = {
        {0.5f, -0.5f, 0.5f, 1.0f},
        {0.0f, 1.0f}
    };

    // Top face
    // y = +0.5, from front-left -> front-right -> back-right -> back-left
    VertexData t0 = {
        {-0.5f, 0.5f, 0.5f, 1.0f},
        {0.0f, 0.0f}
    };
    VertexData t1 = {
        {0.5f, 0.5f, 0.5f, 1.0f},
        {1.0f, 0.0f}
    };
    VertexData t2 = {
        {0.5f, 0.5f, -0.5f, 1.0f},
        {1.0f, 1.0f}
    };
    VertexData t3 = {
        {-0.5f, 0.5f, -0.5f, 1.0f},
        {0.0f, 1.0f}
    };

    // Bottom face
    // y = -0.5, from front-left -> front-right -> back-right -> back-left
    VertexData bo0 = {
        {-0.5f, -0.5f, 0.5f, 1.0f},
        {0.0f, 0.0f}
    };
    VertexData bo1 = {
        {0.5f, -0.5f, 0.5f, 1.0f},
        {1.0f, 0.0f}
    };
    VertexData bo2 = {
        {0.5f, -0.5f, -0.5f, 1.0f},
        {1.0f, 1.0f}
    };
    VertexData bo3 = {
        {-0.5f, -0.5f, -0.5f, 1.0f},
        {0.0f, 1.0f}
    };

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

HelloAphrodite::~HelloAphrodite()
{
}

void HelloAphrodite::init()
{
    APH_PROFILER_SCOPE();

    // Initialize engine and systems
    setupEngine();
    setupEventHandlers();
    setupUI();
}

void HelloAphrodite::setupEngine()
{
    // Configure and create the engine
    aph::EngineConfig config;
    config.setMaxFrames(3)
        .setWidth(getOptions().getWindowWidth())
        .setHeight(getOptions().getWindowHeight())
        // for debugging purpose
        .setEnableUIBreadcrumbs(false);

    m_pEngine = aph::Engine::Create(config);

    // Get references to core engine components
    m_pDevice         = m_pEngine->getDevice();
    m_pSwapChain      = m_pEngine->getSwapchain();
    m_pResourceLoader = m_pEngine->getResourceLoader();
    m_pWindowSystem   = m_pEngine->getWindowSystem();
    m_pUI             = m_pEngine->getUI();
    m_pFrameComposer  = m_pEngine->getFrameComposer();
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
}

void HelloAphrodite::setupUI()
{
    // Setup camera control UI
    setupCameraUI();
}

void HelloAphrodite::setupCameraUI()
{
    // Create a window for the camera controls
    auto windowResult = m_pUI->createWindow("Camera Controls");
    aph::VerifyExpected(windowResult);

    m_cameraWindow = windowResult.value();
    m_cameraWindow->setSize({400.0f, 600.0f});
    m_cameraWindow->setPosition({20.0f, 40.0f});

    // Create the camera control widget
    m_cameraControl = m_pUI->createWidget<aph::CameraControlWidget>();
    m_cameraControl->setCamera(&m_camera);
    m_cameraWindow->addWidget(m_cameraControl);
}

void HelloAphrodite::loop()
{
    for (auto frameResource : m_pEngine->loop())
    {
        APH_PROFILER_FRAME("application loop");

        // Rotate the model
        m_mvp.model = aph::Rotate(m_mvp.model, (float)m_pEngine->getCPUFrameTime(), {0.5f, 1.0f, 0.0f});
        m_mvp.view  = m_camera.getView();
        m_mvp.proj  = m_camera.getProjection();

        // Update the transformation matrix buffer
        auto mvpBuffer = m_pFrameComposer->getSharedResource<aph::vk::Buffer>("matrix ubo");
        m_pResourceLoader->update(
            {
                .data = &m_mvp, .range = {0, sizeof(m_mvp)}
        },
            mvpBuffer);

        // Build the render graph for this frame
        buildGraph(frameResource.pGraph);
    }
}

void HelloAphrodite::load()
{
    APH_PROFILER_SCOPE();

    loadResources();
    m_pEngine->load();
}

void HelloAphrodite::loadResources()
{
    // Setup camera and create matrix buffer
    {
        // Initialize the camera (camera parameters will be set by the CameraControlWidget)
        m_camera = aph::Camera(aph::CameraType::Perspective);

        // Set default camera position
        aph::Vec3 cameraPosition = {0.0f, 0.0f, 3.0f};
        aph::Vec3 cameraTarget   = {0.0f, 0.0f, 0.0f};
        aph::Vec3 cameraUp       = {0.0f, 1.0f, 0.0f};

        // Configure the camera
        float aspectRatio =
            static_cast<float>(getOptions().getWindowWidth()) / static_cast<float>(getOptions().getWindowHeight());

        m_camera.setLookAt(cameraPosition, cameraTarget, cameraUp)
            .setProjection(aph::PerspectiveInfo{
                .aspect = aspectRatio,
                .fov    = 60.0f,
                .znear  = 0.1f,
                .zfar   = 100.0f,
            });

        // Initialize the MVP matrices
        m_mvp.view = m_camera.getView();
        m_mvp.proj = m_camera.getProjection();
    }

    // Create sampler and load texture
    {
        // Create a linear clamp sampler
        m_pSampler = m_pDevice->create(aph::vk::SamplerCreateInfo{}.preset(aph::SamplerPreset::LinearClamp));
    }

    // Set up the render graph
    setupRenderGraph();
}

void HelloAphrodite::setupRenderGraph()
{
    // Create cube mesh (vertices and indices)
    std::vector<VertexData> vertices;
    std::vector<uint32_t> indices;
    createCube(vertices, indices);

    // Set up the render graph for each frame resource
    for (const auto& frameResource : m_pFrameComposer->frames())
    {
        auto graph = frameResource.pGraph;
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
            .colorOutput("render output",
                         {
                             .createInfo = renderTargetColorInfo
        })
            .depthOutput("depth buffer", {.createInfo = renderTargetDepthInfo})
            .sharedTextureInput("container texture", {.data = "texture://container2.png",
                                                      .createInfo =
                                                          {
                                                              .usage     = aph::ImageUsage::Sampled,
                                                              .domain    = aph::MemoryDomain::Device,
                                                              .imageType = aph::ImageType::e2D,
                                                          },
                                                      .featureFlags = aph::ImageFeatureBits::GenerateMips})
            .sharedBufferInput("matrix ubo",
                               {.data     = &m_mvp,
                                .dataSize = sizeof(m_mvp),
                                .createInfo =
                                    {
                                        .size   = sizeof(m_mvp),
                                        .usage  = aph::BufferUsage::Uniform,
                                        .domain = aph::MemoryDomain::Host,
                                    },
                                .contentType = aph::BufferContentType::Uniform},
                               aph::BufferUsage::Uniform)
            .sharedBufferInput("cube::vertex_buffer",
                               {.data     = vertices.data(),
                                .dataSize = vertices.size() * sizeof(vertices[0]),
                                .createInfo =
                                    {
                                        .size   = vertices.size() * sizeof(vertices[0]),
                                        .usage  = aph::BufferUsage::Storage | aph::BufferUsage::Vertex,
                                        .domain = aph::MemoryDomain::Device,
                                    },
                                .contentType = aph::BufferContentType::Vertex})
            .sharedBufferInput("cube::index_buffer",
                               {.data     = indices.data(),
                                .dataSize = indices.size() * sizeof(indices[0]),
                                .createInfo =
                                    {
                                        .size   = indices.size() * sizeof(indices[0]),
                                        .usage  = aph::BufferUsage::Storage | aph::BufferUsage::Index,
                                        .domain = aph::MemoryDomain::Device,
                                    },
                                .contentType = aph::BufferContentType::Index})
            .shader("bindless_mesh_program",
                    aph::ShaderLoadInfo{.data = {"shader_slang://hello_mesh_bindless.slang"},
                                        .stageInfo =
                                            {
                                                {aph::ShaderStage::TS, "taskMain"},
                                                {aph::ShaderStage::MS, "meshMain"},
                                                {aph::ShaderStage::FS, "fragMain"},
                                            },
                                        .pBindlessResource = m_pDevice->getBindlessResource()},
                    [this]()
                    {
                        // This callback runs after resources are loaded but right before this shader
                        // Access shared resources for bindless setup
                        auto textureAsset   = m_pFrameComposer->getSharedResource<aph::vk::Image>("container texture");
                        auto mvpBufferAsset = m_pFrameComposer->getSharedResource<aph::vk::Buffer>("matrix ubo");
                        auto vertexBufferAsset =
                            m_pFrameComposer->getSharedResource<aph::vk::Buffer>("cube::vertex_buffer");
                        auto indexBufferAsset =
                            m_pFrameComposer->getSharedResource<aph::vk::Buffer>("cube::index_buffer");

                        // Register resources with the bindless system
                        auto bindless = m_pDevice->getBindlessResource();
                        bindless->updateResource(textureAsset->getImage(), "texture_container");
                        bindless->updateResource(m_pSampler, "samp");
                        bindless->updateResource(mvpBufferAsset->getBuffer(), "transform_cube");
                        bindless->updateResource(vertexBufferAsset->getBuffer(), "vertex_cube");
                        bindless->updateResource(indexBufferAsset->getBuffer(), "index_cube");

                        // Log that bindless setup is complete
                        APP_LOG_INFO("Bindless resources registered successfully for bindless_mesh_program");
                    })
            .build();

        // Create UI pass
        auto* uiPass = renderGroup.addPass("drawing ui", aph::QueueType::Graphics);
        uiPass->configure()
            .colorOutput("render output", {.createInfo     = renderTargetColorInfo,
                                           .attachmentInfo = {.loadOp = aph::AttachmentLoadOp::DontCare}})
            .build();

        // Set the output buffer for display
        graph->setBackBuffer("render output");
    }
}

void HelloAphrodite::buildGraph(aph::RenderGraph* pGraph)
{
    auto drawPass = pGraph->getPass("drawing cube");
    drawPass->pushCommands("bindless_mesh_program",
                           [](auto* pCmd)
                           {
                               // Set common depth test settings
                               pCmd->setDepthState({
                                   .enable    = true,
                                   .write     = true,
                                   .compareOp = aph::CompareOp::Less,
                               });

                               {
                                   pCmd->beginDebugLabel({
                                       .name  = "mesh shading path (bindless)",
                                       .color = {0.5f, 0.3f, 0.2f, 1.0f},
                                   });

                                   pCmd->draw(aph::DispatchArguments{1, 1, 1});

                                   pCmd->endDebugLabel();
                               }
                           });

    auto uiPass = pGraph->getPass("drawing ui");

    uiPass->recordExecute([this](auto* pCmd) { m_pUI->render(pCmd); });

    pGraph->build(m_pSwapChain);
}

void HelloAphrodite::unload()
{
    APH_PROFILER_SCOPE();
    m_pEngine->unload();
}

void HelloAphrodite::finish()
{
    APH_PROFILER_SCOPE();

    // Wait for device to be idle before cleanup
    APH_VERIFY_RESULT(m_pDevice->waitIdle());

    if (m_cameraWindow)
    {
        m_pUI->destroyWindow(m_cameraWindow);
    }

    // Destroy the sampler
    m_pDevice->destroy(m_pSampler);

    // Destroy the engine last
    aph::Engine::Destroy(m_pEngine);
}

int main(int argc, char** argv)
{
    HelloAphrodite app{};

    auto result = app.getOptions().setVsync(false).parse(argc, argv);

    APH_VERIFY_RESULT(result);

    app.run();
}
