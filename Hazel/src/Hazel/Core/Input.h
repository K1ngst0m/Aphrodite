// Input.h

// Check Keyboard, Mouse Input


#ifndef HAZEL_ENGINE_INPUT_H
#define HAZEL_ENGINE_INPUT_H

#include <glm/glm.hpp>
#include "Hazel/Core/KeyCodes.h"
#include "Hazel/Core/MouseCodes.h"

namespace Hazel {
    class Input {
    public:
        static bool IsKeyPressed(KeyCode key);
        static bool IsMouseButtonPressed(MouseCode button);
        static glm::vec2 GetMousePosition();
        static float GetMouseX();
        static float GetMouseY();
    private:
        static Scope<Input> s_Instance;
    };
}// namespace Hazel

#endif// HAZEL_ENGINE_INPUT_H
