#include "app.h"
#include "allocator/allocator.h"
#include "exception/exception.h"
#include "global/globalManager.h"

namespace aph
{

// Constructor and destructor
App::App(std::string sessionName)
    : m_sessionName(std::move(sessionName))
{
}

App::~App() = default;

auto App::getOptions() -> AppOptions&
{
    return options;
}

// Run method
auto App::run() -> int
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
