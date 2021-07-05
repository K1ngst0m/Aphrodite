//
// Created by npchitman on 5/31/21.
//

#include "LayerStack.h"

#include "pch.h"

Aph::LayerStack::~LayerStack() {
    for (auto layer : m_Layers){
        layer->OnDetach();
        delete layer;
    }
}

void Aph::LayerStack::PushLayer(Aph::Layer *layer) {
    m_Layers.emplace(m_Layers.begin() + m_LayerInsertIndex, layer);
    m_LayerInsertIndex++;
}

void Aph::LayerStack::PushOverlay(Aph::Layer *overlay) {
    m_Layers.emplace_back(overlay);
}

void Aph::LayerStack::PopLayer(Aph::Layer *layer) {
    auto it =
            std::find(m_Layers.begin(), m_Layers.begin() + m_LayerInsertIndex, layer);
    if (it != m_Layers.begin() + m_LayerInsertIndex) {
        layer->OnDetach();
        m_Layers.erase(it);
        m_LayerInsertIndex--;
    }
}

void Aph::LayerStack::PopOverlay(Aph::Layer *overlay) {
    auto it = std::find(m_Layers.begin(), m_Layers.begin() + m_LayerInsertIndex,
                        overlay);
    if (it != m_Layers.end()) {
        overlay->OnDetach();
        m_Layers.erase(it);
    }
}
