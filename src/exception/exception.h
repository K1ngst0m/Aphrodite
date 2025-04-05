#pragma once

#include "errorHandler.h"
#include "errorMacros.h"
#include "stackTraceProvider.h"

#include <source_location>
#include <string_view>

namespace aph
{
// Initialize error handling system with default settings
inline void initializeErrorHandling()
{
    ErrorHandler::initialize();
    StackTraceProvider::setMaxStackDepth(64);
    StackTraceProvider::setSkipCommonFrames(true);
    // Using the default project root "Aphrodite" from StackTraceProvider initialization
}

// Initialize error handling system with custom settings and project root path
inline void initializeErrorHandling(bool abortOnFatalError, int stackDepth = 64,
                                    const std::string& projectRoot = "Aphrodite")
{
    ErrorHandler::initialize();
    StackTraceProvider::setMaxStackDepth(stackDepth);
    StackTraceProvider::setSkipCommonFrames(true);

    // Always set the project root, whether it's the default or custom
    StackTraceProvider::setProjectRootPath(projectRoot);

    if (!abortOnFatalError)
    {
        ErrorHandler::setFatalErrorAction(FatalErrorAction::Continue);
    }
}

// Explicitly set the project root path for relative file paths in stack traces
inline void setStackTraceProjectRoot(const std::string& projectRoot)
{
    StackTraceProvider::setProjectRootPath(projectRoot);
}

// Shutdown error handling system
inline void shutdownErrorHandling()
{
    ErrorHandler::shutdown();
}

// Capture and return the current stack trace as a string
inline std::string getStackTrace(int skipFrames = 1)
{
    return StackTraceProvider::captureStackTrace(skipFrames);
}

// Report a non-fatal error with the current stack trace
inline void reportError(Result::Code code, std::string_view message,
                        const std::source_location& loc = std::source_location::current())
{
    CM_LOG_ERR("Error at %s:%d: %s", loc.file_name(), loc.line(), message.data());
    ErrorHandler::reportError(code, message);
}

// Handle a signal (for use in custom signal handlers)
inline void handleSignal(int signal, void* context)
{
    ErrorHandler::handleSignal(signal, context);
}

} // namespace aph