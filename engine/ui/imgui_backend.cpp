#include "imgui_backend.h"
#include "common/common.h"
#include "filesystem/filesystem.h"
#include "renderer/renderer.h"
#include "wsi/wsi.h"

// Include ImGui headers
#include "imgui.h"
#include "imgui_impl_vulkan.h"

// Include platform-specific headers based on build config
#ifdef WSI_USE_SDL
#include "imgui_impl_sdl3.h"
#endif

namespace aph
{
bool ImGuiBackend::initialize(const UICreateInfo& createInfo)
{
    APH_PROFILER_SCOPE();

    // init imgui context
    {
        APH_PROFILER_SCOPE_NAME("Init ImGui Context");
        if (!createInfo.pWindow)
        {
            UI_LOG_ERR("Failed to initialize ImGui backend: No window provided");
            APH_ASSERT(false);
            return false;
        }

        m_window = createInfo.pWindow;
        m_configFile = createInfo.configFile;
        m_flags = createInfo.flags;

        // Create ImGui context
        IMGUI_CHECKVERSION();
        m_context = ImGui::CreateContext();
        if (!m_context)
        {
            return false;
        }

        // Configure ImGui
        ImGuiIO& io = ImGui::GetIO();
        if (!m_configFile.empty())
        {
            io.IniFilename = m_configFile.c_str();
        }

        // Enable docking if requested
        if (m_flags & UIFlagBits::Docking)
        {
            io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        }

        // Enable viewports if requested
        if (m_flags & UIFlagBits::ViewportEnable)
        {
            io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
        }

        // Set up style
        ImGui::StyleColorsDark();
        ImGuiStyle& style = ImGui::GetStyle();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            style.WindowRounding = 0.0f;
            style.Colors[ImGuiCol_WindowBg].w = 1.0f;
        }
    }

    // init platform backend
    {
        APH_PROFILER_SCOPE_NAME("Init Platform Backend");
        if (!m_window || !ImGui_ImplSDL3_InitForVulkan((SDL_Window*)m_window->getNativeHandle()))
        {
            UI_LOG_ERR("Failed to init ImGui SDL backend");
            APH_ASSERT(false);
            return false;
        }

        UI_LOG_INFO("ImGui SDL backend initialized");
    }

    // init renderer backend
    {
        APH_PROFILER_SCOPE_NAME("Init Renderer Backend");
        const auto& pRenderer = createInfo.pRenderer;
        APH_ASSERT(pRenderer);

        m_renderer = pRenderer;
        m_device = pRenderer->getDevice();
        m_instance = pRenderer->getInstance();
        m_graphicsQueue = m_device->getQueue(QueueType::Graphics);

        // Set up error callback for ImGui Vulkan
        auto checkResult = [](VkResult err)
        {
            if (err == 0)
                return;
            UI_LOG_ERR("Vulkan error: VkResult = %d", err);
            if (err < 0)
                std::abort();
        };

        // Load Vulkan functions for ImGui
        ImGui_ImplVulkan_LoadFunctions(
            [](const char* function_name, void* vulkan_instance)
            {
                return VULKAN_HPP_DEFAULT_DISPATCHER.vkGetInstanceProcAddr(
                    *(reinterpret_cast<VkInstance*>(vulkan_instance)), function_name);
            },
            &m_instance->getHandle());

        auto format = static_cast<VkFormat>(vk::utils::VkCast(m_renderer->getSwapchain()->getFormat()));
        // Initialize ImGui Vulkan implementation
        ImGui_ImplVulkan_InitInfo initInfo{
            .Instance = m_instance->getHandle(),
            .PhysicalDevice = m_device->getPhysicalDevice()->getHandle(),
            .Device = m_device->getHandle(),
            .QueueFamily = m_graphicsQueue->getFamilyIndex(),
            .Queue = m_graphicsQueue->getHandle(),
            .MinImageCount = m_renderer->getSwapchain()->getCreateInfo().imageCount,
            .ImageCount = m_renderer->getSwapchain()->getCreateInfo().imageCount,
            .MSAASamples = VK_SAMPLE_COUNT_1_BIT,
            .DescriptorPoolSize = 512,
            .UseDynamicRendering = true,
            .PipelineRenderingCreateInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
                                             .colorAttachmentCount = 1,
                                             .pColorAttachmentFormats = &format },
            .Allocator = vk::vkAllocator(),
            .CheckVkResultFn = checkResult,
        };

        if (!ImGui_ImplVulkan_Init(&initInfo))
        {
            UI_LOG_ERR("Failed to init ImGui Vulkan backend");
            APH_ASSERT(false);
            return false;
        }

        UI_LOG_INFO("ImGui Vulkan backend initialized");
    }

    // Load default font
    {
        APH_PROFILER_SCOPE_NAME("Load Default Font");
        addFont("font://Roboto-Medium.ttf", 18.0f);
    }

    return true;
}

void ImGuiBackend::shutdown()
{
    APH_PROFILER_SCOPE();

    {
        APH_PROFILER_SCOPE_NAME("Shutdown Vulkan Backend");
        ImGui_ImplVulkan_Shutdown();
    }

    {
        APH_PROFILER_SCOPE_NAME("Shutdown SDL Backend");
        ImGui_ImplSDL3_Shutdown();
    }

    // Shutdown ImGui context
    if (m_context)
    {
        APH_PROFILER_SCOPE_NAME("Destroy ImGui Context");
        ImGui::DestroyContext(m_context);
        m_context = nullptr;
    }

    m_fonts.clear();
    m_device = nullptr;
    m_instance = nullptr;
    m_graphicsQueue = nullptr;
    m_renderer = nullptr;

    UI_LOG_INFO("ImGui backend shutdown");
}

void ImGuiBackend::showDemoWindow(bool show)
{
    m_showDemoWindow = show;
}

void ImGuiBackend::newFrame()
{
    APH_PROFILER_SCOPE();
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
    
    if (m_showDemoWindow)
    {
        ImGui::ShowDemoWindow(&m_showDemoWindow);
    }
}

void ImGuiBackend::render(vk::CommandBuffer* pCmd)
{
    APH_PROFILER_SCOPE();
    APH_ASSERT(pCmd);

    // Finish the ImGui frame and render it
    ImGui::Render();
    {
        // APH_PROFILER_SCOPE_NAME("Render ImGui Draw Data");
        ImDrawData* drawData = ImGui::GetDrawData();
        APH_ASSERT(drawData);
        // Begin ImGui debug region
        pCmd->beginDebugLabel({ .name = "Drawing UI", .color = { 0.4f, 0.3f, 0.2f, 1.0f } });

        // Render ImGui using the Vulkan command buffer
        ImGui_ImplVulkan_RenderDrawData(drawData, pCmd->getHandle());

        // End ImGui debug region
        pCmd->endDebugLabel();
    }

    // Update and render additional platform windows if viewports are enabled
    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        APH_PROFILER_SCOPE_NAME("Update and Render Platform Windows");
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }
}

uint32_t ImGuiBackend::addFont(const std::string& fontPath, float fontSize)
{
    APH_PROFILER_SCOPE_NAME("Add Font");

    // Access the filesystem to get the actual font path
    auto& filesystem = APH_DEFAULT_FILESYSTEM;
    std::string resolvedPath = filesystem.resolvePath(fontPath);

    // Add the font to ImGui
    ImGuiIO& io = ImGui::GetIO();
    ImFont* font = io.Fonts->AddFontFromFileTTF(resolvedPath.c_str(), fontSize);
    io.Fonts->Build();

    // Store the font and return its index
    if (font)
    {
        m_fonts.push_back(font);
        return m_fonts.size() - 1;
    }

    // Load and create font textures
    ImGui_ImplVulkan_DestroyFontsTexture();
    if (!ImGui_ImplVulkan_CreateFontsTexture())
    {
        UI_LOG_ERR("Failed to create ImGui font textures");
        APH_ASSERT(false);
        return 0;
    }

    return 0; // Return default font if failed
}

void ImGuiBackend::setActiveFont(uint32_t fontIndex)
{
    APH_PROFILER_SCOPE();

    if (fontIndex >= m_fonts.size())
    {
        UI_LOG_ERR("Invalid font index: %d", fontIndex);
        APH_ASSERT(false);
        return;
    }

    m_activeFontIndex = fontIndex;
    ImGui::GetIO().FontDefault = m_fonts[fontIndex];
}

} // namespace aph
