//
// Created by npchitman on 5/31/21.
//

#ifndef APHRODITE_ENGINE_APHRODITE_H
#define APHRODITE_ENGINE_APHRODITE_H

// Base
#include "Aphrodite/Core/Application.h"
#include "Aphrodite/Core/Base.h"
#include "Aphrodite/Core/Layer.h"
#include "Aphrodite/Core/TimeStep.h"
#include "Aphrodite/Debug/Assert.h"
#include "Aphrodite/Debug/Log.h"

// Input
#include "Aphrodite/Input/Input.h"
#include "Aphrodite/Input/KeyCodes.h"
#include "Aphrodite/Input/MouseCodes.h"

// UI
#include "Aphrodite/UI/UILayer.h"

// Renderer
#include "Aphrodite/Renderer/Buffer.h"
#include "Aphrodite/Renderer/Framebuffer.h"
#include "Aphrodite/Renderer/RenderCommand.h"
#include "Aphrodite/Renderer/Renderer.h"
#include "Aphrodite/Renderer/Renderer2D.h"
#include "Aphrodite/Renderer/Shader.h"
#include "Aphrodite/Renderer/Texture.h"
#include "Aphrodite/Renderer/VertexArray.h"

// Scene
#include "Aphrodite/Scene/Components.h"
#include "Aphrodite/Scene/Entity.h"
#include "Aphrodite/Scene/Scene.h"
#include "Aphrodite/Scene/ScriptableEntity.h"
#include "Aphrodite/Scene/SceneCamera.h"

#endif// APHRODITE_ENGINE_APHRODITE_H
