//
// Created by Npchitman on 2021/2/21.
//

#ifndef HAZELENGINE_LAYERSTACK_H
#define HAZELENGINE_LAYERSTACK_H

#include "Hazel/Core.h"
#include "Hazel/Layer.h"
#include <vector>

namespace Hazel{
    class HAZEL_API LayerStack{
    public:
        LayerStack();
        ~LayerStack();

        void PushLayer(Layer* layer);
        void PushOverlay(Layer* overlay);
        void PopLayer(Layer* layer);
        void PopOverlay(Layer* overlay);

        std::vector<Layer*>::iterator begin() {return m_Layers.begin();}
        std::vector<Layer*>::iterator end() {return m_Layers.end();}

    private:
        std::vector<Layer*> m_Layers;
        unsigned int m_LayerInsertIndex;
    };
}

#endif //HAZELENGINE_LAYERSTACK_H
