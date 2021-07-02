//
// Created by npchitman on 5/31/21.
//

#ifndef Aphrodite_ENGINE_IMGUILAYER_H
#define Aphrodite_ENGINE_IMGUILAYER_H

#include "Aphrodite/Core/Base.h"
#include "Aphrodite/Core/Layer.h"
#include "Aphrodite/Events/ApplicationEvent.h"
#include "Aphrodite/Events/KeyEvent.h"
#include "Aphrodite/Events/MouseEvent.h"

namespace Aph {

    class ImGuiLayer : public Layer {
    public:
        ImGuiLayer();

        ~ImGuiLayer() override;

        void OnAttach() override;
        void OnDetach() override;
        void OnEvent(Event& e) override;


        static void Begin();
        static void End();

        void BlockEvents(bool block) { m_BlockEvents = block; }

        static void SetDarkThemeColors();

    private:
        bool m_BlockEvents = true;
    };
}// namespace Aph

#endif// Aphrodite_ENGINE_IMGUILAYER_H
