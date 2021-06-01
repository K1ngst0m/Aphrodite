//
// Created by npchitman on 5/31/21.
//

#include "hzpch.h"
#include "Layer.h"

Hazel::Layer::Layer(std::string name)
        : m_DebugName(std::move(name)) {}

Hazel::Layer::~Layer() = default;
