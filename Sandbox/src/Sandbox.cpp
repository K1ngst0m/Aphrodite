//
// Created by Npchitman on 2021/1/17.
//
#include <Hazel.h>

class Sandbox: public Hazel::Application{
public:
    Sandbox()= default;
    ~Sandbox() override = default;
};


Hazel::Application* Hazel::CreateApplication(){
    return new Sandbox();
}