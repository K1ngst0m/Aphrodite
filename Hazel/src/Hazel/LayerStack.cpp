//
// Created by npchitman on 5/31/21.
//

#include "LayerStack.h"

Hazel::LayerStack::LayerStack() = default;

Hazel::LayerStack::~LayerStack() {
    for (auto layer : m_Layers)
        delete layer;
}

void Hazel::LayerStack::PushLayer(Hazel::Layer *layer) {
    m_Layers.emplace(m_Layers.begin() + m_LayerInsertIndex, layer);
    layer->OnAttach();
}

void Hazel::LayerStack::PushOverlay(Hazel::Layer *overlay) {
    m_Layers.emplace_back(overlay);
    overlay->OnAttach();
}

void Hazel::LayerStack::PopLayer(Hazel::Layer *layer) {
    auto it = std::find(m_Layers.begin(), m_Layers.end(), layer);
    if (it != m_Layers.end()) {
        m_Layers.erase(it);
        m_LayerInsertIndex--;
        layer->OnDetach();
    }
}

void Hazel::LayerStack::PopOverlay(Hazel::Layer *overlay) {
    auto it = std::find(m_Layers.begin(), m_Layers.end(), overlay);
    if (it != m_Layers.end()) {
        m_Layers.erase(it);
        overlay->OnDetach();
    }
}
