#include "cli.h"

#include <algorithm>

#include "common/logger.h"

namespace aph
{
Expected<bool> CLICallbacks::parse(int& argc, char* argv[], int& exit_code)
{
    auto& cbs = *this;

    if (argc == 0)
    {
        exit_code = 1;
        return { Result::Code::ArgumentOutOfRange, "No arguments provided" };
    }

    exit_code = 0;
    SmallVector<char*> filtered;
    filtered.reserve(argc + 1);
    filtered.push_back(argv[0]);

    cbs.m_defaultHandler = [&filtered](std::string_view arg)
    {
        filtered.push_back(const_cast<char*>(arg.data()));
    };

    CLIParser parser(std::move(cbs), std::span(argv + 1, argc - 1));
    parser.ignoreUnknownArguments();

    auto parseResult = parser.parse();
    if (!parseResult)
    {
        exit_code = 1;
        return parseResult;
    }

    if (parser.isEndedState())
    {
        exit_code = 0;
        return false;
    }

    argc = static_cast<int>(filtered.size());
    std::ranges::copy(filtered, argv);
    argv[argc] = nullptr;
    return true;
}
} // namespace aph

namespace aph
{
CLIParser::CLIParser(CLICallbacks cbs, std::span<char*> args)
    : m_cbs(std::move(cbs))
    , m_args(args)
{
}

Expected<bool> CLIParser::parse()
{
    while (!m_args.empty() && !m_endedState)
    {
        const std::string_view next = m_args[0];
        m_args                      = m_args.subspan(1);

        if (next.empty())
        {
            continue;
        }

        if (next[0] != '-' && m_cbs.m_defaultHandler)
        {
            m_cbs.m_defaultHandler(next);
        }
        else
        {
            const auto itr = m_cbs.m_callbacks.find(std::string(next));
            if (itr == std::end(m_cbs.m_callbacks))
            {
                // Handle combined short options like -abc as -a -b -c
                if (next.size() > 2 && next[0] == '-' && next[1] != '-')
                {
                    // Check if all individual options are valid
                    bool handled = true;
                    for (size_t i = 1; i < next.size(); ++i)
                    {
                        const std::string singleOpt = std::string("-") + next[i];
                        if (m_cbs.m_callbacks.find(singleOpt) == std::end(m_cbs.m_callbacks))
                        {
                            handled = false;
                            break;
                        }
                    }

                    if (handled)
                    {
                        // Process each option independently
                        for (size_t i = 1; i < next.size(); ++i)
                        {
                            const std::string singleOpt = std::string("-") + next[i];
                            m_cbs.m_callbacks[singleOpt](*this);
                            if (m_endedState)
                                break;
                        }
                        continue;
                    }
                }

                if (m_unknownArgumentIsDefault && m_cbs.m_defaultHandler)
                {
                    m_cbs.m_defaultHandler(next);
                }
                else
                {
                    const std::string errorMsg = "Unknown argument: " + std::string(next);
                    if (m_cbs.m_errorHandler)
                    {
                        CLIErrorInfo info{ .type = CLIError::eUnknownArgument, .message = errorMsg };
                        m_cbs.m_errorHandler(info);
                    }
                    CM_LOG_ERR("CLI error: %s", errorMsg.c_str());
                    return { Result::Code::RuntimeError, errorMsg };
                }
            }
            else
            {
                itr->second(*this);
            }
        }
    }

    return true;
}

void CLIParser::end()
{
    m_endedState = true;
}

std::optional<std::string_view> CLIParser::peekNext() const
{
    return m_args.empty() ? std::nullopt : std::optional<std::string_view>(m_args[0]);
}

Expected<std::string_view> CLIParser::nextString() const
{
    if (m_args.empty())
    {
        return { Result::Code::ArgumentOutOfRange, "Expected string argument but none available" };
    }

    const std::string_view ret = m_args[0];
    m_args                     = m_args.subspan(1);
    return ret;
}

bool CLIParser::isEndedState() const
{
    return m_endedState;
}

void CLIParser::ignoreUnknownArguments()
{
    m_unknownArgumentIsDefault = true;
}
} // namespace aph
