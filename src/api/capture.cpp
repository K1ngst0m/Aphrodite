#include "capture.h"
#include "renderdoc_app.h"

namespace aph
{
static RENDERDOC_API_1_6_0* rdcDispatchTable = {};

// Helper function to convert from aph::Key to RENDERDOC_InputButton
static RENDERDOC_InputButton ConvertToRenderDocKey(Key key)
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

Result DeviceCapture::initialize()
{
    m_renderdocModule.open("librenderdoc.so");
    if (!m_renderdocModule)
    {
        return {Result::RuntimeError, "Failed to load RenderDoc module."};
    }

    pRENDERDOC_GetAPI getAPI = m_renderdocModule.getSymbol<pRENDERDOC_GetAPI>("RENDERDOC_GetAPI");
    if (!getAPI)
    {
        return {Result::RuntimeError, "Failed to get module symbol."};
    }

    if (!getAPI(eRENDERDOC_API_Version_1_6_0, (void**)&rdcDispatchTable))
    {
        return {Result::RuntimeError, "Failed to get dispatch table."};
    }

    return Result::Success;
}

void DeviceCapture::beginCapture()
{
    if (rdcDispatchTable)
    {
        rdcDispatchTable->StartFrameCapture({}, {});
    }
}

void DeviceCapture::endCapture()
{
    if (rdcDispatchTable)
    {
        rdcDispatchTable->EndFrameCapture({}, {});
    }
}

void DeviceCapture::triggerCapture()
{
    if (rdcDispatchTable)
    {
        rdcDispatchTable->TriggerCapture();
    }
}

void DeviceCapture::triggerMultiFrameCapture(uint32_t numFrames)
{
    if (rdcDispatchTable)
    {
        rdcDispatchTable->TriggerMultiFrameCapture(numFrames);
    }
}

void DeviceCapture::setCaptureTitle(const char* title)
{
    if (rdcDispatchTable)
    {
        rdcDispatchTable->SetCaptureTitle(title);
    }
}

bool DeviceCapture::discardCapture()
{
    if (rdcDispatchTable)
    {
        return rdcDispatchTable->DiscardFrameCapture({}, {}) != 0;
    }
    return false;
}

bool DeviceCapture::setCaptureOption(CaptureOption option, uint32_t value)
{
    if (rdcDispatchTable)
    {
        return rdcDispatchTable->SetCaptureOptionU32(static_cast<RENDERDOC_CaptureOption>(option), value) != 0;
    }
    return false;
}

bool DeviceCapture::setCaptureOptionFloat(CaptureOption option, float value)
{
    if (rdcDispatchTable)
    {
        return rdcDispatchTable->SetCaptureOptionF32(static_cast<RENDERDOC_CaptureOption>(option), value) != 0;
    }
    return false;
}

uint32_t DeviceCapture::getCaptureOption(CaptureOption option) const
{
    if (rdcDispatchTable)
    {
        return rdcDispatchTable->GetCaptureOptionU32(static_cast<RENDERDOC_CaptureOption>(option));
    }
    return 0xFFFFFFFF;
}

float DeviceCapture::getCaptureOptionFloat(CaptureOption option) const
{
    if (rdcDispatchTable)
    {
        return rdcDispatchTable->GetCaptureOptionF32(static_cast<RENDERDOC_CaptureOption>(option));
    }
    return -std::numeric_limits<float>::max();
}

uint32_t DeviceCapture::getOverlayBits() const
{
    if (rdcDispatchTable)
    {
        return rdcDispatchTable->GetOverlayBits();
    }
    return 0;
}

void DeviceCapture::maskOverlayBits(uint32_t andMask, uint32_t orMask)
{
    if (rdcDispatchTable)
    {
        rdcDispatchTable->MaskOverlayBits(andMask, orMask);
    }
}

void DeviceCapture::setOverlayBits(OverlayBits bits)
{
    if (rdcDispatchTable)
    {
        uint32_t value = static_cast<uint32_t>(bits);
        rdcDispatchTable->MaskOverlayBits(0, value);
    }
}

void DeviceCapture::enableOverlay(bool enable)
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

void DeviceCapture::setCaptureFilePath(const char* pathTemplate)
{
    if (rdcDispatchTable)
    {
        rdcDispatchTable->SetCaptureFilePathTemplate(pathTemplate);
    }
}

const char* DeviceCapture::getCaptureFilePath() const
{
    if (rdcDispatchTable)
    {
        return rdcDispatchTable->GetCaptureFilePathTemplate();
    }
    return nullptr;
}

void DeviceCapture::setCaptureComments(const char* comments)
{
    if (rdcDispatchTable)
    {
        rdcDispatchTable->SetCaptureFileComments(nullptr, comments);
    }
}

void DeviceCapture::setCaptureKeys(Key* keys, int numKeys)
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

uint32_t DeviceCapture::launchReplayUI(bool connectToApp, const char* cmdLine)
{
    if (rdcDispatchTable)
    {
        return rdcDispatchTable->LaunchReplayUI(connectToApp ? 1 : 0, cmdLine);
    }
    return 0;
}

bool DeviceCapture::showReplayUI()
{
    if (rdcDispatchTable)
    {
        return rdcDispatchTable->ShowReplayUI() != 0;
    }
    return false;
}

bool DeviceCapture::isTargetControlConnected() const
{
    if (rdcDispatchTable)
    {
        return rdcDispatchTable->IsTargetControlConnected() != 0;
    }
    return false;
}

bool DeviceCapture::isAvailable() const
{
    return rdcDispatchTable != nullptr;
}

bool DeviceCapture::isCapturing() const
{
    if (rdcDispatchTable)
    {
        return rdcDispatchTable->IsFrameCapturing() != 0;
    }
    return false;
}
} // namespace aph
