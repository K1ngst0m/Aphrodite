//
// Created by npchitman on 6/1/21.
//

#ifndef Aphrodite_ENGINE_GRAPHICSCONTEXT_H
#define Aphrodite_ENGINE_GRAPHICSCONTEXT_H

namespace Aph {
    struct ContextInfo{
        unsigned char* Vendor;
        unsigned char* Renderer;
        unsigned char* Version;
    };

    class GraphicsContext {
    public:
        virtual ~GraphicsContext() = default;

        virtual void Init() = 0;
        virtual void SwapBuffers() = 0;

        virtual ContextInfo GetContextInfo() = 0;

        static Scope<GraphicsContext> Create(void * window);
    };
}// namespace Aph

#endif// Aphrodite_ENGINE_GRAPHICSCONTEXT_H
