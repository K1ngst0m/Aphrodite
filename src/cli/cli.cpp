#include "cli.h"

#include "common/logger.h"

namespace aph
{
bool CLICallbacks::parse(int& argc, char* argv[], int& exit_code)
{
    auto& cbs = *this;

    if (argc == 0)
    {
        exit_code = 1;
        return false;
    }

    exit_code = 0;
    SmallVector<char*> filtered;
    filtered.reserve(argc + 1);
    filtered.push_back(argv[0]);

    cbs.m_defaultHandler = [&](std::string_view arg) { filtered.push_back(const_cast<char*>(arg.data())); };

    CLIParser parser(std::move(cbs), std::span(argv + 1, argc - 1));
    parser.ignoreUnknownArguments();

    try
    {
        if (!parser.parse())
        {
            exit_code = 1;
            return false;
        }
    }
    catch (const std::exception& e)
    {
        CM_LOG_ERR("CLI parsing failed: %s", e.what());
        exit_code = 1;
        return false;
    }

    if (parser.isEndedState())
    {
        exit_code = 0;
        return false;
    }

    argc = int(filtered.size());
    std::copy(filtered.begin(), filtered.end(), argv);
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

bool CLIParser::parse()
{
    try
    {
        while (!m_args.empty() && !m_endedState)
        {
            std::string_view next = m_args[0];
            m_args = m_args.subspan(1);

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
                auto itr = m_cbs.m_callbacks.find(std::string(next));
                if (itr == std::end(m_cbs.m_callbacks))
                {
                    // Handle combined short options like -abc as -a -b -c
                    if (next.size() > 2 && next[0] == '-' && next[1] != '-')
                    {
                        // Put back the current argument and try to parse individual options
                        bool handled = true;
                        for (size_t i = 1; i < next.size(); ++i)
                        {
                            std::string singleOpt = std::string("-") + next[i];
                            auto singleItr = m_cbs.m_callbacks.find(singleOpt);
                            if (singleItr == std::end(m_cbs.m_callbacks))
                            {
                                handled = false;
                                break;
                            }
                        }

                        if (handled)
                        {
                            for (size_t i = 1; i < next.size(); ++i)
                            {
                                std::string singleOpt = std::string("-") + next[i];
                                m_cbs.m_callbacks[singleOpt](*this);
                                if (m_endedState)
                                    break;
                            }
                            continue;
                        }
                    }

                    if (m_unknownArgumentIsDefault && m_cbs.m_defaultHandler)
                        m_cbs.m_defaultHandler(next);
                    else
                        reportError(CLIError::UnknownArgument, std::string("Unknown argument: ") + std::string(next));
                }
                else
                    itr->second(*this);
            }
        }

        return true;
    }
    catch (const std::exception& e)
    {
        reportError(CLIError::Custom, e.what());
        return false;
    }
}

void CLIParser::end()
{
    m_endedState = true;
}

std::optional<std::string_view> CLIParser::peekNext() const
{
    if (m_args.empty())
    {
        return std::nullopt;
    }
    return m_args[0];
}

void CLIParser::reportError(CLIError type, std::string_view message) const
{
    CM_LOG_ERR("CLI error: %s", std::string(message).c_str());

    if (m_cbs.m_errorHandler)
    {
        CLIErrorInfo info{ type, std::string(message) };
        m_cbs.m_errorHandler(info);
    }

    throw std::runtime_error(std::string(message));
}

std::string_view CLIParser::nextString() const
{
    if (m_args.empty())
    {
        reportError(CLIError::MissingArgument, "Expected string argument but none available");
        return {};
    }

    std::string_view ret = m_args[0];
    m_args = m_args.subspan(1);
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
