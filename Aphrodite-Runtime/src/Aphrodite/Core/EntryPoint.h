// EntryPoint.h

// engine's entry point:
// - init logging system
// - create application
// - run the game circle


#ifndef Aphrodite_ENTRYPOINT_H
#define Aphrodite_ENTRYPOINT_H

#include "Application.h"
#include "Base.h"

#ifdef APH_PLATFORM_LINUX

int main(int argc, char **argv) {
    Aph::Log::Init();

    APH_PROFILE_BEGIN_SESSION("Startup", "APH-Profile-Startup.json");
    auto app = Aph::CreateApplication({argc, argv});
    APH_PROFILE_END_SESSION();

    APH_PROFILE_BEGIN_SESSION("Runtime", "APH-Profile-Runtime.json");
    app->Run();
    APH_PROFILE_END_SESSION();

    APH_PROFILE_BEGIN_SESSION("Shutdown", "APH-Profile-Shutdown.json");
    delete app;
    APH_PROFILE_END_SESSION();
}

#endif

#endif