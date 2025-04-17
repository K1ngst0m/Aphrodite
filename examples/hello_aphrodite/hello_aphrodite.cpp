#include "hello_aphrodite.h"

HelloAphrodite::HelloAphrodite()
    : aph::App("hello aphrdite")
{
}

HelloAphrodite::~HelloAphrodite() = default;

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
        .setEnableCapture(false)
        // for debugging purpose
        .setEnableUIBreadcrumbs(false)
        .setResourceForceUncached(true)
        .setEnableDeviceDebug(false);

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
        [this](const aph::WindowResizeEvent& /*e*/) -> bool
        {
            m_pSwapChain->reCreate();
            return true;
        });
}

void HelloAphrodite::setupUI()
{
    // Setup camera control UI
    setupCameraUI();

    // Setup shader debug UI
    setupShaderDebugUI();
}

void HelloAphrodite::setupCameraUI()
{
    // Create a window for the camera controls
    auto windowResult = m_pUI->createWindow("Camera Controls");
    aph::VerifyExpected(windowResult);

    m_cameraWindow = windowResult.value();
    m_cameraWindow->setSize({ 400.0F, 600.0f });
    m_cameraWindow->setPosition({ 20.0f, 40.0f });

    // Create the camera control widget
    m_cameraControl = m_pUI->createWidget<aph::CameraControlWidget>();
    m_cameraControl->setCamera(&m_camera);
    m_cameraWindow->addWidget(m_cameraControl);
}

void HelloAphrodite::setupShaderDebugUI()
{
    // Create a window for shader debugging
    auto windowResult = m_pUI->createWindow("Shader Debug Info");
    aph::VerifyExpected(windowResult);

    m_shaderInfoWindow = windowResult.value();
    m_shaderInfoWindow->setSize({ 600.0f, 700.0f });
    m_shaderInfoWindow->setPosition({ 450.0f, 40.0f });

    // Create the shader info widget
    m_shaderInfoWidget = m_pUI->createWidget<aph::ShaderInfoWidget>();
    m_shaderInfoWindow->addWidget(m_shaderInfoWidget);
}

void HelloAphrodite::loop()
{
    for (auto frameResource : m_pEngine->loop())
    {
        APH_PROFILER_FRAME("application loop");

        // Rotate the model
        m_mvp.model = aph::Rotate(m_mvp.model, static_cast<float>(m_pEngine->getCPUFrameTime()), { 0.5f, 1.0f, 0.0f });
        m_mvp.view  = m_camera.getView();
        m_mvp.proj  = m_camera.getProjection();

        // Update the transformation matrix buffer
        auto* mvpBuffer = m_pFrameComposer->getResource<aph::BufferAsset>("matrix ubo");
        m_pResourceLoader->update(
            {
                .data = &m_mvp, .range = { .offset = 0, .size = sizeof(m_mvp) }
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
}

void HelloAphrodite::loadResources()
{
    // Setup camera and create matrix buffer
    {
        // Initialize the camera (camera parameters will be set by the CameraControlWidget)
        m_camera = aph::Camera(aph::CameraType::Perspective);

        // Set default camera position
        aph::Vec3 cameraPosition = { 0.0f, 0.0f, 3.0f };
        aph::Vec3 cameraTarget   = { 0.0f, 0.0f, 0.0f };
        aph::Vec3 cameraUp       = { 0.0f, 1.0f, 0.0f };

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

    // Set up the render graph
    setupRenderGraph();
}

void HelloAphrodite::setupRenderGraph()
{
    {
        aph::GeometryLoadInfo geometryLoadInfo{ .path         = "model://Cube/glTF/Cube.gltf",
                                                .debugName    = "cube_model",
                                                .meshletFlags = aph::MeshletFeatureBits::eCullingData |
                                                                aph::MeshletFeatureBits::eOptimizeForGPUCulling,
                                                .optimizationFlags  = aph::GeometryOptimizationBits::eAll,
                                                .attributeFlags     = aph::GeometryAttributeBits::eNone,
                                                .maxVertsPerMeshlet = 64,
                                                .maxPrimsPerMeshlet = 124,
                                                .preferMeshShading  = true,
                                                .usage              = aph::GeometryUsage::eStatic,
                                                .forceUncached      = true };
        auto loadRequest = m_pResourceLoader->createRequest();
        loadRequest.add(geometryLoadInfo, &m_pGeometryAsset);
        loadRequest.load();
    }

    // Create submesh buffer with proper GPU alignment
    std::vector<aph::Submesh> submeshes;
    for (const auto* submesh : m_pGeometryAsset->submeshes())
    {
        submeshes.push_back(*submesh);
    }

    aph::BufferLoadInfo submeshBuffer{
        .data = submeshes.data(),
        .dataSize = submeshes.size() * sizeof(aph::Submesh),
        .createInfo = {
            .size = submeshes.size() * sizeof(aph::Submesh),
            .usage = aph::BufferUsage::Storage,
            .domain = aph::MemoryDomain::Device,
        },
        .contentType = aph::BufferContentType::Storage
    };

    // Create metadata buffer with mesh information
    struct MeshMetadata
    {
        uint32_t meshletCount;
        uint32_t vertexCount;
        uint32_t indexCount;
        uint32_t submeshCount;
    };

    MeshMetadata metadata = { .meshletCount = m_pGeometryAsset->getMeshletCount(),
                              .vertexCount  = m_pGeometryAsset->getVertexCount(),
                              .indexCount   = m_pGeometryAsset->getIndexCount(),
                              .submeshCount = m_pGeometryAsset->getSubmeshCount() };

    // Create buffer for metadata
    aph::BufferLoadInfo metadataBuffer{
        .data = &metadata,
        .dataSize = sizeof(metadata),
        .createInfo = {
            .size = sizeof(metadata),
            .usage = aph::BufferUsage::Storage,
            .domain = aph::MemoryDomain::Device,
        },
        .contentType = aph::BufferContentType::Storage
    };

    // Create shader resource
    aph::ShaderLoadInfo shaderInfo{
            .data = {"shader_slang://hello_mesh_bindless.slang"},
            .stageInfo = {
                {aph::ShaderStage::TS, "taskMain"},
                {aph::ShaderStage::MS, "meshMain"},
                {aph::ShaderStage::FS, "fragMain"},
            },
            .pBindlessResource = m_pDevice->getBindlessResource()
        };

    // Create matrix buffer resource
    aph::BufferLoadInfo matrixBuffer{
            .data = &m_mvp,
            .dataSize = sizeof(m_mvp),
            .createInfo = {
                .size = sizeof(m_mvp),
                .usage = aph::BufferUsage::Uniform,
                .domain = aph::MemoryDomain::Host,
            },
            .contentType = aph::BufferContentType::Uniform
        };

    // Create container texture resource
    aph::ImageLoadInfo containerTexture{
            .data = "texture://container2.ktx2",
            .createInfo = {
                .usage = aph::ImageUsage::Sampled,
                .domain = aph::MemoryDomain::Device,
                .imageType = aph::ImageType::e2D,
            },
            .featureFlags = aph::ImageFeatureBits::eGenerateMips
        };

    // Setup shader callback
    auto shaderCallback = [this]()
    {
        // This callback runs after resources are loaded but right before this shader
        // Access shared resources for bindless setup
        auto* textureAsset   = m_pFrameComposer->getResource<aph::ImageAsset>("container texture");
        auto* mvpBufferAsset = m_pFrameComposer->getResource<aph::BufferAsset>("matrix ubo");
        auto* sampler        = m_pDevice->getSampler(aph::vk::PresetSamplerType::eLinearWrapMipmap);
        auto* metaDataBuffer = m_pFrameComposer->getResource<aph::BufferAsset>("cube::mesh_metadata");
        auto* submeshBuffer  = m_pFrameComposer->getResource<aph::BufferAsset>("cube::submesh_buffer");

        // Register resources with the bindless system
        auto* bindless = m_pDevice->getBindlessResource();

        bindless->updateResource(textureAsset->getImage(), "texture_container");
        bindless->updateResource(sampler, "samp");
        bindless->updateResource(mvpBufferAsset->getBuffer(), "transform_cube");
        bindless->updateResource(m_pGeometryAsset->getPositionBuffer(), "position_cube");
        bindless->updateResource(m_pGeometryAsset->getAttributeBuffer(), "attribute_cube");
        bindless->updateResource(m_pGeometryAsset->getIndexBuffer(), "index_cube");
        bindless->updateResource(m_pGeometryAsset->getMeshletBuffer(), "meshlets_cube");
        bindless->updateResource(m_pGeometryAsset->getMeshletVertexBuffer(), "meshlet_vertices");
        bindless->updateResource(m_pGeometryAsset->getMeshletIndexBuffer(), "meshlet_indices");
        bindless->updateResource(metaDataBuffer->getBuffer(), "mesh_metadata");
        bindless->updateResource(submeshBuffer->getBuffer(), "submeshes_cube");

        // Log that bindless setup is complete
        APP_LOG_INFO("Bindless resources registered successfully for bindless_mesh_program");
    };

    // Set up the render graph for each frame resource
    for (const auto& frameResource : m_pFrameComposer->frames())
    {
        auto* graph = frameResource.pGraph;
        // Create descriptions for color and depth attachments using framebuffer dimensions
        aph::vk::ImageCreateInfo renderTargetColorInfo{
            .extent = { .width = m_pEngine->getPixelWidth(), .height = m_pEngine->getPixelHeight(), .depth = 1 },
            .format = m_pSwapChain->getFormat(),
        };

        aph::vk::ImageCreateInfo renderTargetDepthInfo{
            .extent = { .width = m_pEngine->getPixelWidth(), .height = m_pEngine->getPixelHeight(), .depth = 1 },
            .format = aph::Format::D32,
        };

        // Create a render pass group for main rendering
        auto renderGroup = graph->createPassGroup("MainRender");

        auto* drawPass = renderGroup.addPass("drawing cube", aph::QueueType::Graphics);
        drawPass->configure()
            .attachment("render output", { .createInfo = renderTargetColorInfo }, false)
            .attachment("depth buffer", { .createInfo = renderTargetDepthInfo }, true)
            .input(aph::ImageResourceInfo{ .name     = "container texture",
                                           .shared   = true,
                                           .resource = containerTexture,
                                           .usage    = aph::ImageUsage::Sampled })
            .input(aph::BufferResourceInfo{
                .name = "matrix ubo", .shared = true, .resource = matrixBuffer, .usage = aph::BufferUsage::Uniform })
            .input(aph::BufferResourceInfo{ .name     = "cube::position_buffer",
                                            .shared   = true,
                                            .resource = m_pGeometryAsset->getPositionBuffer(),
                                            .usage    = aph::BufferUsage::Uniform })
            .input(aph::BufferResourceInfo{ .name     = "cube::attribute_buffer",
                                            .shared   = true,
                                            .resource = m_pGeometryAsset->getAttributeBuffer(),
                                            .usage    = aph::BufferUsage::Uniform })
            .input(aph::BufferResourceInfo{ .name     = "cube::index_buffer",
                                            .shared   = true,
                                            .resource = m_pGeometryAsset->getIndexBuffer(),
                                            .usage    = aph::BufferUsage::Uniform })
            .input(aph::BufferResourceInfo{ .name     = "cube::meshlet_buffer",
                                            .shared   = true,
                                            .resource = m_pGeometryAsset->getMeshletBuffer(),
                                            .usage    = aph::BufferUsage::Uniform })
            .input(aph::BufferResourceInfo{ .name     = "cube::meshlet_vertices",
                                            .shared   = true,
                                            .resource = m_pGeometryAsset->getMeshletVertexBuffer(),
                                            .usage    = aph::BufferUsage::Uniform })
            .input(aph::BufferResourceInfo{ .name     = "cube::meshlet_indices",
                                            .shared   = true,
                                            .resource = m_pGeometryAsset->getMeshletIndexBuffer(),
                                            .usage    = aph::BufferUsage::Uniform })
            .input(aph::BufferResourceInfo{ .name     = "cube::mesh_metadata",
                                            .shared   = true,
                                            .resource = metadataBuffer,
                                            .usage    = aph::BufferUsage::Uniform })
            .input(aph::BufferResourceInfo{ .name     = "cube::submesh_buffer",
                                            .shared   = true,
                                            .resource = submeshBuffer,
                                            .usage    = aph::BufferUsage::Uniform })
            .shader(aph::ShaderResourceInfo{
                .name = "bindless_mesh_program", .preCallback = shaderCallback, .resource = shaderInfo })
            .build();

        // Create UI pass
        auto* uiPass = renderGroup.addPass("drawing ui", aph::QueueType::Graphics);
        uiPass->configure()
            .attachment("render output",
                        { .createInfo     = renderTargetColorInfo,
                          .attachmentInfo = { .loadOp = aph::AttachmentLoadOp::DontCare } },
                        false)
            .build();

        // Set the output buffer for display
        graph->setBackBuffer("render output");
    }

    m_shaderInfoWidget->setShaderAsset(m_pFrameComposer->getResource<aph::ShaderAsset>("bindless_mesh_program"));
}

void HelloAphrodite::buildGraph(aph::RenderGraph* pGraph)
{
    auto* drawPass = pGraph->getPass("drawing cube");

    drawPass->configure().resetExecute().execute(
        "bindless_mesh_program",
        [this](auto* pCmd)
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
                    .color = { 0.5F, 0.3f, 0.2f, 1.0f },
                });

                uint32_t meshletCount = m_pGeometryAsset->getMeshletCount();
                pCmd->draw(aph::DispatchArguments{ .x = meshletCount, .y = 1, .z = 1 });

                pCmd->endDebugLabel();
            }
        });

    auto* uiPass = pGraph->getPass("drawing ui");

    uiPass->configure().execute(
        [this](auto* pCmd)
        {
            m_pUI->render(pCmd);
        });
}

void HelloAphrodite::unload()
{
    APH_PROFILER_SCOPE();
}

void HelloAphrodite::finish()
{
    APH_PROFILER_SCOPE();

    // Wait for device to be idle before cleanup
    APH_VERIFY_RESULT(m_pDevice->waitIdle());

    if (m_cameraWindow != nullptr)
    {
        m_pUI->destroyWindow(m_cameraWindow);
    }

    if (m_shaderInfoWindow != nullptr)
    {
        m_pUI->destroyWindow(m_shaderInfoWindow);
    }

    // Destroy the engine last
    aph::Engine::Destroy(m_pEngine);
}

auto main(int argc, char** argv) -> int
{
    HelloAphrodite app{};

    auto result = app.getOptions().setVsync(false).parse(argc, argv);

    APH_VERIFY_RESULT(result);

    app.run();
}
