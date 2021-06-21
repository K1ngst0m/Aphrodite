//
// Created by npchitman on 6/20/21.
//

#ifndef HAZEL_ENGINE_TIMESTEP_H
#define HAZEL_ENGINE_TIMESTEP_H

namespace Hazel {
    class Timestep {
    public:
        Timestep(float time = 0.0f) : m_Time(time) {}

        operator float() const { return m_Time; }
        float GetSeconds() const { return m_Time; }
        float GetMilliseconds() const { return m_Time * 1000.0f; }

    private:
        float m_Time;
    };
}// namespace Hazel

#endif// HAZEL_ENGINE_TIMESTEP_H
