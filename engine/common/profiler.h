#pragma once

#if defined(APH_ENABLE_TRACY)
#include "tracy/Tracy.hpp"
// predefined RGB colors for "heavy" point-of-interest operations
#define APH_PROFILER_COLOR_WAIT 0xff0000
#define APH_PROFILER_COLOR_SUBMIT 0x0000ff
#define APH_PROFILER_COLOR_PRESENT 0x00ff00
#define APH_PROFILER_COLOR_CREATE 0xff6600
#define APH_PROFILER_COLOR_DESTROY 0xffa500
#define APH_PROFILER_COLOR_BARRIER 0xffffff
//
#define APH_PROFILER_SCOPE() ZoneScoped
#define APH_PROFILER_SCOPE_NAME(name) ZoneScopedN(name)
#define APH_PROFILER_SCOPE_COLOR(color) ZoneScopedC(color)
#define APH_PROFILER_ZONE(name, color) \
    {                                  \
        ZoneScopedC(color);            \
        ZoneName(name, strlen(name))
#define APH_PROFILER_ZONE_END() }
#define APH_PROFILER_THREAD(name) tracy::SetThreadName(name)
#define APH_PROFILER_FRAME(name) FrameMarkNamed(name)
#else
#define APH_PROFILER_SCOPE()
#define APH_PROFILER_SCOPE_NAME(name)
#define APH_PROFILER_SCOPE_COLOR(color)
#define APH_PROFILER_ZONE(name, color) {
#define APH_PROFILER_ZONE_END() }
#define APH_PROFILER_THREAD(name)
#define APH_PROFILER_FRAME(name)
#endif // APH_WITH_TRACY
