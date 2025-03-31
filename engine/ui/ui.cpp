#include "imgui_backend.h"
#include "ui.h"

#include "common/common.h"

namespace aph
{

UI::UI(const UICreateInfo& createInfo)
    : m_createInfo(createInfo)
{
}

UI::~UI()
{
    shutdown();
}

bool UI::initialize()
{
    // Already initialized
    if (m_backend)
    {
        return true;
    }

    // Create and initialize backend
    m_backend = createBackend();
    if (!m_backend || !m_backend->initialize(m_createInfo))
    {
        m_backend.reset();
        return false;
    }

    return true;
}

void UI::shutdown()
{
    if (m_backend)
    {
        m_backend->shutdown();
        m_backend.reset();
    }
}

void UI::beginFrame()
{
    m_backend->newFrame();
}

void UI::endFrame()
{
    if (m_updateCallback)
    {
        m_updateCallback();
    }
}

void UI::render(vk::CommandBuffer* pCmd)
{
    APH_ASSERT(pCmd);
    m_backend->render(pCmd);
}

void UI::setUpdateCallback(UIUpdateCallback&& callback)
{
    m_updateCallback = std::move(callback);
}

uint32_t UI::addFont(const std::string& fontPath, float fontSize)
{
    ImGuiBackend* imguiBackend = static_cast<ImGuiBackend*>(m_backend.get());
    return imguiBackend->addFont(fontPath, fontSize);
}

void UI::setActiveFont(uint32_t fontIndex)
{
    ImGuiBackend* imguiBackend = static_cast<ImGuiBackend*>(m_backend.get());
    imguiBackend->setActiveFont(fontIndex);
}

std::unique_ptr<UIBackend> UI::createBackend()
{
    return std::make_unique<ImGuiBackend>();
}

std::unique_ptr<UI> createUI(const UICreateInfo& createInfo)
{
    auto ui = std::make_unique<UI>(createInfo);
    if (!ui->initialize())
    {
        UI_LOG_ERR("Failed to initialize UI system");
        APH_ASSERT(false);
        return {};
    }
    return ui;
}

} // namespace aph
