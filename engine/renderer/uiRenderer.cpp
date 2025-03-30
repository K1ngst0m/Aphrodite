#include "uiRenderer.h"
#include "filesystem/filesystem.h"
#include "global/globalManager.h"
#include "renderer.h"

#include "imgui.h"
#include "imgui_impl_vulkan.h"

namespace aph::vk
{
enum
{
    VERTEX_BUFFER_SIZE = 1024 * 64 * sizeof(ImDrawVert),
    INDEX_BUFFER_SIZE = 128 * 1024 * sizeof(ImDrawIdx)
};

UI::UI(const UICreateInfo& ci)
    : m_pWSI(ci.pRenderer->getWindowSystem())
    , m_pRenderer(ci.pRenderer)
    , m_pDevice(ci.pRenderer->getDevice())
    , m_pDefaultQueue(m_pDevice->getQueue(QueueType::Graphics))
{
    // init imgui
    IMGUI_CHECKVERSION();
    m_pContext = ImGui::CreateContext();
    auto& io = ImGui::GetIO();
    io.IniFilename = ci.configFile.c_str();
    if (ci.flags & UI_Docking)
    {
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    }

    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }
    addFont("font://Roboto-Medium.ttf", 18.0f);
}

UI::~UI()
{
    ImGui::DestroyContext();
}

void UI::load()
{
    // init platform data
    {
        auto result = m_pWSI->initUI();
        APH_ASSERT(result);
    }

    // init api data
    {
        // TODO remove: descriptor pool manually creation
        {
            ::vk::DescriptorPoolSize poolSizes{};
            poolSizes.setType(::vk::DescriptorType::eCombinedImageSampler).setDescriptorCount(1);

            ::vk::DescriptorPoolCreateInfo poolInfo{};
            poolInfo.setFlags(::vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet)
                .setMaxSets(1)
                .setPoolSizes({ poolSizes });

            auto [result, pool] = m_pDevice->getHandle().createDescriptorPool(poolInfo, vk_allocator());
            VK_VR(result);
            m_pool = pool;
        }

        auto checkResult = [](VkResult err)
        {
            if (err == 0)
                return;
            VK_LOG_ERR("[VK] Error: VkResult = %d\n", err);
            if (err < 0)
                std::abort();
        };

        ImGui_ImplVulkan_LoadFunctions(
            [](const char* function_name, void* vulkan_instance)
            {
                return VULKAN_HPP_DEFAULT_DISPATCHER.vkGetInstanceProcAddr(
                    *(reinterpret_cast<VkInstance*>(vulkan_instance)), function_name);
            },
            &m_pRenderer->getInstance()->getHandle());

        ImGui_ImplVulkan_InitInfo initInfo = {};
        initInfo.Instance = m_pRenderer->getInstance()->getHandle();
        initInfo.PhysicalDevice = m_pDevice->getPhysicalDevice()->getHandle();
        initInfo.Device = m_pDevice->getHandle();
        initInfo.QueueFamily = m_pDefaultQueue->getFamilyIndex();
        initInfo.Queue = m_pDefaultQueue->getHandle();
        initInfo.PipelineCache = VK_NULL_HANDLE;
        initInfo.DescriptorPool = m_pool;
        initInfo.MinImageCount = m_pRenderer->getSwapchain()->getCreateInfo().imageCount;
        initInfo.ImageCount = m_pRenderer->getSwapchain()->getCreateInfo().imageCount;
        initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
        initInfo.Allocator = vkAllocator();
        initInfo.CheckVkResultFn = checkResult;
        initInfo.UseDynamicRendering = true;
        initInfo.Allocator = vkAllocator();
        initInfo.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
        auto format = static_cast<VkFormat>(utils::VkCast(m_pRenderer->getSwapchain()->getFormat()));
        initInfo.PipelineRenderingCreateInfo.pColorAttachmentFormats = &format;
        ImGui_ImplVulkan_Init(&initInfo);
    }

    // load fonts
    {
        ImGui_ImplVulkan_CreateFontsTexture();
    }
}

void UI::unload()
{
    ImGui_ImplVulkan_Shutdown();
    m_pDevice->getHandle().destroyDescriptorPool(m_pool, vk_allocator());
}

void UI::draw(CommandBuffer* pCmd)
{
    pCmd->beginDebugLabel({ .name = "Drawing UI", .color = { 0.4f, 0.3f, 0.2f, 1.0f } });
    ImDrawData* main_draw_data = ImGui::GetDrawData();
    ImGui_ImplVulkan_RenderDrawData(main_draw_data, pCmd->getHandle());
    pCmd->endDebugLabel();
}

void UI::update()
{
    ImGui_ImplVulkan_NewFrame();
    ImGui::NewFrame();
    bool showDemoWindow = true;
    ImGui::ShowDemoWindow(&showDemoWindow);
    if (m_upateCB)
    {
        m_upateCB();
    }
    ImGui::Render();
}

uint32_t UI::addFont(std::string_view fontPath, float pixelSize)
{
    ImGuiIO& io = ImGui::GetIO();
    auto& filesystem = APH_DEFAULT_FILESYSTEM;
    auto font = io.Fonts->AddFontFromFileTTF(filesystem.resolvePath(fontPath).c_str(), pixelSize);
    m_fonts.push_back(font);
    return m_fonts.size() - 1;
}
} // namespace aph::vk
