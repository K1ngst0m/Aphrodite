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
    static Logger* g_logger;
    static log_t*  Get()
    {
        if(g_logger == nullptr)
        {
            g_logger = new Logger();
        }
        return &g_logger->m_logger;
    }

private:
    reckless::file_writer   m_fileWriter{"log.txt"};
    reckless::stdout_writer m_stdcoutWriter{};
    log_t                   m_logger;
};
inline Logger* Logger::g_logger = nullptr;
}  // namespace aph

#define CM_LOG_DEBUG(...) do { ::aph::Logger::Get()->debug("[APH] " __VA_ARGS__); } while(0)
#define CM_LOG_WARN(...) do { ::aph::Logger::Get()->warn("[APH] " __VA_ARGS__); } while(0)
#define CM_LOG_INFO(...) do { ::aph::Logger::Get()->info("[APH] " __VA_ARGS__); } while(0)
#define CM_LOG_ERR(...) do { ::aph::Logger::Get()->error("[APH] " __VA_ARGS__); } while(0)

#define VK_LOG_DEBUG(...) do { ::aph::Logger::Get()->debug("[VK] " __VA_ARGS__); } while(0)
#define VK_LOG_WARN(...) do { ::aph::Logger::Get()->warn("[VK] " __VA_ARGS__); } while(0)
#define VK_LOG_INFO(...) do { ::aph::Logger::Get()->info("[VK] " __VA_ARGS__); } while(0)
#define VK_LOG_ERR(...) do { ::aph::Logger::Get()->error("[VK] " __VA_ARGS__); } while(0)

#endif  // LOGGER_H_
