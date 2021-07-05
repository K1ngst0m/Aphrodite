//
// Created by npchitman on 6/1/21.
//

#ifndef Aphrodite_ENGINE_GRAPHICSCONTEXT_H
#define Aphrodite_ENGINE_GRAPHICSCONTEXT_H

namespace Aph {
    struct ContextInfo{
        const unsigned char* Vendor;
        const unsigned char* Renderer;
        const unsigned char* Version;

        ContextInfo() = default;
        ContextInfo(const unsigned char* vendor,
                    const unsigned char* renderer,
                    const unsigned char* version):
        Vendor(vendor), Renderer(renderer), Version(version){}
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
