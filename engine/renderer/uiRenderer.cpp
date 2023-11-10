#include "uiRenderer.h"
#include "renderer.h"
#include "filesystem/filesystem.h"

#include "imgui.h"
#include "imgui_impl_vulkan.h"

namespace aph::vk
{
enum
{
    VERTEX_BUFFER_SIZE = 1024 * 64 * sizeof(ImDrawVert),
    INDEX_BUFFER_SIZE  = 128 * 1024 * sizeof(ImDrawIdx)
};

UI::UI(const UICreateInfo& ci) :
    m_pWSI(ci.pRenderer->getWSI()),
    m_pRenderer(ci.pRenderer),
    m_pDevice(ci.pRenderer->getDevice()),
    m_pDefaultQueue(m_pDevice->getQueue(QueueType::Graphics))
{
    // init imgui
    IMGUI_CHECKVERSION();
    m_pContext     = ImGui::CreateContext();
    auto& io       = ImGui::GetIO();
    io.IniFilename = ci.configFile.c_str();
    if(ci.flags & UI_Docking)
    {
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    }

    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    if(io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding              = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }
    addFont("font://Roboto-Medium.ttf", 18.0f);

    m_showDemoWindow = ci.flags & UI_Demo;
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
            VkDescriptorPoolSize poolSizes[] = {
                {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1},
            };
            VkDescriptorPoolCreateInfo poolInfo = {};
            poolInfo.sType                      = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            poolInfo.flags                      = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
            poolInfo.maxSets                    = 1;
            poolInfo.poolSizeCount              = (uint32_t)IM_ARRAYSIZE(poolSizes);
            poolInfo.pPoolSizes                 = poolSizes;
            _VR(m_pDevice->getDeviceTable()->vkCreateDescriptorPool(m_pDevice->getHandle(), &poolInfo, vkAllocator(),
                                                                    &m_pool));
        }

        auto checkResult = [](VkResult err) {
            if(err == 0)
                return;
            VK_LOG_ERR("[VK] Error: VkResult = %d\n", err);
            if(err < 0)
                std::abort();
        };

        ImGui_ImplVulkan_LoadFunctions(
            [](const char* function_name, void* vulkan_instance) {
                return vkGetInstanceProcAddr(*(reinterpret_cast<VkInstance*>(vulkan_instance)), function_name);
            },
            &m_pRenderer->getInstance()->getHandle());

        ImGui_ImplVulkan_InitInfo initInfo = {};
        initInfo.Instance                  = m_pRenderer->getInstance()->getHandle();
        initInfo.PhysicalDevice            = m_pDevice->getPhysicalDevice()->getHandle();
        initInfo.Device                    = m_pDevice->getHandle();
        initInfo.QueueFamily               = m_pDefaultQueue->getFamilyIndex();
        initInfo.Queue                     = m_pDefaultQueue->getHandle();
        initInfo.PipelineCache             = VK_NULL_HANDLE;
        initInfo.DescriptorPool            = m_pool;
        initInfo.MinImageCount             = m_pRenderer->getSwapchain()->getCreateInfo().imageCount;
        initInfo.ImageCount                = m_pRenderer->getSwapchain()->getCreateInfo().imageCount;
        initInfo.MSAASamples               = VK_SAMPLE_COUNT_1_BIT;
        initInfo.Allocator                 = vkAllocator();
        initInfo.CheckVkResultFn           = checkResult;
        initInfo.UseDynamicRendering       = true;
        initInfo.ColorAttachmentFormat     = utils::VkCast(m_pRenderer->getSwapchain()->getFormat());
        ImGui_ImplVulkan_Init(&initInfo, VK_NULL_HANDLE);
    }

    // load fonts
    {
        m_pDevice->executeSingleCommands(m_pDefaultQueue,
                                         [](auto* pCmd) { ImGui_ImplVulkan_CreateFontsTexture(pCmd->getHandle()); });

        ImGui_ImplVulkan_DestroyFontUploadObjects();
    }
}

void UI::unload()
{
    ImGui_ImplVulkan_Shutdown();
    {
        m_pDevice->getDeviceTable()->vkDestroyDescriptorPool(m_pDevice->getHandle(), m_pool, vkAllocator());
    }
}

void UI::draw(CommandBuffer* pCmd)
{
    pCmd->beginDebugLabel({.name = "Drawing UI", .color = {0.4f, 0.3f, 0.2f, 1.0f}});
    ImDrawData* main_draw_data = ImGui::GetDrawData();
    ImGui_ImplVulkan_RenderDrawData(main_draw_data, pCmd->getHandle());
    pCmd->endDebugLabel();
}

void UI::update()
{
    ImGui_ImplVulkan_NewFrame();
    ImGui::NewFrame();
    if(m_showDemoWindow)
    {
        ImGui::ShowDemoWindow(&m_showDemoWindow);
    }
    if(m_upateCB)
    {
        m_upateCB();
    }
    ImGui::Render();
}

uint32_t UI::addFont(std::string_view fontPath, float pixelSize)
{
    ImGuiIO& io = ImGui::GetIO();
    auto font = io.Fonts->AddFontFromFileTTF(aph::Filesystem::GetInstance().resolvePath(fontPath).c_str(), pixelSize);
    m_fonts.push_back(font);
    return m_fonts.size() - 1;
}

void UI::pushFont(uint32_t id) const
{
    APH_ASSERT(id >= m_fonts.size());
    ImGui::PushFont(m_fonts[id]);
}

void UI::popFont() const
{
    ImGui::PopFont();
}
}  // namespace aph::vk
