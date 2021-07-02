//
// Created by npchitman on 7/3/21.
//

#include <Aphrodite.h>
#include <Aphrodite/Core/EntryPoint.h>

#include "Sandbox2D.h"
#include "ExampleLayer.h"

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