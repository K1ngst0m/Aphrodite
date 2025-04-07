#include "errorHandler.h"
#include "stackTraceProvider.h"

#include <csignal>
#include <cstdlib>

// Forward declaration for the crash handler initialization
void initializeCrashHandler();

namespace aph
{

bool ErrorHandler::s_initialized = false;
FatalErrorAction ErrorHandler::s_fatalErrorAction = FatalErrorAction::Abort;
std::function<void(Result::Code, std::string_view, const std::string&)> ErrorHandler::s_customFatalHandler = nullptr;
HashMap<int, std::function<void(int, void*)>> ErrorHandler::s_customSignalHandlers;

void ErrorHandler::initialize()
{
    if (s_initialized)
        return;

    // Initialize the stack trace provider
    StackTraceProvider::initialize();

    // Setup signal handlers
    setupSignalHandlers();

    s_initialized = true;

    CM_LOG_INFO("Error handling system initialized");
}

void ErrorHandler::shutdown()
{
    if (!s_initialized)
        return;

    // Reset signal handlers to default
    std::signal(SIGSEGV, SIG_DFL);
    std::signal(SIGILL, SIG_DFL);
    std::signal(SIGABRT, SIG_DFL);
    std::signal(SIGFPE, SIG_DFL);

    s_initialized = false;
    s_customFatalHandler = nullptr;
    s_customSignalHandlers.clear();
}

void ErrorHandler::setFatalErrorAction(FatalErrorAction action)
{
    s_fatalErrorAction = action;
}

void ErrorHandler::setCustomFatalErrorHandler(
    std::function<void(Result::Code, std::string_view, const std::string&)> handler)
{
    s_customFatalHandler = std::move(handler);
    s_fatalErrorAction = FatalErrorAction::Custom;
}

void ErrorHandler::reportError(Result::Code code, std::string_view message)
{
    // Log error with context but don't crash
    std::string stackTrace = StackTraceProvider::captureStackTrace();

    CM_LOG_ERR("%s: %s", Result(code).toString(), message);
    CM_LOG_ERR("Stack trace:\n%s", stackTrace.c_str());
    LOG_FLUSH();
}

void ErrorHandler::reportFatalError(Result::Code code, std::string_view message)
{
    // Capture stack trace
    std::string stackTrace = StackTraceProvider::captureStackTrace();

    // Log the error
    CM_LOG_ERR("FATAL: %s: %s", Result(code).toString(), message);
    CM_LOG_ERR("Stack trace:\n%s", stackTrace.c_str());
    LOG_FLUSH();

    // Handle according to the configured action
    switch (s_fatalErrorAction)
    {
    case FatalErrorAction::Abort:
        std::abort();
        break;

    case FatalErrorAction::Custom:
        if (s_customFatalHandler)
        {
            s_customFatalHandler(code, message, stackTrace);
            std::abort();
        }
        break;

    case FatalErrorAction::Continue:
        // Do nothing, allow execution to continue
        break;
    }
}

void ErrorHandler::handleSignal(int signal, void* context)
{
    // Check for custom handler first
    auto it = s_customSignalHandlers.find(signal);
    if (it != s_customSignalHandlers.end() && it->second)
    {
        it->second(signal, context);
        return;
    }

    // Default handling for signals
    Result::Code errorCode = mapSignalToErrorCode(signal);
    std::string signalStr = signalToString(signal);
    std::string stackTrace = StackTraceProvider::captureStackTraceFromSignal(context);

    // Log the signal and stack trace
    CM_LOG_ERR("Caught signal %s", signalStr.c_str());
    CM_LOG_ERR("Stack trace:\n%s", stackTrace.c_str());
    LOG_FLUSH();

    // Handle fatal signal based on configured action
    if (s_fatalErrorAction == FatalErrorAction::Custom && s_customFatalHandler)
    {
        s_customFatalHandler(errorCode, signalStr, stackTrace);
    }

    // We always abort for signals since they indicate serious issues
    std::abort();
}

void ErrorHandler::registerSignalHandler(int signal, std::function<void(int, void*)> handler)
{
    s_customSignalHandlers[signal] = std::move(handler);
}

void ErrorHandler::setupSignalHandlers()
{
    // Call the crash handler initialization function - fixed namespace
    initializeCrashHandler();
}

Result::Code ErrorHandler::mapSignalToErrorCode(int signal)
{
    // Map OS signals to our Result::Code enum
    switch (signal)
    {
    case SIGSEGV:
    case SIGBUS:
    case SIGILL:
    case SIGFPE:
    case SIGABRT:
        return Result::Code::RuntimeError;
    default:
        return Result::Code::RuntimeError;
    }
}

const char* ErrorHandler::signalToString(int signal)
{
    switch (signal)
    {
    case SIGSEGV:
        return "SIGSEGV (Segmentation Violation)";
    case SIGILL:
        return "SIGILL (Illegal Instruction)";
    case SIGABRT:
        return "SIGABRT (Abort)";
    case SIGFPE:
        return "SIGFPE (Floating Point Exception)";
    case SIGBUS:
        return "SIGBUS (Bus Error)";
    default:
        return "Unknown Signal";
    }
}

} // namespace aph
