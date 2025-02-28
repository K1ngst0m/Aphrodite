#pragma once

#include "cli/cli.h"
#include "common/hash.h"
#include <string>

GENERATE_LOG_FUNCS(APP)

namespace aph
{
struct AppOptions
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
};

class BaseAppImpl
{
public:
    void addCLIOption(const char* cli, const std::function<void(CLIParser&)>& func);
    void loadConfig(int argc, char** argv, std::string configPath);

    void printOptions() const;

    const AppOptions& getOptions() const;
    AppOptions& getMutableOptions();

    int getExitCode() const;

private:
    AppOptions m_options;
    int m_exitCode = 0;
    aph::CLICallbacks m_callbacks;
};

template <typename T>
class BaseApp
{
public:
    BaseApp(std::string sessionName)
        : m_sessionName(std::move(sessionName))
    {
        m_impl = std::make_unique<BaseAppImpl>();
    }

    virtual ~BaseApp() = default;

    const AppOptions& getOptions() const
    {
        return m_impl->getOptions();
    }

    T& addCLIOption(const char* cli, const std::function<void(CLIParser&)>& func)
    {
        m_impl->addCLIOption(cli, func);
        return *static_cast<T*>(this);
    }

    T& loadConfig(int argc, char** argv, std::string configPath = "config.toml")
    {
        m_impl->loadConfig(argc, argv, configPath);
        return *static_cast<T*>(this);
    }

    int run()
    {
        init();
        load();
        loop();
        unload();
        finish();

        return m_impl->getExitCode();
    }

    // windowWidth
    uint32_t getWindowWidth() const
    {
        return m_impl->getOptions().windowWidth;
    }
    T& setWindowWidth(uint32_t width)
    {
        m_impl->getMutableOptions().windowWidth = width;
        return *static_cast<T*>(this);
    }

    // windowHeight
    uint32_t getWindowHeight() const
    {
        return m_impl->getOptions().windowHeight;
    }
    T& setWindowHeight(uint32_t height)
    {
        m_impl->getMutableOptions().windowHeight = height;
        return *static_cast<T*>(this);
    }

    // vsync
    bool getVsync() const
    {
        return m_impl->getOptions().vsync;
    }
    T& setVsync(bool flag)
    {
        m_impl->getMutableOptions().vsync = flag;
        return *static_cast<T*>(this);
    }

    // protocols
    const aph::HashMap<std::string, std::string>& getProtocols(std::string_view protocol) const
    {
        return m_impl->getOptions().protocols;
    }

    T& addProtocols(const std::string protocol, const std::string& path)
    {
        m_impl->getMutableOptions().protocols[protocol] = path;
        return *static_cast<T*>(this);
    }

    // numThreads
    uint32_t getNumThreads() const
    {
        return m_impl->getOptions().numThreads;
    }
    T& setNumThreads(uint32_t numThreads)
    {
        m_impl->getMutableOptions().numThreads = numThreads;
        return *static_cast<T*>(this);
    }

    // logLevel
    uint32_t getLogLevel() const
    {
        return m_impl->getOptions().logLevel;
    }
    T& setLogLevel(uint32_t logLevel)
    {
        m_impl->getMutableOptions().logLevel = logLevel;
        return *static_cast<T*>(this);
    }

    // backtrace
    uint32_t getBacktrace() const
    {
        return m_impl->getOptions().backtrace;
    }
    T& setBacktrace(uint32_t backtrace)
    {
        m_impl->getMutableOptions().backtrace = backtrace;
        return *static_cast<T*>(this);
    }

protected:
    virtual void init() = 0;
    virtual void load() = 0;
    virtual void loop() = 0;
    virtual void unload() = 0;
    virtual void finish() = 0;

private:
    std::unique_ptr<BaseAppImpl> m_impl;
    const std::string m_sessionName;
};

} // namespace aph
