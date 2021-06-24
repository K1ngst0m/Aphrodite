//
// Created by npchitman on 6/1/21.
//

#ifndef HAZEL_ENGINE_GRAPHICSCONTEXT_H
#define HAZEL_ENGINE_GRAPHICSCONTEXT_H

namespace Hazel {
    class GraphicsContext {
    public:
        virtual ~GraphicsContext() = default;

        virtual void Init() = 0;
        virtual void SwapBuffers() = 0;

        static Scope<GraphicsContext> Create(void * window);
    };
}// namespace Hazel

#endif// HAZEL_ENGINE_GRAPHICSCONTEXT_H
