#include "app.h"

namespace aph
{

BaseApp::BaseApp(std::string sessionName) : m_sessionName(std::move(sessionName)) {}

}  // namespace aph
