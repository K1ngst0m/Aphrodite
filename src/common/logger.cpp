#include "logger.h"
#include "global/globalManager.h"

#include <cstdarg>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>

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
                size_t pos              = 0;
                const std::string esc   = "\033[";

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

// Private implementation class
class LoggerImpl
{
public:
    LoggerImpl()
        : m_logLevel(Logger::Level::Debug)
        , m_enableTime(false)
        , m_enableColor(true)
        , m_enableLineInfo(true)
        , m_initialized(false)
    {
    }

    // Structure to store staged log messages
    struct StagedLogMessage
    {
        Logger::Level level;
        std::string message;
    };

    // Structure for sink entries
    struct SinkEntry
    {
        std::function<void(const std::string&)> writeCallback;
        std::function<void()> flushCallback;
        bool isFileSink = false;
    };

    // ANSI color constants
    static constexpr const char* RESET       = "\033[0m";
    static constexpr const char* DEBUG_COLOR = "\033[37m";
    static constexpr const char* INFO_COLOR  = "\033[0m";
    static constexpr const char* WARN_COLOR  = "\033[33m";
    static constexpr const char* ERROR_COLOR = "\033[31m";

    // Member variables
    Logger::Level m_logLevel;
    bool m_enableTime;
    bool m_enableColor;
    bool m_enableLineInfo;
    bool m_initialized;
    std::mutex m_mutex;
    SmallVector<StagedLogMessage> m_stagedLogs;
    SmallVector<SinkEntry> m_sinks;

    // Helper methods
    void writeToSinks(const std::string& message)
    {
        for (auto& sink : m_sinks)
        {
            sink.writeCallback(message);
        }
    }

    std::string getCurrentTime()
    {
        auto t  = std::time(nullptr);
        auto tm = *std::localtime(&t);
        std::ostringstream oss;
        oss << std::put_time(&tm, "[%Y-%m-%d %H:%M:%S]");
        return oss.str();
    }

    const char* getLevelString(Logger::Level level) const
    {
        switch (level)
        {
        case Logger::Level::Debug:
            return "D";
        case Logger::Level::Info:
            return "I";
        case Logger::Level::Warn:
            return "W";
        case Logger::Level::Error:
            return "E";
        default:
            return "?";
        }
    }

    const char* getLevelColor(Logger::Level level) const
    {
        switch (level)
        {
        case Logger::Level::Debug:
            return DEBUG_COLOR;
        case Logger::Level::Info:
            return INFO_COLOR;
        case Logger::Level::Warn:
            return WARN_COLOR;
        case Logger::Level::Error:
            return ERROR_COLOR;
        default:
            return RESET;
        }
    }

    std::string formatLogMessage(Logger::Level level, std::string_view message)
    {
        std::ostringstream ss;

        if (m_enableTime)
        {
            ss << getCurrentTime();
        }

        if (m_enableColor)
        {
            ss << getLevelColor(level);
        }

        ss << " [" << getLevelString(level) << "] ";
        ss << message;

        if (m_enableColor)
        {
            ss << RESET;
        }

        ss << '\n';

        return ss.str();
    }

    // Core log implementation
    void log(Logger::Level level, std::string_view message)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        // Apply unified log level filtering
        if (level < m_logLevel)
            return;

        std::string logMessage = formatLogMessage(level, message);

        if (!m_initialized)
        {
            // Store the log message for later
            m_stagedLogs.push_back({level, std::move(logMessage)});
        }
        else
        {
            // Write directly to sinks
            writeToSinks(logMessage);
        }
    }

    void addSink(std::function<void(const std::string&)> writeFunc, std::function<void()> flushFunc, bool isFileSink)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_sinks.push_back(
            {.writeCallback = std::move(writeFunc), .flushCallback = std::move(flushFunc), .isFileSink = isFileSink});
    }
};

// Logger implementation using PIMPL

Logger::Logger()
    : m_impl(std::make_unique<LoggerImpl>())
{
    // Add default sinks
    addSink(ConsoleSink());
    addSink(FileSink("log.txt", true), true);
}

Logger::~Logger()                            = default;
Logger::Logger(Logger&&) noexcept            = default;
Logger& Logger::operator=(Logger&&) noexcept = default;

void Logger::initialize()
{
    std::lock_guard<std::mutex> lock(m_impl->m_mutex);

    if (m_impl->m_initialized)
    {
        return; // Already initialized
    }

    m_impl->m_initialized = true;

    // Process any staged logs that pass the current log level filter
    for (const auto& stagedLog : m_impl->m_stagedLogs)
    {
        // Only write logs that pass the current log level filter
        if (stagedLog.level >= m_impl->m_logLevel)
        {
            m_impl->writeToSinks(stagedLog.message);
        }
    }

    // Clear the staged logs after processing
    m_impl->m_stagedLogs.clear();
}

void Logger::log_impl(Level level, std::string_view message)
{
    m_impl->log(level, message);
}

void Logger::flush()
{
    std::lock_guard<std::mutex> lock(m_impl->m_mutex);
    for (auto& sink : m_impl->m_sinks)
    {
        sink.flushCallback();
    }
}

void Logger::addSinkWrapper(std::function<void(const std::string&)> writeFunc, std::function<void()> flushFunc,
                            bool isFileSink)
{
    m_impl->addSink(std::move(writeFunc), std::move(flushFunc), isFileSink);
}

void Logger::setLogFile(const std::string& filename)
{
    std::lock_guard<std::mutex> lock(m_impl->m_mutex);

    // Remove any existing file sinks
    auto it = m_impl->m_sinks.begin();
    while (it != m_impl->m_sinks.end())
    {
        if (it->isFileSink)
        {
            it = m_impl->m_sinks.erase(it);
        }
        else
        {
            ++it;
        }
    }

    // Add the new file sink with color stripping enabled
    addSink(FileSink(filename, true), true);
}

void Logger::setLogLevel(Level level)
{
    m_impl->m_logLevel = level;
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

void Logger::setEnableTime(bool value)
{
    m_impl->m_enableTime = value;
}

void Logger::setEnableColor(bool value)
{
    m_impl->m_enableColor = value;
}

void Logger::setEnableLineInfo(bool value)
{
    m_impl->m_enableLineInfo = value;
}

bool Logger::isInitialized() const
{
    return m_impl->m_initialized;
}

bool Logger::getEnableLineInfo() const
{
    return m_impl->m_enableLineInfo;
}

// Helper function to retrieve the logger from GlobalManager
Logger* getActiveLogger()
{
    return GlobalManager::instance().getSubsystem<Logger>(GlobalManager::LOGGER_NAME);
}

} // namespace aph
