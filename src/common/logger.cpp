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
    bool stripColors;

    FileSink(const std::string& filename, bool stripColors = true)
        : file(filename, std::ofstream::out | std::ofstream::trunc)
        , stripColors(stripColors)
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
            if (stripColors)
            {
                // Strip ANSI color codes from the message
                std::string strippedMsg = msg;
                size_t pos = 0;
                const std::string esc = "\033[";
                
                while ((pos = strippedMsg.find(esc, pos)) != std::string::npos)
                {
                    size_t endPos = strippedMsg.find('m', pos);
                    if (endPos != std::string::npos)
                    {
                        strippedMsg.erase(pos, endPos - pos + 1);
                    }
                    else
                    {
                        break;
                    }
                }
                
                file << strippedMsg;
            }
            else
            {
                file << msg;
            }
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
    addSink(FileSink("log.txt", true), true);
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
    // Handle invalid levels more gracefully
    if (level >= static_cast<uint32_t>(Level::None))
    {
        setLogLevel(Level::None);
    }
    else
    {
        setLogLevel(static_cast<Level>(level));
    }
}

void Logger::setLogFile(const std::string& filename)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Remove any existing file sinks
    auto it = m_sinks.begin();
    while (it != m_sinks.end())
    {
        if (it->isFileSink)
        {
            it = m_sinks.erase(it);
        }
        else
        {
            ++it;
        }
    }
    
    // Add the new file sink with color stripping enabled
    addSink(FileSink(filename, true), true);
}

} // namespace aph
