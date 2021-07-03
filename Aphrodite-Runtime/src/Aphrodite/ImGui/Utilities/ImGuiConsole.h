//
// Created by npchitman on 7/2/21.
//

#ifndef APHRODITE_IMGUICONSOLE_H
#define APHRODITE_IMGUICONSOLE_H

#include "pch.h"
#include "Aphrodite/Core/Base.h"

namespace Aph {
    static const char * logSign = "\uF292   ";

    class Message {
    public:
        enum class Level : int8_t {
            Info = 0,
            Warn = 1,
            Error = 2
        };

    public:
        Message() = default;
        Message(std::string message, Level level = Level::Info) : m_MessageData(std::move(message)),// NOLINT(google-explicit-constructor)
                                                                  m_MessageLevel(level) {
        }

    public:
        std::string m_MessageData;
        Level m_MessageLevel{};
    };

    class ImGuiConsole {
    public:
        ImGuiConsole() = default;

        static void Draw();

        template<typename... Args>
        static void Log(const std::string& data, Args&&... args);

        template<typename... Args>
        static void LogWarning(const std::string& data, Args&&... args);

        template<typename... Args>
        static void LogError(const std::string& data, Args&&... args);

    private:
        static void Clear();

        template<typename... Args>
        static std::string Format(const std::string& fmt, Args&&... args);

    private:
        static std::vector<Message> s_MessageBuffer;
        static uint32_t s_LogMessageCount;
    };

    // Logging Implementations
    template<typename... Args>
    inline void ImGuiConsole::Log(const std::string& data, Args&&... args) {
        s_MessageBuffer.push_back({ImGuiConsole::Format(data, args...)});
        s_LogMessageCount++;
    }

    template<typename... Args>
    inline std::string ImGuiConsole::Format(const std::string& fmt, Args&&... args) {
        return fmt::format((logSign + fmt), args...);
    }


    template<typename... Args>
    inline void ImGuiConsole::LogWarning(const std::string& data, Args&&... args) {
        s_MessageBuffer.push_back({ImGuiConsole::Format(data, args...), Message::Level::Warn});
        s_LogMessageCount++;
    }

    template<typename... Args>
    void ImGuiConsole::LogError(const std::string& data, Args&&... args) {
        s_MessageBuffer.push_back({ImGuiConsole::Format(data, args...), Message::Level::Error});
        s_LogMessageCount++;
    }
}// namespace Aph

#endif//APHRODITE_IMGUICONSOLE_H
