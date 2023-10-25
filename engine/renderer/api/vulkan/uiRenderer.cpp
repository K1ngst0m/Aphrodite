#include "uiRenderer.h"
#include "renderer.h"

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
    m_pDevice(ci.pRenderer->m_pDevice.get())
{
#if 0
    // create sampler
    {
        SamplerCreateInfo samplerCI = init::samplerCreateInfo2(SamplerPreset::Linear);
        APH_CHECK_RESULT(m_pDevice->create(samplerCI, &m_pDefaultSampler));
    }

    // init vertex buffer
    {
        BufferLoadInfo loadInfo{.createInfo = {
                                    .size   = VERTEX_BUFFER_SIZE * m_pRenderer->getConfig().maxFrames,
                                    .usage  = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                    .domain = BufferDomain::LinkedDeviceHostPreferDevice,
                                }};

        m_pRenderer->m_pResourceLoader->load(loadInfo, &m_pVertexBuffer);
    }

    // init index buffer
    {
        BufferLoadInfo loadInfo{.createInfo = {
                                    .size   = INDEX_BUFFER_SIZE * m_pRenderer->getConfig().maxFrames,
                                    .usage  = VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                                    .domain = BufferDomain::LinkedDeviceHostPreferDevice,
                                }};
        m_pRenderer->m_pResourceLoader->load(loadInfo, &m_pIndexBuffer);
    }

    // frame uniform buffer
    {
        BufferLoadInfo loadInfo = {.createInfo = {
                                       .size   = sizeof(glm::mat4),
                                       .usage  = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                       .domain = BufferDomain::LinkedDeviceHostPreferDevice,
                                   }};

        m_pUniformBuffers.resize(m_pRenderer->getConfig().maxFrames);
        for(auto& buffer : m_pUniformBuffers)
        {
            m_pRenderer->m_pResourceLoader->load(loadInfo, &buffer);
        }
    }

    // init vertex input layout
    {
        struct VertexLayout
        {
            glm::vec2   pos;
            glm::vec2   uv;
            glm::i8vec4 color;
        };

        m_vertexInput.attributes = {
            {0, 0, Format::RG_F32, offsetof(VertexLayout, pos)},
            {1, 0, Format::RG_F32, offsetof(VertexLayout, uv)},
            {2, 0, Format::RGBA_UN8, offsetof(VertexLayout, color)},
        };

        m_vertexInput.bindings = {{.stride = sizeof(VertexLayout)}};
    }
#endif

    // getQueue
    {
        m_pDefaultQueue = m_pDevice->getQueueByFlags(QueueType::GRAPHICS);
    }

    // init imgui
    // IMGUI_CHECKVERSION();
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

    m_showDemoWindow = ci.flags & UI_Demo;
}

UI::~UI()
{
    ImGui::DestroyContext();
#if 0
    m_pDevice->destroy(m_pDefaultSampler, m_pVertexBuffer, m_pIndexBuffer);
    for(auto& buffer : m_pUniformBuffers)
    {
        m_pDevice->destroy(buffer);
    }
#endif
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
            _VR(vkCreateDescriptorPool(m_pDevice->getHandle(), &poolInfo, vkAllocator(), &m_pool));
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
        initInfo.MinImageCount             = m_pRenderer->m_pSwapChain->getCreateInfo().imageCount;
        initInfo.ImageCount                = m_pRenderer->m_pSwapChain->getCreateInfo().imageCount;
        initInfo.MSAASamples               = VK_SAMPLE_COUNT_1_BIT;
        initInfo.Allocator                 = vkAllocator();
        initInfo.CheckVkResultFn           = checkResult;
        initInfo.UseDynamicRendering       = true;
        initInfo.ColorAttachmentFormat     = utils::VkCast(m_pRenderer->m_pSwapChain->getFormat());
        ImGui_ImplVulkan_Init(&initInfo, VK_NULL_HANDLE);
    }

    // load fonts
    {
        m_pDevice->executeSingleCommands(
            m_pDefaultQueue, [](CommandBuffer* pCmd) { ImGui_ImplVulkan_CreateFontsTexture(pCmd->getHandle()); });

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
    ImGui::Render();
}
}  // namespace aph::vk
