//
// Created by npchitman on 6/1/21.
//

#ifndef HAZEL_ENGINE_RENDERER_H
#define HAZEL_ENGINE_RENDERER_H

namespace Hazel {
    enum class RendererAPI {
        None = 0, OpenGL = 1
    };

    class Renderer {
    public:
        inline static RendererAPI GetAPI() { return s_RendererAPI; }

    private:
        static RendererAPI s_RendererAPI;
    };
}


#endif //HAZEL_ENGINE_RENDERER_H
