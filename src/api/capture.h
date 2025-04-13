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
    AllowVSync                       = 0,
    AllowFullscreen                  = 1,
    APIValidation                    = 2,
    CaptureCallstacks                = 3,
    CaptureCallstacksOnlyActions     = 4,
    DelayForDebugger                 = 5,
    VerifyBufferAccess               = 6,
    HookIntoChildren                 = 7,
    RefAllResources                  = 8,
    SaveAllInitials                  = 9,
    CaptureAllCmdLists               = 10,
    DebugOutputMute                  = 11,
    AllowUnsupportedVendorExtensions = 12,
    SoftMemoryLimit                  = 13
};

enum class OverlayBits : uint32_t
{
    Enabled     = 0x1,
    FrameRate   = 0x2,
    FrameNumber = 0x4,
    CaptureList = 0x8,
    Default     = 0xF, // Enabled | FrameRate | FrameNumber | CaptureList
    All         = ~0U,
    None        = 0
};

class DeviceCapture final
{
private:
    DeviceCapture()  = default;
    ~DeviceCapture() = default;
    auto initialize() -> Result;

public:
    // Factory methods
    static auto Create() -> Expected<DeviceCapture*>;
    static auto Destroy(DeviceCapture* pCapture) -> void;

    auto beginCapture() -> void;
    auto endCapture() -> void;
    auto triggerCapture() -> void;
    auto triggerMultiFrameCapture(uint32_t numFrames) -> void;
    auto setCaptureTitle(const char* title) -> void;
    auto discardCapture() -> bool;

    // Capture options
    auto setCaptureOption(CaptureOption option, uint32_t value) -> bool;
    auto setCaptureOptionFloat(CaptureOption option, float value) -> bool;
    auto getCaptureOption(CaptureOption option) const -> uint32_t;
    auto getCaptureOptionFloat(CaptureOption option) const -> float;

    // Overlay control
    auto getOverlayBits() const -> uint32_t;
    auto maskOverlayBits(uint32_t andMask, uint32_t orMask) -> void;
    auto setOverlayBits(OverlayBits bits) -> void;
    auto enableOverlay(bool enable = true) -> void;

    // Capture path configuration
    auto setCaptureFilePath(const char* pathTemplate) -> void;
    auto getCaptureFilePath() const -> const char*;

    // Capture file comments
    auto setCaptureComments(const char* comments) -> void;

    // Capture key configuration
    auto setCaptureKeys(Key* keys, int numKeys) -> void;

    // UI control
    auto launchReplayUI(bool connectToApp = true, const char* cmdLine = nullptr) -> uint32_t;
    auto showReplayUI() -> bool;
    auto isTargetControlConnected() const -> bool;

    auto isAvailable() const -> bool;
    auto isCapturing() const -> bool;

private:
    Module m_renderdocModule{};
};
} // namespace aph
