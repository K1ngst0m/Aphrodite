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
    using log_t = reckless::severity_log<reckless::indent<4>,       // 4 spaces of indent
                                         ' ',                       // Field separator
                                         reckless::severity_field,  // Show severity marker (D/I/W/E) first
                                         reckless::timestamp_field  // Then timestamp field
                                         >;

    Logger() : m_writer{"log.txt"}, m_logger{&m_writer} {}
public:
    static Logger* g_logger;
    static log_t* Get()
    {
        if (g_logger == nullptr){
            g_logger = new Logger();
        }
        return &g_logger->m_logger;
    }

private:
    reckless::file_writer m_writer{"log.txt"};
    log_t                 m_logger;
};
inline Logger * Logger::g_logger = nullptr;
}  // namespace aph

#endif  // LOGGER_H_
