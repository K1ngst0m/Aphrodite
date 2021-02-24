//
// Created by Npchitman on 2021/1/17.
//

#include <Hazel.h>

class ExampleLayer: public Hazel::Layer{
public:
    ExampleLayer(): Layer("Example"){

    }

    void OnUpdate() override{
//        HZ_INFO("ExampleLayer::Update");
        if(Hazel::Input::IsKeyPressed(HZ_KEY_TAB)){
            HZ_TRACE("Tab key is pressed (poll)");
        }
    }

    void OnEvent(Hazel::Event& event) override{
        if (event.GetEventType() == Hazel::EventType::KeyPressed){
            Hazel::KeyPressedEvent & e = static_cast<Hazel::KeyPressedEvent&>(event);
            if(Hazel::Input::IsKeyPressed(HZ_KEY_TAB)){
                HZ_TRACE("Tab key is pressed (event)");
            }
            HZ_TRACE("{0}", static_cast<char>(e.GetKeyCode()));
        }
    }

};

class Sandbox: public Hazel::Application{
public:
    Sandbox(){
        PushLayer(new ExampleLayer());
        PushOverLay(new Hazel::ImGuiLayer());
    }

    ~Sandbox() = default;
};

Hazel::Application* Hazel::CreateApplication(){
    return new Sandbox();
}