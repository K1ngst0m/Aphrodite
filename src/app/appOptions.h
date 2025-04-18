#pragma once

#include "cli/cli.h"
#include "common/common.h"
#include "common/functiontraits.h"
#include "common/hash.h"

namespace aph
{
struct AppOptions
{
    auto parse(int argc, char** argv, std::string configPath = "config.toml") -> Result;

    // Getters
    auto getWindowWidth() const -> uint32_t;
    auto getWindowHeight() const -> uint32_t;
    auto getVsync() const -> bool;
    auto getNumThreads() const -> uint32_t;
    auto getLogLevel() const -> uint32_t;
    auto getBacktrace() const -> bool;
    auto getAbortOnFatalError() const -> bool;
    auto getLogTime() const -> bool;
    auto getLogColor() const -> bool;
    auto getLogLineInfo() const -> bool;
    auto getProtocols() const -> const HashMap<std::string, std::string>&;

    // Builder pattern methods
    auto setWindowWidth(uint32_t width) -> AppOptions&;
    auto setWindowHeight(uint32_t height) -> AppOptions&;
    auto setVsync(bool enabled) -> AppOptions&;
    auto setNumThreads(uint32_t threads) -> AppOptions&;
    auto setLogLevel(uint32_t level) -> AppOptions&;
    auto setBacktrace(bool enabled) -> AppOptions&;
    auto setAbortOnFatalError(bool enabled) -> AppOptions&;
    auto setLogTime(bool enabled) -> AppOptions&;
    auto setLogColor(bool enabled) -> AppOptions&;
    auto setLogLineInfo(bool enabled) -> AppOptions&;
    auto addProtocol(const std::string& protocol, const std::string& path) -> AppOptions&;

    // CLI callback registration
    template <typename Func>
    auto addCLICallback(const char* cli, Func&& func) -> AppOptions&;

    template <typename T>
    auto registerCLIValue(const char* cli, T& value) -> AppOptions&;

private:
    // Configuration processing
    auto processCLI(int argc, char** argv) -> Result;
    auto processConfigFile(const std::string& configPath) -> Result;
    auto setupSystems() -> void;
    auto printOptions() const -> void;

    // window
    uint32_t windowWidth  = 1440;
    uint32_t windowHeight = 900;
    bool vsync            = true;

    // fs protocol
    HashMap<std::string, std::string> protocols;

    // thread
    uint32_t numThreads = 0;

    // debug
    uint32_t logLevel      = 0;
    bool backtrace         = true;
    bool abortOnFatalError = true;
    bool logTime           = false;
    bool logColor          = true;
    bool logLineInfo       = true;

private:
    aph::CLICallbacks callbacks;
};

template <typename T>
auto AppOptions::registerCLIValue(const char* cli, T& value) -> AppOptions&
{
    addCLICallback(cli,
                   [&value](T v)
                   {
                       value = v;
                   });
    return *this;
}

template <typename Func>
auto AppOptions::addCLICallback(const char* cli, Func&& func) -> AppOptions&
{
    using argType = FunctionArgumentType<Func, 0>;
    auto callback = [f = APH_FWD(func)](const CLIParser& parser)
    {
        f(parser.next<argType>());
    };
    callbacks.add(cli, std::move(callback));
    return *this;
}
} // namespace aph
