//
// Created by npchitman on 5/30/21.
//

#include <Hazel.h>
#include <Hazel/Core/EntryPoint.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "../../Hazel/3rdparty/imgui/imgui.h"
#include "Platform/OpenGL/OpenGLShader.h"
#include "Sandbox2D.h"
#include "ExampleLayer.h"

class Sandbox : public Hazel::Application {
public:
    Sandbox() { PushLayer(new Sandbox2D()); }

    ~Sandbox() override = default;
};

Hazel::Application* Hazel::CreateApplication() {
    return new Sandbox();
}