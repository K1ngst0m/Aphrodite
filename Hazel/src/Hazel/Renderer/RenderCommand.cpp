//
// Created by npchitman on 6/1/21.
//

#include "RenderCommand.h"
#include "hzpch.h"

#include "Platform/OpenGL/OpenGLRendererAPI.h"

namespace Hazel {
RendererAPI *RenderCommand::s_RendererAPI = new OpenGLRendererAPI;
}