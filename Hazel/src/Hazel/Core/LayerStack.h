//
// Created by npchitman on 5/31/21.
//

#ifndef HAZEL_ENGINE_LAYERSTACK_H
#define HAZEL_ENGINE_LAYERSTACK_H

#include <vector>

#include "Hazel/Core/Core.h"
#include "Hazel/Core/Layer.h"

namespace Hazel {
    class LayerStack final {
    public:
        LayerStack() = default;

        ~LayerStack();

        void PushLayer(Layer *layer);

        void PushOverlay(Layer *overlay);

        void PopLayer(Layer *layer);

        void PopOverlay(Layer *overlay);

        std::vector<Layer *>::iterator begin() { return m_Layers.begin(); }

        std::vector<Layer *>::iterator end() { return m_Layers.end(); }

    private:
        std::vector<Layer *> m_Layers;
        unsigned int m_LayerInsertIndex = 0;
    };
}// namespace Hazel

#endif// HAZEL_ENGINE_LAYERSTACK_H
