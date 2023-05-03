#ifndef APH_CLI_H_
#define APH_CLI_H_

#include "common/common.h"

namespace aph
{
class CLIParser;

struct CLICallbacks
{
    void add(const char* cli, const std::function<void(CLIParser&)>& func) { m_callbacks[cli] = func; }

    std::unordered_map<std::string, std::function<void(CLIParser&)>> m_callbacks;
    std::function<void()>                                            m_errorHandler;
    std::function<void(const char*)>                                 m_defaultHandler;
};

class CLIParser
{
public:
    // Don't pass in argv[0], which is the application name.
    // Pass in argc - 1, argv + 1.
    CLIParser(CLICallbacks cbs, int argc, char* argv[]);

    bool parse();
    void end();

    uint32_t    nextUint();
    double      nextDouble();
    const char* nextString();

    bool isEndedState() const { return m_endedState; }

    void ignoreUnknownArguments() { m_unknownArgumentIsDefault = true; }

private:
    CLICallbacks m_cbs;
    int          m_argc;
    char**       m_argv;
    bool         m_endedState               = false;
    bool         m_unknownArgumentIsDefault = false;
};

// Returns false is parsing requires an exit, either because of error, or by request.
// In that case, exit_code should be returned from main().
// argc / argv must contain the full argc, argv, where argv[0] holds program name.
bool parseCliFiltered(CLICallbacks cbs, int& argc, char* argv[], int& exit_code);
}  // namespace aph

#endif
