//
// Created by npchitman on 5/31/21.
//

#ifndef HAZEL_ENGINE_IMGUILAYER_H
#define HAZEL_ENGINE_IMGUILAYER_H

#include "Hazel/Core/Layer.h"
#include "Hazel/Events/ApplicationEvent.h"
#include "Hazel/Events/KeyEvent.h"
#include "Hazel/Events/MouseEvent.h"

namespace Hazel {

    class ImGuiLayer : public Layer {
    public:
        ImGuiLayer();

        ~ImGuiLayer() override;

        void OnAttach() override;

        void OnDetach() override;
        void OnEvent(Event& e) override;


        void Begin();

        void End();

        void BlockEvents(bool block) { m_BlockEvents = block; }

    private:
        bool m_BlockEvents = true;
        float m_Time = 0.0f;
    };
}// namespace Hazel

#endif// HAZEL_ENGINE_IMGUILAYER_H
