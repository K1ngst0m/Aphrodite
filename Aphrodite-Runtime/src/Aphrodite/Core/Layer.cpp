//
// Created by npchitman on 5/31/21.
//

#include "Layer.h"

#include "pch.h"

namespace Aph {
    Layer::Layer(std::string name) : m_DebugName(std::move(name)) {}
    Layer::~Layer() = default;
}// namespace Aph-Runtime
