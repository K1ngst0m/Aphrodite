#ifndef LOGGER_H_
#define LOGGER_H_

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
    void setLogLevel(Level level) { m_logLevel = level; }
    void setEnableTime(bool value) { m_enableTime = value; }

    template <LogSinkConcept Sink>
    void addSink(Sink&& sink)
    {
        auto sinkPtr = std::make_shared<std::decay_t<Sink>>(std::forward<Sink>(sink));
        m_sinks.push_back({
            .writeCallback = [sinkPtr](const std::string & msg) {
                sinkPtr->write(msg);
            },
            .flushCallback = [sinkPtr]() {
                sinkPtr->flush();
            }
        });
    }

    void flush();

    template <typename... Args>
    void debug(std::string_view fmt, Args&&... args)
    {
        if(m_logLevel <= Level::Debug)
            log("D", fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void warn(std::string_view fmt, Args&&... args)
    {
        if(m_logLevel <= Level::Warn)
            log("W", fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void info(std::string_view fmt, Args&&... args)
    {
        if(m_logLevel <= Level::Info)
            log("I", fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void error(std::string_view fmt, Args&&... args)
    {
        if(m_logLevel <= Level::Error)
            log("E", fmt, std::forward<Args>(args)...);
    }

private:
    // conversion for most types
    template <typename T>
    T toFormat(const T& val)
    {
        return val;
    }

    const char* toFormat(const char* val) { return val; }
    const char* toFormat(const std::string& val) { return val.c_str(); }
    const char* toFormat(const std::filesystem::path& val) { return val.c_str(); }
    const char* toFormat(std::string_view val) { return val.data(); }

    template <typename... Args>
    void log(const char* level, std::string_view fmt, Args&&... args)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        std::ostringstream ss;
        if(m_enableTime)
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

    Level         m_logLevel;
    bool          m_enableTime = false;
    std::mutex    m_mutex;

    struct SinkEntry
    {
        std::function<void(const std::string&)> writeCallback;
        std::function<void()> flushCallback;
    };

    std::vector<SinkEntry> m_sinks;
};

}  // namespace aph

#define LOG_SETUP_LEVEL_DEBUG() \
    do \
    { \
        ::aph::Logger::GetInstance().setLogLevel(::aph::Logger::Level::Debug); \
    } while(0)
#define LOG_SETUP_LEVEL_INFO() \
    do \
    { \
        ::aph::Logger::GetInstance().setLogLevel(::aph::Logger::Level::Info); \
    } while(0)
#define LOG_SETUP_LEVEL_WARN() \
    do \
    { \
        ::aph::Logger::GetInstance().setLogLevel(::aph::Logger::Level::Warn); \
    } while(0)
#define LOG_SETUP_LEVEL_ERR() \
    do \
    { \
        ::aph::Logger::GetInstance().setLogLevel(::aph::Logger::Level::Error); \
    } while(0)
#define LOG_SETUP_LEVEL_NONE() \
    do \
    { \
        ::aph::Logger::GetInstance().setLogLevel(::aph::Logger::Level::None); \
    } while(0)

#define LOG_FLUSH() \
    do \
    { \
        ::aph::Logger::GetInstance().flush(); \
    } while(0)

#define CM_LOG_DEBUG(...) \
    do \
    { \
        ::aph::Logger::GetInstance().debug("[APH] " __VA_ARGS__); \
    } while(0)
#define CM_LOG_WARN(...) \
    do \
    { \
        ::aph::Logger::GetInstance().warn("[APH] " __VA_ARGS__); \
    } while(0)
#define CM_LOG_INFO(...) \
    do \
    { \
        ::aph::Logger::GetInstance().info("[APH] " __VA_ARGS__); \
    } while(0)
#define CM_LOG_ERR(...) \
    do \
    { \
        ::aph::Logger::GetInstance().error("[APH] " __VA_ARGS__); \
    } while(0)

#define VK_LOG_DEBUG(...) \
    do \
    { \
        ::aph::Logger::GetInstance().debug("[VK] " __VA_ARGS__); \
    } while(0)
#define VK_LOG_WARN(...) \
    do \
    { \
        ::aph::Logger::GetInstance().warn("[VK] " __VA_ARGS__); \
    } while(0)
#define VK_LOG_INFO(...) \
    do \
    { \
        ::aph::Logger::GetInstance().info("[VK] " __VA_ARGS__); \
    } while(0)
#define VK_LOG_ERR(...) \
    do \
    { \
        ::aph::Logger::GetInstance().error("[VK] " __VA_ARGS__); \
        ::aph::Logger::GetInstance().flush(); \
    } while(0)

#endif  // LOGGER_H_
