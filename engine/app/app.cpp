#include "app.h"

namespace aph
{

// App implementation
class App::Impl
{
public:
    Impl() = default;

    AppOptions options;
    int exitCode = 0;
};

// Constructor and destructor
App::App(std::string sessionName)
    : m_impl(std::make_unique<Impl>())
    , m_sessionName(std::move(sessionName))
{
}

App::~App() = default;

AppOptions& App::getOptions() const
{
    return m_impl->options;
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
} // namespace aph
