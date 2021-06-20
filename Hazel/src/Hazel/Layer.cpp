//
// Created by npchitman on 5/31/21.
//

#include "Layer.h"
#include "hzpch.h"

Hazel::Layer::Layer(std::string name) : m_DebugName(std::move(name)) {}

Hazel::Layer::~Layer() = default;
