//
// Created by Npchitman on 2021/1/18.
//
#include "Log.h"


namespace Hazel{
    std::shared_ptr<spdlog::logger> Log::s_CoreLogger;
    std::shared_ptr<spdlog::logger> Log::s_ClientLogger;

    void Log::Init() {
        spdlog::set_pattern("%^[%T] %n: %v%$");
        s_CoreLogger = spdlog::stderr_color_mt("Hazel");
        s_CoreLogger->set_level(spdlog::level::trace);

        s_ClientLogger = spdlog::stderr_color_mt("APP");
        s_ClientLogger->set_level(spdlog::level::trace);
    }
}
