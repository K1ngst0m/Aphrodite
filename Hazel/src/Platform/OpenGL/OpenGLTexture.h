//
// Created by npchitman on 6/21/21.
//

#ifndef HAZEL_ENGINE_OPENGLTEXTURE_H
#define HAZEL_ENGINE_OPENGLTEXTURE_H

#include "Hazel/Renderer/Texture.h"

namespace Hazel {
    class OpenGLTexture2D : public Texture2D {
    public:
        explicit OpenGLTexture2D(const std::string& path);
        ~OpenGLTexture2D() override;
        uint32_t GetWidth() const override { return m_Width; }
        uint32_t GetHeight() const override { return m_Height; }

        void Bind(uint32_t slot = 0) const override;

    private:
        std::string m_Path;
        uint32_t m_Width, m_Height;
        uint32_t m_RendererID;
    };
}// namespace Hazel


#endif//HAZEL_ENGINE_OPENGLTEXTURE_H
