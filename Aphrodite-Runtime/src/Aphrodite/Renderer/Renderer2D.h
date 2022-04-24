//
// Created by npchitman on 6/22/21.
//

#ifndef Aphrodite_ENGINE_RENDERER2D_H
#define Aphrodite_ENGINE_RENDERER2D_H

#include "Aphrodite/Renderer/Texture.h"
#include "Aphrodite/Scene/Camera.h"
#include "Aphrodite/Scene/EditorCamera.h"
#include "Aphrodite/Scene/Components.h"

namespace Aph {
    class Renderer2D {
    public:
        static void Init();
        static void Shutdown();

        static void BeginScene(const EditorCamera& camera);
        static void BeginScene(const Camera& camera, const glm::mat4& transform);
        static void EndScene();
        static void Flush();

        //Primitives
        static void DrawQuad(uint32_t entityID, const glm::vec2& position, float rotation, const glm::vec2& size, const Ref<Texture2D>& texture = nullptr, const glm::vec4& tintColor = glm::vec4(1.0f), float tilingFactor = 1.0f);
        static void DrawQuad(uint32_t entityID, const glm::vec3& position, float rotation, const glm::vec2& size, const Ref<Texture2D>& texture = nullptr, const glm::vec4& tintColor = glm::vec4(1.0f), float tilingFactor = 1.0f);

        static void DrawQuad(uint32_t entityID, const glm::mat4& transform, const glm::vec4& color);
        static void DrawQuad(uint32_t entityID, const glm::mat4& transform, const Ref<Texture2D>& texture = nullptr, const glm::vec4& tintColor = glm::vec4(1.0f), float tilingFactor = 1.0f);

        struct Statistics {
            uint32_t DrawCalls = 0;
            uint32_t QuadCount = 0;

            uint32_t GetTotalVertexCount() const { return QuadCount * 4; }
            uint32_t GetTotalIndexCount() const { return QuadCount * 6; }
        };

        static void ResetStats();
        static Statistics GetStats();

    private:
        static void StartBatch();
        static void NextBatch();
    };
}// namespace Aph


#endif//Aphrodite_ENGINE_RENDERER2D_H
