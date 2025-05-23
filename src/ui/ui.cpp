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

#include <algorithm>
#else
#error "UI backend not supported"
#endif

namespace aph
{

auto UI::Create(const UICreateInfo& createInfo) -> Expected<UI*>
{
    APH_PROFILER_SCOPE();

    auto* pUI = new UI(createInfo);
    if (!pUI)
    {
        return { Result::RuntimeError, "Failed to allocate UI instance" };
    }

    Result initResult = pUI->initialize(createInfo);
    if (!initResult.success())
    {
        delete pUI;
        return { initResult.getCode(), initResult.toString() };
    }

    return pUI;
}

void UI::Destroy(UI* pUI)
{
    APH_ASSERT(pUI);

    APH_PROFILER_SCOPE();

    pUI->shutdown();
    delete pUI;
}

UI::UI(const UICreateInfo& createInfo)
    : m_createInfo(createInfo)
    , m_breadcrumbTracker(createInfo.breadcrumbsEnabled, "UI Rendering")
{
    // Initialize DPI scaling properties
    if (createInfo.pWindow && createInfo.pWindow->isHighDPIEnabled())
    {
        m_highDPIEnabled = true;
        m_dpiScale       = createInfo.pWindow->getDPIScale();
        UI_LOG_INFO("High DPI enabled in UI initialization: scale=%.2f", m_dpiScale);
    }
    else
    {
        m_highDPIEnabled = false;
        m_dpiScale       = 1.0f;
    }
}

auto UI::initialize(const UICreateInfo& createInfo) -> Result
{
    APH_PROFILER_SCOPE();

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
            return { Result::RuntimeError, "No window provided for UI initialization" };
        }

        m_window = m_createInfo.pWindow;

        IMGUI_CHECKVERSION();
        m_context = ImGui::CreateContext();
        if (!m_context)
        {
            return { Result::RuntimeError, "Failed to create ImGui context" };
        }

        ImGuiIO& io = ImGui::GetIO();
        if (!m_createInfo.configFile.empty())
        {
            io.IniFilename = m_createInfo.configFile.c_str();
        }

        if (m_createInfo.flags & UIFlagBits::Docking)
        {
            io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        }

        if (m_createInfo.flags & UIFlagBits::ViewportEnable)
        {
            io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
        }

        // Set up high DPI support
        if (m_highDPIEnabled)
        {
            // Configure ImGui for high DPI
            UI_LOG_INFO("Configuring ImGui for high DPI with scale factor: %.2f", m_dpiScale);

            // Set DisplayFramebufferScale to 1.0 - we'll handle scaling ourselves
            // This ensures ImGui doesn't try to do its own scaling
            io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
        }

        ImGui::StyleColorsDark();
        ImGuiStyle& style = ImGui::GetStyle();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            style.WindowRounding              = 0.0f;
            style.Colors[ImGuiCol_WindowBg].w = 1.0f;
        }

        // Scale style for high DPI if needed
        if (m_highDPIEnabled && m_dpiScale > 1.0f)
        {
            // Scale ImGui style properties that are in screen-space pixels
            UI_LOG_INFO("Scaling ImGui style for high DPI (scale=%.2f)", m_dpiScale);
            style.ScaleAllSizes(m_dpiScale);
        }
    }

    // Initialize platform backend
    {
        APH_PROFILER_SCOPE_NAME("Init Platform Backend");
        if (!m_window || !ImGui_ImplSDL3_InitForVulkan(static_cast<SDL_Window*>(m_window->getNativeHandle())))
        {
            UI_LOG_ERR("Failed to init ImGui SDL backend");
            return { Result::RuntimeError, "Failed to initialize ImGui SDL backend" };
        }

        UI_LOG_INFO("ImGui SDL backend initialized");
    }

    // Initialize renderer backend
    {
        APH_PROFILER_SCOPE_NAME("Init Renderer Backend");

        m_device        = m_createInfo.pDevice;
        m_instance      = m_createInfo.pInstance;
        m_swapchain     = m_createInfo.pSwapchain;
        m_graphicsQueue = m_device->getQueue(QueueType::Graphics);

        auto checkResult = [](VkResult err)
        {
            if (err == 0)
                return;
            UI_LOG_ERR("Vulkan error: VkResult = %d", err);
            if (err < 0)
                std::abort();
        };

        ImGui_ImplVulkan_LoadFunctions(
            [](const char* function_name, void* vulkan_instance)
            {
                return VULKAN_HPP_DEFAULT_DISPATCHER.vkGetInstanceProcAddr(
                    *(reinterpret_cast<VkInstance*>(vulkan_instance)), function_name);
            },
            &m_instance->getHandle());

        auto format = static_cast<VkFormat>(vk::utils::VkCast(m_swapchain->getFormat()));
        ImGui_ImplVulkan_InitInfo initInfo{
            .Instance                    = m_instance->getHandle(),
            .PhysicalDevice              = m_device->getPhysicalDevice()->getHandle(),
            .Device                      = m_device->getHandle(),
            .QueueFamily                 = m_graphicsQueue->getFamilyIndex(),
            .Queue                       = m_graphicsQueue->getHandle(),
            .MinImageCount               = m_swapchain->getCreateInfo().imageCount,
            .ImageCount                  = m_swapchain->getCreateInfo().imageCount,
            .MSAASamples                 = VK_SAMPLE_COUNT_1_BIT,
            .DescriptorPoolSize          = 512,
            .UseDynamicRendering         = true,
            .PipelineRenderingCreateInfo = { .sType                = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
                                            .colorAttachmentCount = 1,
                                            .pColorAttachmentFormats = &format,
                                            .depthAttachmentFormat   = VK_FORMAT_D32_SFLOAT },
            .Allocator                   = vk::vkAllocator(),
            .CheckVkResultFn             = checkResult,
        };

        if (!ImGui_ImplVulkan_Init(&initInfo))
        {
            UI_LOG_ERR("Failed to init ImGui Vulkan backend");
            return { Result::RuntimeError, "Failed to initialize ImGui Vulkan backend" };
        }

        UI_LOG_INFO("ImGui Vulkan backend initialized");
    }

    // Load default font with appropriate scaling
    if (m_highDPIEnabled)
    {
        UI_LOG_INFO("Loading default font with high DPI scaling");
    }
    addFont("font://Roboto-Medium.ttf", 18.0f);

    m_breadcrumbTracker.setEnabled(createInfo.breadcrumbsEnabled);

    return Result::Success;
}

void UI::shutdown()
{
    APH_PROFILER_SCOPE();

    if (!m_context)
    {
        return;
    }

    for (auto it = m_containers.begin(); it != m_containers.end();)
    {
        auto* container = *it;
        if (container && container->getType() == ContainerType::Window)
        {
            auto* window = static_cast<WidgetWindow*>(container);
            it           = m_containers.erase(it);
            m_windowPool.free(window);
        }
        else
        {
            ++it;
        }
    }

    clearContainers();

    {
        APH_PROFILER_SCOPE_NAME("Shutdown Vulkan Backend");
        ImGui_ImplVulkan_Shutdown();
    }

    {
        APH_PROFILER_SCOPE_NAME("Shutdown SDL Backend");
        ImGui_ImplSDL3_Shutdown();
    }

    {
        APH_PROFILER_SCOPE_NAME("Destroy ImGui Context");
        ImGui::DestroyContext(m_context);
        m_context = nullptr;
    }

    m_fonts.clear();
    m_device        = nullptr;
    m_instance      = nullptr;
    m_graphicsQueue = nullptr;
    m_swapchain     = nullptr;
    m_window        = nullptr;

    m_breadcrumbTracker.clear();

    UI_LOG_INFO("UI system shutdown");
}

void UI::beginFrame()
{
    APH_PROFILER_SCOPE();
    if (!m_context)
    {
        return;
    }

    // Check for DPI changes at the beginning of each frame
    if (m_highDPIEnabled && m_window)
    {
        float newScale = m_window->getDPIScale();
        if (std::abs(newScale - m_dpiScale) > 0.01f)
        {
            UI_LOG_INFO("DPI scale changed (detected in beginFrame): %.2f -> %.2f", m_dpiScale, newScale);
            onDPIChange(); // Handle the DPI change
        }
    }

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
}

void UI::endFrame()
{
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

    if (m_breadcrumbTracker.isEnabled())
    {
        m_breadcrumbTracker.clear();
    }

    uint32_t renderCrumb = addBreadcrumb("Render", "Starting render process");
    uint32_t frameCrumb  = addBreadcrumb("BeginFrame", "Starting new UI frame", renderCrumb);
    beginFrame();

    if (m_updateCallback)
    {
        addBreadcrumb("UpdateCallback", "Executing user update callback", frameCrumb);
        m_updateCallback();
    }

    uint32_t containerUpdateCrumb = addBreadcrumb("ContainerUpdate", "Beginning container updates", frameCrumb);

    for (auto container : m_containers)
    {
        if (container)
        {
            std::string containerInfo = "Unknown";

            if (container->getType() == ContainerType::Window)
            {
                auto* window  = static_cast<WidgetWindow*>(container);
                containerInfo = window->getTitle();

                bool isLast          = (container == m_containers.back());
                uint32_t windowCrumb = addBreadcrumb("DrawWindow", containerInfo, containerUpdateCrumb, isLast);

                container->m_breadcrumbId = windowCrumb;
                window->draw();
            }
            else
            {
                bool isLast = (container == m_containers.back());
                uint32_t containerCrumb =
                    addBreadcrumb("Draw" + ToString(container->getType()), containerInfo, containerUpdateCrumb, isLast);

                container->m_breadcrumbId = containerCrumb;
                container->drawAll();
            }
        }
    }

    ImGui::Render();
    {
        ImDrawData* drawData = ImGui::GetDrawData();
        APH_ASSERT(drawData);

        pCmd->beginDebugLabel({
            .name = "Drawing UI", .color = { 0.4f, 0.3f, 0.2f, 1.0f }
        });

        ImGui_ImplVulkan_RenderDrawData(drawData, pCmd->getHandle());
        pCmd->endDebugLabel();
    }

    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        addBreadcrumb("ViewportRender", "Updating platform windows", frameCrumb);
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }

    addBreadcrumb("EndFrame", "Finishing UI frame", renderCrumb, true);
    endFrame();

    if (m_breadcrumbTracker.isEnabled())
    {
        UI_LOG_INFO("%s", m_breadcrumbTracker.toString().c_str());
    }
}

void UI::setUpdateCallback(UIUpdateCallback&& callback)
{
    m_updateCallback = std::move(callback);
}

auto UI::addFont(const std::string& fontPath, float fontSize) -> uint32_t
{
    APH_PROFILER_SCOPE_NAME("Add Font");

    if (!m_context)
    {
        UI_LOG_ERR("Cannot add font: UI not initialized");
        return 0;
    }

    auto& filesystem         = APH_DEFAULT_FILESYSTEM;
    std::string resolvedPath = filesystem.resolvePath(fontPath);

    // Scale the font size according to DPI if high DPI is enabled
    float scaledFontSize = m_highDPIEnabled ? fontSize * m_dpiScale : fontSize;

    UI_LOG_INFO("Adding font '%s' at size %.1f (DPI scale: %.2f, scaled size: %.1f)", fontPath.c_str(), fontSize,
                m_dpiScale, scaledFontSize);

    ImGuiIO& io  = ImGui::GetIO();
    ImFont* font = io.Fonts->AddFontFromFileTTF(resolvedPath.c_str(), scaledFontSize);
    io.Fonts->Build();

    if (font)
    {
        m_fonts.push_back(font);

        ImGui_ImplVulkan_DestroyFontsTexture();
        if (!ImGui_ImplVulkan_CreateFontsTexture())
        {
            UI_LOG_ERR("Failed to create ImGui font textures");
            return 0;
        }

        return m_fonts.size() - 1;
    }

    return 0;
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

    m_activeFontIndex          = fontIndex;
    ImGui::GetIO().FontDefault = m_fonts[fontIndex];
}

auto UI::createWindow(const std::string& title) -> Expected<WidgetWindow*>
{
    APH_PROFILER_SCOPE();

    if (!m_context)
    {
        return { Result::RuntimeError, "Cannot create window: UI not initialized" };
    }

    WidgetWindow* window = m_windowPool.allocate(this);
    if (!window)
    {
        return { Result::RuntimeError, "Failed to allocate widget window from pool" };
    }

    window->setTitle(title);
    registerContainer(window);

    return window;
}

void UI::registerContainer(WidgetContainer* container)
{
    if (container != nullptr)
    {
        m_containers.push_back(container);
    }
}

void UI::unregisterContainer(WidgetContainer* container)
{
    auto it = std::ranges::find(m_containers, container);
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

    unregisterContainer(window);
    m_windowPool.free(window);
}

void UI::destroyWidget(Widget* widget)
{
    if (!widget)
    {
        return;
    }

    APH_PROFILER_SCOPE();

    m_widgetPool.free(widget);
}

auto UI::addBreadcrumb(const std::string& name, const std::string& details, uint32_t parentIndex, bool isLeafNode)
    -> uint32_t
{
    APH_PROFILER_SCOPE();
    return m_breadcrumbTracker.addBreadcrumb(name, details, parentIndex, isLeafNode);
}

void UI::enableBreadcrumbs(bool enable)
{
    m_breadcrumbTracker.setEnabled(enable);
}

UI::~UI()
{
    m_windowPool.clear();
    m_widgetPool.clear();
    m_containers.clear();
}

float UI::getDPIScale() const
{
    return m_dpiScale;
}

void UI::onDPIChange()
{
    APH_PROFILER_SCOPE();

    if (!m_window || !m_highDPIEnabled || !m_context)
    {
        return;
    }

    // Get the new DPI scale
    float newScale = m_window->getDPIScale();
    if (std::abs(newScale - m_dpiScale) <= 0.01f)
    {
        // No significant change
        return;
    }

    UI_LOG_INFO("UI handling DPI change: %.2f -> %.2f", m_dpiScale, newScale);

    // Calculate scale ratio for adjustment
    float scaleRatio = newScale / m_dpiScale;

    // Update stored scale
    m_dpiScale = newScale;

    // Scale ImGui style
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(scaleRatio);

    // Recreate fonts at the new scale
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->Clear();

    // For now we'll just recreate the default font
    // In a more complete implementation, you would track all loaded fonts and their original sizes
    auto& filesystem            = APH_DEFAULT_FILESYSTEM;
    std::string defaultFontPath = filesystem.resolvePath("font://Roboto-Medium.ttf");
    float scaledFontSize        = 18.0f * m_dpiScale; // Scaling the default font size

    io.Fonts->AddFontFromFileTTF(defaultFontPath.c_str(), scaledFontSize);
    io.Fonts->Build();

    // Update Vulkan font texture
    ImGui_ImplVulkan_DestroyFontsTexture();
    ImGui_ImplVulkan_CreateFontsTexture();

    UI_LOG_INFO("UI fonts rebuilt at DPI scale %.2f", m_dpiScale);
}

} // namespace aph
