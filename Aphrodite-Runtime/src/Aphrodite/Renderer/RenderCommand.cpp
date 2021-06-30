//
// Created by npchitman on 6/1/21.
//

#include "Aphrodite/Renderer/RenderCommand.h"

#include "pch.h"

namespace Aph {
    Scope<RendererAPI> RenderCommand::s_RendererAPI = RendererAPI::Create();
}