#ifndef RENDERER_H_
#define RENDERER_H_

#include "device.h"

namespace vkl {
    class Renderer{
    public:
        virtual void init() = 0;
        virtual void destroy() = 0;

    protected:
        GraphicsDevice * _device;
    };
}

#endif // RENDERER_H_
