#pragma once

#include <filesystem>
#include <string>

// Platform support indicators
#if !defined(_WIN32) && !defined(__linux__)
#error "Only Windows and Linux platforms are supported"
#endif

// Forward declaration to avoid including backward.hpp in the header
namespace backward
{
class StackTrace;
}

namespace aph
{

class StackTraceProvider
{
public:
    // Initialize stack trace system
    static void initialize();

    // Capture current stack trace
    static std::string captureStackTrace(int skipFrames = 1);

    // Capture stack trace from signal context
    static std::string captureStackTraceFromSignal(void* context);

    // Configure stack trace options
    static void setMaxStackDepth(int depth);
    static void setSkipCommonFrames(bool skip);

    // Set the project root path for making paths relative
    static void setProjectRootPath(const std::string& path);

private:
    static int s_maxStackDepth;
    static bool s_skipCommonFrames;
    static bool s_initialized;
    static std::string s_projectRootPath;

    // Format a stack trace
    static std::string formatStackTrace(backward::StackTrace& trace);

    // Format a single stack trace line
    static std::string formatStackFrame(const std::string& objectFunction, const std::string& objectFile,
                                        const std::string& sourceFunction, const std::string& sourceFile, int line,
                                        int column);

    // Make a path relative to project root
    static std::string makeRelativePath(const std::string& path);
};

} // namespace aph