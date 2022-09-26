#ifndef OPENGLRENDERER_H_
#define OPENGLRENDERER_H_

#include "common.h"

#include "renderer/renderer.h"

namespace vkl{
    class OpenGLRenderer : public Renderer{
    public:
        void initDevice() override;
        void destroyDevice() override;
        void idleDevice() override;

        std::shared_ptr<SceneRenderer> createSceneRenderer() override;
    };
}

#endif // RENDERER_H_
