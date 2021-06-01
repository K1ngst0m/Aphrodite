//
// Created by npchitman on 5/31/21.
//

#ifndef HAZEL_ENGINE_APPLICATIONEVENT_H
#define HAZEL_ENGINE_APPLICATIONEVENT_H

#include "Event.h"

namespace Hazel {
    class WindowResizeEvent : public Event {
    public:
        WindowResizeEvent(unsigned int width, unsigned int height)
                : m_Width(width), m_Height(height) {}

        inline unsigned int GetWidth() const { return m_Width; }

        inline unsigned int GetHeight() const { return m_Height; }

        std::string ToString() const override {
            std::stringstream ss;
            ss << "WindowResizeEvent: " << m_Width << ", " << m_Height;
            return ss.str();
        }

        EVENT_CLASS_TYPE(WindowResize);

        EVENT_CLASS_CATEGORY(EventCategoryApplication);

    private:
        unsigned int m_Width, m_Height;
    };

    class WindowCloseEvent : public Event {
    public:
        WindowCloseEvent() = default;

        EVENT_CLASS_TYPE(WindowClose)

        EVENT_CLASS_CATEGORY(EventCategoryApplication)
    };

    class AppTickEvent : public Event {
    public:
        AppTickEvent() = default;

        EVENT_CLASS_TYPE(AppTick)

        EVENT_CLASS_CATEGORY(EventCategoryApplication)
    };

    class AppUpdateEvent : public Event {
    public:
        AppUpdateEvent() = default;

        EVENT_CLASS_TYPE(AppUpdate)

        EVENT_CLASS_CATEGORY(EventCategoryApplication)
    };

    class AppRenderEvent : public Event {
    public:
        AppRenderEvent() = default;

        EVENT_CLASS_TYPE(AppRender)

        EVENT_CLASS_CATEGORY(EventCategoryApplication)
    };
}

#endif //HAZEL_ENGINE_APPLICATIONEVENT_H
