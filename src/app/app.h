#pragma once

#include "appOptions.h"

GENERATE_LOG_FUNCS(APP)

namespace aph
{
class App
{
public:
    App(std::string sessionName);
    virtual ~App();

public:
    auto run() -> int;
    auto getOptions() -> AppOptions&;

protected:
    virtual auto init() -> void = 0;
    virtual auto load() -> void = 0;
    virtual auto loop() -> void = 0;
    virtual auto unload() -> void = 0;
    virtual auto finish() -> void = 0;

private:
    AppOptions options;
    int exitCode = 0;
    const std::string m_sessionName;
};

} // namespace aph
