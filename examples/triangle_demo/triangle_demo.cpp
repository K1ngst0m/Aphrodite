#include "triangle_demo.h"

triangle_demo::triangle_demo() : aph::BaseApp("triangle_demo")
{
}

void triangle_demo::init()
{
    // setup window
    m_window = aph::Window::Create(1366, 768);

    // renderer config
    aph::RenderConfig config{
        .enableDebug         = true,
        .enableUI            = false,
        .initDefaultResource = true,
        .maxFrames           = 2,
    };

    // setup renderer
    m_renderer = aph::Renderer::Create<aph::Renderer>(m_window->getWindowData(), config);
    m_device   = m_renderer->getDevice();
    setupPipeline();
}

void triangle_demo::run()
{
    // get frame deltatime
    auto timer = aph::Timer(m_deltaTime);

    // loop
    while(!m_window->shouldClose())
    {
        m_window->pollEvents();

        m_renderer->beginFrame();
        buildCommands();
        m_renderer->endFrame();
    }
}

void triangle_demo::finish()
{
    // wait device idle before cleanup
    m_renderer->idleDevice();
    m_device->destroyPipeline(m_demoPipeline);
    m_renderer->cleanup();
}

int main()
{
    triangle_demo app;

    app.init();
    app.run();
    app.finish();
}
void triangle_demo::setupPipeline()
{
    {
        VkAttachmentDescription colorAttachment{
            .format         = m_renderer->getSwapChain()->getSurfaceFormat(),
            .samples        = VK_SAMPLE_COUNT_1_BIT,
            .loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,

        };
        VkAttachmentDescription depthAttachment{
            .format         = m_device->getDepthFormat(),
            .samples        = VK_SAMPLE_COUNT_1_BIT,
            .loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        };

        aph::RenderPassCreateInfo createInfo{
            .colorAttachments = {colorAttachment},
            .depthAttachment  = {depthAttachment},
        };

        VK_CHECK_RESULT(m_device->createRenderPass(createInfo, &m_pRenderPass));
    }

    {
        m_framebuffers.resize(m_renderer->getSwapChain()->getImageCount());
        m_colorAttachments.resize(m_renderer->getSwapChain()->getImageCount());
        m_depthAttachments.resize(m_renderer->getSwapChain()->getImageCount());

        // color and depth attachment
        for(auto idx = 0; idx < m_renderer->getSwapChain()->getImageCount(); idx++)
        {
            // color image view
            {
                auto& colorImage = m_colorAttachments[idx];
                colorImage       = m_renderer->getSwapChain()->getImage(idx);
            }

            // depth image view
            {
                auto& depthImage = m_depthAttachments[idx];
                {
                    aph::ImageCreateInfo createInfo{
                        .extent   = {m_renderer->getSwapChain()->getExtent().width,
                                     m_renderer->getSwapChain()->getExtent().height, 1},
                        .usage    = aph::IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                        .property = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                        .format   = static_cast<aph::Format>(m_device->getDepthFormat()),
                        .tiling   = aph::IMAGE_TILING_OPTIMAL,
                    };
                    VK_CHECK_RESULT(m_device->createImage(createInfo, &depthImage));
                }

                m_device->executeSingleCommands(aph::QUEUE_GRAPHICS, [&](aph::CommandBuffer* cmd) {
                    cmd->transitionImageLayout(depthImage, VK_IMAGE_LAYOUT_UNDEFINED,
                                               VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
                });
            }

            // framebuffers
            {
                auto& framebuffer     = m_framebuffers[idx];
                auto  colorAttachment = m_colorAttachments[idx]->getImageView();
                auto  depthAttachment = m_depthAttachments[idx]->getImageView();
                {
                    std::vector<aph::ImageView*> attachments{colorAttachment, depthAttachment};
                    aph::FramebufferCreateInfo   createInfo{
                          .width       = m_renderer->getSwapChain()->getExtent().width,
                          .height      = m_renderer->getSwapChain()->getExtent().height,
                          .attachments = {attachments},
                    };
                    VK_CHECK_RESULT(m_device->createFramebuffers(createInfo, &framebuffer));
                }
            }
        }
    }

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{
        .sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount   = 0,
        .vertexAttributeDescriptionCount = 0,
    };

    aph::GraphicsPipelineCreateInfo createInfo{};
    createInfo.vertexInputInfo = vertexInputInfo;

    // build Shader
    std::filesystem::path shaderDir = "assets/shaders/glsl/default";

    createInfo.shaderMapList[VK_SHADER_STAGE_VERTEX_BIT] =
        m_renderer->getShaderCache()->getShaders(shaderDir / "triangle.vert.spv");
    createInfo.shaderMapList[VK_SHADER_STAGE_FRAGMENT_BIT] =
        m_renderer->getShaderCache()->getShaders(shaderDir / "triangle.frag.spv");
    VK_CHECK_RESULT(m_device->createGraphicsPipeline(createInfo, m_pRenderPass, &m_demoPipeline));
}
void triangle_demo::buildCommands()
{
    VkViewport viewport = aph::init::viewport(m_renderer->getSwapChain()->getExtent());
    VkRect2D   scissor  = aph::init::rect2D(m_renderer->getSwapChain()->getExtent());

    aph::RenderPassBeginInfo renderPassBeginInfo{};
    renderPassBeginInfo.pRenderPass       = m_pRenderPass;
    renderPassBeginInfo.pFramebuffer      = m_framebuffers[m_renderer->getCurrentImageIndex()];
    renderPassBeginInfo.renderArea.offset = {0, 0};
    renderPassBeginInfo.renderArea.extent = m_renderer->getSwapChain()->getExtent();
    std::vector<VkClearValue> clearValues(2);
    clearValues[0].color                = {{0.1f, 0.1f, 0.1f, 1.0f}};
    clearValues[1].depthStencil         = {1.0f, 0};
    renderPassBeginInfo.clearValueCount = clearValues.size();
    renderPassBeginInfo.pClearValues    = clearValues.data();

    VkCommandBufferBeginInfo beginInfo = aph::init::commandBufferBeginInfo();

    // record command
    auto  commandIndex  = m_renderer->getCurrentFrameIndex();
    auto* commandBuffer = m_renderer->getDefaultCommandBuffer(commandIndex);

    commandBuffer->begin();

    // dynamic state
    commandBuffer->setViewport(&viewport);
    commandBuffer->setSissor(&scissor);
    commandBuffer->bindPipeline(m_demoPipeline);

    // render pass
    commandBuffer->beginRenderPass(&renderPassBeginInfo);
    commandBuffer->draw(3, 1, 0, 0);
    commandBuffer->endRenderPass();

    commandBuffer->end();
}
