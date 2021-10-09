// TimeStep.h

// TimeStep class, provide for game loop, framerate calculate, etc.

#ifndef Aphrodite_ENGINE_TIMESTEP_H
#define Aphrodite_ENGINE_TIMESTEP_H

namespace Aph {
    class Timestep {
    public:
        explicit Timestep(float time = 0.0f) : m_Time(time) {}

        explicit operator float() const { return m_Time; }
        float GetSeconds() const { return m_Time; }
        float GetMilliseconds() const { return m_Time * 1000.0f; }

    private:
        float m_Time;
    };
}// namespace Aph-Runtime

#endif// Aphrodite_ENGINE_TIMESTEP_H
