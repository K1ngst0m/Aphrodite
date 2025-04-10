#include "errorHandler.h"
#include "stackTraceProvider.h"

// Include backward-cpp
#include <backward.hpp>

#include <csignal>
#include <cstdlib>

// Forward declaration for function call
extern "C" int sigemptyset(sigset_t* set);
extern "C" int sigaction(int signum, const struct sigaction* act, struct sigaction* oldact);

// Implementation function to be called from errorHandler.cpp
// Must be in global namespace to match the forward declaration
void initializeCrashHandler();

namespace aph
{

// Custom signal handler function that integrates with our ErrorHandler
void custom_signal_handler(int sig, siginfo_t* info, void* _ctx)
{
    // Call our error handler
    ErrorHandler::handleSignal(sig, _ctx);

    // backward-cpp will call its own handler after this if we don't abort
    // If we're still alive at this point, terminate
    std::abort();
}

// This class handles the integration with backward-cpp
class CrashHandler
{
private:
    // Static instance of SignalHandling with empty signals
    // We'll initialize this outside the class
    static backward::SignalHandling s_signalHandling;

public:
    static void initialize()
    {
        // Define the signals we want to handle
        std::vector<int> signals = backward::SignalHandling::make_default_signals();

        // Set up signal handlers for each signal
        for (int sig : signals)
        {
            struct sigaction action;
            memset(&action, 0, sizeof(action));

            // Use our custom handler
            action.sa_sigaction = &custom_signal_handler;
            action.sa_flags     = SA_SIGINFO | SA_ONSTACK;

            // Call the sigemptyset function (now declared properly)
            sigemptyset(&action.sa_mask);
            // Call the sigaction function (now declared properly)
            sigaction(sig, &action, nullptr);
        }

        CM_LOG_INFO("Signal handlers registered for crash handling");
    }
};

// Initialize the static member
// Fixed: Use proper syntax for static member initialization
// Use an empty vector as constructor argument
std::vector<int> empty_signals;
backward::SignalHandling CrashHandler::s_signalHandling = backward::SignalHandling(empty_signals);

} // namespace aph

// Implementation of the function declared at the top of the file
// This is now in the global namespace
void initializeCrashHandler()
{
    aph::CrashHandler::initialize();
}
