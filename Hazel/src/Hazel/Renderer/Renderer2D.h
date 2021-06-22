//
// Created by npchitman on 6/22/21.
//

#ifndef HAZEL_ENGINE_RENDERER2D_H
#define HAZEL_ENGINE_RENDERER2D_H

#include "OrthographicCamera.h"

namespace Hazel{
    class Renderer2D {
    public:
        static void Init();
        static void Shutdown();

        static void BeginScene(const OrthographicCamera& camera);
        static void EndScene();

        static void DrawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color);
        static void DrawQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color);
    };
}


#endif//HAZEL_ENGINE_RENDERER2D_H
