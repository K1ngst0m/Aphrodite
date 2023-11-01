#ifndef LOGGER_H_
#define LOGGER_H_

#include "singleton.h"

namespace aph
{
class Logger: public Singleton<Logger>
{
public:
    Logger();
    enum class Level : uint8_t
    {
        Debug,
        Info,
        Warn,
        Error,
        None
    };

    void setLogLevel(Level level) { m_logLevel = level; }

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
        ss << getCurrentTime() << " [" << level << "] ";
        char buffer[512];
        std::snprintf(buffer, sizeof(buffer), fmt.data(), toFormat(args)...);
        ss << buffer << '\n';

        std::cout << ss.str();
        if(m_fileStream.is_open())
        {
            m_fileStream << ss.str();
        }
    }

    std::string getCurrentTime();

    Level         m_logLevel;
    std::ofstream m_fileStream;
    std::mutex    m_mutex;
};

}  // namespace aph

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
