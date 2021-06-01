//
// Created by npchitman on 6/1/21.
//

#include "hzpch.h"
#include "LinuxInput.h"

#include "Hazel/Application.h"
#include <GLFW/glfw3.h>

namespace Hazel {
    Input *Input::s_Instance = new LinuxInput();
}

bool Hazel::LinuxInput::IsKeyPressedImpl(int keycode) {
    auto window = static_cast<GLFWwindow *>(Application::Get().GetWindow().GetNativeWindow());
    auto state = glfwGetKey(window, keycode);
    return state == GLFW_PRESS || state == GLFW_REPEAT;
}

bool Hazel::LinuxInput::IsMouseButtonPressedImpl(int button) {
    auto window = static_cast<GLFWwindow *>(Application::Get().GetWindow().GetNativeWindow());
    auto state = glfwGetMouseButton(window, button);
    return state == GLFW_PRESS;
}

std::pair<float, float> Hazel::LinuxInput::GetMousePositionImpl() {
    auto window = static_cast<GLFWwindow *>(Application::Get().GetWindow().GetNativeWindow());
    double xPos, yPos;
    glfwGetCursorPos(window, &xPos, &yPos);

    return {static_cast<float>(xPos), static_cast<float>(yPos)};
}

float Hazel::LinuxInput::GetMouseXImpl() {
    auto[x, y] = GetMousePositionImpl();
    return x;
}

float Hazel::LinuxInput::GetMouseYImpl() {
    auto[x, y] = GetMousePositionImpl();
    return y;
}
