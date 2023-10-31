#include "app.h"
#include "common/logger.h"

namespace aph
{

BaseApp::BaseApp(std::string sessionName) : m_sessionName(std::move(sessionName))
{
    Logger::GetInstance().setLogLevel(Logger::Level::Info);
}

}  // namespace aph
