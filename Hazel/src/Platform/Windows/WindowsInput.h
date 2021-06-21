//
// Created by Npchitman on 2021/2/22.
//

#ifndef HAZELENGINE_WINDOWSINPUT_H
#define HAZELENGINE_WINDOWSINPUT_H

#include "Hazel/Input.h"

namespace Hazel {
    class WindowsInput : public Input {

    protected:
        bool IsKeyPressedImpl(int keycode) override;
        bool IsMouseButtonPressedImpl(int button) override;
        float GetMouseXImpl() override;
        float GetMouseYImpl() override;
        std::pair<float, float> GetMousePositionImpl() override;
    };
}// namespace Hazel

#endif// HAZELENGINE_WINDOWSINPUT_H
