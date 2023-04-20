#include "renderer.h"

#include <utility>
#include "sceneRenderer.h"
#include "api/vulkan/device.h"

#include "scene/mesh.h"

namespace aph
{
VulkanRenderer::VulkanRenderer(std::shared_ptr<Window> window, const RenderConfig& config) :
    IRenderer(std::move(window), config)
{
    // create instance
    {
        volkInitialize();

        std::vector<const char*> extensions{};
        {
            uint32_t     glfwExtensionCount = 0;
            const char** glfwExtensions;
            glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
            extensions     = std::vector<const char*>(glfwExtensions, glfwExtensions + glfwExtensionCount);

            if(m_config.enableDebug)
            {
                extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
            }
            extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
        }

        InstanceCreateInfo instanceCreateInfo{ .enabledExtensions = extensions };

        if(m_config.enableDebug)
        {
            const std::vector<const char*> validationLayers = {
                "VK_LAYER_KHRONOS_validation",
            };
            instanceCreateInfo.flags         = INSTANCE_CREATION_ENABLE_DEBUG;
            instanceCreateInfo.enabledLayers = validationLayers;
        }

        VK_CHECK_RESULT(VulkanInstance::Create(instanceCreateInfo, &m_pInstance));
    }

    // create device
    {
        const std::vector<const char*> deviceExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
            VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
            VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME,
            VK_KHR_MAINTENANCE_4_EXTENSION_NAME,
        };

        DeviceCreateInfo createInfo{
            .enabledExtensions = deviceExtensions,
            // TODO select physical device
            .pPhysicalDevice = m_pInstance->getPhysicalDevices(0),
        };

        VK_CHECK_RESULT(VulkanDevice::Create(createInfo, &m_pDevice));

        // get 3 type queue
        m_queue.graphics = m_pDevice->getQueueByFlags(QUEUE_GRAPHICS);
        m_queue.compute  = m_pDevice->getQueueByFlags(QUEUE_COMPUTE);
        m_queue.transfer = m_pDevice->getQueueByFlags(QUEUE_TRANSFER);
        if(!m_queue.compute)
        {
            m_queue.compute = m_queue.graphics;
        }
        if(!m_queue.transfer)
        {
            m_queue.transfer = m_queue.compute;
        }

        // check sample count support
        {
            auto limit = createInfo.pPhysicalDevice->getProperties().limits;
            auto counts = limit.framebufferColorSampleCounts & limit.framebufferDepthSampleCounts;
            if (!(counts & m_config.sampleCount))
            {
                m_config.sampleCount = SAMPLE_COUNT_1_BIT;
            }
        }

    }

    // setup swapchain
    {
        VK_CHECK_RESULT(glfwCreateWindowSurface(m_pInstance->getHandle(), m_window->getHandle(), nullptr, &m_surface));
        SwapChainCreateInfo createInfo{
            .surface      = m_surface,
            .windowHandle = m_window->getHandle(),
        };
        VK_CHECK_RESULT(m_pDevice->createSwapchain(createInfo, &m_pSwapChain));
    }

    // init default resources
    if(m_config.initDefaultResource)
    {
        m_frameFences.resize(m_config.maxFrames);
        m_commandBuffers.resize(m_config.maxFrames);
        m_renderSemaphore.resize(m_config.maxFrames);
        m_presentSemaphore.resize(m_config.maxFrames);

        {
            m_pSyncPrimitivesPool = new VulkanSyncPrimitivesPool(m_pDevice);
        }

        // command buffer
        {
            m_pDevice->allocateCommandBuffers(m_commandBuffers.size(), m_commandBuffers.data(), getGraphicsQueue());
        }

        {
            m_pSyncPrimitivesPool->acquireSemaphore(m_presentSemaphore.size(), m_presentSemaphore.data());
            m_pSyncPrimitivesPool->acquireSemaphore(m_renderSemaphore.size(), m_renderSemaphore.data());

            for(uint32_t idx = 0; idx < m_config.maxFrames; idx++)
            {
                m_pSyncPrimitivesPool->acquireFence(m_frameFences[idx]);
            }
        }

        // pipeline cache
        {
            VkPipelineCacheCreateInfo pipelineCacheCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO };
            VK_CHECK_RESULT(
                vkCreatePipelineCache(m_pDevice->getHandle(), &pipelineCacheCreateInfo, nullptr, &m_pipelineCache));
        }
    }
}

void VulkanRenderer::beginFrame()
{
    VK_CHECK_RESULT(m_pDevice->waitForFence({ m_frameFences[m_frameIdx] }));
    VK_CHECK_RESULT(m_pSwapChain->acquireNextImage(&m_imageIdx, m_renderSemaphore[m_frameIdx]));
    VK_CHECK_RESULT(m_pSyncPrimitivesPool->releaseFence(m_frameFences[m_frameIdx]));

    {
        m_timer = std::chrono::high_resolution_clock::now();
    }
}

void VulkanRenderer::endFrame()
{
    auto* queue = getGraphicsQueue();

    QueueSubmitInfo submitInfo{
        .commandBuffers   = { m_commandBuffers[m_frameIdx] },
        .waitStages       = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT },
        .waitSemaphores   = { m_renderSemaphore[m_frameIdx] },
        .signalSemaphores = { m_presentSemaphore[m_frameIdx] },
    };

    VK_CHECK_RESULT(queue->submit({ submitInfo }, m_frameFences[m_frameIdx]));
    VK_CHECK_RESULT(m_pSwapChain->presentImage(m_imageIdx, queue, { m_presentSemaphore[m_frameIdx] }));

    m_frameIdx = (m_frameIdx + 1) % m_config.maxFrames;

    {
        m_frameCounter++;
        auto tEnd      = std::chrono::high_resolution_clock::now();
        auto tDiff     = std::chrono::duration<double, std::milli>(tEnd - m_timer).count();
        m_frameTimer   = (float)tDiff / 1000.0f;
        float fpsTimer = (float)(std::chrono::duration<double, std::milli>(tEnd - m_lastTimestamp).count());
        if(fpsTimer > 1000.0f)
        {
            m_lastFPS       = static_cast<uint32_t>((float)m_frameCounter * (1000.0f / fpsTimer));
            m_frameCounter  = 0;
            m_lastTimestamp = tEnd;
        }
        m_tPrevEnd = tEnd;
    }
}

void VulkanRenderer::cleanup()
{
    for(auto& [key, shaderModule] : shaderModuleCaches)
    {
        vkDestroyShaderModule(m_pDevice->getHandle(), shaderModule->getHandle(), nullptr);
        delete shaderModule;
    }

    if(m_pSyncPrimitivesPool)
    {
        delete m_pSyncPrimitivesPool;
    }

    vkDestroyPipelineCache(m_pDevice->getHandle(), m_pipelineCache, nullptr);

    m_pDevice->destroySwapchain(m_pSwapChain);
    VulkanDevice::Destroy(m_pDevice);
    vkDestroySurfaceKHR(m_pInstance->getHandle(), m_surface, nullptr);
    VulkanInstance::Destroy(m_pInstance);
}

void VulkanRenderer::idleDevice()
{
    m_pDevice->waitIdle();
}

VulkanShaderModule* VulkanRenderer::getShaders(const std::filesystem::path& path)
{
    if(!shaderModuleCaches.count(path))
    {
        std::vector<char> spvCode;
        if(path.extension() == ".spv")
        {
            spvCode = aph::utils::loadSpvFromFile(path);
        }
        else
        {
            spvCode = aph::utils::loadGlslFromFile(path);
        }
        auto shaderModule        = VulkanShaderModule::Create(m_pDevice, spvCode);
        shaderModuleCaches[path] = shaderModule;
    }
    return shaderModuleCaches[path];
}
}  // namespace aph
