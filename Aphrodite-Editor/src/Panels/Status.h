//
// Created by npchitman on 7/8/21.
//

#ifndef APHRODITE_STATUS_H
#define APHRODITE_STATUS_H

#include <vector>
namespace Aph::Editor {
    class Status {
    public:
        void OnUIRender();

    private:
        void DrawStatusBar();
        void DrawStatusPanel();

    private:
        float m_Time = 0.0f;

        float m_FpsValues[50];
        std::vector<float> m_FrameTimes;
    };
}// namespace Aph::Editor


#endif//APHRODITE_STATUS_H
