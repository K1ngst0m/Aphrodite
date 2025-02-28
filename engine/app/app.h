#pragma once

#include "cli/cli.h"
#include "common/hash.h"
#include <string>

GENERATE_LOG_FUNCS(APP)

namespace aph
{
class BaseApp
{
public:
    BaseApp(std::string sessionName = "Untitled");
    virtual ~BaseApp() = default;

    virtual void init() = 0;
    virtual void load() = 0;
    virtual void run() = 0;
    virtual void unload() = 0;
    virtual void finish() = 0;

    void registerOption(const char* cli, const std::function<void(CLIParser&)>& func);
    void loadConfig(int argc, char** argv, std::string configPath = "config.toml");

    struct
    {
        // window
        uint32_t windowWidth = { 1440 };
        uint32_t windowHeight = { 900 };
        bool vsync = true;

        // fs protocol
        aph::HashMap<std::string, std::string> protocols;

        // thread
        uint32_t numThreads = 0;

        // debug
        uint32_t logLevel = 0;
        uint32_t backtrace = 0;
    } m_options;

protected:
    const std::string m_sessionName;
    int m_exitCode = 0;
    aph::CLICallbacks m_callbacks;
};
} // namespace aph
