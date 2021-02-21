//
// Created by Npchitman on 2021/2/21.
//
#include "hzpch.h"
#include "Layer.h"

namespace Hazel{
    Layer::Layer(const std::string &debugName) : m_DebugName(debugName) {}

    Layer::~Layer() = default;
}
