// Layer.h

#ifndef Aphrodite_ENGINE_LAYER_H
#define Aphrodite_ENGINE_LAYER_H

#include "Aphrodite/Events/Event.h"
#include "Base.h"
#include "TimeStep.h"

namespace Aph {
    class Layer {
    public:
        explicit Layer(std::string name = "Layer");

        virtual ~Layer();

        virtual void OnAttach() {}

        virtual void OnDetach() {}

        virtual void OnUpdate(Timestep timestep) {}

        virtual void OnUIRender() {}

        virtual void OnEvent(Event &event) {}

        const std::string &GetName() const { return m_DebugName; }

    protected:
        std::string m_DebugName;
    };
}// namespace Aph-Runtime

#endif// Aphrodite_ENGINE_LAYER_H
