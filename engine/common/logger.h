#pragma once

#include "singleton.h"

namespace aph
{

template <typename T>
concept LogSinkConcept = requires(T t, const std::string& msg) {
    { t.write(msg) } -> std::same_as<void>;
    { t.flush() } -> std::same_as<void>;
};

class Logger : public Singleton<Logger>
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

    template <LogSinkConcept Sink>
    void addSink(Sink&& sink)
    {
        auto sinkPtr = std::make_shared<std::decay_t<Sink>>(std::forward<Sink>(sink));
        m_sinks.push_back({ .writeCallback = [sinkPtr](const std::string& msg) { sinkPtr->write(msg); },
                            .flushCallback = [sinkPtr]() { sinkPtr->flush(); } });
    }

    void flush();

    template <typename... Args>
    void debug(std::string_view fmt, Args&&... args)
    {
        if (m_logLevel <= Level::Debug)
            log("D", fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void warn(std::string_view fmt, Args&&... args)
    {
        if (m_logLevel <= Level::Warn)
            log("W", fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void info(std::string_view fmt, Args&&... args)
    {
        if (m_logLevel <= Level::Info)
            log("I", fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void error(std::string_view fmt, Args&&... args)
    {
        if (m_logLevel <= Level::Error)
            log("E", fmt, std::forward<Args>(args)...);
    }

private:
    // conversion for most types
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

    template <typename... Args>
    void log(const char* level, std::string_view fmt, Args&&... args)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        std::ostringstream ss;
        if (m_enableTime)
        {
            ss << getCurrentTime();
        }
        ss << " [" << level << "] ";
        char buffer[1024];
        std::snprintf(buffer, sizeof(buffer), fmt.data(), toFormat(args)...);
        ss << buffer << '\n';

        for (auto& sink : m_sinks)
        {
            sink.writeCallback(ss.str());
        }
    }

    std::string getCurrentTime();

    Level m_logLevel;
    bool m_enableTime = false;
    std::mutex m_mutex;

    struct SinkEntry
    {
        std::function<void(const std::string&)> writeCallback;
        std::function<void()> flushCallback;
    };

    std::vector<SinkEntry> m_sinks;
};

} // namespace aph

inline void LOG_FLUSH()
{
    ::aph::Logger::GetInstance().flush();
}

#define GENERATE_LOG_FUNCS(TAG)                                                    \
    template <typename... Args>                                                    \
    void TAG##_LOG_DEBUG(std::string_view fmt, Args&&... args)                     \
    {                                                                              \
        std::string combined = std::string("[") + #TAG + "] " + std::string(fmt);  \
        ::aph::Logger::GetInstance().debug(combined, std::forward<Args>(args)...); \
    }                                                                              \
    template <typename... Args>                                                    \
    void TAG##_LOG_WARN(std::string_view fmt, Args&&... args)                      \
    {                                                                              \
        std::string combined = std::string("[") + #TAG + "] " + std::string(fmt);  \
        ::aph::Logger::GetInstance().warn(combined, std::forward<Args>(args)...);  \
    }                                                                              \
    template <typename... Args>                                                    \
    void TAG##_LOG_INFO(std::string_view fmt, Args&&... args)                      \
    {                                                                              \
        std::string combined = std::string("[") + #TAG + "] " + std::string(fmt);  \
        ::aph::Logger::GetInstance().info(combined, std::forward<Args>(args)...);  \
    }                                                                              \
    template <typename... Args>                                                    \
    void TAG##_LOG_ERR(std::string_view fmt, Args&&... args)                       \
    {                                                                              \
        std::string combined = std::string("[") + #TAG + "] " + std::string(fmt);  \
        ::aph::Logger::GetInstance().error(combined, std::forward<Args>(args)...); \
        ::aph::Logger::GetInstance().flush();                                      \
    }

GENERATE_LOG_FUNCS(CM)
GENERATE_LOG_FUNCS(VK)
GENERATE_LOG_FUNCS(MM)
