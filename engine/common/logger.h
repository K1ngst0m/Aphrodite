#ifndef LOGGER_H_
#define LOGGER_H_

namespace aph
{
class Logger
{
public:
    enum class Level: uint8_t
    {
        Debug,
        Info,
        Warn,
        Error,
        None
    };

    void setLogLevel(Level level) { log_level = level; }

    Logger(Logger const&)            = delete;
    Logger& operator=(Logger const&) = delete;

    static Logger& Get()
    {
        static Logger instance;
        return instance;
    }

    void flush();

    template <typename... Args>
    void debug(std::string_view fmt, Args&&... args)
    {
        if(log_level <= Level::Debug)
            log("D", fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void warn(std::string_view fmt, Args&&... args)
    {
        if(log_level <= Level::Warn)
            log("W", fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void info(std::string_view fmt, Args&&... args)
    {
        if(log_level <= Level::Info)
            log("I", fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void error(std::string_view fmt, Args&&... args)
    {
        if(log_level <= Level::Error)
            log("E", fmt, std::forward<Args>(args)...);
    }

private:
    Logger();

    // conversion for most types
    template<typename T>
    T to_format(const T& val) {
        return val;
    }

    // specialization for std::string
    const char* to_format(const std::string& val) {
        return val.c_str();
    }

    template <typename... Args>
    void log(const char* level, std::string_view fmt, Args&&... args)
    {
        std::lock_guard<std::mutex> lock(mutex);

        std::ostringstream ss;
        ss << getCurrentTime() << " [" << level << "] ";
        char buffer[512];
        std::snprintf(buffer, sizeof(buffer), fmt.data(), to_format(args)...);
        ss << buffer << "\n";

        std::cout << ss.str();
        if(file_stream.is_open())
        {
            file_stream << ss.str();
        }
    }

    std::string getCurrentTime();

    Level         log_level;
    std::ofstream file_stream;
    std::mutex    mutex;
};

}  // namespace aph

#define LOG_FLUSH() \
    do \
    { \
        ::aph::Logger::Get().flush(); \
    } while(0)

#define CM_LOG_DEBUG(...) \
    do \
    { \
        ::aph::Logger::Get().debug("[APH] " __VA_ARGS__); \
    } while(0)
#define CM_LOG_WARN(...) \
    do \
    { \
        ::aph::Logger::Get().warn("[APH] " __VA_ARGS__); \
    } while(0)
#define CM_LOG_INFO(...) \
    do \
    { \
        ::aph::Logger::Get().info("[APH] " __VA_ARGS__); \
    } while(0)
#define CM_LOG_ERR(...) \
    do \
    { \
        ::aph::Logger::Get().error("[APH] " __VA_ARGS__); \
    } while(0)

#define VK_LOG_DEBUG(...) \
    do \
    { \
        ::aph::Logger::Get().debug("[VK] " __VA_ARGS__); \
    } while(0)
#define VK_LOG_WARN(...) \
    do \
    { \
        ::aph::Logger::Get().warn("[VK] " __VA_ARGS__); \
    } while(0)
#define VK_LOG_INFO(...) \
    do \
    { \
        ::aph::Logger::Get().info("[VK] " __VA_ARGS__); \
    } while(0)
#define VK_LOG_ERR(...) \
    do \
    { \
        ::aph::Logger::Get().error("[VK] " __VA_ARGS__); \
        ::aph::Logger::Get().flush(); \
    } while(0)

#endif  // LOGGER_H_
