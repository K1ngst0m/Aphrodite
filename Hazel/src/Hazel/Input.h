//
// Created by npchitman on 5/31/21.
//

#ifndef HAZEL_ENGINE_INPUT_H
#define HAZEL_ENGINE_INPUT_H

#include "Hazel/Core.h"

namespace Hazel {
    class Input {
    public:
        inline static bool IsKeyPressed(int keycode) {
            return s_Instance->IsKeyPressedImpl(keycode);
        }

        inline static bool IsMouseButtonPressed(int button) {
            return s_Instance->IsMouseButtonPressedImpl(button);
        }

        inline static std::pair<float, float> GetMousePosition() {
            return s_Instance->GetMousePositionImpl();
        }

        inline static float GetMouseX() { return s_Instance->GetMouseXImpl(); }

        inline static float GetMouseY() { return s_Instance->GetMouseYImpl(); }

    protected:
        virtual bool IsKeyPressedImpl(int keycode) = 0;

        virtual bool IsMouseButtonPressedImpl(int button) = 0;

        virtual std::pair<float, float> GetMousePositionImpl() = 0;

        virtual float GetMouseXImpl() = 0;

        virtual float GetMouseYImpl() = 0;

    private:
        static Input *s_Instance;
    };
}// namespace Hazel

#endif// HAZEL_ENGINE_INPUT_H
