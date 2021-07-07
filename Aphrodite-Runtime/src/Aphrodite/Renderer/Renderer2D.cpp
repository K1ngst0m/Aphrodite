#include "Aphrodite/Renderer/Renderer2D.h"

#include <glm/gtc/matrix_transform.hpp>

#include "Aphrodite/Renderer/RenderCommand.h"
#include "Aphrodite/Renderer/Shader.h"
#include "Aphrodite/Renderer/UniformBuffer.h"
#include "Aphrodite/Renderer/VertexArray.h"
#include "pch.h"

namespace Aph {

    struct QuadVertex {
        glm::vec3 Position;
        glm::vec4 Color{};
        glm::vec2 TexCoord{};
        float TexIndex{};
        float TilingFactor{};

        // Editor only
        int EntityID{};
    };

    struct Renderer2DData {
        static const uint32_t MaxQuads = 20000;
        static const uint32_t MaxVertices = MaxQuads * 4;
        static const uint32_t MaxIndices = MaxQuads * 6;
        static const uint32_t MaxTextureSlots = 32;

        Ref<VertexArray> QuadVertexArray;
        Ref<VertexBuffer> QuadVertexBuffer;
        Ref<Shader> TextureShader;
        Ref<Texture2D> WhiteTexture;

        uint32_t QuadIndexCount = 0;
        QuadVertex* QuadVertexBufferBase = nullptr;
        QuadVertex* QuadVertexBufferPtr = nullptr;

        std::array<Ref<Texture2D>, MaxTextureSlots> TextureSlots;
        uint32_t TextureSlotIndex = 1;// 0 = white texture

        glm::vec4 QuadVertexPositions[4]{};

        Renderer2D::Statistics Stats;

        struct CameraData {
            glm::mat4 ViewProjection;
        };
        CameraData CameraBuffer{};
        Ref<UniformBuffer> CameraUniformBuffer;
    };

    static Renderer2DData s_Data;

    void Renderer2D::Init() {
        APH_PROFILE_FUNCTION();

        s_Data.QuadVertexArray = VertexArray::Create();

        s_Data.QuadVertexBuffer = VertexBuffer::Create(Aph::Renderer2DData::MaxVertices * sizeof(QuadVertex));
        s_Data.QuadVertexBuffer->SetLayout({{ShaderDataType::Float3, "a_Position"},
                                            {ShaderDataType::Float4, "a_Color"},
                                            {ShaderDataType::Float2, "a_TexCoord"},
                                            {ShaderDataType::Float, "a_TexIndex"},
                                            {ShaderDataType::Float, "a_TilingFactor"},
                                            {ShaderDataType::Int, "a_EntityID"}});
        s_Data.QuadVertexArray->AddVertexBuffer(s_Data.QuadVertexBuffer);

        s_Data.QuadVertexBufferBase = new QuadVertex[Aph::Renderer2DData::MaxVertices];

        auto* quadIndices = new uint32_t[Aph::Renderer2DData::MaxIndices];

        uint32_t offset = 0;
        for (uint32_t i = 0; i < Aph::Renderer2DData::MaxIndices; i += 6) {
            quadIndices[i + 0] = offset + 0;
            quadIndices[i + 1] = offset + 1;
            quadIndices[i + 2] = offset + 2;

            quadIndices[i + 3] = offset + 2;
            quadIndices[i + 4] = offset + 3;
            quadIndices[i + 5] = offset + 0;

            offset += 4;
        }

        Ref<IndexBuffer> quadIB = IndexBuffer::Create(quadIndices, Aph::Renderer2DData::MaxIndices);
        s_Data.QuadVertexArray->SetIndexBuffer(quadIB);
        delete[] quadIndices;

        s_Data.WhiteTexture = Texture2D::Create(1, 1);
        uint32_t whiteTextureData = 0xffffffff;
        s_Data.WhiteTexture->SetData(&whiteTextureData, sizeof(uint32_t));

        int32_t samplers[Aph::Renderer2DData::MaxTextureSlots];
        for (uint32_t i = 0; i < Aph::Renderer2DData::MaxTextureSlots; i++)
            samplers[i] = static_cast<int>(i);

        s_Data.TextureShader = Shader::Create("assets/shaders/Texture.glsl");
        s_Data.TextureShader->Bind();
        s_Data.TextureShader->SetIntArray("u_Textures", samplers, Aph::Renderer2DData::MaxTextureSlots);

        // Set all texture slots to 0
        s_Data.TextureSlots[0] = s_Data.WhiteTexture;

        s_Data.QuadVertexPositions[0] = {-0.5f, -0.5f, 0.0f, 1.0f};
        s_Data.QuadVertexPositions[1] = {0.5f, -0.5f, 0.0f, 1.0f};
        s_Data.QuadVertexPositions[2] = {0.5f, 0.5f, 0.0f, 1.0f};
        s_Data.QuadVertexPositions[3] = {-0.5f, 0.5f, 0.0f, 1.0f};

        s_Data.CameraUniformBuffer = UniformBuffer::Create(sizeof(Renderer2DData::CameraData), 0);
    }

    void Renderer2D::Shutdown() {
        APH_PROFILE_FUNCTION();

        delete[] s_Data.QuadVertexBufferBase;
    }

    void Renderer2D::BeginScene(const EditorCamera& camera) {
        APH_PROFILE_FUNCTION();

        s_Data.TextureShader->Bind();
        s_Data.TextureShader->SetMat4("u_ViewProjection", camera.GetViewProjection());

        StartBatch();
    }


    void Renderer2D::BeginScene(const Camera& camera, const glm::mat4& transform) {
        APH_PROFILE_FUNCTION();

        s_Data.CameraBuffer.ViewProjection = camera.GetProjection() * glm::inverse(transform);
        s_Data.CameraUniformBuffer->SetData(&s_Data.CameraBuffer, sizeof(Renderer2DData::CameraData), 0);

        StartBatch();
    }


    void Renderer2D::EndScene() {
        APH_PROFILE_FUNCTION();

        Flush();
    }

    void Renderer2D::StartBatch() {
        s_Data.QuadIndexCount = 0;
        s_Data.QuadVertexBufferPtr = s_Data.QuadVertexBufferBase;

        s_Data.TextureSlotIndex = 1;
    }


    void Renderer2D::Flush() {
        if (s_Data.QuadIndexCount == 0)
            return;

        auto dataSize = (uint32_t) ((uint8_t*) s_Data.QuadVertexBufferPtr - (uint8_t*) s_Data.QuadVertexBufferBase);
        s_Data.QuadVertexBuffer->SetData(s_Data.QuadVertexBufferBase, dataSize);

        // Bind textures
        for (uint32_t i = 0; i < s_Data.TextureSlotIndex; i++)
            s_Data.TextureSlots[i]->Bind(i);

        s_Data.QuadVertexArray->Bind();
        RenderCommand::DrawIndexed(s_Data.QuadVertexArray, s_Data.QuadIndexCount);
        s_Data.Stats.DrawCalls++;
    }

    void Renderer2D::NextBatch() {
        Flush();
        StartBatch();
    }

    void Renderer2D::DrawQuad(uint32_t entityID,
                              const glm::vec2& position,
                              const float rotation,
                              const glm::vec2& size,
                              const Ref<Texture2D>& texture,
                              const glm::vec4& tintColor,
                              float tilingFactor) {
        DrawQuad(entityID, {position.x, position.y, 0.0f}, rotation, size, texture, tintColor, tilingFactor);
    }

    void Renderer2D::DrawQuad(uint32_t entityID,
                              const glm::vec3& position,
                              const float rotation,
                              const glm::vec2& size,
                              const Ref<Texture2D>& texture,
                              const glm::vec4& tintColor,
                              float tilingFactor) {
        APH_PROFILE_FUNCTION();

        const glm::mat4 translation = glm::translate(glm::mat4(1.0f), position);
        const glm::mat4 rotate = glm::rotate(glm::mat4(1.0f), glm::radians(rotation), {0.0f, 0.0f, 1.0f});
        const glm::mat4 scale = glm::scale(glm::mat4(1.0f), {size.x, size.y, 1.0f});

        const glm::mat4 transform = translation * rotate * scale;

        DrawQuad(entityID, transform, texture, tintColor);
    }

    void Renderer2D::DrawQuad(uint32_t entityID,
                              const glm::mat4& transform,
                              const glm::vec4& color) {
        APH_PROFILE_FUNCTION();

        constexpr size_t quadVertexCount = 4;
        const float textureIndex = 0.0f; // White Texture
        constexpr glm::vec2 textureCoords[] = {{0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}};
        const float tilingFactor = 1.0f;

        if (s_Data.QuadIndexCount >= Renderer2DData::MaxIndices)
            NextBatch();

        for (size_t i = 0; i < quadVertexCount; i++) {
            s_Data.QuadVertexBufferPtr->Position = transform * s_Data.QuadVertexPositions[i];
            s_Data.QuadVertexBufferPtr->Color = color;
            s_Data.QuadVertexBufferPtr->TexCoord = textureCoords[i];
            s_Data.QuadVertexBufferPtr->TexIndex = textureIndex;
            s_Data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
            s_Data.QuadVertexBufferPtr->EntityID = static_cast<int>(entityID);
            s_Data.QuadVertexBufferPtr++;
        }

        s_Data.QuadIndexCount += 6;

        s_Data.Stats.QuadCount++;
    }

    void Renderer2D::DrawQuad(uint32_t entityID,
                              const glm::mat4& transform,
                              const Ref<Texture2D>& texture,
                              const glm::vec4& tintColor,
                              float tilingFactor) {
        APH_PROFILE_FUNCTION();

        float textureIndex = 0.0f;

        if (texture) {
            for (uint32_t i = 1; i < s_Data.TextureSlotIndex; i++) {
                if (*s_Data.TextureSlots[i] == *texture) {
                    textureIndex = static_cast<float>(i);
                    break;
                }
            }

            if (textureIndex == 0.0f) {
                if (s_Data.QuadIndexCount >= Renderer2DData::MaxIndices)
                    NextBatch();

                textureIndex = static_cast<float>(s_Data.TextureSlotIndex);
                s_Data.TextureSlots[s_Data.TextureSlotIndex] = texture;
                s_Data.TextureSlotIndex++;
            }
        }

        constexpr size_t quadVertexCount = 4;
        constexpr glm::vec2 textureCoords[] = {{0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}};

        for (size_t i = 0; i < quadVertexCount; i++) {
            s_Data.QuadVertexBufferPtr->Position = transform * s_Data.QuadVertexPositions[i];
            s_Data.QuadVertexBufferPtr->Color = tintColor;
            s_Data.QuadVertexBufferPtr->TexCoord = textureCoords[i];
            s_Data.QuadVertexBufferPtr->TexIndex = textureIndex;
            s_Data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
            s_Data.QuadVertexBufferPtr->EntityID = static_cast<int>(entityID);
            s_Data.QuadVertexBufferPtr++;
        }

        s_Data.QuadIndexCount += 6;

        s_Data.Stats.QuadCount++;
    }

    void Renderer2D::ResetStats() {
        std::memset(&s_Data.Stats, 0, sizeof(Statistics));
    }

    Renderer2D::Statistics Renderer2D::GetStats() {
        return s_Data.Stats;
    }

}// namespace Aph