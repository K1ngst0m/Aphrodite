//
// Created by npchitman on 5/31/21.
//
// Logging system



#ifndef HAZEL_ENGINE_LOG_H
#define HAZEL_ENGINE_LOG_H

#include <spdlog/fmt/ostr.h>
#include <spdlog/spdlog.h>

#include "Hazel/Core/Base.h"

namespace Hazel {
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
}// namespace Hazel

// Core log macros
#define HZ_CORE_TRACE(...) ::Hazel::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define HZ_CORE_INFO(...) ::Hazel::Log::GetCoreLogger()->info(__VA_ARGS__)
#define HZ_CORE_WARN(...) ::Hazel::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define HZ_CORE_ERROR(...) ::Hazel::Log::GetCoreLogger()->error(__VA_ARGS__)
#define HZ_CORE_CRITICAL(...) \
    ::Hazel::Log::GetCoreLogger()->critical(__VA_ARGS__)

// Client log macros
#define HZ_TRACE(...) ::Hazel::Log::GetClientLogger()->trace(__VA_ARGS__)
#define HZ_INFO(...) ::Hazel::Log::GetClientLogger()->info(__VA_ARGS__)
#define HZ_WARN(...) ::Hazel::Log::GetClientLogger()->warn(__VA_ARGS__)
#define HZ_ERROR(...) ::Hazel::Log::GetClientLogger()->error(__VA_ARGS__)
#define HZ_CRITICAL(...) ::Hazel::Log::GetClientLogger()->critical(__VA_ARGS__)

#endif// HAZEL_ENGINE_LOG_H