#include "hello_aphrodite.h"

namespace
{
// Vertex structure for the cube with position and texture coordinates
struct VertexData
{
    aph::Vec4 pos;
    aph::Vec2 uv;
    aph::Vec2 padding;
};

// Creates a 3D cube mesh with position and texture coordinates and meshlet data
auto CreateCube(aph::vk::Device* pDevice, aph::ResourceLoader* pResourceLoader)
    -> std::unique_ptr<aph::IGeometryResource>
{
    // Create vertices and indices for a cube
    std::vector<VertexData> vertices;
    std::vector<uint32_t> indices;

    // Each face is defined in a counter-clockwise (CCW) order
    // when viewed from the outside of the cube.

    // Front face
    // z = +0.5, from top-left -> top-right -> bottom-right -> bottom-left
    VertexData f0 = {
        .pos = { -0.5f, 0.5f, 0.5f, 1.0f },
          .uv = { 0.0f, 0.0f }
    };
    VertexData f1 = {
        .pos = { 0.5f, 0.5f, 0.5f, 1.0f },
          .uv = { 1.0f, 0.0f }
    };
    VertexData f2 = {
        .pos = { 0.5f, -0.5f, 0.5f, 1.0f },
          .uv = { 1.0f, 1.0f }
    };
    VertexData f3 = {
        .pos = { -0.5f, -0.5f, 0.5f, 1.0f },
          .uv = { 0.0f, 1.0f }
    };

    // Back face
    // z = -0.5, from top-left -> top-right -> bottom-right -> bottom-left
    VertexData b0 = {
        .pos = { 0.5f, 0.5f, -0.5f, 1.0f },
          .uv = { 0.0f, 0.0f }
    };
    VertexData b1 = {
        .pos = { -0.5f, 0.5f, -0.5f, 1.0f },
          .uv = { 1.0f, 0.0f }
    };
    VertexData b2 = {
        .pos = { -0.5f, -0.5f, -0.5f, 1.0f },
          .uv = { 1.0f, 1.0f }
    };
    VertexData b3 = {
        .pos = { 0.5f, -0.5f, -0.5f, 1.0f },
          .uv = { 0.0f, 1.0f }
    };

    // Left face
    // x = -0.5, from top-left -> top-right -> bottom-right -> bottom-left
    VertexData l0 = {
        .pos = { -0.5f, 0.5f, -0.5f, 1.0f },
          .uv = { 0.0f, 0.0f }
    };
    VertexData l1 = {
        .pos = { -0.5f, 0.5f, 0.5f, 1.0f },
          .uv = { 1.0f, 0.0f }
    };
    VertexData l2 = {
        .pos = { -0.5f, -0.5f, 0.5f, 1.0f },
          .uv = { 1.0f, 1.0f }
    };
    VertexData l3 = {
        .pos = { -0.5f, -0.5f, -0.5f, 1.0f },
          .uv = { 0.0f, 1.0f }
    };

    // Right face
    // x = +0.5, from top-left -> top-right -> bottom-right -> bottom-left
    VertexData r0 = {
        .pos = { 0.5f, 0.5f, 0.5f, 1.0f },
          .uv = { 0.0f, 0.0f }
    };
    VertexData r1 = {
        .pos = { 0.5f, 0.5f, -0.5f, 1.0f },
          .uv = { 1.0f, 0.0f }
    };
    VertexData r2 = {
        .pos = { 0.5f, -0.5f, -0.5f, 1.0f },
          .uv = { 1.0f, 1.0f }
    };
    VertexData r3 = {
        .pos = { 0.5f, -0.5f, 0.5f, 1.0f },
          .uv = { 0.0f, 1.0f }
    };

    // Top face
    // y = +0.5, from front-left -> front-right -> back-right -> back-left
    VertexData t0 = {
        .pos = { -0.5f, 0.5f, 0.5f, 1.0f },
          .uv = { 0.0f, 0.0f }
    };
    VertexData t1 = {
        .pos = { 0.5f, 0.5f, 0.5f, 1.0f },
          .uv = { 1.0f, 0.0f }
    };
    VertexData t2 = {
        .pos = { 0.5f, 0.5f, -0.5f, 1.0f },
          .uv = { 1.0f, 1.0f }
    };
    VertexData t3 = {
        .pos = { -0.5f, 0.5f, -0.5f, 1.0f },
          .uv = { 0.0f, 1.0f }
    };

    // Bottom face
    // y = -0.5, from front-left -> front-right -> back-right -> back-left
    VertexData bo0 = {
        .pos = { -0.5f, -0.5f, 0.5f, 1.0f },
          .uv = { 0.0f, 0.0f }
    };
    VertexData bo1 = {
        .pos = { 0.5f, -0.5f, 0.5f, 1.0f },
          .uv = { 1.0f, 0.0f }
    };
    VertexData bo2 = {
        .pos = { 0.5f, -0.5f, -0.5f, 1.0f },
          .uv = { 1.0f, 1.0f }
    };
    VertexData bo3 = {
        .pos = { -0.5f, -0.5f, -0.5f, 1.0f },
          .uv = { 0.0f, 1.0f }
    };

    // Collect all 24 vertices in a single array
    vertices = { // Front
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
    indices = { // front
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

    // Generate meshlets using MeshletBuilder
    std::vector<aph::Meshlet> meshlets;
    std::vector<uint32_t> meshletVertices;
    std::vector<uint32_t> meshletIndices;

    {
        // Generate meshlets using MeshletBuilder
        aph::MeshletBuilder meshletBuilder;

        // Extract positions from vertex data
        std::vector<float> positions;
        positions.reserve(vertices.size() * 3);

        for (const auto& vertex : vertices)
        {
            positions.push_back(vertex.pos.x);
            positions.push_back(vertex.pos.y);
            positions.push_back(vertex.pos.z);
        }

        // Add mesh data to the builder
        meshletBuilder.addMesh(positions.data(), // positions
                               sizeof(float) * 3, // position stride (3 floats)
                               vertices.size(), // vertex count
                               indices.data(), // indices
                               indices.size() // index count
        );

        // Build meshlets (using default parameters)
        meshletBuilder.build();

        // Extract the generated meshlet data
        meshletBuilder.exportMeshletData(meshlets, meshletVertices, meshletIndices);

        APP_LOG_INFO("MeshletBuilder generated {} meshlets", meshlets.size());
        for (size_t i = 0; i < meshlets.size(); ++i)
        {
            auto logout = std::format("Meshlet {}: {} vertices, {} triangles, vertexOffset={}, triangleOffset={}", i,
                                      meshlets[i].vertexCount, meshlets[i].triangleCount, meshlets[i].vertexOffset,
                                      meshlets[i].triangleOffset);
            APP_LOG_INFO("%s", logout);
        }
    }

    // Create separate buffers for positions and attributes
    std::vector<aph::Vec4> positionData;
    std::vector<aph::Vec2> attributeData;

    positionData.reserve(vertices.size());
    attributeData.reserve(vertices.size());

    for (const auto& vertex : vertices)
    {
        positionData.push_back(vertex.pos);
        attributeData.push_back(vertex.uv);
    }

    // Create buffer load infos
    aph::BufferLoadInfo positionBufferLoadInfo{
        .data = positionData.data(),
        .dataSize = positionData.size() * sizeof(positionData[0]),
        .createInfo = {
            .size = positionData.size() * sizeof(positionData[0]),
            .usage = aph::BufferUsage::Storage | aph::BufferUsage::Vertex,
            .domain = aph::MemoryDomain::Device,
        },
        .contentType = aph::BufferContentType::Vertex
    };

    aph::BufferLoadInfo attributeBufferLoadInfo{
        .data = attributeData.data(),
        .dataSize = attributeData.size() * sizeof(attributeData[0]),
        .createInfo = {
            .size = attributeData.size() * sizeof(attributeData[0]),
            .usage = aph::BufferUsage::Storage | aph::BufferUsage::Vertex,
            .domain = aph::MemoryDomain::Device,
        },
        .contentType = aph::BufferContentType::Vertex
    };

    aph::BufferLoadInfo indexBufferLoadInfo{
        .data = indices.data(),
        .dataSize = indices.size() * sizeof(indices[0]),
        .createInfo = {
            .size = indices.size() * sizeof(indices[0]),
            .usage = aph::BufferUsage::Storage | aph::BufferUsage::Index,
            .domain = aph::MemoryDomain::Device,
        },
        .contentType = aph::BufferContentType::Index
    };

    aph::BufferLoadInfo meshletBufferLoadInfo{
        .data = meshlets.data(),
        .dataSize = meshlets.size() * sizeof(aph::Meshlet),
        .createInfo = {
            .size = meshlets.size() * sizeof(aph::Meshlet),
            .usage = aph::BufferUsage::Storage,
            .domain = aph::MemoryDomain::Device,
        },
        .contentType = aph::BufferContentType::Storage
    };

    aph::BufferLoadInfo meshletVertexBufferLoadInfo{
        .data = meshletVertices.data(),
        .dataSize = meshletVertices.size() * sizeof(uint32_t),
        .createInfo = {
            .size = meshletVertices.size() * sizeof(uint32_t),
            .usage = aph::BufferUsage::Storage,
            .domain = aph::MemoryDomain::Device,
        },
        .contentType = aph::BufferContentType::Storage
    };

    aph::BufferLoadInfo meshletIndexBufferLoadInfo{
        .data = meshletIndices.data(),
        .dataSize = meshletIndices.size() * sizeof(uint32_t),
        .createInfo = {
            .size = meshletIndices.size() * sizeof(uint32_t),
            .usage = aph::BufferUsage::Storage,
            .domain = aph::MemoryDomain::Device,
        },
        .contentType = aph::BufferContentType::Storage
    };

    // Create and populate GeometryGpuData
    aph::GeometryGpuData geometryData;

    // Load all the buffers
    {
        auto loadRequest                           = pResourceLoader->createRequest();
        aph::BufferAsset* positionBufferAsset      = {};
        aph::BufferAsset* attributeBufferAsset     = {};
        aph::BufferAsset* indexBufferAsset         = {};
        aph::BufferAsset* meshletBufferAsset       = {};
        aph::BufferAsset* meshletVertexBufferAsset = {};
        aph::BufferAsset* meshletIndexBufferAsset  = {};

        loadRequest.add(positionBufferLoadInfo, &positionBufferAsset);
        loadRequest.add(attributeBufferLoadInfo, &attributeBufferAsset);
        loadRequest.add(indexBufferLoadInfo, &indexBufferAsset);
        loadRequest.add(meshletBufferLoadInfo, &meshletBufferAsset);
        loadRequest.add(meshletVertexBufferLoadInfo, &meshletVertexBufferAsset);
        loadRequest.add(meshletIndexBufferLoadInfo, &meshletIndexBufferAsset);
        loadRequest.load();

        // Store buffers in GeometryGpuData
        geometryData.pPositionBuffer      = positionBufferAsset->getBuffer();
        geometryData.pAttributeBuffer     = attributeBufferAsset->getBuffer();
        geometryData.pIndexBuffer         = indexBufferAsset->getBuffer();
        geometryData.pMeshletBuffer       = meshletBufferAsset->getBuffer();
        geometryData.pMeshletVertexBuffer = meshletVertexBufferAsset->getBuffer();
        geometryData.pMeshletIndexBuffer  = meshletIndexBufferAsset->getBuffer();

        // Store metadata in GeometryGpuData
        geometryData.vertexCount             = { static_cast<uint32_t>(vertices.size()) };
        geometryData.indexCount              = { static_cast<uint32_t>(indices.size()) };
        geometryData.meshletCount            = { static_cast<uint32_t>(meshlets.size()) };
        geometryData.meshletMaxVertexCount   = 64; // Max vertices per meshlet
        geometryData.meshletMaxTriangleCount = 124; // Max triangles per meshlet
    }

    // Create submeshes (just one in this simple case)
    std::vector<aph::Submesh> submeshes;
    aph::Submesh submesh;
    submesh.meshletOffset = 0;
    submesh.meshletCount  = meshlets.size();
    submesh.materialIndex = 0;

    // Create AABB bounding box for the submesh
    aph::BoundingBox bounds;
    bounds.min     = { -0.5f, -0.5f, -0.5f };
    bounds.max     = { 0.5f, 0.5f, 0.5f };
    submesh.bounds = bounds;

    submeshes.push_back(submesh);

    // Create and return a MeshletGeometryResource
    return aph::GeometryResourceFactory::createGeometryResource(pDevice, geometryData, submeshes, {});
}
} // namespace

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
    // Create cube with meshlets and GPU data
    m_pGeometryResource = CreateCube(m_pDevice, m_pResourceLoader);

    // Create metadata buffer with mesh information
    struct MeshMetadata
    {
        uint32_t meshletCount;
        uint32_t vertexCount;
        uint32_t indexCount;
        uint32_t padding;
    };

    MeshMetadata metadata = { .meshletCount = m_pGeometryResource->getMeshletCount(),
                              .vertexCount  = m_pGeometryResource->getVertexCount(),
                              .indexCount   = m_pGeometryResource->getIndexCount(),
                              .padding      = 0 };

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

    // Log metadata information
    APP_LOG_INFO("Mesh metadata: {} meshlets, {} vertices, {} indices", metadata.meshletCount, metadata.vertexCount,
                 metadata.indexCount);

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

            // Register resources with the bindless system
            auto* bindless = m_pDevice->getBindlessResource();

            bindless->updateResource(textureAsset->getImage(), "texture_container");
            bindless->updateResource(sampler, "samp");
            bindless->updateResource(mvpBufferAsset->getBuffer(), "transform_cube");
            bindless->updateResource(m_pGeometryResource->getPositionBuffer(), "position_cube");
            bindless->updateResource(m_pGeometryResource->getAttributeBuffer(), "attribute_cube");
            bindless->updateResource(m_pGeometryResource->getIndexBuffer(), "index_cube");
            bindless->updateResource(m_pGeometryResource->getMeshletBuffer(), "meshlets_cube");
            bindless->updateResource(m_pGeometryResource->getMeshletVertexBuffer(), "meshlet_vertices");
            bindless->updateResource(m_pGeometryResource->getMeshletIndexBuffer(), "meshlet_indices");
            bindless->updateResource(metaDataBuffer->getBuffer(), "mesh_metadata");

            // Log that bindless setup is complete
            APP_LOG_INFO("Bindless resources registered successfully for bindless_mesh_program");
        };

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
                                            .resource = m_pGeometryResource->getPositionBuffer(),
                                            .usage    = aph::BufferUsage::Uniform })
            .input(aph::BufferResourceInfo{ .name     = "cube::attribute_buffer",
                                            .shared   = true,
                                            .resource = m_pGeometryResource->getAttributeBuffer(),
                                            .usage    = aph::BufferUsage::Uniform })
            .input(aph::BufferResourceInfo{ .name     = "cube::index_buffer",
                                            .shared   = true,
                                            .resource = m_pGeometryResource->getIndexBuffer(),
                                            .usage    = aph::BufferUsage::Uniform })
            .input(aph::BufferResourceInfo{ .name     = "cube::meshlet_buffer",
                                            .shared   = true,
                                            .resource = m_pGeometryResource->getMeshletBuffer(),
                                            .usage    = aph::BufferUsage::Uniform })
            .input(aph::BufferResourceInfo{ .name     = "cube::meshlet_vertices",
                                            .shared   = true,
                                            .resource = m_pGeometryResource->getMeshletVertexBuffer(),
                                            .usage    = aph::BufferUsage::Uniform })
            .input(aph::BufferResourceInfo{ .name     = "cube::meshlet_indices",
                                            .shared   = true,
                                            .resource = m_pGeometryResource->getMeshletIndexBuffer(),
                                            .usage    = aph::BufferUsage::Uniform })
            .input(aph::BufferResourceInfo{ .name     = "cube::mesh_metadata",
                                            .shared   = true,
                                            .resource = metadataBuffer,
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

    drawPass->configure().resetExecute().execute("bindless_mesh_program",
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
                                                             .color = { 0.5F, 0.3f, 0.2f, 1.0f },
                                                         });

                                                         pCmd->draw(aph::DispatchArguments{ .x = 1, .y = 1, .z = 1 });

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
