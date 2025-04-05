#pragma once

#include "cli/cli.h"
#include "common/common.h"
#include "common/functiontraits.h"
#include "common/hash.h"

namespace aph
{
struct AppOptions
{
    Result parse(int argc, char** argv, std::string configPath = "config.toml");

    // Getters
    uint32_t getWindowWidth() const;
    uint32_t getWindowHeight() const;
    bool getVsync() const;
    uint32_t getNumThreads() const;
    uint32_t getLogLevel() const;
    bool getBacktrace() const;
    bool getAbortOnFatalError() const;
    bool getLogTime() const;
    bool getLogColor() const;
    bool getLogLineInfo() const;
    const HashMap<std::string, std::string>& getProtocols() const;

    // Builder pattern methods
    AppOptions& setWindowWidth(uint32_t width);
    AppOptions& setWindowHeight(uint32_t height);
    AppOptions& setVsync(bool enabled);
    AppOptions& setNumThreads(uint32_t threads);
    AppOptions& setLogLevel(uint32_t level);
    AppOptions& setBacktrace(bool enabled);
    AppOptions& setAbortOnFatalError(bool enabled);
    AppOptions& setLogTime(bool enabled);
    AppOptions& setLogColor(bool enabled);
    AppOptions& setLogLineInfo(bool enabled);
    AppOptions& addProtocol(const std::string& protocol, const std::string& path);

    // CLI callback registration
    template <typename Func>
    AppOptions& addCLICallback(const char* cli, Func&& func);

    template <typename T>
    AppOptions& registerCLIValue(const char* cli, T& value);

private:
    // Configuration processing
    Result processCLI(int argc, char** argv);
    Result processConfigFile(const std::string& configPath);
    void setupSystems();
    void printOptions() const;

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
    bool backtrace = true;
    bool abortOnFatalError = true;
    bool logTime = false;
    bool logColor = true;
    bool logLineInfo = true;

private:
    aph::CLICallbacks callbacks;
};

template <typename T>
AppOptions& AppOptions::registerCLIValue(const char* cli, T& value)
{
    addCLICallback(cli, [&value](T v) { value = v; });
    return *this;
}

template <typename Func>
AppOptions& AppOptions::addCLICallback(const char* cli, Func&& func)
{
    using argType = FunctionArgumentType<Func, 0>;
    auto callback = [f = APH_FWD(func)](const CLIParser& parser) { f(parser.next<argType>()); };
    callbacks.add(cli, std::move(callback));
    return *this;
}
} // namespace aph
