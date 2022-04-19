#include "EntryPoint.h"

int main(int argc, char **argv) {
    // Set current working directory to program binary path
    {
        const std::string& myPath = argv[0];
        const std::string& workDir = myPath.substr(0, myPath.rfind('/'));
        if (chdir(workDir.c_str()) != 0)
        {
            fprintf(stderr, "Error: Cannot change current working directory to \"%s\", exit...\n", workDir.c_str());
            exit(1);
        }
    }

    // init core utils
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
