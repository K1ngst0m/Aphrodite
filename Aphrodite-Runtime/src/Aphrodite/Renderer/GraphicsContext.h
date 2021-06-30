//
// Created by npchitman on 6/1/21.
//

#ifndef Aphrodite_ENGINE_GRAPHICSCONTEXT_H
#define Aphrodite_ENGINE_GRAPHICSCONTEXT_H

namespace Aph {
    class GraphicsContext {
    public:
        virtual ~GraphicsContext() = default;

        virtual void Init() = 0;
        virtual void SwapBuffers() = 0;

        static Scope<GraphicsContext> Create(void * window);
    };
}// namespace Aph

#endif// Aphrodite_ENGINE_GRAPHICSCONTEXT_H
