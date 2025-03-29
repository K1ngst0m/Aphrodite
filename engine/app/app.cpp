#include "app.h"
#include "cli/cli.h"
#include "common/functiontraits.h"
#include "common/hash.h"
#include "common/logger.h"
#include "filesystem/filesystem.h"

#define TOML_EXCEPTIONS 0
#include "toml++/toml.hpp"

namespace aph
{

class App::Impl
{
public:
    Impl() = default;

    AppOptions options;
    int exitCode = 0;
    aph::CLICallbacks callbacks;

    void loadConfig(int argc, char** argv, std::string configPath);
    void printOptions() const;

    template <typename Func>
    void addCLICallback(const char* cli, Func&& func)
    {
        callbacks.add(cli, APH_FWD(func));
    }
};

// Constructor and destructor
App::App(std::string sessionName)
    : m_impl(std::make_unique<Impl>())
    , m_sessionName(std::move(sessionName))
{
}

App::~App() = default;

// Config methods
App& App::loadConfig(int argc, char** argv, std::string configPath)
{
    m_impl->loadConfig(argc, argv, configPath);
    return *this;
}

void App::Impl::loadConfig(int argc, char** argv, std::string configPath)
{
    auto& opt = options;

    // parse toml config file
    {
        auto result = toml::parse_file(configPath);
        if (!result)
        {
            CM_LOG_ERR("Parsing failed:\n%s\n", result.error().description());
            exitCode = -1;
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
        callbacks.setErrorHandler([&](const CLIErrorInfo& info)
                                  { CM_LOG_ERR("Failed to parse CLI arguments. %s", info.message); });
        if (!callbacks.parse(argc, argv, exitCode))
        {
            std::cout << "Failed to parse command line arguments.\n";
            APH_ASSERT(false);
        }
    }

    // registering protocol
    {
        aph::Filesystem::GetInstance().registerProtocol(opt.protocols);
    }

    // setup logger
    {
        aph::Logger::GetInstance().setLogLevel(opt.logLevel);
    }

    printOptions();
}

void App::Impl::printOptions() const
{
    APP_LOG_INFO("=== Application Options ===");
    APP_LOG_INFO("windowWidth: %u", options.windowWidth);
    APP_LOG_INFO("windowHeight: %u", options.windowHeight);
    APP_LOG_INFO("vsync: %d", options.vsync);
    for (const auto& [protocol, path] : options.protocols)
    {
        APP_LOG_INFO("protocol: %s => %s", protocol.c_str(), path.c_str());
    }
    APP_LOG_INFO("numThreads: %u", options.numThreads);
    APP_LOG_INFO("logLevel: %u", options.logLevel);
    APP_LOG_INFO("backtrace: %u", options.backtrace);
    APP_LOG_INFO(" === Application Options ===\n");
}

// Run method
int App::run()
{
    init();
    load();
    loop();
    unload();
    finish();

    return m_impl->exitCode;
}

// Options access
const AppOptions& App::getOptions() const
{
    return m_impl->options;
}

AppOptions& App::getMutableOptions()
{
    return m_impl->options;
}

// Window options
uint32_t App::getWindowWidth() const
{
    return m_impl->options.windowWidth;
}

App& App::setWindowWidth(uint32_t width)
{
    m_impl->options.windowWidth = width;
    return *this;
}

uint32_t App::getWindowHeight() const
{
    return m_impl->options.windowHeight;
}

App& App::setWindowHeight(uint32_t height)
{
    m_impl->options.windowHeight = height;
    return *this;
}

bool App::getVsync() const
{
    return m_impl->options.vsync;
}

App& App::setVsync(bool flag)
{
    m_impl->options.vsync = flag;
    return *this;
}

// This is the non-template method used by the template method in the header
void App::addCLICallbackHelper(const char* cli, std::function<void(const CLIParser&)> callback)
{
    m_impl->addCLICallback(cli, std::move(callback));
}

// Protocol options
const HashMap<std::string, std::string>& App::getProtocols() const
{
    return m_impl->options.protocols;
}

App& App::addProtocol(const std::string& protocol, const std::string& path)
{
    m_impl->options.protocols[protocol] = path;
    return *this;
}

// Thread options
uint32_t App::getNumThreads() const
{
    return m_impl->options.numThreads;
}

App& App::setNumThreads(uint32_t numThreads)
{
    m_impl->options.numThreads = numThreads;
    return *this;
}

// Debug options
uint32_t App::getLogLevel() const
{
    return m_impl->options.logLevel;
}

App& App::setLogLevel(uint32_t logLevel)
{
    m_impl->options.logLevel = logLevel;
    return *this;
}

uint32_t App::getBacktrace() const
{
    return m_impl->options.backtrace;
}

App& App::setBacktrace(uint32_t backtrace)
{
    m_impl->options.backtrace = backtrace;
    return *this;
}

} // namespace aph
