//
// Created by npchitman on 5/31/21.
//

#ifndef HAZEL_ENGINE_INPUT_H
#define HAZEL_ENGINE_INPUT_H

#include "Hazel/Core/Base.h"
#include "Hazel/Core/KeyCodes.h"
#include "Hazel/Core/MouseCodes.h"

namespace Hazel {
    class Input {
    public:
        static bool IsKeyPressed(KeyCode key);

    private:
        static Scope<Input> s_Instance;
        static bool IsMouseButtonPressed(MouseCode button);
        static std::pair<float, float> GetMousePosition();
        static float GetMouseX();
        static float GetMouseY();
    };
}// namespace Hazel

#endif// HAZEL_ENGINE_INPUT_H
