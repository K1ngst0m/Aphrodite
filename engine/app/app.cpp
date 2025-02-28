#include "app.h"
#include "common/logger.h"
#include "filesystem/filesystem.h"

#define TOML_EXCEPTIONS 0
#include "toml++/toml.hpp"

namespace aph
{

const AppOptions& BaseAppImpl::getOptions() const
{
    return m_options;
}

AppOptions& BaseAppImpl::getMutableOptions()
{
    return m_options;
}

void BaseAppImpl::addCLIOption(const char* cli, const std::function<void(CLIParser&)>& func)
{
    m_callbacks.add(cli, func);
}

void BaseAppImpl::loadConfig(int argc, char** argv, std::string configPath)
{

    auto& opt = m_options;

    // parse toml config file
    {
        auto result = toml::parse_file(configPath);
        if (!result)
        {
            CM_LOG_ERR("Parsing failed:\n%s\n", result.error().description());
            m_exitCode = -1;
        }

        const toml::table& table = result.table();

        opt.windowWidth = table.at_path("window.width").value_or(1920U);
        opt.windowHeight = table.at_path("window.height").value_or(1080U);
        opt.vsync = table.at_path("window.vsync").as_boolean();

        for (auto&& [k, v] : *table.at_path("fs_protocol").as_table())
        {
            opt.protocols[std::string{ k.data() }] = v.value_or("");
        }

        opt.numThreads = table.at_path("thread.num_override").value_or(0U);
        opt.logLevel = table.at_path("debug.log_level").value_or(1U);
        opt.backtrace = table.at_path("debug.backtrace").value_or(1U);
    }

    // parse command
    {
        m_callbacks.add("--width", [&](aph::CLIParser& parser) { opt.windowWidth = parser.nextUint(); });
        m_callbacks.add("--height", [&](aph::CLIParser& parser) { opt.windowHeight = parser.nextUint(); });
        m_callbacks.add("--vsync", [&](aph::CLIParser& parser) { opt.vsync = parser.nextUint(); });
        m_callbacks.m_errorHandler = [&]() { CM_LOG_ERR("Failed to parse CLI arguments."); };
        if (!aph::parseCliFiltered(m_callbacks, argc, argv, m_exitCode))
        {
            std::cout << "Failed to parse command line arguments.\n";
            APH_ASSERT(false);
        }
    }

    // registering protocol
    {
        aph::Filesystem::GetInstance().registerProtocol(m_options.protocols);
    }

    // setup logger
    {
        aph::Logger::GetInstance().setLogLevel(m_options.logLevel);
    }

    //
    {
    }
}

int BaseAppImpl::getExitCode() const
{
    return m_exitCode;
}

void BaseAppImpl::printOptions() const
{
    APP_LOG_INFO("windowWidth: %u", m_options.windowWidth);
    APP_LOG_INFO("windowHeight: %u", m_options.windowHeight);
    APP_LOG_INFO("vsync: %d", m_options.vsync);
    for (const auto& [protocol, path] : m_options.protocols)
    {
        APP_LOG_INFO("protocol: %s => %s", protocol.c_str(), path);
    }
    APP_LOG_INFO("numThreads: %u", m_options.numThreads);
    APP_LOG_INFO("logLevel: %u", m_options.logLevel);
    APP_LOG_INFO("backtrace: %u", m_options.backtrace);
}
} // namespace aph
