#include "logger.h"

namespace aph
{
template <typename T>
std::string format_duration(T xms)
{
    uint64_t seconds = static_cast<uint64_t>(xms);
    uint64_t days    = 0;
    uint64_t hours   = 0;
    uint64_t minutes = 0;

    if(seconds >= 86400)
    {
        days = seconds / 86400;
        seconds -= (days * 86400);
    }
    if(seconds >= 3600)
    {
        hours = seconds / 3600;
        seconds -= (hours * 3600);
    }
    if(seconds >= 60)
    {
        minutes = seconds / 60;
        seconds -= (minutes * 60);
    }

    std::stringstream ss;
    if(days > 0) { ss << std::setfill('0') << std::setw(2) << days << '-'; }
    if(hours > 0) { ss << std::setfill('0') << std::setw(2) << hours << ':'; }
    if(minutes > 0) { ss << std::setfill('0') << std::setw(2) << minutes << ':'; }
    // Always display seconds no matter what
    ss << std::setfill('0') << std::setw(2) << seconds;
    return ss.str();
}

std::string prep_level(Logger& l)
{
    switch(l._message_level)
    {
    case LOG_ERR: return APH_LOG_ERROR; break;
    case LOG_WARN: return APH_LOG_WARNING; break;
    case LOG_INFO: return APH_LOG_INFO; break;
    case LOG_DEBUG: return APH_LOG_DEBUG; break;
    case LOG_TIME: return APH_LOG_TIME; break;
    default: return "";
    }
    return "";
}

std::string prep_time(Logger& l)
{
    time(&l._now);
    struct tm* t;
    t = localtime(&l._now);
    std::string s, m, h, D, M, Y;
    s = std::to_string(t->tm_sec);
    m = std::to_string(t->tm_min);
    h = std::to_string(t->tm_hour);
    D = std::to_string(t->tm_mday);
    M = std::to_string(t->tm_mon + 1);
    Y = std::to_string(t->tm_year + 1900);

    if(t->tm_sec < 10) s = "0" + s;
    if(t->tm_min < 10) m = "0" + m;
    if(t->tm_hour < 10) h = "0" + h;
    if(t->tm_mday < 10) D = "0" + D;
    if(t->tm_mon + 1 < 10) M = "0" + M;

    std::string ret = "[ " + Y + "-" + M + "-" + D + "T" + h + ":" + m + ":" + s + " ]";

    return ret;
}

std::string prep_name(Logger& l) { return "[ " + l._name + " ]"; }

template <typename T>
Logger& operator<<(Logger& l, const T& s)
{
    if(l._message_level <= Logger::_loglevel())
    {
        l._fac << s;
        return l;
    }
    return l;
}

Logger::Logger(std::ostream& f, std::string n) : _message_level(LOG_SILENT), _fac(f), _name(std::move(n))
{
    time(&_now);
    time(&_start);
}

Logger::Logger(std::ostream& f, unsigned ll, std::string n) : _message_level(LOG_SILENT), _fac(f), _name(std::move(n))
{
    time(&_now);
    time(&_start);
    _loglevel() = ll;
}

Logger& Logger::operator()(unsigned ll)
{
    _message_level = ll;
    if(_message_level <= _loglevel()) { _fac << prep_level(*this) << prep_time(*this) << prep_name(*this) << ": "; }
    return *this;
}

void Logger::time_since_start()
{
    if(_loglevel() >= LOG_TIME)
    {
        time(&_now);
        _message_level = LOG_TIME;
        _fac << prep_level(*this) << prep_time(*this) << prep_name(*this) << ": " << difftime(_now, _start)
             << "s since instantiation\n";
    }
}

void Logger::time_since_last_snap()
{
    if(_loglevel() >= LOG_TIME && !_snap_ns.empty())
    {
        time(&_now);
        _message_level = LOG_TIME;
        _fac << prep_level(*this) << prep_time(*this) << prep_name(*this) << ": " << difftime(_now, _snaps.back())
             << "s since snap '" << _snap_ns.back() << "'\n";
    }
}

void Logger::time_since_snap(const std::string& s)
{
    if(_loglevel() >= LOG_TIME)
    {
        time(&_now);
        auto it = find(_snap_ns.begin(), _snap_ns.end(), s);
        if(it == _snap_ns.end())
        {
            _message_level = LOG_WARN;
            _fac << prep_level(*this) << prep_time(*this) << prep_name(*this) << ": "
                 << "Could not find snapshot " << s << '\n';
            return;
        }
        unsigned long dist = std::distance(_snap_ns.begin(), it);

        _message_level = LOG_TIME;
        _fac << prep_level(*this) << prep_time(*this) << prep_name(*this) << ": " << difftime(_now, _snaps[dist])
             << "s since snap '" << _snap_ns[dist] << "'\n";
    }
}

void Logger::add_snapshot(const std::string& n, bool quiet)
{
    time_t now;
    time(&now);
    _snaps.push_back(now);
    _snap_ns.push_back(n);
    if(_loglevel() >= LOG_TIME && !quiet)
        _fac << APH_LOG_TIME << prep_time(*this) << prep_name(*this) << ": Added snap '" << n << "'\n";
}

unsigned& Logger::_loglevel()
{
    static unsigned _ll_internal = LOG_DEFAULT;
    return _ll_internal;
};

void Logger::set_log_level(unsigned ll) { _loglevel() = ll; }

void Logger::flush() { _fac.flush(); }

}  // namespace aph
