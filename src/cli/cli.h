#pragma once

#include "common/common.h"
#include "common/hash.h"

namespace aph
{
class CLIParser;

enum class CLIError
{
    None,
    MissingArgument,
    InvalidArgumentType,
    UnknownArgument,
    Custom
};

struct CLIErrorInfo
{
    CLIError type = CLIError::None;
    std::string message;
};

struct CLICallbacks
{
    /**
     * Adds a CLI command handler
     * @param cli - token for the command
     * @param func - Callable taking CLIParser& and returning void
     */
    void add(auto&& cli, auto&& func)
    {
        m_callbacks[APH_FWD(cli)] = APH_FWD(func);
    }

    /**
     * Sets the error handler
     * @param func - Callable taking const CLIErrorInfo& and returning void
     */
    void setErrorHandler(auto&& func)
    {
        m_errorHandler = APH_FWD(func);
    }

    bool parse(int& argc, char* argv[], int& exit_code);

private:
    friend class CLIParser;
    HashMap<std::string, std::function<void(CLIParser&)>> m_callbacks;
    std::function<void(const CLIErrorInfo&)> m_errorHandler;
    std::function<void(std::string_view)> m_defaultHandler;
};

class CLIParser
{
public:
    // Type-safe argument parsing with templates
    template <typename T>
    T next() const
    {
        static_assert(dependent_false_v<T>, "Invalid value type of the argument.");
        return T{};
    }

    template <NumericType T>
    T next() const
    {
        if (m_args.empty())
        {
            reportError(CLIError::MissingArgument, "Missing numeric argument");
        }

        try
        {
            T value;
            if constexpr (std::is_integral_v<T> && std::is_unsigned_v<T>)
            {
                value = static_cast<T>(std::stoull(m_args[0]));
            }
            else if constexpr (std::is_integral_v<T>)
            {
                value = static_cast<T>(std::stoll(m_args[0]));
            }
            else
            {
                value = static_cast<T>(std::stod(m_args[0]));
            }

            m_args = m_args.subspan(1);
            return value;
        }
        catch (const std::exception& e)
        {
            reportError(CLIError::InvalidArgumentType, std::string("Failed to parse numeric value: ") + e.what());
            return T{};
        }
    }

    template <StringType T>
    T next() const
    {
        return nextString();
    }

    std::string_view nextString() const;

    // Peek at next argument without consuming
    std::optional<std::string_view> peekNext() const;

private:
    bool parse();
    void end();
    bool isEndedState() const;
    void ignoreUnknownArguments();
    void reportError(CLIError type, std::string_view message) const;

private:
    friend struct CLICallbacks;
    // Don't pass in argv[0], which is the application name.
    // Pass in argc - 1, argv + 1.
    CLIParser(CLICallbacks cbs, std::span<char*> args);

    CLICallbacks m_cbs;
    mutable std::span<char*> m_args;
    bool m_endedState               = false;
    bool m_unknownArgumentIsDefault = false;
};
} // namespace aph
