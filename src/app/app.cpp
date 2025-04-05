#include "app.h"
#include "exception/exception.h"

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
    aph::initializeErrorHandling(options.getAbortOnFatalError(), 64, m_sessionName);

    init();
    load();
    loop();
    unload();
    finish();
    
    aph::shutdownErrorHandling();

    return exitCode;
}
} // namespace aph
