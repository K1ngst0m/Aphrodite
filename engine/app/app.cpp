#include "app.h"
#include "common/logger.h"
#include "cli/cli.h"
#include "filesystem/filesystem.h"

#define TOML_EXCEPTIONS 0
#include "toml++/toml.hpp"

namespace aph
{
BaseApp::BaseApp(std::string sessionName) : m_sessionName(std::move(sessionName))
{
}

void BaseApp::loadConfig(int argc, char** argv, std::string configPath)
{
    auto& opt = m_options;

    // parse toml config file
    {
        auto result = toml::parse_file(configPath);
        if(!result)
        {
            CM_LOG_ERR("Parsing failed:\n%s\n", result.error());
            m_exitCode = -1;
        }

        const toml::table& table = result.table();

        opt.windowWidth  = table.at_path("window.width").value_or(1920U);
        opt.windowHeight = table.at_path("window.height").value_or(1080U);
        opt.vsync        = table.at_path("window.vsync").as_boolean();

        for(auto&& [k, v] : *table.at_path("fs_protocol").as_table())
        {
            opt.protocols[std::string{k.data()}] = v.value_or("");
        }

        opt.numThreads = table.at_path("thread.num_override").value_or(0U);
    }

    // parse command
    {
        aph::CLICallbacks cbs;
        cbs.add("--width", [&](aph::CLIParser& parser) { opt.windowWidth = parser.nextUint(); });
        cbs.add("--height", [&](aph::CLIParser& parser) { opt.windowHeight = parser.nextUint(); });
        cbs.add("--vsync", [&](aph::CLIParser& parser) { opt.vsync = parser.nextUint(); });
        cbs.m_errorHandler = [&]() { CM_LOG_ERR("Failed to parse CLI arguments."); };
        if(!aph::parseCliFiltered(cbs, argc, argv, m_exitCode))
        {
            CM_LOG_ERR("Failed to parse command line arguments.");
            APH_ASSERT(false);
        }
    }

    // registering protocol
    {
        aph::Filesystem::GetInstance().registerProtocol(m_options.protocols);
    }
};
}  // namespace aph
