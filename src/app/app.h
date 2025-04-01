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
    int run();
    AppOptions& getOptions();

protected:
    virtual void init() = 0;
    virtual void load() = 0;
    virtual void loop() = 0;
    virtual void unload() = 0;
    virtual void finish() = 0;

private:
    AppOptions options;
    int exitCode = 0;
    const std::string m_sessionName;
};

} // namespace aph
