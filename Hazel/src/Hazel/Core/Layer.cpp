//
// Created by npchitman on 5/31/21.
//

#include "Hazel/Core/Layer.h"

#include "hzpch.h"

namespace Hazel {
    Layer::Layer(std::string name) : m_DebugName(std::move(name)) {}
    Layer::~Layer() = default;
}// namespace Hazel
