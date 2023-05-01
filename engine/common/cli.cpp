#include "cli.h"
#include "common/logger.h"

namespace aph
{
CLIParser::CLIParser(CLICallbacks cbs, int argc, char* argv[]) : m_cbs(std::move(cbs)), m_argc(argc), m_argv(argv)
{
}

bool CLIParser::parse()
{
    try
    {
        while(m_argc && !m_endedState)
        {
            const char* next = *m_argv++;
            m_argc--;

            if(*next != '-' && m_cbs.m_defaultHandler)
            {
                m_cbs.m_defaultHandler(next);
            }
            else
            {
                auto itr = m_cbs.m_callbacks.find(next);
                if(itr == std::end(m_cbs.m_callbacks))
                {
                    if(m_unknownArgumentIsDefault)
                        m_cbs.m_defaultHandler(next);
                    else
                        throw std::invalid_argument("Invalid argument");
                }
                else
                    itr->second(*this);
            }
        }

        return true;
    }
    catch(const std::exception& e)
    {
        CM_LOG_ERR("Failed to parse arguments: %s\n", e.what());
        if(m_cbs.m_errorHandler)
        {
            m_cbs.m_errorHandler();
        }
        return false;
    }
}

void CLIParser::end()
{
    m_endedState = true;
}

unsigned CLIParser::nextUint()
{
    if(!m_argc)
    {
        throw std::invalid_argument("Tried to parse uint, but nothing left in arguments");
    }

    auto val = std::stoul(*m_argv);
    if(val > std::numeric_limits<unsigned>::max())
    {
        throw std::invalid_argument("next_uint() out of range");
    }

    m_argc--;
    m_argv++;

    return unsigned(val);
}

double CLIParser::nextDouble()
{
    if(!m_argc)
    {
        throw std::invalid_argument("Tried to parse double, but nothing left in arguments");
    }

    double val = std::stod(*m_argv);

    m_argc--;
    m_argv++;

    return val;
}

const char* CLIParser::nextString()
{
    if(!m_argc)
    {
        throw std::invalid_argument("Tried to parse string, but nothing left in arguments");
    }

    const char* ret = *m_argv;
    m_argc--;
    m_argv++;
    return ret;
}

bool parseCliFiltered(CLICallbacks cbs, int& argc, char* argv[], int& exit_code)
{
    if(argc == 0)
    {
        exit_code = 1;
        return false;
    }

    exit_code = 0;
    std::vector<char*> filtered;
    filtered.reserve(argc + 1);
    filtered.push_back(argv[0]);

    cbs.m_defaultHandler = [&](const char* arg) { filtered.push_back(const_cast<char*>(arg)); };

    CLIParser parser(std::move(cbs), argc - 1, argv + 1);
    parser.ignoreUnknownArguments();

    if(!parser.parse())
    {
        exit_code = 1;
        return false;
    }

    if(parser.isEndedState())
    {
        exit_code = 0;
        return false;
    }

    argc = int(filtered.size());
    std::copy(filtered.begin(), filtered.end(), argv);
    argv[argc] = nullptr;
    return true;
}
}  // namespace aph
