#pragma once

#include "common/common.h"
#include "common/hash.h"
#include "common/result.h"

namespace aph
{
class CLIParser;

enum class CLIError: uint8_t
{
    eNone,
    eMissingArgument,
    eInvalidArgumentType,
    eUnknownArgument,
    eCustom
};

struct CLIErrorInfo
{
    CLIError type = CLIError::eNone;
    std::string message;
};

struct CLICallbacks
{
    /**
     * Adds a CLI command handler
     * @param cli - token for the command
     * @param func - Callable taking CLIParser& and returning void
     */
    void add(auto&& cli, auto&& func);

    /**
     * Sets the error handler
     * @param func - Callable taking const CLIErrorInfo& and returning void
     */
    void setErrorHandler(auto&& func);

    /**
     * Parse command line arguments
     * @param argc - Reference to argument count, will be modified
     * @param argv - Array of argument strings, will be modified
     * @param exit_code - Reference to exit code, will be set based on parsing result
     * @return Expected<bool> - true if parsing succeeded and program should continue,
     *                         false if program should exit (with exit_code)
     */
    Expected<bool> parse(int& argc, char* argv[], int& exit_code);

private:
    friend class CLIParser;
    HashMap<std::string, std::function<void(CLIParser&)>> m_callbacks;
    std::function<void(const CLIErrorInfo&)> m_errorHandler;
    std::function<void(std::string_view)> m_defaultHandler;
};

class CLIParser
{
public:
    /**
     * Parse an argument as a value of type T
     * @return Expected<T> containing the parsed value or an error
     */
    template <typename T>
    Expected<T> next() const;

    /**
     * Parse an argument as a numeric value
     * @return Expected<T> containing the parsed numeric value or an error
     */
    template <NumericType T>
    Expected<T> next() const;

    /**
     * Parse an argument as a string type
     * @return Expected<T> containing the parsed string or an error
     */
    template <StringType T>
    Expected<T> next() const;

    /**
     * Get the next argument as a string_view
     * @return Expected<std::string_view> containing the next argument or an error
     */
    Expected<std::string_view> nextString() const;

    /**
     * Peek at the next argument without consuming it
     * @return std::optional<std::string_view> containing the next argument or nullopt if none
     */
    std::optional<std::string_view> peekNext() const;

private:
    /**
     * Parse all CLI arguments and execute appropriate callbacks
     * @return Expected<bool> indicating success or an error
     */
    Expected<bool> parse();
    
    /**
     * Mark parsing as complete
     */
    void end();
    
    /**
     * Check if parsing is marked as complete
     * @return true if parsing is complete
     */
    bool isEndedState() const;
    
    /**
     * Configure parser to treat unknown arguments as default arguments
     */
    void ignoreUnknownArguments();

private:
    friend struct CLICallbacks;
    
    /**
     * Construct a parser with callbacks and arguments
     * @param cbs Callbacks for command handling
     * @param args Arguments to parse (without program name)
     */
    CLIParser(CLICallbacks cbs, std::span<char*> args);

    CLICallbacks m_cbs;
    mutable std::span<char*> m_args;
    bool m_endedState               = false;
    bool m_unknownArgumentIsDefault = false;
};

} // namespace aph

// Include template implementations
#include "cli.inl"
