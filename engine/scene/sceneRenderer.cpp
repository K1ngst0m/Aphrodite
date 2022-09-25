#include "sceneRenderer.h"

namespace vkl {
void SceneRenderer::setScene(SceneManager *scene) {
    _sceneManager = scene;
    if (isSceneLoaded){
        cleanupResources();
    }
}
} // namespace vkl
