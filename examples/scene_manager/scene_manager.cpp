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

        // update scene object
        // m_modelNode->setTransform(glm::rotate(m_modelNode->getTransform(), 1.0f * m_deltaTime, {0.0f, 1.0f, 0.0f}));

        // update resource data
        m_cameraNode->getObject<vkl::Camera>()->update(m_deltaTime);
        m_sceneRenderer->update(m_deltaTime);
        // m_uiRenderer->update(m_deltaTime);

        // draw and submit
        m_sceneRenderer->drawScene();
    }
}

void scene_manager::finish() {
    m_renderer->idleDevice();
    m_sceneRenderer->cleanupResources();
    // m_uiRenderer->cleanup();
    m_renderer->cleanup();
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
        auto camera = m_scene->createCamera(m_window->getAspectRatio());
        camera->setType(vkl::CameraType::FIRSTPERSON);
        camera->setPosition({0.0f, 0.0f, -3.0f});
        camera->setFlipY(true);
        // camera->setRotation(glm::vec3(0.0f, 90.0f, 0.0f));
        camera->rotate({0.0f, 180.0f, 0.0f});
        camera->setPerspective(60.0f, m_window->getAspectRatio(), 0.1f, 96.0f);
        camera->setMovementSpeed(2.5f);
        camera->setRotationSpeed(0.1f);

        // camera 1 (main)
        m_cameraNode = m_scene->getRootNode()->createChildNode();
        m_cameraNode->attachObject(camera);
        m_scene->setMainCamera(camera);

        // // camera 2
        // m_scene->getRootNode()->createChildNode()->attachObject(camera);
    }

    // direction light
    {
        auto dirLight = m_scene->createLight();
        dirLight->setColor(glm::vec3{1.0f});
        dirLight->setDirection({-0.2f, -1.0f, -0.3f});
        dirLight->setType(vkl::LightType::DIRECTIONAL);

        // light1
        m_directionalLightNode = m_scene->getRootNode()->createChildNode();
        m_directionalLightNode->attachObject(dirLight);

        // // #light 2
        // m_scene->getRootNode()->createChildNode()->attachObject(dirLight);
    }

    // load from gltf file
    {
        // auto model    = m_scene->createEntityFromGLTF(vkl::AssetManager::GetModelDir() / "DamagedHelmet/glTF-Binary/DamagedHelmet.glb");
        auto model    = m_scene->createEntityFromGLTF(vkl::AssetManager::GetModelDir() / "Sponza/glTF/Sponza.gltf");
        // auto model    = m_scene->createEntityFromGLTF(vkl::AssetManager::GetModelDir() / "FlightHelmet/glTF/FlightHelmet.gltf");
        m_modelNode = m_scene->getRootNode()->createChildNode(glm::translate(glm::mat4(1.0f), {3.0f, 0.0f, 0.0f}));
        m_modelNode->attachObject(model);
    }

    {
        // auto model    = m_scene->createEntityFromGLTF(vkl::AssetManager::GetModelDir() / "prefab/Box.glb");
        // m_scene->getRootNode()->createChildNode()->attachObject(model);
    }

    {
        m_sceneRenderer->setScene(m_scene);
        m_sceneRenderer->setShadingModel(vkl::ShadingModel::PBR);
        m_sceneRenderer->loadResources();
    }
}

void scene_manager::setupRenderer() {
    m_renderer      = vkl::VulkanRenderer::Create(&config, m_window->getWindowData());
    m_sceneRenderer = vkl::VulkanSceneRenderer::Create(m_renderer);
    // m_uiRenderer    = vkl::VulkanUIRenderer::Create(m_renderer, m_window->getWindowData());
}

void scene_manager::keyboardHandleDerive(int key, int scancode, int action, int mods) {
    auto camera = m_cameraNode->getObject<vkl::Camera>();
    if (action == VKL_PRESS) {
        switch (key) {
        case VKL_KEY_ESCAPE:
            m_window->close();
            break;
        case VKL_KEY_1:
            m_window->toggleCurosrVisibility();
            break;
        case VKL_KEY_W:
            camera->setMovement(vkl::CameraDirection::UP, true);
            break;
        case VKL_KEY_A:
            camera->setMovement(vkl::CameraDirection::LEFT, true);
            break;
        case VKL_KEY_S:
            camera->setMovement(vkl::CameraDirection::DOWN, true);
            break;
        case VKL_KEY_D:
            camera->setMovement(vkl::CameraDirection::RIGHT, true);
            break;
        }
    }

    if (action == VKL_RELEASE) {
        switch (key) {
        case VKL_KEY_W:
            camera->setMovement(vkl::CameraDirection::UP, false);
            break;
        case VKL_KEY_A:
            camera->setMovement(vkl::CameraDirection::LEFT, false);
            break;
        case VKL_KEY_S:
            camera->setMovement(vkl::CameraDirection::DOWN, false);
            break;
        case VKL_KEY_D:
            camera->setMovement(vkl::CameraDirection::RIGHT, false);
            break;
        }
    }
}

void scene_manager::mouseHandleDerive(double xposIn, double yposIn) {
    float dx = m_window->getCursorXpos() - xposIn;
    float dy = m_window->getCursorYpos() - yposIn;


    auto camera = m_cameraNode->getObject<vkl::Camera>();
    camera->rotate(glm::vec3(dy * camera->getRotationSpeed(), -dx * camera->getRotationSpeed(), 0.0f));
}

int main() {
    scene_manager app;

    app.init();
    app.run();
    app.finish();
}
