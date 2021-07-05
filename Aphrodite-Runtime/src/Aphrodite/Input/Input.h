// Input.h

// Check Keyboard, Mouse Input


#ifndef Aphrodite_ENGINE_INPUT_H
#define Aphrodite_ENGINE_INPUT_H

#include <glm/glm.hpp>

#include "KeyCodes.h"
#include "MouseCodes.h"

namespace Aph {
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
}// namespace Aph-Runtime

#endif// Aphrodite_ENGINE_INPUT_H
