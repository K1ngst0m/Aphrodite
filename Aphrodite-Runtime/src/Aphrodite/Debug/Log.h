//
// Created by npchitman on 5/31/21.
//
// Logging system



#ifndef Aphrodite_ENGINE_LOG_H
#define Aphrodite_ENGINE_LOG_H

#define SPDLOG_FMT_EXTERNAL
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>

#include "Aphrodite/Core/Base.h"

namespace Aph {
    class Log {
    public:
        static void Init();

        static Ref<spdlog::logger> &GetCoreLogger() {
            return s_CoreLogger;
        }

        static Ref<spdlog::logger> &GetClientLogger() {
            return s_ClientLogger;
        }

    private:
        static Ref<spdlog::logger> s_CoreLogger;
        static Ref<spdlog::logger> s_ClientLogger;
    };
}// namespace Aph-Runtime

// Core log macros
#define APH_CORE_TRACE(...) ::Aph::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define APH_CORE_INFO(...) ::Aph::Log::GetCoreLogger()->info(__VA_ARGS__)
#define APH_CORE_WARN(...) ::Aph::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define APH_CORE_ERROR(...) ::Aph::Log::GetCoreLogger()->error(__VA_ARGS__)
#define APH_CORE_CRITICAL(...) \
    ::Aph::Log::GetCoreLogger()->critical(__VA_ARGS__)

// Client log macros
#define APH_TRACE(...) ::Aph::Log::GetClientLogger()->trace(__VA_ARGS__)
#define APH_INFO(...) ::Aph::Log::GetClientLogger()->info(__VA_ARGS__)
#define APH_WARN(...) ::Aph::Log::GetClientLogger()->warn(__VA_ARGS__)
#define APH_ERROR(...) ::Aph::Log::GetClientLogger()->error(__VA_ARGS__)
#define APH_CRITICAL(...) ::Aph::Log::GetClientLogger()->critical(__VA_ARGS__)

#endif// Aphrodite_ENGINE_LOG_H
