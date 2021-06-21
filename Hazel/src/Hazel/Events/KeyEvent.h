//
// Created by npchitman on 5/31/21.
//

#ifndef HAZEL_ENGINE_KEYEVENT_H
#define HAZEL_ENGINE_KEYEVENT_H

#include "Event.h"

namespace Hazel {
    class KeyEvent : public Event {
    public:
        inline int GetKeyCode() const { return m_KeyCode; }

        EVENT_CLASS_CATEGORY(EventCategoryKeyboard | EventCategoryInput);

    protected:
        explicit KeyEvent(int keyCode) : m_KeyCode(keyCode) {}

        int m_KeyCode;
    };

    class KeyPressedEvent : public KeyEvent {
    public:
        KeyPressedEvent(int keycode, int repeatCount)
            : KeyEvent(keycode),
              m_RepeatCount(repeatCount) {}

        inline int GetRepeatCount() const { return m_RepeatCount; }

        std::string ToString() const override {
            std::stringstream ss;
            ss << "KeyPressedEvent: " << m_KeyCode << " (" << m_RepeatCount
               << " repeats)";
            return ss.str();
        }

        EVENT_CLASS_TYPE(KeyPressed);

    private:
        int m_RepeatCount;
    };

    class KeyReleasedEvent : public KeyEvent {
    public:
        explicit KeyReleasedEvent(int keycode) : KeyEvent(keycode) {}

        std::string ToString() const override {
            std::stringstream ss;
            ss << "KeyReleasedEvent: " << m_KeyCode;
            return ss.str();
        }

        EVENT_CLASS_TYPE(KeyReleased)
    };

    class KeyTypedEvent : public KeyEvent {
    public:
        explicit KeyTypedEvent(int keycode) : KeyEvent(keycode) {}

        std::string ToString() const override {
            std::stringstream ss;
            ss << "KeyTypedEvent: " << m_KeyCode;
            return ss.str();
        }

        EVENT_CLASS_TYPE(KeyTyped)
    };
}// namespace Hazel

#endif// HAZEL_ENGINE_KEYEVENT_H
