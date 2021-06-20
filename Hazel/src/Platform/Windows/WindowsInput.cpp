//
// Created by Npchitman on 2021/2/22.
//

#include "WindowsInput.h"
#include "hzpch.h"

#include "Hazel/Application.h"
#include <GLFW/glfw3.h>

namespace Hazel {

Input *Input::s_Instance = new WindowsInput();

bool WindowsInput::IsKeyPressedImpl(int keycode) {
  auto window = static_cast<GLFWwindow *>(
      Application::Get().GetWindow().GetNativeWindow());
  auto state = glfwGetKey(window, keycode);
  return state == GLFW_PRESS || state == GLFW_REPEAT;
}

bool WindowsInput::IsMouseButtonPressedImpl(int keycode) {
  auto window = static_cast<GLFWwindow *>(
      Application::Get().GetWindow().GetNativeWindow());
  auto state = glfwGetMouseButton(window, keycode);
  return state == GLFW_PRESS;
}

std::pair<float, float> WindowsInput::GetMousePositionImpl() {
  auto window = static_cast<GLFWwindow *>(
      Application::Get().GetWindow().GetNativeWindow());
  double xPos, yPos;
  glfwGetCursorPos(window, &xPos, &yPos);
  return std::pair<float, float>(static_cast<float>(xPos),
                                 static_cast<float>(yPos));
}

float WindowsInput::GetMouseXImpl() {
  auto [x, y] = GetMousePositionImpl();
  return x;
}

float WindowsInput::GetMouseYImpl() {
  auto [x, y] = GetMousePositionImpl();
  return y;
}

} // namespace Hazel
