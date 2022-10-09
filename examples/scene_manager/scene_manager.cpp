#include "scene_manager.h"

scene_manager::scene_manager()
    : vkl::BaseApp("scene_manager") {
}

scene_manager::~scene_manager() {
    m_deletionQueue.flush();
}

void scene_manager::init() {
    setupWindow();
    setupRenderer();
    loadScene();
    buildCommands();
}

void scene_manager::run() {
    auto timer = vkl::Timer(m_deltaTime);

    while (!m_window->shouldClose()) {
        m_window->pollEvents();
        update();
    }

    m_renderer->idleDevice();
}

void scene_manager::finish() {
}

void scene_manager::loadScene() {
    // scene global argument setup
    {
        m_sceneManager = vkl::SceneManager::Create(vkl::SceneManagerType::DEFAULT);
        m_sceneManager->setAmbient(glm::vec4(0.2f));
    }

    // scene camera
    {
        m_defaultCamera = m_sceneManager->createCamera(m_window->getAspectRatio());
        m_defaultCamera->setType(vkl::CameraType::FIRSTPERSON);
        m_defaultCamera->setPosition({0.0f, 0.0f, -3.0f, 1.0f});
        m_defaultCamera->setPerspective(60.0f, m_window->getAspectRatio(), 0.1f, 256.0f);
        m_defaultCamera->setMovementSpeed(2.5f);
        m_defaultCamera->setRotationSpeed(0.1f);

        auto &node = m_sceneManager->getRootNode()->createChildNode();
        node->attachObject(m_defaultCamera);

        m_sceneManager->setMainCamera(m_defaultCamera);
    }

    // point light
    {
        m_pointLight = m_sceneManager->createLight();
        m_pointLight->setPosition({1.2f, 1.0f, 2.0f, 1.0f});
        m_pointLight->setDiffuse({0.5f, 0.5f, 0.5f, 1.0f});
        m_pointLight->setSpecular({1.0f, 1.0f, 1.0f, 1.0f});
        m_pointLight->setType(vkl::LightType::POINT);

        auto &node = m_sceneManager->getRootNode()->createChildNode();
        node->attachObject(m_pointLight);
    }

    // direction light
    {
        m_directionalLight = m_sceneManager->createLight();
        m_directionalLight->setDirection({-0.2f, -1.0f, -0.3f, 1.0f});
        m_directionalLight->setDiffuse({0.5f, 0.5f, 0.5f, 1.0f});
        m_directionalLight->setSpecular({1.0f, 1.0f, 1.0f, 1.0f});
        m_directionalLight->setType(vkl::LightType::DIRECTIONAL);

        auto &node = m_sceneManager->getRootNode()->createChildNode();
        node->attachObject(m_directionalLight);
    }

    // load from gltf file
    {
        // glm::mat4 modelTransform = glm::scale(glm::mat4(1.0f), glm::vec3(3.0f));
        // modelTransform           = glm::rotate(modelTransform, 3.14f, glm::vec3(0.0f, 1.0f, 0.0f));
        // m_model                  = m_sceneManager->createEntity(vkl::AssetManager::GetModelDir() / "Sponza/glTF/Sponza.gltf");
        // auto &node               = m_sceneManager->getRootNode()->createChildNode(modelTransform);
        // node->attachObject(m_model);
    }

    // box prefab
    {
        // glm::mat4 modelTransform = glm::scale(glm::mat4(1.0f), glm::vec3(2.0f));
        // modelTransform           = glm::rotate(modelTransform, 3.14f, glm::vec3(0.0f, 1.0f, 0.0f));
        // auto prefab_cube_model        = m_sceneManager->getEntityWithId(vkl::PREFAB_ENTITY_BOX);
        // auto &node = m_sceneManager->getRootNode()->createChildNode(modelTransform);
        // node->attachObject(prefab_cube_model);
    }

    // plane prefab
    {
        // glm::mat4 modelTransform = glm::scale(glm::mat4(1.0f), glm::vec3(2.0f));
        // modelTransform           = glm::rotate(modelTransform, 3.14f, glm::vec3(0.0f, 1.0f, 0.0f));
        // auto prefab_plane_model        = m_sceneManager->getEntityWithId(vkl::PREFAB_ENTITY_PLANE);
        // auto &node = m_sceneManager->getRootNode()->createChildNode(modelTransform);
        // node->attachObject(prefab_plane_model);
    }

    // sphere
    {
        // glm::mat4 modelTransform = glm::scale(glm::mat4(1.0f), glm::vec3(2.0f));
        // modelTransform           = glm::rotate(modelTransform, 3.14f, glm::vec3(0.0f, 1.0f, 0.0f));
        // auto prefab_sphere_model        = m_sceneManager->getEntityWithId(vkl::PREFAB_ENTITY_SPHERE);
        // auto &node = m_sceneManager->getRootNode()->createChildNode(modelTransform);
        // node->attachObject(prefab_sphere_model);
    }

    {
        m_sceneRenderer = m_renderer->getSceneRenderer();
        m_sceneRenderer->setScene(m_sceneManager);
        m_sceneRenderer->setShadingModel(vkl::ShadingModel::UNLIT);
        m_sceneRenderer->loadResources();
    }

    m_deletionQueue.push_function([&]() {
        m_sceneRenderer->cleanupResources();
    });
}

void scene_manager::buildCommands() {
    m_sceneRenderer->drawScene();
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

void scene_manager::setupRenderer() {
    vkl::RenderConfig config{
        .enableDebug = true,
        .enableUI    = false,
        .maxFrames   = 2,
    };
    m_renderer = vkl::Renderer::Create(vkl::RenderBackend::VULKAN, &config, m_window->getWindowData());

    m_deletionQueue.push_function([&]() {
        m_renderer->destroy();
    });
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

    m_defaultCamera->processMovement(m_deltaTime);
}

void scene_manager::mouseHandleDerive(double xposIn, double yposIn) {
    float dx = m_window->getCursorXpos() - xposIn;
    float dy = m_window->getCursorYpos() - yposIn;

    m_defaultCamera->rotate(glm::vec3(dy * m_defaultCamera->getRotationSpeed(), -dx * m_defaultCamera->getRotationSpeed(), 0.0f));
}

void scene_manager::update() {
    m_sceneManager->update();
    m_sceneRenderer->update();
}

int main() {
    scene_manager app;

    app.init();
    app.run();
    app.finish();
}
