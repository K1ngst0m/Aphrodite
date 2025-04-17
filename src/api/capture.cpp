#include "capture.h"
#include "common/profiler.h"
#include "renderdoc_app.h"

namespace aph
{
static RENDERDOC_API_1_6_0* rdcDispatchTable = {};

// Helper function to convert from aph::Key to RENDERDOC_InputButton
static auto ConvertToRenderDocKey(Key key) -> RENDERDOC_InputButton
{
    // ASCII values for keys 0-9 and A-Z match between our Key enum and RenderDoc
    if ((key >= Key::A && key <= Key::Z) || (key >= Key::_0 && key <= Key::_9))
    {
        return static_cast<RENDERDOC_InputButton>(key);
    }

    // Map special keys to RenderDoc values
    switch (key)
    {
    case Key::Return:
        return static_cast<RENDERDOC_InputButton>(0x0D); // Enter/Return
    case Key::Escape:
        return static_cast<RENDERDOC_InputButton>(0x1B); // Escape
    case Key::Left:
        return eRENDERDOC_Key_Home;
    case Key::Right:
        return eRENDERDOC_Key_End;
    case Key::Up:
        return eRENDERDOC_Key_PageUp;
    case Key::Down:
        return eRENDERDOC_Key_PageDn;
    case Key::Space:
        return static_cast<RENDERDOC_InputButton>(0x20); // Space
    // Add more key mappings as needed
    default:
        return static_cast<RENDERDOC_InputButton>(0); // Unknown key
    }
}

auto DeviceCapture::Create() -> Expected<DeviceCapture*>
{
    APH_PROFILER_SCOPE();

    // Create the DeviceCapture with minimal initialization
    auto* pCapture = new DeviceCapture();
    if (!pCapture)
    {
        return { Result::RuntimeError, "Failed to allocate DeviceCapture instance" };
    }

    // Complete the initialization process
    Result initResult = pCapture->initialize();
    if (!initResult.success())
    {
        delete pCapture;
        return { initResult.getCode(), initResult.toString() };
    }

    return pCapture;
}

auto DeviceCapture::Destroy(DeviceCapture* pCapture) -> void
{
    if (pCapture)
    {
        delete pCapture;
    }
}

auto DeviceCapture::initialize() -> Result
{
    auto result = m_renderdocModule.open("librenderdoc.so");
    if (!result)
    {
        return result;
    }

    auto getAPI = m_renderdocModule.getSymbol<pRENDERDOC_GetAPI>("RENDERDOC_GetAPI");
    if (!getAPI)
    {
        return { Result::RuntimeError, "Failed to get module symbol." };
    }

    if (!getAPI(eRENDERDOC_API_Version_1_6_0, reinterpret_cast<void**>(&rdcDispatchTable)))
    {
        return { Result::RuntimeError, "Failed to get dispatch table." };
    }

    return Result::Success;
}

auto DeviceCapture::beginCapture() -> void
{
    if (rdcDispatchTable)
    {
        rdcDispatchTable->StartFrameCapture({}, {});
    }
}

auto DeviceCapture::endCapture() -> void
{
    if (rdcDispatchTable)
    {
        rdcDispatchTable->EndFrameCapture({}, {});
    }
}

auto DeviceCapture::triggerCapture() -> void
{
    if (rdcDispatchTable)
    {
        rdcDispatchTable->TriggerCapture();
    }
}

auto DeviceCapture::triggerMultiFrameCapture(uint32_t numFrames) -> void
{
    if (rdcDispatchTable)
    {
        rdcDispatchTable->TriggerMultiFrameCapture(numFrames);
    }
}

auto DeviceCapture::setCaptureTitle(const char* title) -> void
{
    if (rdcDispatchTable)
    {
        rdcDispatchTable->SetCaptureTitle(title);
    }
}

auto DeviceCapture::discardCapture() -> bool
{
    if (rdcDispatchTable)
    {
        return rdcDispatchTable->DiscardFrameCapture({}, {}) != 0;
    }
    return false;
}

auto DeviceCapture::setCaptureOption(CaptureOption option, uint32_t value) -> bool
{
    if (rdcDispatchTable)
    {
        return rdcDispatchTable->SetCaptureOptionU32(static_cast<RENDERDOC_CaptureOption>(option), value) != 0;
    }
    return false;
}

auto DeviceCapture::setCaptureOptionFloat(CaptureOption option, float value) -> bool
{
    if (rdcDispatchTable)
    {
        return rdcDispatchTable->SetCaptureOptionF32(static_cast<RENDERDOC_CaptureOption>(option), value) != 0;
    }
    return false;
}

auto DeviceCapture::getCaptureOption(CaptureOption option) const -> uint32_t
{
    if (rdcDispatchTable)
    {
        return rdcDispatchTable->GetCaptureOptionU32(static_cast<RENDERDOC_CaptureOption>(option));
    }
    return 0xFFFFFFFF;
}

auto DeviceCapture::getCaptureOptionFloat(CaptureOption option) const -> float
{
    if (rdcDispatchTable)
    {
        return rdcDispatchTable->GetCaptureOptionF32(static_cast<RENDERDOC_CaptureOption>(option));
    }
    return -std::numeric_limits<float>::max();
}

auto DeviceCapture::getOverlayBits() const -> uint32_t
{
    if (rdcDispatchTable)
    {
        return rdcDispatchTable->GetOverlayBits();
    }
    return 0;
}

auto DeviceCapture::maskOverlayBits(uint32_t andMask, uint32_t orMask) -> void
{
    if (rdcDispatchTable)
    {
        rdcDispatchTable->MaskOverlayBits(andMask, orMask);
    }
}

auto DeviceCapture::setOverlayBits(OverlayBits bits) -> void
{
    if (rdcDispatchTable)
    {
        uint32_t value = static_cast<uint32_t>(bits);
        rdcDispatchTable->MaskOverlayBits(0, value);
    }
}

auto DeviceCapture::enableOverlay(bool enable) -> void
{
    if (rdcDispatchTable)
    {
        if (enable)
        {
            rdcDispatchTable->MaskOverlayBits(~0U, eRENDERDOC_Overlay_Enabled);
        }
        else
        {
            rdcDispatchTable->MaskOverlayBits(~eRENDERDOC_Overlay_Enabled, 0);
        }
    }
}

auto DeviceCapture::setCaptureFilePath(const char* pathTemplate) -> void
{
    if (rdcDispatchTable)
    {
        rdcDispatchTable->SetCaptureFilePathTemplate(pathTemplate);
    }
}

auto DeviceCapture::getCaptureFilePath() const -> const char*
{
    if (rdcDispatchTable)
    {
        return rdcDispatchTable->GetCaptureFilePathTemplate();
    }
    return nullptr;
}

auto DeviceCapture::setCaptureComments(const char* comments) -> void
{
    if (rdcDispatchTable)
    {
        rdcDispatchTable->SetCaptureFileComments(nullptr, comments);
    }
}

auto DeviceCapture::setCaptureKeys(Key* keys, int numKeys) -> void
{
    if (rdcDispatchTable && keys && numKeys > 0)
    {
        // Convert our Key enum to RenderDoc's RENDERDOC_InputButton
        auto* rdcKeys = new RENDERDOC_InputButton[numKeys];
        for (int i = 0; i < numKeys; ++i)
        {
            rdcKeys[i] = ConvertToRenderDocKey(keys[i]);
        }

        rdcDispatchTable->SetCaptureKeys(rdcKeys, numKeys);
        delete[] rdcKeys;
    }
    else if (rdcDispatchTable)
    {
        // Disable capture keys
        rdcDispatchTable->SetCaptureKeys(nullptr, 0);
    }
}

auto DeviceCapture::launchReplayUI(bool connectToApp, const char* cmdLine) -> uint32_t
{
    if (rdcDispatchTable)
    {
        return rdcDispatchTable->LaunchReplayUI(connectToApp ? 1 : 0, cmdLine);
    }
    return 0;
}

auto DeviceCapture::showReplayUI() -> bool
{
    if (rdcDispatchTable)
    {
        return rdcDispatchTable->ShowReplayUI() != 0;
    }
    return false;
}

auto DeviceCapture::isTargetControlConnected() const -> bool
{
    if (rdcDispatchTable)
    {
        return rdcDispatchTable->IsTargetControlConnected() != 0;
    }
    return false;
}

auto DeviceCapture::isAvailable() const -> bool
{
    return rdcDispatchTable != nullptr;
}

auto DeviceCapture::isCapturing() const -> bool
{
    if (rdcDispatchTable)
    {
        return rdcDispatchTable->IsFrameCapturing() != 0;
    }
    return false;
}
} // namespace aph
