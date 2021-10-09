//
// Created by npchitman on 6/1/21.
//

#include <Aphrodite/Input/Input.h>
#include <GLFW/glfw3.h>

#include "Aphrodite/Core/Application.h"
#include "pch.h"

namespace Aph {
    bool Input::IsKeyPressed(const KeyCode keycode) {
        auto window = static_cast<GLFWwindow *>(Application::Get().GetWindow().GetNativeWindow());
        auto state = glfwGetKey(window, static_cast<int32_t>(keycode));
        return state == GLFW_PRESS || state == GLFW_REPEAT;
    }

    bool Input::IsMouseButtonPressed(const MouseCode button) {
        auto window = static_cast<GLFWwindow *>(
                Application::Get().GetWindow().GetNativeWindow());
        auto state = glfwGetMouseButton(window, static_cast<int32_t>(button));
        return state == GLFW_PRESS;
    }

    glm::vec2 Input::GetMousePosition() {
        auto *window = static_cast<GLFWwindow *>(
                Application::Get().GetWindow().GetNativeWindow());
        double xPos, yPos;
        glfwGetCursorPos(window, &xPos, &yPos);

        return {static_cast<float>(xPos), static_cast<float>(yPos)};
    }

    float Input::GetMouseX() {
        return GetMousePosition().x;
    }

    float Input::GetMouseY() {
        return GetMousePosition().y;
    }
}// namespace Aph
