#pragma once

#include "common/hash.h"
#include "common/logger.h"
#include "common/result.h"

#include <functional>
#include <string_view>

namespace aph
{

// Forward declarations
class StackTraceProvider;

enum class FatalErrorAction
{
    Abort, // Immediately abort
    Continue, // Try to continue execution
    Custom // Use custom handler
};

class ErrorHandler
{
public:
    static void initialize();
    static void shutdown();

    // Register fatal error callback
    static void setFatalErrorAction(FatalErrorAction action);
    static void setCustomFatalErrorHandler(
        std::function<void(Result::Code code, std::string_view message, const std::string& stackTrace)> handler);

    // Error reporting methods
    static void reportError(Result::Code code, std::string_view message);
    static void reportFatalError(Result::Code code, std::string_view message);

    // Used internally by signal handlers
    static void handleSignal(int signal, void* context);

    // Add custom signal handlers
    static void registerSignalHandler(int signal, std::function<void(int, void*)> handler);

private:
    static bool s_initialized;
    static FatalErrorAction s_fatalErrorAction;
    static std::function<void(Result::Code, std::string_view, const std::string&)> s_customFatalHandler;
    static HashMap<int, std::function<void(int, void*)>> s_customSignalHandlers;

    static void setupSignalHandlers();
    static Result::Code mapSignalToErrorCode(int signal);
    static const char* signalToString(int signal);
};

} // namespace aph