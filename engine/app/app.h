#pragma once

#include "cli/cli.h"
#include "common/functiontraits.h"
#include "common/logger.h"

GENERATE_LOG_FUNCS(APP)

namespace aph
{
struct AppOptions
{
    // window
    uint32_t windowWidth = 1440;
    uint32_t windowHeight = 900;
    bool vsync = true;

    // fs protocol
    HashMap<std::string, std::string> protocols;

    // thread
    uint32_t numThreads = 0;

    // debug
    uint32_t logLevel = 0;
    uint32_t backtrace = 0;
};

class App
{
public:
    App(std::string sessionName);
    virtual ~App();

public:
    // Run the application
    int run();

public:
    // Configuration methods
    App& loadConfig(int argc, char** argv, std::string configPath = "config.toml");

    template <typename T>
    void registerCLIValue(const char* cli, T& value);

    template <typename Func>
    App& addCLICallback(const char* cli, Func&& func);

    // Options access
    const AppOptions& getOptions() const;
    AppOptions& getMutableOptions();

    // Window options
    uint32_t getWindowWidth() const;
    App& setWindowWidth(uint32_t width);

    uint32_t getWindowHeight() const;
    App& setWindowHeight(uint32_t height);

    bool getVsync() const;
    App& setVsync(bool flag);

    // Protocol options
    const HashMap<std::string, std::string>& getProtocols() const;
    App& addProtocol(const std::string& protocol, const std::string& path);

    // Thread options
    uint32_t getNumThreads() const;
    App& setNumThreads(uint32_t numThreads);

    // Debug options
    uint32_t getLogLevel() const;
    App& setLogLevel(uint32_t logLevel);

    uint32_t getBacktrace() const;
    App& setBacktrace(uint32_t backtrace);

protected:
    virtual void init() = 0;
    virtual void load() = 0;
    virtual void loop() = 0;
    virtual void unload() = 0;
    virtual void finish() = 0;

private:
    void addCLICallbackHelper(const char* cli, std::function<void(const CLIParser&)> callback);
    class Impl;
    std::unique_ptr<Impl> m_impl;
    const std::string m_sessionName;
};

template <typename T>
void App::registerCLIValue(const char* cli, T& value)
{
    addCLICallback(cli, [&value](T v) { value = v; });
}

template <typename Func>
App& App::addCLICallback(const char* cli, Func&& func)
{
    class Impl;
    using argType = FunctionArgumentType<Func, 0>;
    auto callback = [f = APH_FWD(func)](const CLIParser& parser) { f(parser.next<argType>()); };
    addCLICallbackHelper(cli, std::move(callback));
    return *this;
}

} // namespace aph
