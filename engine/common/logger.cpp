#include "logger.h"

namespace aph
{
void Logger::flush()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if(m_fileStream.is_open())
    {
        m_fileStream.flush();
    }
    std::cout.flush();
}
Logger::Logger() : m_logLevel(Level::Debug), m_fileStream("log.txt", std::ofstream::app)
{
    if(!m_fileStream.is_open())
    {
        std::cerr << "Failed to open log file." << '\n';
    }
}
std::string Logger::getCurrentTime()
{
    auto               t  = std::time(nullptr);
    auto               tm = *std::localtime(&t);
    std::ostringstream oss;
    oss << std::put_time(&tm, "[%Y-%m-%d %H:%M:%S]");
    return oss.str();
}
void Logger::setLogLevel(uint32_t level)
{
    if(level > static_cast<uint32_t>(Level::None))
    {
        setLogLevel(Level::Info);
    }
    else
    {
        setLogLevel(static_cast<Level>(level));
    }
}
}  // namespace aph
