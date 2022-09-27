#include "scene_manager.h"

void scene_manager::initDerive() {
    loadScene();
    buildCommands();
}

void scene_manager::drawFrame() {
    float currentFrame    = glfwGetTime();
    m_frameData.deltaTime = currentFrame - m_frameData.lastFrame;
    m_frameData.lastFrame = currentFrame;

    m_renderer->prepareFrame();
    m_renderer->submitFrame();
    updateUniformBuffer();
}

void scene_manager::updateUniformBuffer() {
    m_sceneManager->update();
    m_sceneRenderer->update();
}

void scene_manager::loadScene() {
    // scene global argument setup
    {
        m_sceneManager = std::make_shared<vkl::SceneManager>();
        m_sceneManager->setAmbient(glm::vec4(0.2f));
    }

    {
        m_defaultCamera = m_sceneManager->createCamera((float)m_windowData.width / m_windowData.height);
        m_defaultCamera->setType(vkl::CameraType::FIRSTPERSON);
        m_defaultCamera->setPosition({0.0f, 1.0f, 3.0f, 1.0f});
        m_defaultCamera->setPerspective(60.0f, (float)m_windowData.width / (float)m_windowData.height, 0.1f, 256.0f);
        m_defaultCamera->setMovementSpeed(2.5f);
        m_defaultCamera->setRotationSpeed(0.1f);

        auto &node = m_sceneManager->getRootNode()->createChildNode();
        node->attachObject(m_defaultCamera);

        m_sceneManager->setMainCamera(m_defaultCamera);
    }

    {
        m_pointLight = m_sceneManager->createLight();
        m_pointLight->setPosition({1.2f, 1.0f, 2.0f, 1.0f});
        m_pointLight->setDiffuse({0.5f, 0.5f, 0.5f, 1.0f});
        m_pointLight->setSpecular({1.0f, 1.0f, 1.0f, 1.0f});
        m_pointLight->setType(vkl::LightType::POINT);

        auto &node = m_sceneManager->getRootNode()->createChildNode();
        node->attachObject(m_pointLight);
    }

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
        glm::mat4 modelTransform = glm::scale(glm::mat4(1.0f), glm::vec3(3.0f));
        modelTransform           = glm::rotate(modelTransform, 3.14f, glm::vec3(0.0f, 1.0f, 0.0f));
        m_model                  = m_sceneManager->createEntity(modelDir / "Sponza/glTF/Sponza.gltf");
        m_model->setShadingModel(vkl::ShadingModel::PBR);
        auto &node               = m_sceneManager->getRootNode()->createChildNode(modelTransform);
        node->attachObject(m_model);
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
        m_sceneRenderer = m_renderer->createSceneRenderer();
        m_sceneRenderer->setScene(m_sceneManager);
        m_sceneRenderer->loadResources();
    }

    m_deletionQueue.push_function([&]() {
        m_sceneRenderer->cleanupResources();
    });
}

void scene_manager::buildCommands() {
    m_sceneRenderer->drawScene();
}

int main() {
    scene_manager app;

    app.vkl::vklApp::init();
    app.vkl::vklApp::run();
    app.vkl::vklApp::finish();
}
