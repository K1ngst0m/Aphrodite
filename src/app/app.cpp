#include "app.h"

namespace aph
{

// Constructor and destructor
App::App(std::string sessionName)
    : m_sessionName(std::move(sessionName))
{
}

App::~App() = default;

AppOptions& App::getOptions()
{
    return options;
}

// Run method
int App::run()
{
    init();
    load();
    loop();
    unload();
    finish();

    return exitCode;
}
} // namespace aph
