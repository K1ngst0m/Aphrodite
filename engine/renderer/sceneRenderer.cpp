#include "sceneRenderer.h"

namespace vkl {
void SceneRenderer::setScene(const std::shared_ptr<SceneManager>& scene) {
    _sceneManager = scene;
    if (isSceneLoaded){
        cleanupResources();
    }
}
} // namespace vkl
