#include "timer.h"
#include "common.h"

namespace aph
{
class ProfilerScope
{
public:
    ProfilerScope(std::string tag) : m_tag(std::move(tag)) { m_timer.set(startTag()); }

    ~ProfilerScope()
    {
        m_timer.set(endTag());
        double elapsed = m_timer.interval(startTag(), endTag());
        CM_LOG_DEBUG("[%s] took %lf seconds", m_tag, elapsed);
    }

private:
    Timer       m_timer;
    std::string m_tag;

    std::string startTag() const { return m_tag + "_start"; }
    std::string endTag() const { return m_tag + "_end"; }
};

}  // namespace aph

#define PROFILE_FUNCTION() ::aph::ProfilerScope APH_MACRO_CONCAT(scope_profiler_, __LINE__)(__FUNCTION__)
#define PROFILE_SCOPE(msg) ::aph::ProfilerScope APH_MACRO_CONCAT(scope_profiler_, __LINE__)(msg)
