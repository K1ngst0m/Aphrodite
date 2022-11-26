#include "scene_manager.h"
#include "renderer/api/vulkan/uiRenderer.h"
#include "renderer/uiRenderer.h"
#include <memory>

vkl::RenderConfig config{
    .enableDebug = true,
    .enableUI    = false,
    .maxFrames   = 2,
};

scene_manager::scene_manager()
    : vkl::BaseApp("scene_manager") {
}

void scene_manager::init() {
    setupWindow();
    setupRenderer();
    setupScene();
}

void scene_manager::run() {
    while (!m_window->shouldClose()) {
        auto timer = vkl::Timer(m_deltaTime);
        m_window->pollEvents();

        // update resource data
        m_defaultCamera->update(m_deltaTime);
        m_sceneRenderer->update(m_deltaTime);
        m_uiRenderer->update(m_deltaTime);

        // draw and submit
        m_sceneRenderer->drawScene();
    }
}

void scene_manager::finish() {
    m_renderer->idleDevice();
    m_sceneRenderer->cleanupResources();
    m_renderer->cleanup();
    m_uiRenderer->cleanup();
}

void scene_manager::setupWindow() {
    m_window = vkl::Window::Create(1366, 768);

    m_window->setCursorPosCallback([=](double xposIn, double yposIn) {
        this->mouseHandleDerive(xposIn, yposIn);
    });

    m_window->setFramebufferSizeCallback([=](int width, int height) {
        // this->m_framebufferResized = true;
    });

    m_window->setKeyCallback([=](int key, int scancode, int action, int mods) {
        this->keyboardHandleDerive(key, scancode, action, mods);
    });
}

void scene_manager::setupScene() {
    // scene global argument setup
    {
        m_scene = vkl::Scene::Create(vkl::SceneManagerType::DEFAULT);
        m_scene->setAmbient(glm::vec4(0.2f));
    }

    // scene camera
    {
        m_defaultCamera = m_scene->createCamera(m_window->getAspectRatio());
        m_defaultCamera->setType(vkl::CameraType::FIRSTPERSON);
        m_defaultCamera->setPosition({0.0f, -1.0f, 0.0f, 1.0f});
        m_defaultCamera->setFlipY(true);
        m_defaultCamera->setRotation(glm::vec3(0.0f, -90.0f, 0.0f));
        m_defaultCamera->setPerspective(60.0f, m_window->getAspectRatio(), 0.1f, 256.0f);
        m_defaultCamera->setMovementSpeed(2.5f);
        m_defaultCamera->setRotationSpeed(0.1f);

        auto node = m_scene->getRootNode()->createChildNode();
        node->attachObject(m_defaultCamera);

        m_scene->setMainCamera(m_defaultCamera);
    }

    // point light
    {
        m_pointLight = m_scene->createLight();
        m_pointLight->setPosition({1.2f, 1.0f, 2.0f, 1.0f});
        m_pointLight->setDiffuse({0.5f, 0.5f, 0.5f, 1.0f});
        m_pointLight->setSpecular({1.0f, 1.0f, 1.0f, 1.0f});
        m_pointLight->setType(vkl::LightType::POINT);

        auto node = m_scene->getRootNode()->createChildNode();
        node->attachObject(m_pointLight);
    }

    // direction light
    {
        m_directionalLight = m_scene->createLight();
        m_directionalLight->setDirection({-0.2f, -1.0f, -0.3f, 1.0f});
        m_directionalLight->setDiffuse({0.5f, 0.5f, 0.5f, 1.0f});
        m_directionalLight->setSpecular({1.0f, 1.0f, 1.0f, 1.0f});
        m_directionalLight->setType(vkl::LightType::DIRECTIONAL);

        auto node = m_scene->getRootNode()->createChildNode();
        node->attachObject(m_directionalLight);
    }

    // load from gltf file
    {
        // m_model    = m_scene->createEntityFromGLTF(vkl::AssetManager::GetModelDir() / "Sponza/glTF/Sponza.gltf");
        m_model    = m_scene->createEntityFromGLTF(vkl::AssetManager::GetModelDir() / "DamagedHelmet/glTF-Binary/DamagedHelmet.glb");
        auto node = m_scene->getRootNode()->createChildNode();
        node->attachObject(m_model);
    }

    // box prefab
    {
        // glm::mat4 modelTransform    = glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 1.0f, 0.0f));
        // auto      prefab_cube_model = m_scene->getEntityWithId(vkl::PREFAB_ENTITY_BOX);
        // auto     &node              = m_scene->getRootNode()->createChildNode(modelTransform);
        // node->attachObject(prefab_cube_model);
    }

    // plane prefab
    {
        // glm::mat4 modelTransform     = glm::translate(glm::mat4(1.0f), glm::vec3(4.0f, 0.0f, 0.0f));
        // auto      prefab_plane_model = m_scene->getEntityWithId(vkl::PREFAB_ENTITY_PLANE);
        // auto     &node               = m_scene->getRootNode()->createChildNode(modelTransform);
        // node->attachObject(prefab_plane_model);
    }

    // sphere
    {
        // glm::mat4 modelTransform      = glm::translate(glm::mat4(1.0f), glm::vec3(6.0f, 1.0f, 0.0f));
        // auto      prefab_sphere_model = m_scene->getEntityWithId(vkl::PREFAB_ENTITY_SPHERE);
        // auto     &node                = m_scene->getRootNode()->createChildNode(modelTransform);
        // node->attachObject(prefab_sphere_model);
    }

    {
        m_sceneRenderer->setScene(m_scene);
        m_sceneRenderer->setShadingModel(vkl::ShadingModel::DEFAULTLIT);
        m_sceneRenderer->loadResources();
    }
}

void scene_manager::setupRenderer() {
    m_renderer      = vkl::VulkanRenderer::Create(&config, m_window->getWindowData());
    m_sceneRenderer = vkl::VulkanSceneRenderer::Create(m_renderer);
    m_uiRenderer    = vkl::VulkanUIRenderer::Create(m_renderer, m_window->getWindowData());
}

void scene_manager::keyboardHandleDerive(int key, int scancode, int action, int mods) {
    if (action == VKL_PRESS) {
        switch (key) {
        case VKL_KEY_ESCAPE:
            m_window->close();
            break;
        case VKL_KEY_1:
            m_window->toggleCurosrVisibility();
            break;
        case VKL_KEY_W:
            m_defaultCamera->setMovement(vkl::CameraDirection::UP, true);
            break;
        case VKL_KEY_A:
            m_defaultCamera->setMovement(vkl::CameraDirection::LEFT, true);
            break;
        case VKL_KEY_S:
            m_defaultCamera->setMovement(vkl::CameraDirection::DOWN, true);
            break;
        case VKL_KEY_D:
            m_defaultCamera->setMovement(vkl::CameraDirection::RIGHT, true);
            break;
        }
    }

    if (action == VKL_RELEASE) {
        switch (key) {
        case VKL_KEY_W:
            m_defaultCamera->setMovement(vkl::CameraDirection::UP, false);
            break;
        case VKL_KEY_A:
            m_defaultCamera->setMovement(vkl::CameraDirection::LEFT, false);
            break;
        case VKL_KEY_S:
            m_defaultCamera->setMovement(vkl::CameraDirection::DOWN, false);
            break;
        case VKL_KEY_D:
            m_defaultCamera->setMovement(vkl::CameraDirection::RIGHT, false);
            break;
        }
    }
}

void scene_manager::mouseHandleDerive(double xposIn, double yposIn) {
    float dx = m_window->getCursorXpos() - xposIn;
    float dy = m_window->getCursorYpos() - yposIn;

    m_defaultCamera->rotate(glm::vec3(dy * m_defaultCamera->getRotationSpeed(), -dx * m_defaultCamera->getRotationSpeed(), 0.0f));
}

int main() {
    scene_manager app;

    app.init();
    app.run();
    app.finish();
}
