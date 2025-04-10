#pragma once

#include "logger.h"
#include "macros.h"
#include <cassert>
#include <signal.h>
#include <source_location>
#include <stdexcept>
#include <string>

#ifdef _MSC_VER
#include <intrin.h>
#endif

namespace aph
{

APH_ALWAYS_INLINE void DebugBreak()
{
#if defined(_MSC_VER)
    __debugbreak();
#elif defined(__linux__) || defined(__APPLE__)
    raise(SIGTRAP);
#else
    assert(false && "Debugger break triggered");
#endif
}

#ifdef APH_DEBUG
template <typename T>
    requires requires(T t) {
        { static_cast<bool>(t) } -> std::same_as<bool>;
    }
APH_ALWAYS_INLINE void APH_ASSERT(const T& condition, const std::source_location& loc = std::source_location::current())
{
    if (!static_cast<bool>(condition))
    {
        CM_LOG_ERR("Error at %s:%d.", loc.file_name(), loc.line());
        LOG_FLUSH();
        ::aph::DebugBreak();
    }
}

template <typename T>
    requires requires(T t) {
        { static_cast<bool>(t) } -> std::same_as<bool>;
    }
APH_ALWAYS_INLINE void APH_ASSERT(const T& condition, const std::string& msg,
                                  const std::source_location& loc = std::source_location::current())
{
    if (!static_cast<bool>(condition))
    {
        CM_LOG_ERR("Error at %s:%d. %s", loc.file_name(), loc.line(), msg.c_str());
        LOG_FLUSH();
        ::aph::DebugBreak();
    }
}
#else
template <typename T>
inline void APH_ASSERT(const T& condition) {};

template <typename T>
inline void APH_ASSERT(const T& condition, const std::string& msg) {};
#endif

template <typename T>
APH_ALWAYS_INLINE void ASSERT(const T& condition, const std::source_location& loc = std::source_location::current())
{
    APH_ASSERT(condition, loc);
}

} // namespace aph
