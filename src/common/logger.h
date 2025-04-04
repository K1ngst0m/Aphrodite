#pragma once

#include "common/smallVector.h"
#include <vector>
#include <mutex>
#include <sstream>
#include <filesystem>
#include <format>

namespace aph
{

template <typename T>
concept LogSinkConcept = requires(T t, const std::string& msg) {
    { t.write(msg) } -> std::same_as<void>;
    { t.flush() } -> std::same_as<void>;
};

class Logger
{
public:
    Logger();
    
    enum class Level : uint8_t
    {
        Debug = 0,
        Info = 1,
        Warn = 2,
        Error = 3,
        None = 4,
    };

    void setLogLevel(uint32_t level);
    void setLogLevel(Level level)
    {
        m_logLevel = level;
    }
    
    void setEnableTime(bool value)
    {
        m_enableTime = value;
    }
    
    void setEnableColor(bool value)
    {
        m_enableColor = value;
    }
    
    void setEnableLineInfo(bool value)
    {
        m_enableLineInfo = value;
    }
    
    bool getEnableLineInfo() const
    {
        return m_enableLineInfo;
    }

    template <LogSinkConcept Sink>
    void addSink(Sink&& sink, bool isFileSink = false)
    {
        auto sinkPtr = std::make_shared<std::decay_t<Sink>>(std::forward<Sink>(sink));
        m_sinks.push_back({ 
            .writeCallback = [sinkPtr](const std::string& msg) { sinkPtr->write(msg); },
            .flushCallback = [sinkPtr]() { sinkPtr->flush(); },
            .isFileSink = isFileSink
        });
    }

    void setLogFile(const std::string& filename);
    void flush();

    template <typename... Args>
    void debug(std::string_view fmt, Args&&... args)
    {
        if (m_logLevel <= Level::Debug)
            log(LogLevel::Debug, fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void warn(std::string_view fmt, Args&&... args)
    {
        if (m_logLevel <= Level::Warn)
            log(LogLevel::Warn, fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void info(std::string_view fmt, Args&&... args)
    {
        if (m_logLevel <= Level::Info)
            log(LogLevel::Info, fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void error(std::string_view fmt, Args&&... args)
    {
        if (m_logLevel <= Level::Error)
            log(LogLevel::Error, fmt, std::forward<Args>(args)...);
    }

private:
    enum class LogLevel
    {
        Debug,
        Info,
        Warn,
        Error
    };

    template <typename T>
    T toFormat(const T& val)
    {
        return val;
    }

    const char* toFormat(const char* val)
    {
        return val;
    }
    
    const char* toFormat(const std::string& val)
    {
        return val.c_str();
    }
    
    const char* toFormat(const std::filesystem::path& val)
    {
        return val.c_str();
    }
    
    const char* toFormat(std::string_view val)
    {
        return val.data();
    }

    static constexpr const char* RESET = "\033[0m";
    static constexpr const char* DEBUG_COLOR = "\033[37m";
    static constexpr const char* INFO_COLOR = "\033[0m";
    static constexpr const char* WARN_COLOR = "\033[33m";
    static constexpr const char* ERROR_COLOR = "\033[31m";

    const char* getLevelString(LogLevel level) const
    {
        switch (level)
        {
            case LogLevel::Debug: return "D";
            case LogLevel::Info: return "I";
            case LogLevel::Warn: return "W";
            case LogLevel::Error: return "E";
            default: return "?";
        }
    }

    const char* getLevelColor(LogLevel level) const
    {
        switch (level)
        {
            case LogLevel::Debug: return DEBUG_COLOR;
            case LogLevel::Info: return INFO_COLOR;
            case LogLevel::Warn: return WARN_COLOR;
            case LogLevel::Error: return ERROR_COLOR;
            default: return RESET;
        }
    }

    template <typename... Args>
    void log(LogLevel level, std::string_view fmt, Args&&... args)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

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
        
        constexpr size_t BUFFER_SIZE = 4096;
        std::vector<char> buffer(BUFFER_SIZE);
        int result = std::snprintf(buffer.data(), BUFFER_SIZE, fmt.data(), toFormat(args)...);
        
        if (result >= 0 && static_cast<size_t>(result) < BUFFER_SIZE) 
        {
            ss << buffer.data();
        } 
        else 
        {
            std::vector<char> dynamicBuffer(result + 1);
            std::snprintf(dynamicBuffer.data(), dynamicBuffer.size(), fmt.data(), toFormat(args)...);
            ss << dynamicBuffer.data();
        }
        
        if (m_enableColor)
        {
            ss << RESET;
        }
        
        ss << '\n';

        const std::string logMessage = ss.str();
        for (auto& sink : m_sinks)
        {
            sink.writeCallback(logMessage);
        }
    }

    std::string getCurrentTime();

    Level m_logLevel = Level::Info;
    bool m_enableTime = false;
    bool m_enableColor = true;
    bool m_enableLineInfo = true;
    std::mutex m_mutex;

    struct SinkEntry
    {
        std::function<void(const std::string&)> writeCallback;
        std::function<void()> flushCallback;
        bool isFileSink = false;
    };

    SmallVector<SinkEntry> m_sinks;
};

} // namespace aph

namespace aph::details
{
inline Logger& getLogger()
{
    static Logger logger{};
    return logger;
}
} // namespace aph::details

#define APH_LOGGER (::aph::details::getLogger())

inline void LOG_FLUSH()
{
    APH_LOGGER.flush();
}

#define GENERATE_LOG_FUNCS(TAG)                                                   \
    template <typename... Args>                                                   \
    void TAG##_LOG_DEBUG(std::string_view fmt, Args&&... args)                    \
    {                                                                             \
        std::string combined = std::format("[{}] {}", #TAG, fmt);                 \
        APH_LOGGER.debug(combined, std::forward<Args>(args)...);                  \
    }                                                                             \
    template <typename... Args>                                                   \
    void TAG##_LOG_WARN(std::string_view fmt, Args&&... args)                     \
    {                                                                             \
        std::string combined = std::format("[{}] {}", #TAG, fmt);                 \
        APH_LOGGER.warn(combined, std::forward<Args>(args)...);                   \
    }                                                                             \
    template <typename... Args>                                                   \
    void TAG##_LOG_INFO(std::string_view fmt, Args&&... args)                     \
    {                                                                             \
        std::string combined = std::format("[{}] {}", #TAG, fmt);                 \
        APH_LOGGER.info(combined, std::forward<Args>(args)...);                   \
    }                                                                             \
    template <typename... Args>                                                   \
    void TAG##_LOG_ERR(std::string_view fmt, Args&&... args)                      \
    {                                                                             \
        std::string combined = std::format("[{}] {}", #TAG, fmt);                 \
        APH_LOGGER.error(combined, std::forward<Args>(args)...);                  \
        APH_LOGGER.flush();                                                       \
    }

GENERATE_LOG_FUNCS(CM)
GENERATE_LOG_FUNCS(VK)
GENERATE_LOG_FUNCS(MM)
