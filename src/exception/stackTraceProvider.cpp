#include "stackTraceProvider.h"

// Include backward-cpp
#include <backward.hpp>

#include <filesystem>
#include <sstream>

#ifdef _WIN32
#include <Windows.h>
#endif

namespace aph
{

int StackTraceProvider::s_maxStackDepth = 64;
bool StackTraceProvider::s_skipCommonFrames = true;
bool StackTraceProvider::s_initialized = false;
std::string StackTraceProvider::s_projectRootPath = "Aphrodite";

void StackTraceProvider::initialize()
{
    if (s_initialized)
        return;

    // Any one-time initialization for backward-cpp would go here
    s_initialized = true;
}

void StackTraceProvider::setProjectRootPath(const std::string& path)
{
    s_projectRootPath = path;

    // Ensure path ends with a separator
    if (!s_projectRootPath.empty() && s_projectRootPath.back() != '/' && s_projectRootPath.back() != '\\')
    {
        s_projectRootPath += '/';
    }
}

std::string StackTraceProvider::makeRelativePath(const std::string& path)
{
    if (s_projectRootPath.empty() || path.empty())
    {
        return path;
    }

    // Check if the path starts with the project root
    if (path.find(s_projectRootPath) == 0)
    {
        return path.substr(s_projectRootPath.size());
    }

    // Try to use std::filesystem to get a relative path
    try
    {
        std::filesystem::path absolute(path);
        std::filesystem::path root(s_projectRootPath);

        // If possible, get relative path
        namespace fs = std::filesystem;
        auto reRooted = [](const fs::path& fullPath, const fs::path& marker, std::string_view label)
        {
            auto it = fullPath.begin();
            for (; it != fullPath.end(); ++it)
            {
                if (*it == marker)
                {
                    ++it;
                    break;
                }
            }

            if (it == fullPath.end())
            {
                return fullPath;
            }

            fs::path newPath = fs::path("{ProjectRoot}");
            for (; it != fullPath.end(); ++it)
            {
                newPath /= *it;
            }
            return newPath;
        };

        return reRooted(absolute, root, "{ProjectRoot}").string();
    }
    catch (...)
    {
        // If anything fails, fall back to the original path
    }

    return path;
}

std::string StackTraceProvider::captureStackTrace(int skipFrames)
{
    backward::StackTrace st;
    st.load_here(s_maxStackDepth);

    // Skip N frames (including our own functions)
    st.skip_n_firsts(skipFrames);

    backward::TraceResolver tr;
    tr.load_stacktrace(st);

    std::stringstream ss;

    for (size_t i = 0; i < st.size(); ++i)
    {
        backward::ResolvedTrace trace = tr.resolve(st[i]);

        // Skip common noise in stack trace if configured
        if (s_skipCommonFrames)
        {
            // Skip certain system frames, signal handlers, etc.
            if (trace.object_function.find("__libc_start") != std::string::npos ||
                trace.object_function.find("__cxa_") != std::string::npos)
            {
                continue;
            }
        }

        ss << formatStackFrame(trace.object_function, trace.object_filename, trace.source.function,
                               trace.source.filename, trace.source.line, trace.source.col)
           << "\n";
    }

    return ss.str();
}

std::string StackTraceProvider::captureStackTraceFromSignal(void* context)
{
    // When a signal occurs, use the context info for a more accurate stack trace
    backward::StackTrace st;
    if (context)
    {
        // Get error address from context if possible
        void* error_addr = nullptr;
#ifdef _WIN32
        // On Windows, try to extract the instruction pointer from the CONTEXT structure
        CONTEXT* ctx = static_cast<CONTEXT*>(context);
        if (ctx)
        {
            // Note: We only support AMD64 but don't use _M_X64 macro
            error_addr = reinterpret_cast<void*>(ctx->Rip);
        }
#elif defined(__linux__)
        // On Linux, try to extract the instruction pointer from the ucontext_t
        ucontext_t* uctx = static_cast<ucontext_t*>(context);

        // Note: We only support AMD64 but don't use REG_RIP macro
        // We'll use a runtime check to verify if we can access the program counter
        // This assumes x86_64 on Linux without using architecture-specific macros

        // The following is equivalent to uctx->uc_mcontext.gregs[REG_RIP] for x86_64
        // But avoids using REG_RIP macro directly
        struct
        {
            long r8;
            long r9;
            long r10;
            long r11;
            long r12;
            long r13;
            long r14;
            long r15;
            long rdi;
            long rsi;
            long rbp;
            long rbx;
            long rdx;
            long rax;
            long rcx;
            long rsp;
            long rip;
            long eflags; /* plus others */
        }* regs;

        // Cast mcontext_t to our compatible structure to extract RIP (instruction pointer)
        regs = reinterpret_cast<decltype(regs)>(&uctx->uc_mcontext);
        error_addr = reinterpret_cast<void*>(regs->rip);
#else
        // Unsupported platform
        st.load_here(s_maxStackDepth);
        return formatStackTrace(st);
#endif

        // If we have an error address, load from it
        if (error_addr)
        {
            st.load_from(error_addr, s_maxStackDepth);
        }
        else
        {
            st.load_here(s_maxStackDepth);
        }
    }
    else
    {
        st.load_here(s_maxStackDepth);
    }

    return formatStackTrace(st);
}

// Helper method to format a stack trace
std::string StackTraceProvider::formatStackTrace(backward::StackTrace& st)
{
    backward::TraceResolver tr;
    tr.load_stacktrace(st);

    std::stringstream ss;

    for (size_t i = 0; i < st.size(); ++i)
    {
        backward::ResolvedTrace trace = tr.resolve(st[i]);
        if (s_skipCommonFrames)
        {
            // Skip certain system frames, signal handlers, etc.
            if (trace.object_function.find("__libc_start") != std::string::npos ||
                trace.object_function.find("__cxa_") != std::string::npos)
            {
                continue;
            }
        }
        ss << formatStackFrame(trace.object_function, trace.object_filename, trace.source.function,
                               trace.source.filename, trace.source.line, trace.source.col)
           << "\n";
    }

    return ss.str();
}

void StackTraceProvider::setMaxStackDepth(int depth)
{
    s_maxStackDepth = depth;
}

void StackTraceProvider::setSkipCommonFrames(bool skip)
{
    s_skipCommonFrames = skip;
}

std::string StackTraceProvider::formatStackFrame(const std::string& objectFunction, const std::string& objectFile,
                                                 const std::string& sourceFunction, const std::string& sourceFile,
                                                 int line, int column)
{
    std::stringstream ss;

    // Create a rich frame description with source information when available
    if (!sourceFile.empty() && line > 0)
    {
        // We have source information
        std::string relativeSourcePath = makeRelativePath(sourceFile);

        ss << "#" << relativeSourcePath << ":" << line;
        if (column > 0)
        {
            ss << ":" << column;
        }

        if (!sourceFunction.empty())
        {
            ss << " in " << sourceFunction;
        }
        else if (!objectFunction.empty())
        {
            // Fall back to object function if source function not available
            ss << " in " << objectFunction;
        }
    }
    else
    {
        // No source info available, use object info
        std::string relativeObjectPath = makeRelativePath(objectFile);

        ss << "#" << (relativeObjectPath.empty() ? "???" : relativeObjectPath);
        if (line > 0)
        {
            ss << ":" << line;
        }

        ss << " in " << (objectFunction.empty() ? "???" : objectFunction);
    }

    return ss.str();
}

} // namespace aph
