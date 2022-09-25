#ifndef RENDERER_H_
#define RENDERER_H_

#include "device.h"
#include <memory>

namespace vkl {
class SceneRenderer;
class Renderer {
public:
    struct {
        bool           enableDebug = true;
        bool           enableUI    = false;
        const uint32_t maxFrames   = 2;
    } m_settings;

public:
    virtual void initDevice()            = 0;
    virtual void destroyDevice()         = 0;
    virtual void idleDevice()            = 0;
    virtual void setWindow(void *window) = 0;

    virtual void prepareFrame() = 0;
    virtual void submitFrame()  = 0;

    virtual std::shared_ptr<SceneRenderer> createSceneRenderer() = 0;

protected:
    GraphicsDevice                *_device;
    std::shared_ptr<SceneRenderer> _sceneRenderer;
};
} // namespace vkl

#endif // RENDERER_H_
