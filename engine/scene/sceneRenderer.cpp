#include "sceneRenderer.h"

namespace vkl {
SceneRenderer::SceneRenderer(SceneManager *scene)
    : _sceneManager(scene) {
    _sceneManager->renderer = this;
}

void SceneRenderer::setScene(SceneManager *scene) {
    _sceneManager = scene;
    loadResources();
}
} // namespace vkl
