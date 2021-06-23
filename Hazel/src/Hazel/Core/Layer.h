//
// Created by npchitman on 5/31/21.
//

#ifndef HAZEL_ENGINE_LAYER_H
#define HAZEL_ENGINE_LAYER_H

#include "Base.h"
#include "Hazel/Core/TimeStep.h"
#include "Hazel/Events/Event.h"

namespace Hazel {
    class Layer {
    public:
        explicit Layer(std::string name = "Layer");

        virtual ~Layer();

        virtual void OnAttach() {}

        virtual void OnDetach() {}

        virtual void OnUpdate(Timestep ts) {}

        virtual void OnImGuiRender() {}

        virtual void OnEvent(Event &event) {}

        const std::string &GetName() const { return m_DebugName; }

    protected:
        std::string m_DebugName;
    };
}// namespace Hazel

#endif// HAZEL_ENGINE_LAYER_H
