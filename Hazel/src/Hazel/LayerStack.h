//
// Created by npchitman on 5/31/21.
//

#ifndef HAZEL_ENGINE_LAYERSTACK_H
#define HAZEL_ENGINE_LAYERSTACK_H

#include "Hazel/Core.h"
#include "Layer.h"

#include <vector>

namespace Hazel {
    class LayerStack final {
    public:
        LayerStack();

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
}


#endif //HAZEL_ENGINE_LAYERSTACK_H

