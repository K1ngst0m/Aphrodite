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
    m_LayerInsertIndex++;
    layer->OnAttach();
}

void Hazel::LayerStack::PushOverlay(Hazel::Layer *overlay) {
    m_Layers.emplace_back(overlay);
    overlay->OnAttach();
}

void Hazel::LayerStack::PopLayer(Hazel::Layer *layer) {
    auto it =
            std::find(m_Layers.begin(), m_Layers.begin() + m_LayerInsertIndex, layer);
    if (it != m_Layers.begin() + m_LayerInsertIndex) {
        layer->OnDetach();
        m_Layers.erase(it);
        m_LayerInsertIndex--;
    }
}

void Hazel::LayerStack::PopOverlay(Hazel::Layer *overlay) {
    auto it = std::find(m_Layers.begin(), m_Layers.begin() + m_LayerInsertIndex,
                        overlay);
    if (it != m_Layers.end()) {
        overlay->OnDetach();
        m_Layers.erase(it);
    }
}
