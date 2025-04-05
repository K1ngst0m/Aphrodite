#pragma once

#include "common/result.h"
#include "exception/errorHandler.h"
#include <source_location>

namespace aph
{

// Enhanced version of APH_VR that reports errors with stack traces
#ifdef APH_DEBUG
#define APH_VERIFY_RESULT(expr)                                                         \
    do                                                                                  \
    {                                                                                   \
        ::aph::Result _result = (expr);                                                  \
        if (!_result.success())                                                          \
        {                                                                               \
            ::aph::ErrorHandler::reportFatalError(_result.getCode(), _result.toString()); \
            ::aph::ASSERT(false && "Fatal error encountered");                      \
        }                                                                               \
    } while (0)
#else
#define APH_VERIFY_RESULT(expr) ::aph::APH_VR(expr)
#endif

// Non-fatal error reporting with stack trace
#define APH_REPORT_ERROR(code, message) ::aph::ErrorHandler::reportError(code, message)

// Fatal error reporting with stack trace
#define APH_FATAL_ERROR(code, message) ::aph::ErrorHandler::reportFatalError(code, message)

// Utility function for verifying Expected results
template <typename T>
inline bool VerifyExpected(const Expected<T>& expected,
                           const std::source_location& location = std::source_location::current())
{
    if (!expected.success())
    {
        const auto& error = expected.error();
        CM_LOG_ERR("Expected error: %s at %s:%d", error.message.c_str(), location.file_name(), location.line());
        return false;
    }
    return true;
}

// Convenience macro that returns on failure
#define APH_RETURN_IF_ERROR(expr) \
    do                            \
    {                             \
        auto result = (expr);     \
        if (!result.success())    \
        {                         \
            return result;        \
        }                         \
    } while (0)

} // namespace aph
