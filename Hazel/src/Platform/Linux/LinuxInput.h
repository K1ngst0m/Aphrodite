//
// Created by npchitman on 6/1/21.
//

#ifndef HAZEL_ENGINE_LINUXINPUT_H
#define HAZEL_ENGINE_LINUXINPUT_H

#include "Hazel/Core/Input.h"

namespace Hazel {
    class LinuxInput : public Input {
    protected:
        bool IsKeyPressedImpl(KeyCode keycode) override;

        bool IsMouseButtonPressedImpl(MouseCode button) override;

        std::pair<float, float> GetMousePositionImpl() override;

        float GetMouseXImpl() override;

        float GetMouseYImpl() override;
    };
}// namespace Hazel

#endif// HAZEL_ENGINE_LINUXINPUT_H
