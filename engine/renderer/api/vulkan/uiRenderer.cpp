#include "uiRenderer.h"
#include "renderpass.h"
#include "commandBuffer.h"

#include <imgui/imgui.h>
#include <imgui/imgui_impl_vulkan.h>
#include <imgui_impl_glfw.h>

namespace vkl {
VulkanUIRenderer::VulkanUIRenderer(VulkanRenderer *renderer, const std::shared_ptr<WindowData>& windowData)
    : UIRenderer(windowData), _renderer(renderer), m_device(renderer->getDevice()) {
}

void VulkanUIRenderer::initUI() {
    // 1: create descriptor pool for IMGUI
    //  the size of the pool is very oversize, but it's copied from imgui demo itself.
    VkDescriptorPoolSize poolSizes[] = {{VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
                                        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
                                        {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
                                        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
                                        {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
                                        {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
                                        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
                                        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
                                        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
                                        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
                                        {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}};

    VkDescriptorPoolCreateInfo poolInfo = {
        .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
        .maxSets       = 1000,
        .poolSizeCount = std::size(poolSizes),
        .pPoolSizes    = poolSizes,
    };

    VkDescriptorPool imguiPool;
    VK_CHECK_RESULT(vkCreateDescriptorPool(m_device->getHandle(), &poolInfo, nullptr, &imguiPool));

    // 2: initialize imgui library

    // this initializes the core structures of imgui
    ImGui::CreateContext();

    ImGui_ImplGlfw_InitForVulkan(_windowData->window, true);

    // this initializes imgui for Vulkan
    ImGui_ImplVulkan_InitInfo initInfo = {
        .Instance       = _renderer->getInstance()->getHandle(),
        .PhysicalDevice = m_device->getPhysicalDevice()->getHandle(),
        .Device         = m_device->getHandle(),
        .Queue          = _renderer->getDefaultDeviceQueue(QUEUE_TYPE_GRAPHICS),
        .DescriptorPool = imguiPool,
        .MinImageCount  = 3,
        .ImageCount     = 3,
        .MSAASamples    = VK_SAMPLE_COUNT_1_BIT,
    };

    ImGui_ImplVulkan_Init(&initInfo, _renderer->getDefaultRenderPass()->getHandle());

    // execute a gpu command to upload imgui font textures
    VulkanCommandBuffer *cmd = m_device->beginSingleTimeCommands();
    ImGui_ImplVulkan_CreateFontsTexture(cmd->getHandle());
    m_device->endSingleTimeCommands(cmd);

    // clear font textures from cpu data
    ImGui_ImplVulkan_DestroyFontUploadObjects();

    // TODO
    // glfwSetKeyCallback(_windowData->window, ImGui_ImplGlfw_KeyCallback);
    // glfwSetMouseButtonCallback(_windowData->window, ImGui_ImplGlfw_MouseButtonCallback);

    m_deletionQueue.push_function([=]() {
        vkDestroyDescriptorPool(m_device->getHandle(), imguiPool, nullptr);
        ImGui_ImplVulkan_Shutdown();
    });
}
void VulkanUIRenderer::drawUI() {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGui::ShowDemoWindow();
    ImGui::Render();
}
} // namespace vkl
