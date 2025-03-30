#include "appOptions.h"
#include "app/app.h"
#include "common/functiontraits.h"
#include "common/hash.h"
#include "common/logger.h"
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

AppOptions& AppOptions::setBacktrace(uint32_t level)
{
    backtrace = level;
    return *this;
}

Result AppOptions::processCLI(int argc, char** argv)
{
    callbacks.setErrorHandler([&](const CLIErrorInfo& info)
                              { CM_LOG_ERR("Failed to parse CLI arguments. %s", info.message); });
    // TODO exist code
    int exitCode;
    if (!callbacks.parse(argc, argv, exitCode))
    {
        APH_ASSERT(false);
        return { Result::RuntimeError, "Failed to parse command line arguments.\n" };
    }

    return { Result::Success };
}

Result AppOptions::processConfigFile(const std::string& configPath)
{
    auto result = toml::parse_file(configPath);
    if (!result)
    {
        return { Result::RuntimeError, std::format("Parsing failed:\n{}\n", result.error().description()) };
    }

    const toml::table& table = result.table();

    windowWidth = table.at_path("window.width").value_or(1920U);
    windowHeight = table.at_path("window.height").value_or(1080U);
    vsync = table.at_path("window.vsync").as_boolean();

    for (auto&& [k, v] : *table.at_path("fs_protocol").as_table())
    {
        protocols[std::string{ k.data() }] = v.value_or("");
    }

    numThreads = table.at_path("thread.num_override").value_or(0U);
    logLevel = table.at_path("debug.log_level").value_or(1U);
    backtrace = table.at_path("debug.backtrace").value_or(1U);

    return Result::Success;
}

void AppOptions::setupSystems()
{
    // registering protocol
    auto& fs = APH_DEFAULT_FILESYSTEM;
    fs.registerProtocol(protocols);

    // setup logger
    APH_LOGGER.setLogLevel(logLevel);
}

void AppOptions::printOptions() const
{
    APP_LOG_INFO("=== Application Options ===");
    APP_LOG_INFO("windowWidth: %u", windowWidth);
    APP_LOG_INFO("windowHeight: %u", windowHeight);
    APP_LOG_INFO("vsync: %d", vsync);
    for (const auto& [protocol, path] : protocols)
    {
        APP_LOG_INFO("protocol: %s => %s", protocol.c_str(), path.c_str());
    }
    APP_LOG_INFO("numThreads: %u", numThreads);
    APP_LOG_INFO("logLevel: %u", logLevel);
    APP_LOG_INFO("backtrace: %u", backtrace);
    APP_LOG_INFO(" === Application Options ===\n");
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
uint32_t AppOptions::getBacktrace() const
{
    return backtrace;
}
const HashMap<std::string, std::string>& AppOptions::getProtocols() const
{
    return protocols;
}
} // namespace aph
