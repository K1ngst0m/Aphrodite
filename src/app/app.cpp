#include "app.h"
#include "exception/exception.h"
#include "allocator/allocator.h"
#include "global/globalManager.h"

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

    std::string report = APH_MEMORY_TRACKER.generateSummaryReport();
    MM_LOG_INFO("%s", report.c_str());

    aph::shutdownErrorHandling();

    return exitCode;
}
} // namespace aph
