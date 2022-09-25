#include "sceneRenderer.h"

namespace vkl {
SceneRenderer::SceneRenderer(SceneManager *scene)
    : _sceneManager(scene) {
}

void SceneRenderer::setScene(SceneManager *scene) {
    _sceneManager = scene;
    loadResources();
}
} // namespace vkl
