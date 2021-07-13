//
// Created by npchitman on 6/1/21.
//

#include "OpenGLRendererAPI.h"

#include <glad/glad.h>

#include "pch.h"

namespace Aph {

    void OpenGLMessageCallback(
            unsigned source,
            unsigned type,
            unsigned id,
            unsigned severity,
            int length,
            const char* message,
            const void* userParam) {
        switch (severity) {
            case GL_DEBUG_SEVERITY_HIGH:
                APH_CORE_CRITICAL(message);
                return;
            case GL_DEBUG_SEVERITY_MEDIUM:
                APH_CORE_ERROR(message);
                return;
            case GL_DEBUG_SEVERITY_LOW:
                APH_CORE_WARN(message);
                return;
            case GL_DEBUG_SEVERITY_NOTIFICATION:
                APH_CORE_TRACE(message);
                return;
        }

        APH_CORE_ASSERT(false, "Unknown severity level!");
    }

    void OpenGLRendererAPI::Init() {
        APH_PROFILE_FUNCTION();

#ifdef APH_DEBUG
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(OpenGLMessageCallback, nullptr);

        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE);
#endif


        glEnable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    void OpenGLRendererAPI::SetClearColor(const glm::vec4& color) {
        glClearColor(color.r, color.g, color.b, color.a);
    }

    void OpenGLRendererAPI::Clear() {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void OpenGLRendererAPI::DrawIndexed(const Ref<VertexArray>& vertexArray, uint32_t indexCount) {
        uint32_t count = indexCount ? indexCount : vertexArray->GetIndexBuffer()->GetCount();
        glDrawElements(GL_TRIANGLES, static_cast<int>(count), GL_UNSIGNED_INT, nullptr);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    void OpenGLRendererAPI::SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
        glViewport(static_cast<int>(x), static_cast<int>(y), static_cast<int>(width), static_cast<int>(height));
    }

    void OpenGLRendererAPI::DrawArray(uint32_t first, uint32_t count) {
        glDrawArrays(GL_TRIANGLES, first, count);
    }

    void OpenGLRendererAPI::SetDepthMask(bool flag) {
        glDepthMask(flag);
    }

    void OpenGLRendererAPI::SetDepthTest(bool flag) {
        if (flag)
            glEnable(GL_DEPTH_TEST);
        else
            glDisable(GL_DEPTH_TEST);
    }
}// namespace Aph