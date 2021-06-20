//
// Created by npchitman on 5/30/21.
//

#include <Hazel.h>
#include <memory>
#include "../../Hazel/3rdparty/imgui/imgui.h"

class ExampleLayer : public Hazel::Layer {
public:
    ExampleLayer() : Layer("Example") {}

    void OnUpdate() override {
        if(Hazel::Input::IsKeyPressed(HZ_KEY_TAB))
            HZ_TRACE("Tab key is pressed (poll)!");
    }

    void OnImGuiRender() override{
        ImGui::Begin("Test");
        ImGui::Text("Hello World");
        ImGui::End();
    }

    void OnEvent(Hazel::Event &event) override {
        if(event.GetEventType() == Hazel::EventType::KeyPressed){
            auto& e = (Hazel::KeyPressedEvent&) event;
            if(e.GetKeyCode() == HZ_KEY_TAB)
                HZ_TRACE("Tab key is pressed (event)!");
            HZ_TRACE("{0}", static_cast<char>(e.GetKeyCode()));
        }
    }
};

class Sandbox : public Hazel::Application {
public:
    Sandbox(){
        PushLayer(new ExampleLayer());
    }

    ~Sandbox() override = default;
};

Hazel::Application *Hazel::CreateApplication() {
    return new Sandbox();
}