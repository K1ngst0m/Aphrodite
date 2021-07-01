//
// Created by npchitman on 6/29/21.
//

#ifndef Aphrodite_ENGINE_TIMER_H
#define Aphrodite_ENGINE_TIMER_H

#include <chrono>

namespace Aph{
    class Timer {
    public:
        Timer() {
            Reset();
        }

        void Reset() {
            m_Start = std::chrono::high_resolution_clock::now();
        }

        float Elapsed() {
            return static_cast<float>(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - m_Start).count()) * 0.001f * 0.001f * 0.001f;
        }

        float ElapsedMillis() {
            return Elapsed() * 1000.0f;
        }

    private:
        std::chrono::time_point<std::chrono::high_resolution_clock> m_Start;
    };

}// namespace Aph

#endif//Aphrodite_ENGINE_TIMER_H
