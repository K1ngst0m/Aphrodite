//
// Created by Npchitman on 2021/2/21.
//

#ifndef HAZELENGINE_LAYER_H
#define HAZELENGINE_LAYER_H

#include "Hazel/Core.h"
#include "Hazel/Events/Event.h"

namespace Hazel{
    class HAZEL_API Layer{
    public:
        Layer(const std::string& name = "Layer");
        virtual ~Layer();

        virtual void OnAttach() {}
        virtual void OnDetach() {}
        virtual void OnUpdate() {}
        virtual void OnEvent(Event& event){}

        inline const std::string & GetName() const { return m_DebugName; }

    private:
        std::string m_DebugName;
    };
}

#endif //HAZELENGINE_LAYER_H
