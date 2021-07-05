//
// Created by npchitman on 7/3/21.
//

#include <Aphrodite/Core/EntryPoint.h>

#include <Aphrodite.hpp>
#include "ExampleLayer.h"
#include "Sandbox2D.h"

class Sandbox : public Aph::Application
{
public:
    Sandbox()
    {
        // PushLayer(new ExampleLayer());
        PushLayer(new Sandbox2D());
    }

    ~Sandbox() override = default;
};

Aph::Application* Aph::CreateApplication(ApplicationCommandLineArgs args)
{
    return new Sandbox();
}