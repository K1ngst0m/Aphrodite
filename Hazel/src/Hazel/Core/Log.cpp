//
// Created by npchitman on 5/31/21.
//

#include "Hazel/Core/Log.h"

#include <spdlog/sinks/stdout_color_sinks.h>

#include "hzpch.h"

namespace Hazel {
    Ref<spdlog::logger> Log::s_CoreLogger;
    Ref<spdlog::logger> Log::s_ClientLogger;

    void Log::Init() {
        spdlog::set_pattern("%^[%T] %n: %v%$");
        s_CoreLogger = spdlog::stdout_color_mt("HAZEL");
        s_CoreLogger->set_level(spdlog::level::trace);

        s_ClientLogger = spdlog::stdout_color_mt("APP");
        s_ClientLogger->set_level(spdlog::level::trace);
    }
}// namespace Hazel