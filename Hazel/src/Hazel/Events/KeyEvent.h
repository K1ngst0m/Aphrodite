//
// Created by Npchitman on 2021/1/18.
//

#ifndef HAZELENGINE_KEYEVENT_H
#define HAZELENGINE_KEYEVENT_H

#include "Event.h"
#include <sstream>

namespace Hazel{
    class HAZEL_API KeyEvent : public Event{
    public:
        inline int GetKeyCode() const { return m_KeyCode; }

        EVENT_CLASS_CATEGORY(EventCategoryKeyboard | EventCategoryInput)

    protected:
        KeyEvent(int keycode) : m_KeyCode(keycode){}

        int m_KeyCode;
    };

    class HAZEL_API KeyPressedEvent : public KeyEvent{
    public:
        KeyPressedEvent(int keycode, int repeatCount)
            : KeyEvent(keycode), m_RepearCount(repeatCount){}

        inline int GetRepeatCount() const { return m_RepeatCount; }

        std::string ToString() const override{
            std::stringstream ss;
            ss << "KeyPressedEvent: " << m_KeyCode << " (" << m_RepeatCount << " repeats";
            return ss.str();
        }

        EVENT_CLASS_TYPE(KeyPressed)
    };


    class HAZEL_API KeyReleaseEvent : public KeyEvent{
    public:
        KeyReleaseEvent(int keycode, int repeatCount) : KeyEvent(keycode), m_RepearCount(repeatCount){}

        inline int GetRepeatCount() const { return m_RepeatCount; }

        std::string ToString() const override{
            std::stringstream ss;
            ss << "KeyReleaseEvent: " << m_KeyCode << " (" << m_RepeatCount << " repeats";
            return ss.str();
        }

    };
}


#endif //HAZELENGINE_KEYEVENT_H
