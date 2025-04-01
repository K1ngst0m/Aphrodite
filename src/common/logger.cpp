#include "logger.h"
namespace
{
struct ConsoleSink
{
    void write(const std::string& msg)
    {
        std::cout << msg;
    }
    void flush()
    {
        std::cout.flush();
    }
};

struct FileSink
{
    std::ofstream file;

    FileSink(const std::string& filename)
        : file(filename, std::ofstream::out | std::ofstream::trunc)
    {
        if (!file.is_open())
        {
            std::cerr << "Failed to open log file: " << filename << "\n";
        }
    }

    void write(const std::string& msg)
    {
        if (file.is_open())
        {
            file << msg;
        }
    }

    void flush()
    {
        if (file.is_open())
        {
            file.flush();
        }
    }
};
} // namespace

namespace aph
{
Logger::Logger()
    : m_logLevel(Level::Debug)
{
    addSink(ConsoleSink());
    addSink(FileSink("log.txt"));
}

void Logger::flush()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& sink : m_sinks)
    {
        sink.flushCallback();
    }
}
std::string Logger::getCurrentTime()
{
    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);
    std::ostringstream oss;
    oss << std::put_time(&tm, "[%Y-%m-%d %H:%M:%S]");
    return oss.str();
}
void Logger::setLogLevel(uint32_t level)
{
    if (level > static_cast<uint32_t>(Level::None))
    {
        setLogLevel(Level::Info);
    }
    else
    {
        setLogLevel(static_cast<Level>(level));
    }
}
} // namespace aph
