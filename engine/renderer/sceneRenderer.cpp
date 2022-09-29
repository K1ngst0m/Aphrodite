#include "sceneRenderer.h"

namespace vkl {
void SceneRenderer::setScene(const std::shared_ptr<SceneManager>& scene) {
    _sceneManager = scene;
    if (isSceneLoaded){
        cleanupResources();
        isSceneLoaded = false;
    }
}
ShadingModel SceneRenderer::getShadingModel() const {
    return _shadingModel;
}
void SceneRenderer::setShadingModel(ShadingModel model) {
    _shadingModel = model;
}
} // namespace vkl
