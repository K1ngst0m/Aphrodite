//
// Created by npchitman on 6/1/21.
//

#include "LinuxInput.h"

#include <GLFW/glfw3.h>

#include "Hazel/Core/Application.h"
#include "hzpch.h"

namespace Hazel {
    Scope<Input> Input::s_Instance = CreateScope<LinuxInput>();

    bool LinuxInput::IsKeyPressedImpl(KeyCode keycode) {
        auto window = static_cast<GLFWwindow *>(
                Application::Get().GetWindow().GetNativeWindow());
        auto state = glfwGetKey(window, static_cast<int32_t>(keycode));
        return state == GLFW_PRESS || state == GLFW_REPEAT;
    }

    bool LinuxInput::IsMouseButtonPressedImpl(MouseCode button) {
        auto window = static_cast<GLFWwindow *>(
                Application::Get().GetWindow().GetNativeWindow());
        auto state = glfwGetMouseButton(window, static_cast<int32_t>(button));
        return state == GLFW_PRESS;
    }

    std::pair<float, float> LinuxInput::GetMousePositionImpl() {
        auto window = static_cast<GLFWwindow *>(
                Application::Get().GetWindow().GetNativeWindow());
        double xPos, yPos;
        glfwGetCursorPos(window, &xPos, &yPos);

        return {static_cast<float>(xPos), static_cast<float>(yPos)};
    }

    float LinuxInput::GetMouseXImpl() {
        auto [x, y] = GetMousePositionImpl();
        return x;
    }

    float LinuxInput::GetMouseYImpl() {
        auto [x, y] = GetMousePositionImpl();
        return y;
    }
}
