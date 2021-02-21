//
// Created by Npchitman on 2021/2/21.
//

#ifndef HAZELENGINE_IMGUILAYER_H
#define HAZELENGINE_IMGUILAYER_H

#include "Hazel/Layer.h"

namespace Hazel{

    class HAZEL_API ImGuiLayer : public Layer{
    public:
        ImGuiLayer();
        ~ImGuiLayer() override;

        void OnAttach() override;
        void OnDetach() override;
        void OnUpdate() override;
        void OnEvent(Event& event) override;

    private:
        float m_Time = 0.0f;
    };
}

#endif //HAZELENGINE_IMGUILAYER_H
