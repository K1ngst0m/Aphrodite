#ifndef LOGGER_H_
#define LOGGER_H_

#include <reckless/severity_log.hpp>
#include <reckless/file_writer.hpp>
#include <reckless/stdout_writer.hpp>

namespace aph
{
class Logger
{
private:
    using log_t = reckless::severity_log<reckless::indent<4>,      // 4 spaces of indent
                                         ' ',                      // Field separator
                                         reckless::severity_field  // Show severity marker (D/I/W/E) first
                                         >;

    Logger() : m_fileWriter{"log.txt"}, m_logger{&m_stdcoutWriter} {}

public:
    // Delete copy constructor and assignment operator
    Logger(Logger const&)                  = delete;
    Logger&       operator=(Logger const&) = delete;
    static Logger& Get()
    {
        static Logger instance;
        return instance;
    }

public:
    void flush() {}

    template <typename... Args>
    void debug(std::string_view fmt, Args&&... args);

    template <typename... Args>
    void warn(std::string_view fmt, Args&&... args);

    template <typename... Args>
    void info(std::string_view fmt, Args&&... args);

    template <typename... Args>
    void error(std::string_view fmt, Args&&... args);

private:
    reckless::file_writer   m_fileWriter{"log.txt"};
    reckless::stdout_writer m_stdcoutWriter{};
    log_t                   m_logger;
};

template <typename... Args>
void Logger::debug(std::string_view fmt, Args&&... args)
{
    m_logger.debug(fmt.data(), std::forward<Args>(args)...);
}

template <typename... Args>
void Logger::warn(std::string_view fmt, Args&&... args)
{
    m_logger.warn(fmt.data(), std::forward<Args>(args)...);
}
template <typename... Args>
void Logger::info(std::string_view fmt, Args&&... args)
{
    m_logger.info(fmt.data(), std::forward<Args>(args)...);
}
template <typename... Args>
void Logger::error(std::string_view fmt, Args&&... args)
{
    m_logger.error(fmt.data(), std::forward<Args>(args)...);
}
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
