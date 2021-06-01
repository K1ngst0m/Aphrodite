//
// Created by npchitman on 5/31/21.
//

#ifndef HAZEL_ENGINE_IMGUILAYER_H
#define HAZEL_ENGINE_IMGUILAYER_H

#include "Hazel/Layer.h"

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

        void OnImGuiRender() override;

        void Begin();

        void End();

    private:
        float m_Time = 0.0f;
    };
}


#endif //HAZEL_ENGINE_IMGUILAYER_H
