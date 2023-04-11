#ifndef LOGGER_H_
#define LOGGER_H_

#include <algorithm>
#include <chrono>
#include <cmath>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace aph
{
#define LOG_SILENT 0
#define LOG_ERR 1
#define LOG_ERROR 1
#define LOG_WARN 2
#define LOG_WARNING 2
#define LOG_INFO 3
#define LOG_TIME 4
#define LOG_DEBUG 5
#define LOG_DEFAULT 4

#define LOG_INIT_COUT(obj_name) Logger obj_name(std::cout, __PRETTY_FUNCTION__)
#define LOG_INIT_CERR(obj_name) Logger obj_name(std::cerr, __PRETTY_FUNCTION__)
#define LOG_INIT_CLOG(obj_name) Logger obj_name(std::clog, __PRETTY_FUNCTION__)
#define LOG_INIT_CUSTOM(X) Logger log((X), __PRETTY_FUNCTION__)

#ifdef APH_LOG_NO_COLORS

#    define APH_LOG_TIME "[ TIME    ]"
#    define APH_LOG_DEBUG "[ DEBUG   ]"
#    define APH_LOG_ERROR "[ ERROR   ]"
#    define APH_LOG_WARNING "[ WARNING ]"
#    define APH_LOG_INFO "[ INFO    ]"

#else

#    define APH_LOG_TIME "\033[0;35m[ TIME    ]\033[0;0m"
#    define APH_LOG_DEBUG "[ DEBUG   ]"
#    define APH_LOG_ERROR "\033[0;31m[ ERROR   ]\033[0;0m"
#    define APH_LOG_WARNING "\033[0;33m[ WARNING ]\033[0;0m"
#    define APH_LOG_INFO "\033[0;34m[ INFO    ]\033[0;0m"

#endif

class Logger
{
public:
    Logger(std::ostream& /*f*/, unsigned /*ll*/, std::string /*n*/);
    Logger(std::ostream& /*f*/, std::string n);

    Logger& operator()(unsigned ll);
    void    add_snapshot(const std::string& n, bool quiet = true);
    void    time_since_start();
    void    time_since_last_snap();
    void    time_since_snap(const std::string& /*s*/);
    void    flush();
    void    set_log_level(unsigned ll);

    static unsigned& _loglevel();

public:
    template <typename T>
    friend Logger&     operator<<(Logger& l, const T& s);
    friend std::string prep_level(Logger& l);
    friend std::string prep_time(Logger& l);
    friend std::string prep_name(Logger& l);

private:
    time_t                   _now;
    time_t                   _start;
    std::vector<time_t>      _snaps;
    std::vector<std::string> _snap_ns;
    unsigned                 _message_level;
    std::ostream&            _fac;
    std::string              _name;
};

}  // namespace aph

#endif  // LOGGER_H_
