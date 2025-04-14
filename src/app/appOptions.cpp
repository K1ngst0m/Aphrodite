#include "appOptions.h"

#include "common/functiontraits.h"
#include "common/hash.h"
#include "common/logger.h"

#include "app/app.h"
#include "filesystem/filesystem.h"
#include "global/globalManager.h"

#define TOML_EXCEPTIONS 0
#include "toml++/toml.hpp"

namespace aph
{
auto AppOptions::setWindowWidth(uint32_t width) -> AppOptions&
{
    windowWidth = width;
    return *this;
}

auto AppOptions::setWindowHeight(uint32_t height) -> AppOptions&
{
    windowHeight = height;
    return *this;
}

auto AppOptions::setVsync(bool enabled) -> AppOptions&
{
    vsync = enabled;
    return *this;
}

auto AppOptions::addProtocol(const std::string& protocol, const std::string& path) -> AppOptions&
{
    protocols[protocol] = path;
    return *this;
}

auto AppOptions::setNumThreads(uint32_t threads) -> AppOptions&
{
    numThreads = threads;
    return *this;
}

auto AppOptions::setLogLevel(uint32_t level) -> AppOptions&
{
    logLevel = level;
    return *this;
}

auto AppOptions::setBacktrace(bool enabled) -> AppOptions&
{
    backtrace = enabled;
    return *this;
}

auto AppOptions::setAbortOnFatalError(bool enabled) -> AppOptions&
{
    abortOnFatalError = enabled;
    return *this;
}

auto AppOptions::setLogTime(bool enabled) -> AppOptions&
{
    logTime = enabled;
    return *this;
}

auto AppOptions::setLogColor(bool enabled) -> AppOptions&
{
    logColor = enabled;
    return *this;
}

auto AppOptions::setLogLineInfo(bool enabled) -> AppOptions&
{
    logLineInfo = enabled;
    return *this;
}

auto AppOptions::processCLI(int argc, char** argv) -> Result
{
    callbacks.setErrorHandler(
        [&](const CLIErrorInfo& info)
        {
            CM_LOG_ERR("Failed to parse CLI arguments. %s", info.message);
        });

    // Register CLI arguments
    registerCLIValue("--backtrace", backtrace);
    registerCLIValue("--abort-on-fatal-error", abortOnFatalError);

    // TODO exist code
    int exitCode;
    if (!callbacks.parse(argc, argv, exitCode))
    {
        APH_ASSERT(false);
        return { Result::RuntimeError, "Failed to parse command line arguments.\n" };
    }

    return { Result::Success };
}

auto AppOptions::processConfigFile(const std::string& configPath) -> Result
{
    auto result = toml::parse_file(configPath);
    if (!result)
    {
        return { Result::RuntimeError, std::format("Parsing failed:\n{}\n", result.error().description()) };
    }

    const toml::table& table = result.table();

    windowWidth  = table.at_path("window.width").value_or(1920U);
    windowHeight = table.at_path("window.height").value_or(1080U);
    vsync        = table.at_path("window.vsync").value_or(true);

    for (auto&& [k, v] : *table.at_path("fs_protocol").as_table())
    {
        protocols[std::string{ k.data() }] = v.value_or("");
    }

    numThreads = table.at_path("thread.num_override").value_or(0U);
    logLevel   = table.at_path("debug.log_level").value_or(1U);

    // Parse boolean options
    backtrace         = table.at_path("debug.backtrace").value_or(true);
    abortOnFatalError = table.at_path("debug.abort_on_fatal_error").value_or(true);

    // Parse logger boolean options
    logTime     = table.at_path("debug.log_time").value_or(false);
    logColor    = table.at_path("debug.log_color").value_or(true);
    logLineInfo = table.at_path("debug.log_line_info").value_or(true);

    return Result::Success;
}

auto AppOptions::setupSystems() -> void
{
    // registering protocol
    auto& fs = APH_DEFAULT_FILESYSTEM;
    fs.registerProtocol(protocols);

    // setup logger
    APH_LOGGER.setLogLevel(logLevel);
    APH_LOGGER.setEnableTime(logTime);
    APH_LOGGER.setEnableColor(logColor);
    APH_LOGGER.setEnableLineInfo(logLineInfo);
    APH_LOGGER.initialize();
}

auto AppOptions::printOptions() const -> void
{
    APP_LOG_INFO("=== Application Options ===");
    APP_LOG_INFO("Window Width: %u", windowWidth);
    APP_LOG_INFO("Window Height: %u", windowHeight);
    APP_LOG_INFO("VSync: %s", vsync ? "true" : "false");
    for (const auto& [protocol, path] : protocols)
    {
        std::string absPath = APH_DEFAULT_FILESYSTEM.absolutePath(path);
        APP_LOG_INFO("%s:// => %s", protocol, absPath);
    }
    APP_LOG_INFO("Number of Threads: %s",
                 numThreads == 0 ? "Auto (System Hardware Concurrency)" : std::to_string(numThreads));
    APP_LOG_INFO("Log Level: %u", logLevel);
    APP_LOG_INFO("Log Time: %s", logTime ? "true" : "false");
    APP_LOG_INFO("Log Color: %s", logColor ? "true" : "false");
    APP_LOG_INFO("Log Line Info: %s", logLineInfo ? "true" : "false");
    APP_LOG_INFO("Backtrace: %s", backtrace ? "true" : "false");
    APP_LOG_INFO("Abort On Fatal Error: %s", abortOnFatalError ? "true" : "false");
    APP_LOG_INFO("=== Application Options ===");
}

auto AppOptions::parse(int argc, char** argv, std::string configPath) -> Result
{
    ResultGroup result;
    result += processConfigFile(configPath);
    result += processCLI(argc, argv);
    setupSystems();
    printOptions();
    return result;
}

auto AppOptions::getWindowWidth() const -> uint32_t
{
    return windowWidth;
}

auto AppOptions::getWindowHeight() const -> uint32_t
{
    return windowHeight;
}

auto AppOptions::getVsync() const -> bool
{
    return vsync;
}

auto AppOptions::getNumThreads() const -> uint32_t
{
    return numThreads;
}

auto AppOptions::getLogLevel() const -> uint32_t
{
    return logLevel;
}

auto AppOptions::getBacktrace() const -> bool
{
    return backtrace;
}

auto AppOptions::getAbortOnFatalError() const -> bool
{
    return abortOnFatalError;
}

auto AppOptions::getLogTime() const -> bool
{
    return logTime;
}

auto AppOptions::getLogColor() const -> bool
{
    return logColor;
}

auto AppOptions::getLogLineInfo() const -> bool
{
    return logLineInfo;
}

auto AppOptions::getProtocols() const -> const HashMap<std::string, std::string>&
{
    return protocols;
}
} // namespace aph
