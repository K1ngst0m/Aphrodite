#include "sceneRenderer.h"

namespace vkl {
SceneRenderer::SceneRenderer(SceneManager *scene)
    : _sceneManager(scene) {
}

void SceneRenderer::setScene(SceneManager *scene) {
    _sceneManager = scene;
    loadResources();
}
void SceneRenderer::setRenderer(Renderer *renderer) {
    _renderer = renderer;
}
} // namespace vkl
