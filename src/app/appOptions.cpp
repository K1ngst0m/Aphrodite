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
AppOptions& AppOptions::setWindowWidth(uint32_t width)
{
    windowWidth = width;
    return *this;
}

AppOptions& AppOptions::setWindowHeight(uint32_t height)
{
    windowHeight = height;
    return *this;
}

AppOptions& AppOptions::setVsync(bool enabled)
{
    vsync = enabled;
    return *this;
}

AppOptions& AppOptions::addProtocol(const std::string& protocol, const std::string& path)
{
    protocols[protocol] = path;
    return *this;
}

AppOptions& AppOptions::setNumThreads(uint32_t threads)
{
    numThreads = threads;
    return *this;
}

AppOptions& AppOptions::setLogLevel(uint32_t level)
{
    logLevel = level;
    return *this;
}

AppOptions& AppOptions::setBacktrace(bool enabled)
{
    backtrace = enabled;
    return *this;
}

AppOptions& AppOptions::setAbortOnFatalError(bool enabled)
{
    abortOnFatalError = enabled;
    return *this;
}

AppOptions& AppOptions::setLogTime(bool enabled)
{
    logTime = enabled;
    return *this;
}

AppOptions& AppOptions::setLogColor(bool enabled)
{
    logColor = enabled;
    return *this;
}

AppOptions& AppOptions::setLogLineInfo(bool enabled)
{
    logLineInfo = enabled;
    return *this;
}

Result AppOptions::processCLI(int argc, char** argv)
{
    callbacks.setErrorHandler([&](const CLIErrorInfo& info)
                              { CM_LOG_ERR("Failed to parse CLI arguments. %s", info.message); });

    // Register CLI arguments
    registerCLIValue("--backtrace", backtrace);
    registerCLIValue("--abort-on-fatal-error", abortOnFatalError);

    // TODO exist code
    int exitCode;
    if (!callbacks.parse(argc, argv, exitCode))
    {
        APH_ASSERT(false);
        return {Result::RuntimeError, "Failed to parse command line arguments.\n"};
    }

    return {Result::Success};
}

Result AppOptions::processConfigFile(const std::string& configPath)
{
    auto result = toml::parse_file(configPath);
    if (!result)
    {
        return {Result::RuntimeError, std::format("Parsing failed:\n{}\n", result.error().description())};
    }

    const toml::table& table = result.table();

    windowWidth  = table.at_path("window.width").value_or(1920U);
    windowHeight = table.at_path("window.height").value_or(1080U);
    vsync        = table.at_path("window.vsync").value_or(true);

    for (auto&& [k, v] : *table.at_path("fs_protocol").as_table())
    {
        protocols[std::string{k.data()}] = v.value_or("");
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

void AppOptions::setupSystems()
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

void AppOptions::printOptions() const
{
    APP_LOG_INFO("=== Application Options ===");
    APP_LOG_INFO("Window Width: %u", windowWidth);
    APP_LOG_INFO("Window Height: %u", windowHeight);
    APP_LOG_INFO("VSync: %s", vsync ? "true" : "false");
    for (const auto& [protocol, path] : protocols)
    {
        std::string absPath = APH_DEFAULT_FILESYSTEM.absolutePath(path).string();
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

Result AppOptions::parse(int argc, char** argv, std::string configPath)
{
    ResultGroup result;
    result += processConfigFile(configPath);
    result += processCLI(argc, argv);
    setupSystems();
    printOptions();
    return result;
}
uint32_t AppOptions::getWindowWidth() const
{
    return windowWidth;
}
uint32_t AppOptions::getWindowHeight() const
{
    return windowHeight;
}
bool AppOptions::getVsync() const
{
    return vsync;
}
uint32_t AppOptions::getNumThreads() const
{
    return numThreads;
}
uint32_t AppOptions::getLogLevel() const
{
    return logLevel;
}
bool AppOptions::getBacktrace() const
{
    return backtrace;
}
bool AppOptions::getAbortOnFatalError() const
{
    return abortOnFatalError;
}
bool AppOptions::getLogTime() const
{
    return logTime;
}
bool AppOptions::getLogColor() const
{
    return logColor;
}
bool AppOptions::getLogLineInfo() const
{
    return logLineInfo;
}
const HashMap<std::string, std::string>& AppOptions::getProtocols() const
{
    return protocols;
}
} // namespace aph
