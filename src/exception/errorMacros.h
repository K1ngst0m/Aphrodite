#pragma once

#include "common/result.h"
#include "errorHandler.h"
#include <source_location>

namespace aph
{

// Enhanced version of APH_VR that reports errors with stack traces
#ifdef APH_DEBUG
#define APH_VERIFY_RESULT(expr)                                                           \
    do                                                                                    \
    {                                                                                     \
        const ::aph::Result& _result = (expr);                                            \
        if (!_result.success())                                                           \
        {                                                                                 \
            ::aph::ErrorHandler::reportFatalError(_result.getCode(), _result.toString()); \
            ::aph::ASSERT(false && "Fatal error encountered");                            \
        }                                                                                 \
    } while (0)
#else
#define APH_VERIFY_RESULT(expr) ::aph::APH_VERIFY_RESULT(expr)
#endif

// Non-fatal error reporting with stack trace
#define APH_REPORT_ERROR(code, message) ::aph::ErrorHandler::reportError(code, message)

// Fatal error reporting with stack trace
#define APH_FATAL_ERROR(code, message) ::aph::ErrorHandler::reportFatalError(code, message)

// Utility function for verifying Expected results
template <typename T>
inline auto VerifyExpected(const Expected<T>& expected,
                           const std::source_location& location = std::source_location::current()) -> bool
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
#define APH_RETURN_IF_ERROR(expr)     \
    do                                \
    {                                 \
        const auto& _result = (expr); \
        if (!_result.success())       \
        {                             \
            return _result;           \
        }                             \
    } while (0)

#define APH_EXPECTED_RETURN_IF_ERROR(expr) \
    do                                     \
    {                                      \
        const auto& _result = (expr);      \
        if (!_result.success())            \
        {                                  \
            return _result;                \
        }                                  \
    } while (0)

} // namespace aph
