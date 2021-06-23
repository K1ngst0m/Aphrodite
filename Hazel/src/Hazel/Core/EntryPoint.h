#ifndef HAZEL_ENTRYPOINT_H
#define HAZEL_ENTRYPOINT_H

#ifdef HZ_PLATFORM_LINUX

#include "Hazel/Core/Base.h"

int main(int argc, char **argv) {
    Hazel::Log::Init();

    HZ_PROFILE_BEGIN_SESSION("Startup", "HazelProfile-Startup.json");
    auto app = Hazel::CreateApplication();
    HZ_PROFILE_END_SESSION();

    HZ_PROFILE_BEGIN_SESSION("Runtime", "HazelProfile-Runtime.json");
    app->Run();
    HZ_PROFILE_END_SESSION();

    HZ_PROFILE_BEGIN_SESSION("Startup", "HazelProfile-Shutdown.json");
    delete app;
    HZ_PROFILE_END_SESSION();
}

#endif

#endif