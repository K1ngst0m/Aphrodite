#include "ui.h"

#include "api/vulkan/device.h"
#include "common/common.h"

#include "filesystem/filesystem.h"
#include "widget.h"
#include "widgets.h"
#include "wsi/wsi.h"

// Include ImGui headers
#include <imgui.h>
#include <imgui_impl_vulkan.h>

// Include platform-specific headers based on build config
#ifdef WSI_USE_SDL
#include <imgui_impl_sdl3.h>
#else
#error "UI backend not supported"
#endif

namespace aph
{

Expected<UI*> UI::Create(const UICreateInfo& createInfo)
{
    APH_PROFILER_SCOPE();

    // Create UI with minimal initialization in constructor
    auto* pUI = new UI(createInfo);
    if (!pUI)
    {
        return {Result::RuntimeError, "Failed to allocate UI instance"};
    }

    // Complete the initialization process
    Result initResult = pUI->initialize(createInfo);
    if (!initResult.success())
    {
        delete pUI;
        return {initResult.getCode(), initResult.toString()};
    }

    return pUI;
}

void UI::Destroy(UI* pUI)
{
    if (!pUI)
    {
        return;
    }

    APH_PROFILER_SCOPE();

    // Shutdown the UI system
    pUI->shutdown();

    // Delete the UI instance
    delete pUI;
}

UI::UI(const UICreateInfo& createInfo)
    : m_createInfo(createInfo)
{
}

Result UI::initialize(const UICreateInfo& createInfo)
{
    APH_PROFILER_SCOPE();

    // Already initialized, early return
    if (m_context)
    {
        return Result::Success;
    }

    // Initialize ImGui context
    {
        APH_PROFILER_SCOPE_NAME("Init ImGui Context");
        if (!m_createInfo.pWindow)
        {
            UI_LOG_ERR("Failed to initialize UI: No window provided");
            return {Result::RuntimeError, "No window provided for UI initialization"};
        }

        m_window = m_createInfo.pWindow;

        // Create ImGui context
        IMGUI_CHECKVERSION();
        m_context = ImGui::CreateContext();
        if (!m_context)
        {
            return {Result::RuntimeError, "Failed to create ImGui context"};
        }

        // Configure ImGui
        ImGuiIO& io = ImGui::GetIO();
        if (!m_createInfo.configFile.empty())
        {
            io.IniFilename = m_createInfo.configFile.c_str();
        }

        // Enable docking if requested
        if (m_createInfo.flags & UIFlagBits::Docking)
        {
            io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        }

        // Enable viewports if requested
        if (m_createInfo.flags & UIFlagBits::ViewportEnable)
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

    // Initialize platform backend
    {
        APH_PROFILER_SCOPE_NAME("Init Platform Backend");
        if (!m_window || !ImGui_ImplSDL3_InitForVulkan((SDL_Window*)m_window->getNativeHandle()))
        {
            UI_LOG_ERR("Failed to init ImGui SDL backend");
            return {Result::RuntimeError, "Failed to initialize ImGui SDL backend"};
        }

        UI_LOG_INFO("ImGui SDL backend initialized");
    }

    // Initialize renderer backend
    {
        APH_PROFILER_SCOPE_NAME("Init Renderer Backend");

        m_device = m_createInfo.pDevice;
        m_instance = m_createInfo.pInstance;
        m_swapchain = m_createInfo.pSwapchain;
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

        auto format = static_cast<VkFormat>(vk::utils::VkCast(m_swapchain->getFormat()));
        // Initialize ImGui Vulkan implementation
        ImGui_ImplVulkan_InitInfo initInfo{
            .Instance = m_instance->getHandle(),
            .PhysicalDevice = m_device->getPhysicalDevice()->getHandle(),
            .Device = m_device->getHandle(),
            .QueueFamily = m_graphicsQueue->getFamilyIndex(),
            .Queue = m_graphicsQueue->getHandle(),
            .MinImageCount = m_swapchain->getCreateInfo().imageCount,
            .ImageCount = m_swapchain->getCreateInfo().imageCount,
            .MSAASamples = VK_SAMPLE_COUNT_1_BIT,
            .DescriptorPoolSize = 512,
            .UseDynamicRendering = true,
            .PipelineRenderingCreateInfo = {.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
                                            .colorAttachmentCount = 1,
                                            .pColorAttachmentFormats = &format,
                                            .depthAttachmentFormat = VK_FORMAT_D32_SFLOAT},
            .Allocator = vk::vkAllocator(),
            .CheckVkResultFn = checkResult,
        };

        if (!ImGui_ImplVulkan_Init(&initInfo))
        {
            UI_LOG_ERR("Failed to init ImGui Vulkan backend");
            return {Result::RuntimeError, "Failed to initialize ImGui Vulkan backend"};
        }

        UI_LOG_INFO("ImGui Vulkan backend initialized");
    }

    // Load default font
    addFont("font://Roboto-Medium.ttf", 18.0f);

    return Result::Success;
}

void UI::shutdown()
{
    APH_PROFILER_SCOPE();

    if (!m_context)
    {
        return;
    }

    // Find and free all windows and their widgets from the containers
    for (auto it = m_containers.begin(); it != m_containers.end();)
    {
        auto container = *it;
        if (container && container->getType() == ContainerType::Window)
        {
            // We can safely cast to WidgetWindow since we've confirmed the type
            auto* window = static_cast<WidgetWindow*>(container);

            // First remove this window from the containers list to avoid double-free issues
            it = m_containers.erase(it);

            // Free from pool (which also deletes all contained widgets)
            m_windowPool.free(window);
        }
        else
        {
            ++it;
        }
    }

    // Clear all remaining containers
    clearContainers();

    {
        APH_PROFILER_SCOPE_NAME("Shutdown Vulkan Backend");
        ImGui_ImplVulkan_Shutdown();
    }

    {
        APH_PROFILER_SCOPE_NAME("Shutdown SDL Backend");
        ImGui_ImplSDL3_Shutdown();
    }

    // Shutdown ImGui context
    {
        APH_PROFILER_SCOPE_NAME("Destroy ImGui Context");
        ImGui::DestroyContext(m_context);
        m_context = nullptr;
    }

    m_fonts.clear();
    m_device = nullptr;
    m_instance = nullptr;
    m_graphicsQueue = nullptr;
    m_swapchain = nullptr;
    m_window = nullptr;

    UI_LOG_INFO("UI system shutdown");
}

void UI::beginFrame()
{
    APH_PROFILER_SCOPE();
    if (!m_context)
    {
        return;
    }

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
}

void UI::endFrame()
{
    // Don't need to do anything here since render() handles the finalization
    // This method exists for API consistency
}

void UI::render(vk::CommandBuffer* pCmd)
{
    APH_PROFILER_SCOPE();
    APH_ASSERT(pCmd);

    if (!m_context)
    {
        return;
    }

    // Begin a new frame if not already started
    beginFrame();

    // Call the user-provided update callback
    if (m_updateCallback)
    {
        m_updateCallback();
    }

    // Update all registered containers
    for (auto container : m_containers)
    {
        if (container)
        {
            // Check if the container is a WidgetWindow using the container type
            if (container->getType() == ContainerType::Window)
            {
                // We can safely cast to WidgetWindow since we've confirmed the type
                auto* window = static_cast<WidgetWindow*>(container);
                window->draw();
            }
            else
            {
                // For regular containers, just draw all widgets
                container->drawAll();
            }
        }
    }

    // Finish the ImGui frame and render it
    ImGui::Render();
    {
        ImDrawData* drawData = ImGui::GetDrawData();
        APH_ASSERT(drawData);

        // Begin ImGui debug region
        pCmd->beginDebugLabel({.name = "Drawing UI", .color = {0.4f, 0.3f, 0.2f, 1.0f}});

        // Render ImGui using the Vulkan command buffer
        ImGui_ImplVulkan_RenderDrawData(drawData, pCmd->getHandle());

        // End ImGui debug region
        pCmd->endDebugLabel();
    }

    // Update and render additional platform windows if viewports are enabled
    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }
}

void UI::setUpdateCallback(UIUpdateCallback&& callback)
{
    m_updateCallback = std::move(callback);
}

uint32_t UI::addFont(const std::string& fontPath, float fontSize)
{
    APH_PROFILER_SCOPE_NAME("Add Font");

    if (!m_context)
    {
        UI_LOG_ERR("Cannot add font: UI not initialized");
        return 0;
    }

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

        // Load and create font textures
        ImGui_ImplVulkan_DestroyFontsTexture();
        if (!ImGui_ImplVulkan_CreateFontsTexture())
        {
            UI_LOG_ERR("Failed to create ImGui font textures");
            return 0;
        }

        return m_fonts.size() - 1;
    }

    return 0; // Return default font if failed
}

void UI::setActiveFont(uint32_t fontIndex)
{
    APH_PROFILER_SCOPE();

    if (!m_context)
    {
        UI_LOG_ERR("Cannot set active font: UI not initialized");
        return;
    }

    if (fontIndex >= m_fonts.size())
    {
        UI_LOG_ERR("Invalid font index: %d", fontIndex);
        return;
    }

    m_activeFontIndex = fontIndex;
    ImGui::GetIO().FontDefault = m_fonts[fontIndex];
}

Expected<WidgetWindow*> UI::createWindow(const std::string& title)
{
    APH_PROFILER_SCOPE();

    if (!m_context)
    {
        return {Result::RuntimeError, "Cannot create window: UI not initialized"};
    }

    // Allocate from pool
    WidgetWindow* window = m_windowPool.allocate(this);
    if (!window)
    {
        return {Result::RuntimeError, "Failed to allocate widget window from pool"};
    }

    // Set the window title
    window->setTitle(title);

    // Register the container as a raw pointer
    registerContainer(window);

    return window;
}

void UI::registerContainer(WidgetContainer* container)
{
    if (container)
    {
        m_containers.push_back(container);
    }
}

void UI::unregisterContainer(WidgetContainer* container)
{
    auto it = std::find(m_containers.begin(), m_containers.end(), container);
    if (it != m_containers.end())
    {
        m_containers.erase(it);
    }
}

void UI::clearContainers()
{
    m_containers.clear();
}

void UI::destroyWindow(WidgetWindow* window)
{
    if (!window)
    {
        return;
    }

    APH_PROFILER_SCOPE();

    // Find and remove from containers
    unregisterContainer(window);

    // Free the window (the pool handles the memory)
    m_windowPool.free(window);
}

void UI::destroyWidget(Widget* widget)
{
    if (!widget)
    {
        return;
    }

    APH_PROFILER_SCOPE();

    // Free from pool
    m_widgetPool.free(widget);
}

UI::~UI()
{
    // Clear pools and containers
    m_windowPool.clear();
    m_widgetPool.clear();
    m_containers.clear();

    // Proper cleanup should happen through Destroy
}

} // namespace aph
