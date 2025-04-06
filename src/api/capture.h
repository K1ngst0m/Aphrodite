#pragma once

#include "common/common.h"
#include "common/result.h"
#include "exception/errorMacros.h"
#include "input/input.h"
#include "module/module.h"

namespace aph
{
// These enums match the RenderDoc API enums for easier use
enum class CaptureOption : uint32_t
{
    AllowVSync = 0,
    AllowFullscreen = 1,
    APIValidation = 2,
    CaptureCallstacks = 3,
    CaptureCallstacksOnlyActions = 4,
    DelayForDebugger = 5,
    VerifyBufferAccess = 6,
    HookIntoChildren = 7,
    RefAllResources = 8,
    SaveAllInitials = 9,
    CaptureAllCmdLists = 10,
    DebugOutputMute = 11,
    AllowUnsupportedVendorExtensions = 12,
    SoftMemoryLimit = 13
};

enum class OverlayBits : uint32_t
{
    Enabled = 0x1,
    FrameRate = 0x2,
    FrameNumber = 0x4,
    CaptureList = 0x8,
    Default = 0xF, // Enabled | FrameRate | FrameNumber | CaptureList
    All = ~0U,
    None = 0
};

class DeviceCapture final
{
private:
    DeviceCapture() = default;
    ~DeviceCapture() = default;
    Result initialize();

public:
    // Factory methods
    static Expected<DeviceCapture*> Create();
    static void Destroy(DeviceCapture* pCapture);

    void beginCapture();
    void endCapture();
    void triggerCapture();
    void triggerMultiFrameCapture(uint32_t numFrames);
    void setCaptureTitle(const char* title);
    bool discardCapture();

    // Capture options
    bool setCaptureOption(CaptureOption option, uint32_t value);
    bool setCaptureOptionFloat(CaptureOption option, float value);
    uint32_t getCaptureOption(CaptureOption option) const;
    float getCaptureOptionFloat(CaptureOption option) const;

    // Overlay control
    uint32_t getOverlayBits() const;
    void maskOverlayBits(uint32_t andMask, uint32_t orMask);
    void setOverlayBits(OverlayBits bits);
    void enableOverlay(bool enable = true);

    // Capture path configuration
    void setCaptureFilePath(const char* pathTemplate);
    const char* getCaptureFilePath() const;

    // Capture file comments
    void setCaptureComments(const char* comments);

    // Capture key configuration
    void setCaptureKeys(Key* keys, int numKeys);

    // UI control
    uint32_t launchReplayUI(bool connectToApp = true, const char* cmdLine = nullptr);
    bool showReplayUI();
    bool isTargetControlConnected() const;

    bool isAvailable() const;
    bool isCapturing() const;

private:
    Module m_renderdocModule{};
};
} // namespace aph
