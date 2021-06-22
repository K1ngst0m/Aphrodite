//
// Created by npchitman on 6/1/21.
//

#include "Hazel/Renderer/RenderCommand.h"
#include "hzpch.h"

namespace Hazel {
    Scope<RendererAPI> RenderCommand::s_RendererAPI = RendererAPI::Create();
}