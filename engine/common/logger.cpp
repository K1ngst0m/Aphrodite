#include "logger.h"

namespace aph
{
void Logger::flush()
{
    std::lock_guard<std::mutex> lock(mutex);
    if(file_stream.is_open())
    {
        file_stream.flush();
    }
    std::cout.flush();
}
Logger::Logger() : log_level(Level::Debug), file_stream("log.txt", std::ofstream::app)
{
    if(!file_stream.is_open())
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
}  // namespace aph
