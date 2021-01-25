//
// Created by Npchitman on 2021/1/17.
//

#ifndef HAZELENGINE_ENTRYPOINT_H
#define HAZELENGINE_ENTRYPOINT_H

#ifdef HZ_PLATFORM_WINDOWS

extern Hazel::Application *Hazel::CreateApplication();

int main() {
    Hazel::Log::Init();
    HZ_CORE_WARN("Initialized Log!");
    HZ_INFO("Hello!");

    auto app = Hazel::CreateApplication();
    app->Run();
    delete app;
}

#endif
#endif //HAZELENGINE_ENTRYPOINT_H
