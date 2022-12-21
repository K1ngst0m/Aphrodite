#include "triangle_demo.h"

triangle_demo::triangle_demo()
    : vkl::BaseApp("triangle_demo") {
}

void triangle_demo::init() {
    // setup window
    m_window = vkl::Window::Create(1366, 768);

    // renderer config
    vkl::RenderConfig config{
        .enableDebug         = true,
        .enableUI            = false,
        .initDefaultResource = true,
        .maxFrames           = 2,
    };

    // setup renderer
    m_renderer = vkl::Renderer::Create<vkl::VulkanRenderer>(m_window->getWindowData(), config);
    m_device = m_renderer->getDevice();
    setupPipeline();
}

void triangle_demo::run() {
    // get frame deltatime
    auto timer = vkl::Timer(m_deltaTime);

    // loop
    while (!m_window->shouldClose()) {
        m_window->pollEvents();

        m_renderer->prepareFrame();
        buildCommands();
        m_renderer->submitAndPresent();
    }
}

void triangle_demo::finish() {
    // wait device idle before cleanup
    m_renderer->idleDevice();
    m_device->destroyPipeline(m_demoPipeline);
    m_renderer->cleanup();
}

int main() {
    triangle_demo app;

    app.init();
    app.run();
    app.finish();
}
void triangle_demo::setupPipeline() {
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount   = 0;
    vertexInputInfo.vertexAttributeDescriptionCount = 0;

    vkl::GraphicsPipelineCreateInfo createInfo{};
    createInfo.vertexInputInfo = vertexInputInfo;

    // build Shader
    std::filesystem::path shaderDir = "assets/shaders/glsl/default";

    vkl::EffectInfo effectInfo{};
    effectInfo.shaderMapList[VK_SHADER_STAGE_VERTEX_BIT]   = m_device->getShaderCache()->getShaders(shaderDir / "triangle.vert.spv");
    effectInfo.shaderMapList[VK_SHADER_STAGE_FRAGMENT_BIT] = m_device->getShaderCache()->getShaders(shaderDir / "triangle.frag.spv");
    VK_CHECK_RESULT(m_device->createGraphicsPipeline(&createInfo, &effectInfo, m_renderer->getDefaultRenderPass(), &m_demoPipeline));
}
void triangle_demo::buildCommands() {
    VkViewport viewport = vkl::init::viewport(m_renderer->getSwapChain()->getExtent());
    VkRect2D   scissor  = vkl::init::rect2D(m_renderer->getSwapChain()->getExtent());

    vkl::RenderPassBeginInfo renderPassBeginInfo{};
    renderPassBeginInfo.pRenderPass       = m_renderer->getDefaultRenderPass();
    renderPassBeginInfo.pFramebuffer = m_renderer->getDefaultFrameBuffer(m_renderer->getCurrentImageIndex());
    renderPassBeginInfo.renderArea.offset = {0, 0};
    renderPassBeginInfo.renderArea.extent = m_renderer->getSwapChain()->getExtent();
    std::vector<VkClearValue> clearValues(2);
    clearValues[0].color                = {{0.1f, 0.1f, 0.1f, 1.0f}};
    clearValues[1].depthStencil         = {1.0f, 0};
    renderPassBeginInfo.clearValueCount = clearValues.size();
    renderPassBeginInfo.pClearValues    = clearValues.data();

    VkCommandBufferBeginInfo beginInfo = vkl::init::commandBufferBeginInfo();

    // record command
    auto  commandIndex  = m_renderer->getCurrentFrameIndex();
    auto *commandBuffer = m_renderer->getDefaultCommandBuffer(commandIndex);

    commandBuffer->begin(0);

    // render pass
    commandBuffer->cmdBeginRenderPass(&renderPassBeginInfo);

    // dynamic state
    commandBuffer->cmdSetViewport(&viewport);
    commandBuffer->cmdSetSissor(&scissor);
    commandBuffer->cmdBindPipeline(m_demoPipeline);
    commandBuffer->cmdDraw(3, 1, 0, 0);
    commandBuffer->cmdEndRenderPass();

    commandBuffer->end();
}
