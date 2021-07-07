//
// Created by npchitman on 7/2/21.
//

#ifndef APHRODITE_EDITORCONSOLE_H
#define APHRODITE_EDITORCONSOLE_H

#include "Aphrodite/Core/Base.h"

namespace Aph::Editor {
    static const char* logSign = "\uF292   ";

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

    class EditorConsole {
    public:
        EditorConsole() = default;

        static void Draw();

        static std::string_view GetLastMessage() { return s_MessageBuffer.back().m_MessageData; }

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
    inline void EditorConsole::Log(const std::string& data, Args&&... args) {
        s_MessageBuffer.push_back({EditorConsole::Format(data, args...)});
        s_LogMessageCount++;
    }

    template<typename... Args>
    inline std::string EditorConsole::Format(const std::string& fmt, Args&&... args) {
        return fmt::format((logSign + fmt), args...);
    }


    template<typename... Args>
    inline void EditorConsole::LogWarning(const std::string& data, Args&&... args) {
        s_MessageBuffer.push_back({EditorConsole::Format(data, args...), Message::Level::Warn});
        s_LogMessageCount++;
    }

    template<typename... Args>
    void EditorConsole::LogError(const std::string& data, Args&&... args) {
        s_MessageBuffer.push_back({EditorConsole::Format(data, args...), Message::Level::Error});
        s_LogMessageCount++;
    }
}// namespace Aph

#endif//APHRODITE_EDITORCONSOLE_H
