#include "aph_core.hpp"
#include "aph_renderer.hpp"

// Ray tracing acceleration structure
struct AccelerationStructure
{
    VkAccelerationStructureKHR handle;
    uint64_t                   deviceAddress = 0;
    aph::vk::Buffer*           buffer        = {};
};

void createAccelerationStructureBuffer(aph::vk::Device* pDevice, AccelerationStructure& accelerationStructure,
                                       VkAccelerationStructureBuildSizesInfoKHR buildSizeInfo)
{
    APH_VR(pDevice->create(
        aph::vk::BufferCreateInfo{
            .size  = buildSizeInfo.accelerationStructureSize,
            .usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
            .domain = aph::BufferDomain::Device,
        },
        &accelerationStructure.buffer));
}

class ray_tracing : public aph::BaseApp
{
public:
    ray_tracing();

    void init() override;
    void load() override;
    void run() override;
    void unload() override;
    void finish() override;

    struct
    {
        uint32_t windowWidth  = {1440};
        uint32_t windowHeight = {900};
    } m_options;

private:
    void createBLAS()
    {
        // Setup vertices for a single triangle
        struct Vertex
        {
            float pos[3];
        };
        std::vector<Vertex> vertices = {{{1.0f, 1.0f, 0.0f}}, {{-1.0f, 1.0f, 0.0f}}, {{0.0f, -1.0f, 0.0f}}};

        // Setup indices
        std::vector<uint32_t> indices = {0, 1, 2};

        // Setup identity transform matrix
        VkTransformMatrixKHR transformMatrix = {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f};

        // vertex buffer
        m_pResourceLoader->loadAsync(
            aph::BufferLoadInfo{
                .debugName  = "rt::vertexBuffer",
                .data       = vertices.data(),
                .createInfo = {.size  = static_cast<uint32_t>(vertices.size() * sizeof(vertices[0])),
                               .usage = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
                                        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR}},
            &m_pVB);

        // index buffer
        m_pResourceLoader->loadAsync(
            aph::BufferLoadInfo{
                .debugName  = "rt::indexbuffer",
                .data       = indices.data(),
                .createInfo = {.size  = static_cast<uint32_t>(indices.size() * sizeof(indices[0])),
                               .usage = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
                                        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR}},
            &m_pIB);

        // transform buffer
        m_pResourceLoader->loadAsync(
            aph::BufferLoadInfo{
                .debugName  = "rt::transformBuffer",
                .data       = &transformMatrix,
                .createInfo = {.size  = static_cast<uint32_t>(sizeof(transformMatrix)),
                               .usage = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
                                        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR}},
            &m_pTransformBuffer);
        m_pResourceLoader->wait();

        VkDeviceOrHostAddressConstKHR vertexBufferDeviceAddress{m_pVB->getDeviceAddress()};
        VkDeviceOrHostAddressConstKHR indexBufferDeviceAddress{m_pIB->getDeviceAddress()};
        VkDeviceOrHostAddressConstKHR transformBufferDeviceAddress{m_pTransformBuffer->getDeviceAddress()};

        // Build
        VkAccelerationStructureGeometryKHR accelerationStructureGeometry{
            .sType        = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
            .geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR,
            .geometry     = {.triangles{
                    .sType         = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR,
                    .vertexFormat  = VK_FORMAT_R32G32B32_SFLOAT,
                    .vertexData    = vertexBufferDeviceAddress,
                    .vertexStride  = sizeof(Vertex),
                    .maxVertex     = 2,
                    .indexType     = VK_INDEX_TYPE_UINT32,
                    .indexData     = indexBufferDeviceAddress,
                    .transformData = transformBufferDeviceAddress,
            }},
            .flags        = VK_GEOMETRY_OPAQUE_BIT_KHR,
        };

        // Get size info
        VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo{
            .sType         = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
            .type          = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
            .flags         = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
            .geometryCount = 1,
            .pGeometries   = &accelerationStructureGeometry,
        };

        const uint32_t                           numTriangles = 1;
        VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo{
            .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR,
        };
        m_pDevice->getDeviceTable()->vkGetAccelerationStructureBuildSizesKHR(m_pDevice->getHandle(), VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
                                                &accelerationStructureBuildGeometryInfo, &numTriangles,
                                                &accelerationStructureBuildSizesInfo);

        createAccelerationStructureBuffer(m_pDevice, bottomLevelAS, accelerationStructureBuildSizesInfo);

        VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo{
            .sType  = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
            .buffer = bottomLevelAS.buffer->getHandle(),
            .size   = accelerationStructureBuildSizesInfo.accelerationStructureSize,
            .type   = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
        };

        vkCreateAccelerationStructureKHR(m_pDevice->getHandle(), &accelerationStructureCreateInfo, nullptr,
                                         &bottomLevelAS.handle);

        // Create a small scratch buffer used during build of the bottom level acceleration structure
        aph::vk::Buffer* scratchBuffer = {};
        APH_VR(m_pDevice->create(
            {
                .size = accelerationStructureBuildSizesInfo.buildScratchSize,
                .usage =
                    VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                .domain = aph::BufferDomain::Device,
            },
            &scratchBuffer));

        VkAccelerationStructureBuildGeometryInfoKHR accelerationBuildGeometryInfo{
            .sType                    = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
            .type                     = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
            .flags                    = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
            .mode                     = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
            .dstAccelerationStructure = bottomLevelAS.handle,
            .geometryCount            = 1,
            .pGeometries              = &accelerationStructureGeometry,
            .scratchData{.deviceAddress = scratchBuffer->getDeviceAddress()},
        };

        VkAccelerationStructureBuildRangeInfoKHR accelerationStructureBuildRangeInfo{
            .primitiveCount  = numTriangles,
            .primitiveOffset = 0,
            .firstVertex     = 0,
            .transformOffset = 0,
        };
        std::vector<VkAccelerationStructureBuildRangeInfoKHR*> accelerationBuildStructureRangeInfos = {
            &accelerationStructureBuildRangeInfo};

        auto queue = m_pDevice->getQueue(aph::QueueType::Graphics);
        m_pDevice->executeSingleCommands(queue, [&](const auto* cmd) {
            vkCmdBuildAccelerationStructuresKHR(cmd->getHandle(), 1, &accelerationBuildGeometryInfo,
                                                accelerationBuildStructureRangeInfos.data());
        });

        VkAccelerationStructureDeviceAddressInfoKHR accelerationDeviceAddressInfo{
            .sType                 = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR,
            .accelerationStructure = bottomLevelAS.handle,
        };
        bottomLevelAS.deviceAddress =
            vkGetAccelerationStructureDeviceAddressKHR(m_pDevice->getHandle(), &accelerationDeviceAddressInfo);

        m_pDevice->destroy(scratchBuffer);
    }

    void createTLAS()
    {
        VkTransformMatrixKHR transformMatrix = {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f};

        VkAccelerationStructureInstanceKHR instance{
            .transform                              = transformMatrix,
            .instanceCustomIndex                    = 0,
            .mask                                   = 0xFF,
            .instanceShaderBindingTableRecordOffset = 0,
            .flags                                  = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR,
            .accelerationStructureReference         = bottomLevelAS.deviceAddress,
        };

        // Buffer for instance data
        aph::vk::Buffer* instancesBuffer;

        m_pResourceLoader->loadAsync(
            aph::BufferLoadInfo{
                .data = &instance,
                .createInfo{
                    .size  = sizeof(VkAccelerationStructureInstanceKHR),
                    .usage = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
                             VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
                    .domain = aph::BufferDomain::Host,
                },
            },
            &instancesBuffer);

        VkDeviceOrHostAddressConstKHR instanceDataDeviceAddress{
            .deviceAddress = instancesBuffer->getDeviceAddress(),
        };

        VkAccelerationStructureGeometryKHR accelerationStructureGeometry{
            .sType        = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
            .geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR,
            .geometry     = {.instances{
                    .sType           = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR,
                    .arrayOfPointers = VK_FALSE,
                    .data            = instanceDataDeviceAddress,
            }},
            .flags        = VK_GEOMETRY_OPAQUE_BIT_KHR,
        };

        // Get size info
        /*
        The pSrcAccelerationStructure, dstAccelerationStructure, and mode members of pBuildInfo are ignored. Any
        VkDeviceOrHostAddressKHR members of pBuildInfo are ignored by this command, except that the hostAddress member
        of VkAccelerationStructureGeometryTrianglesDataKHR::transformData will be examined to check if it is NULL.*
        */
        VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo{
            .sType         = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
            .type          = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
            .flags         = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
            .geometryCount = 1,
            .pGeometries   = &accelerationStructureGeometry,
        };

        uint32_t primitive_count = 1;

        VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo{
            .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR,
        };
        vkGetAccelerationStructureBuildSizesKHR(m_pDevice->getHandle(), VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
                                                &accelerationStructureBuildGeometryInfo, &primitive_count,
                                                &accelerationStructureBuildSizesInfo);

        createAccelerationStructureBuffer(m_pDevice, topLevelAS, accelerationStructureBuildSizesInfo);

        VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo{
            .sType  = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
            .buffer = topLevelAS.buffer->getHandle(),
            .size   = accelerationStructureBuildSizesInfo.accelerationStructureSize,
            .type   = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
        };
        vkCreateAccelerationStructureKHR(m_pDevice->getHandle(), &accelerationStructureCreateInfo, nullptr,
                                         &topLevelAS.handle);

        // Create a small scratch buffer used during build of the top level acceleration structure
        aph::vk::Buffer* scratchBuffer = {};
        APH_VR(m_pDevice->create(
            {
                .size = accelerationStructureBuildSizesInfo.buildScratchSize,
                .usage =
                    VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                .domain = aph::BufferDomain::Device,
            },
            &scratchBuffer));

        VkAccelerationStructureBuildGeometryInfoKHR accelerationBuildGeometryInfo{
            .sType                    = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
            .type                     = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
            .flags                    = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
            .mode                     = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
            .dstAccelerationStructure = topLevelAS.handle,
            .geometryCount            = 1,
            .pGeometries              = &accelerationStructureGeometry,
            .scratchData{.deviceAddress = scratchBuffer->getDeviceAddress()},
        };

        VkAccelerationStructureBuildRangeInfoKHR accelerationStructureBuildRangeInfo{
            .primitiveCount  = 1,
            .primitiveOffset = 0,
            .firstVertex     = 0,
            .transformOffset = 0,
        };
        std::vector<VkAccelerationStructureBuildRangeInfoKHR*> accelerationBuildStructureRangeInfos = {
            &accelerationStructureBuildRangeInfo};

        // Build the acceleration structure on the device via a one-time command buffer submission
        // Some implementations may support acceleration structure building on the host
        // (VkPhysicalDeviceAccelerationStructureFeaturesKHR->accelerationStructureHostCommands), but we prefer device
        // builds
        auto queue = m_pDevice->getQueue(aph::QueueType::Graphics);
        m_pDevice->executeSingleCommands(queue, [&](const auto* cmd) {
            vkCmdBuildAccelerationStructuresKHR(cmd->getHandle(), 1, &accelerationBuildGeometryInfo,
                                                accelerationBuildStructureRangeInfos.data());
        });

        VkAccelerationStructureDeviceAddressInfoKHR accelerationDeviceAddressInfo{
            .sType                 = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR,
            .accelerationStructure = topLevelAS.handle,
        };
        topLevelAS.deviceAddress =
            vkGetAccelerationStructureDeviceAddressKHR(m_pDevice->getHandle(), &accelerationDeviceAddressInfo);

        m_pDevice->destroy(scratchBuffer);
        m_pDevice->destroy(instancesBuffer);
    }

private:
    aph::vk::Buffer*        m_pVB              = {};
    aph::vk::Buffer*        m_pIB              = {};
    aph::vk::Buffer*        m_pUB              = {};
    aph::vk::Buffer*        m_pTransformBuffer = {};
    aph::vk::Image*         m_pStorageImage    = {};
    aph::vk::ShaderProgram* m_pProgram         = {};

private:
    std::unique_ptr<aph::vk::Renderer> m_renderer        = {};
    aph::WSI*                          m_pWSI            = {};
    aph::vk::Device*                   m_pDevice         = {};
    aph::ResourceLoader*               m_pResourceLoader = {};
    aph::vk::SwapChain*                m_pSwapChain      = {};

    AccelerationStructure bottomLevelAS{};
    AccelerationStructure topLevelAS{};

    struct UniformData
    {
        glm::mat4 viewInverse;
        glm::mat4 projInverse;
    } uniformData;

    VkPipelineLayout      pipelineLayout;
    VkPipeline            pipeline;
    VkDescriptorSetLayout descriptorSetLayout;
    VkDescriptorSet       descriptorSet;

private:
    aph::Timer m_timer;
};

ray_tracing::ray_tracing() : aph::BaseApp("ray_tracing")
{
}

void ray_tracing::init()
{
    APH_PROFILER_SCOPE();

    // setup window
    aph::RenderConfig config{
        .flags     = aph::RENDER_CFG_WITHOUT_UI,
        .maxFrames = 1,
        .width     = m_options.windowWidth,
        .height    = m_options.windowHeight,
    };

    aph::vk::DeviceCreateInfo deviceCI{.enabledFeatures = {.raytracing = true}};
    config.pDeviceCreateInfo = &deviceCI;

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

    // create as
    createBLAS();
    createTLAS();

    // create storage image
    {
        APH_VR(m_pDevice->create(
            aph::vk::ImageCreateInfo{
                .extent =
                    {
                        .width  = m_pSwapChain->getWidth(),
                        .height = m_pSwapChain->getHeight(),
                        .depth  = 1,
                    },
                .usage     = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
                .imageType = VK_IMAGE_TYPE_2D,
                .format    = m_pSwapChain->getFormat(),
            },
            &m_pStorageImage));
    }

    // Ray generation group
    {
        // shader program
        m_pResourceLoader->load(
            aph::ShaderLoadInfo{.stageInfo =
                                    {
                                        {aph::ShaderStage::RayGen, {"shader_slang://raygen.rgen"}},
                                        {aph::ShaderStage::Miss, {"shader_slang://miss.rmiss"}},
                                        {aph::ShaderStage::ClosestHit, {"shader_slang://closesthit.rchit"}},
                                    }},
            &m_pProgram);
    }

    // create uniform buffer
    m_pResourceLoader->loadAsync(aph::BufferLoadInfo{.debugName = "rt:uniformbuffer",
                                                     .data      = &uniformData,
                                                     .createInfo{
                                                         .size   = sizeof(UniformData),
                                                         .usage  = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                                         .domain = aph::BufferDomain::Host,
                                                     }},
                                 &m_pUB);

    // record graph execution
    m_renderer->recordGraph([this](auto* graph) {
        auto drawPass = graph->createPass("drawing triangle", aph::QueueType::Graphics);

        drawPass->setColorOutput("render target",
                                 {
                                     .extent = {m_pSwapChain->getWidth(), m_pSwapChain->getHeight(), 1},
                                     .format = m_pSwapChain->getFormat(),
                                 });

        graph->setBackBuffer("render target");

        drawPass->recordExecute([this](auto* pCmd) {
            pCmd->setProgram(m_pProgram);
            pCmd->bindVertexBuffers(m_pVB);
            pCmd->bindIndexBuffers(m_pIB);
            pCmd->drawIndexed({3, 1, 0, 0, 0});
        });
    });
}

void ray_tracing::run()
{
    while(m_pWSI->update())
    {
        APH_PROFILER_SCOPE_NAME("application loop");
        m_renderer->update();
        m_renderer->render();
    }
}

void ray_tracing::finish()
{
    APH_PROFILER_SCOPE();
    m_pDevice->waitIdle();
    m_pDevice->destroy(m_pVB);
    m_pDevice->destroy(m_pIB);
    m_pDevice->destroy(m_pTransformBuffer);
    m_pDevice->destroy(m_pProgram);
}

void ray_tracing::load()
{
    APH_PROFILER_SCOPE();
    m_renderer->load();
}

void ray_tracing::unload()
{
    APH_PROFILER_SCOPE();
    m_renderer->unload();
}

int main(int argc, char** argv)
{
    LOG_SETUP_LEVEL_INFO();

    ray_tracing app;

    // parse command
    {
        int               exitCode;
        aph::CLICallbacks cbs;
        cbs.add("--width", [&](aph::CLIParser& parser) { app.m_options.windowWidth = parser.nextUint(); });
        cbs.add("--height", [&](aph::CLIParser& parser) { app.m_options.windowHeight = parser.nextUint(); });
        cbs.m_errorHandler = [&]() { CM_LOG_ERR("Failed to parse CLI arguments."); };
        if(!aph::parseCliFiltered(cbs, argc, argv, exitCode))
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
